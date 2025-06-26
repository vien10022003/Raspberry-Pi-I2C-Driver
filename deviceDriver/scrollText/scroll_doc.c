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
static int scroll_speed = 300;

/* Timer cho auto scroll */
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;
static bool auto_scroll = true;
static bool module_active = true;

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

/* ✅ DÙNG FONT THẬT - render đúng cách */
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

        // ✅ Sử dụng font data gốc
        SSD1306_Write(false, font_8x8[font_index][i]);
    }
}

/* Hiển thị text với scroll ngang */
void display_scrolled_text(void)
{
    int char_pos = 0;
    int display_x = 0;

    oled_clear_screen();

    /* SCROLL NGANG: text di chuyển từ trái sang phải */
    display_x = -scroll_offset;

    /* Hiển thị text trên page 0 */
    for (char_pos = 0; char_pos < text_length && display_x < 128; char_pos++)
    {
        if (display_x + 8 > 0) /* Chỉ vẽ nếu ký tự có thể nhìn thấy */
        {
            draw_char_at_position(display_x, 0, display_text[char_pos]);
        }
        display_x += 8; /* Mỗi ký tự cách nhau 8 pixel */
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

        /* Reset khi scroll hết text */
        if (scroll_offset >= (text_length * 8 + 128))
        {
            scroll_offset = 0;
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
        case 103: /* UP arrow - scroll faster */
            scroll_offset -= 8;
            if (scroll_offset < 0)
                scroll_offset = 0;
            if (!auto_scroll)
                display_scrolled_text();
            printk(KERN_INFO "Scroll UP: offset = %d\n", scroll_offset);
            break;

        case 108: /* DOWN arrow - scroll slower */
            scroll_offset += 8;
            if (scroll_offset >= (text_length * 8))
                scroll_offset = text_length * 8 - 1;
            if (!auto_scroll)
                display_scrolled_text();
            printk(KERN_INFO "Scroll DOWN: offset = %d\n", scroll_offset);
            break;

        case 57: /* SPACE - toggle auto scroll */
            auto_scroll = !auto_scroll;
            printk(KERN_INFO "Auto scroll: %s\n", auto_scroll ? "ON" : "OFF");
            if (auto_scroll && module_active)
            {
                queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
            }
            break;

        case 1: /* ESC - reset scroll */
            scroll_offset = 0;
            display_scrolled_text();
            printk(KERN_INFO "Scroll RESET\n");
            break;

        case 105: /* LEFT arrow - increase speed (decrease delay) */
            scroll_speed -= 50;
            if (scroll_speed < 50)
                scroll_speed = 50;
            printk(KERN_INFO "Scroll speed: %d ms (faster)\n", scroll_speed);
            break;

        case 106: /* RIGHT arrow - decrease speed (increase delay) */
            scroll_speed += 50;
            if (scroll_speed > 500)
                scroll_speed = 500;
            printk(KERN_INFO "Scroll speed: %d ms (slower)\n", scroll_speed);
            break;

        case 16: /* Q - test single character */
            printk(KERN_INFO "Testing single characters...\n");
            oled_clear_screen();

            // Test a few characters at fixed positions
            draw_char_at_position(0, 0, 'N'); // First char of NHOM
            draw_char_at_position(8, 0, 'H');
            draw_char_at_position(16, 0, 'O');
            draw_char_at_position(24, 0, 'M');
            draw_char_at_position(32, 0, ' ');
            draw_char_at_position(40, 0, '3');

            // Test on different page
            draw_char_at_position(0, 2, 'L');
            draw_char_at_position(8, 2, 'O');
            draw_char_at_position(16, 2, 'P');
            draw_char_at_position(24, 2, ' ');
            draw_char_at_position(32, 2, 'L');
            draw_char_at_position(40, 2, '0');
            draw_char_at_position(48, 2, '1');

            printk(KERN_INFO "Character test completed. Check if fonts look correct.\n");
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
    printk(KERN_INFO "Controls: SPACE=toggle, ESC=reset, UP/DOWN=manual, LEFT/RIGHT=speed, Q=char test\n");
    return 0;
}

static void __exit scroll_module_exit(void)
{
    module_active = false;

    unregister_keyboard_notifier(&keyboard_notifier_block);
    cancel_delayed_work_sync(&scroll_work);
    destroy_workqueue(scroll_wq);

    /* Reset display state đơn giản */
    SSD1306_Write(true, 0x20); // Set addressing mode
    SSD1306_Write(true, 0x00); // Horizontal mode
    oled_clear_screen();

    printk(KERN_INFO "Scroll module unloaded successfully!\n");
}

module_init(scroll_module_init);
module_exit(scroll_module_exit);