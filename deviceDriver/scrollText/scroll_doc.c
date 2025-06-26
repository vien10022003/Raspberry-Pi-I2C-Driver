#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/string.h>
#include "font_chars.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 3 L01");
MODULE_DESCRIPTION("Scroll text display for team members");
MODULE_VERSION("1.0");

extern void SSD1306_Write(bool is_cmd, unsigned char data);

/* Text hiển thị thông tin nhóm */
static char display_text[] = "NHOM 3 LOP L01: TO QUANG VIEN, BUI DUC KHANH, NGUYEN THI HONG NGAN, THAN NHAN CHINH    ";
static int scroll_offset = 0;
static int text_length;
static int scroll_speed = 300; // ✅ Tăng từ 150ms lên 300ms để mượt hơn

/* Timer cho auto scroll */
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;
static bool auto_scroll = true;
static bool module_active = true;

/* ✅ LOCAL RESET FUNCTION - không cần sửa I2CDriver */
void oled_reset_display(void)
{
    printk(KERN_INFO "Resetting display state...\n");

    // Turn off display
    SSD1306_Write(true, 0xAE); // Display OFF
    msleep(10);

    // Reset addressing mode to horizontal
    SSD1306_Write(true, 0x20); // Set memory addressing mode
    SSD1306_Write(true, 0x00); // Horizontal addressing mode

    // Reset column address range
    SSD1306_Write(true, 0x21); // Set column address
    SSD1306_Write(true, 0x00); // Column start = 0
    SSD1306_Write(true, 0x7F); // Column end = 127

    // Reset page address range
    SSD1306_Write(true, 0x22); // Set page address
    SSD1306_Write(true, 0x00); // Page start = 0
    SSD1306_Write(true, 0x07); // Page end = 7

    // Clear all display
    int i;
    for (i = 0; i < 1024; i++)
    {
        SSD1306_Write(false, 0x00);
    }

    // Deactivate any scroll
    SSD1306_Write(true, 0x2E); // Deactivate scroll

    // Turn display back on
    SSD1306_Write(true, 0xAF); // Display ON

    printk(KERN_INFO "Display reset completed.\n");
}

/* Clear toàn bộ màn hình OLED */
void oled_clear_screen(void)
{
    int page, col;

    for (page = 0; page < 8; page++)
    {
        SSD1306_Write(true, 0xB0 + page);
        SSD1306_Write(true, 0x00);
        SSD1306_Write(true, 0x10);

        for (col = 0; col < 128; col++)
        {
            SSD1306_Write(false, 0x00);
        }
    }
}

/* Hiển thị một ký tự tại vị trí x, page */
void draw_char_at_position(int x, int page, char c)
{
    int font_index;
    int i;

    if (x >= 128 || x < 0 || page >= 8 || page < 0)
        return;

    font_index = char_to_index(c);

    SSD1306_Write(true, 0xB0 + page);

    for (i = 0; i < 8; i++)
    {
        if ((x + i) >= 128)
            break;

        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F));

        SSD1306_Write(false, font_8x8[font_index][i]);
    }
}

/* ✅ FIXED: Hiển thị text với scroll ngang đúng cách */
void display_scrolled_text(void)
{
    int char_pos = 0;
    int display_x = 0;

    oled_clear_screen();

    /* SCROLL NGANG: bắt đầu từ offset và hiển thị từ trái sang phải */
    display_x = -scroll_offset;

    for (char_pos = 0; char_pos < text_length; char_pos++)
    {
        /* Chỉ vẽ nếu ký tự nằm trong vùng nhìn thấy */
        if (display_x >= -8 && display_x < 128)
        {
            draw_char_at_position(display_x, 2, display_text[char_pos]); // Hiển thị ở page 2 (giữa màn hình)
        }

        display_x += 8; /* Mỗi ký tự cách nhau 8 pixel */

        /* Dừng nếu đã vượt quá màn hình */
        if (display_x >= 128)
            break;
    }
}

/* Timer handler cho auto scroll */
static void scroll_work_handler(struct work_struct *work)
{
    if (!module_active)
        return;

    if (auto_scroll)
    {
        scroll_offset++;

        /* ✅ FIXED: Reset khi scroll hết text */
        if (scroll_offset >= (text_length * 8 + 128))
        {
            scroll_offset = 0; // Reset về đầu
        }

        display_scrolled_text();
    }

    if (module_active)
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
    }
}

/* Keyboard handler để điều khiển scroll */
static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code == KBD_KEYCODE && param->down)
    {
        switch (param->value)
        {
        case 103:               /* UP arrow - scroll lên */
            scroll_offset -= 8; /* Giảm theo pixel */
            if (scroll_offset < 0)
                scroll_offset = 0;
            if (!auto_scroll)
                display_scrolled_text();
            printk(KERN_INFO "Scroll UP: offset = %d\n", scroll_offset);
            break;

        case 108:               /* DOWN arrow - scroll xuống */
            scroll_offset += 8; /* Tăng theo pixel */
            if (scroll_offset >= (text_length * 8))
                scroll_offset = text_length * 8 - 1;
            if (!auto_scroll)
                display_scrolled_text();
            printk(KERN_INFO "Scroll DOWN: offset = %d\n", scroll_offset);
            break;

        case 57: /* SPACE */
            auto_scroll = !auto_scroll;
            printk(KERN_INFO "Auto scroll: %s\n", auto_scroll ? "ON" : "OFF");
            if (auto_scroll && module_active)
            {
                queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
            }
            break;

        case 1: /* ESC - reset display */
            scroll_offset = 0;
            oled_reset_display(); // ✅ Reset hoàn toàn display state
            display_scrolled_text();
            printk(KERN_INFO "Display RESET\n");
            break;

        case 105: /* LEFT arrow - tăng tốc độ (giảm delay) */
            scroll_speed -= 50;
            if (scroll_speed < 50)
                scroll_speed = 50;
            printk(KERN_INFO "Scroll speed: %d ms (faster)\n", scroll_speed);
            break;

        case 106: /* RIGHT arrow - giảm tốc độ (tăng delay) */
            scroll_speed += 50;
            if (scroll_speed > 500)
                scroll_speed = 500;
            printk(KERN_INFO "Scroll speed: %d ms (slower)\n", scroll_speed);
            break;
        }
    }

    return NOTIFY_OK;
}

static struct notifier_block keyboard_notifier_block = {
    .notifier_call = keyboard_notify,
};

static int __init scroll_module_init(void)
{
    int ret;

    text_length = strlen(display_text);

    printk(KERN_INFO "=== SCROLL TEXT MODULE ===\n");
    printk(KERN_INFO "Team: %s\n", display_text);
    printk(KERN_INFO "Text length: %d characters\n", text_length);
    printk(KERN_INFO "Scroll speed: %d ms\n", scroll_speed);

    scroll_wq = create_singlethread_workqueue("scroll_workqueue");
    if (!scroll_wq)
    {
        printk(KERN_ERR "Failed to create workqueue\n");
        return -ENOMEM;
    }

    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);

    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret)
    {
        printk(KERN_ERR "Failed to register keyboard notifier\n");
        destroy_workqueue(scroll_wq);
        return ret;
    }

    display_scrolled_text();
    queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(2000));

    printk(KERN_INFO "Scroll module loaded successfully!\n");
    printk(KERN_INFO "Controls: SPACE=toggle, ESC=reset display, UP/DOWN=manual scroll, LEFT/RIGHT=speed\n");
    return 0;
}

/* ✅ FIXED: Cleanup với display reset local */
static void __exit scroll_module_exit(void)
{
    printk(KERN_INFO "Scroll module exiting...\n");

    module_active = false;

    unregister_keyboard_notifier(&keyboard_notifier_block);
    cancel_delayed_work_sync(&scroll_work);
    destroy_workqueue(scroll_wq);

    /* ✅ Reset display state trước khi exit */
    oled_reset_display();

    printk(KERN_INFO "Scroll module unloaded successfully!\n");
}

module_init(scroll_module_init);
module_exit(scroll_module_exit);