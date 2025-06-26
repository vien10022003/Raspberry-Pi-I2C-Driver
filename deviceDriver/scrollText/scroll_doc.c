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

/* ✅ SIMPLE PATTERN TEST - không dùng font */
void draw_char_at_position(int x, int page, char c)
{
    int i;

    if (x >= 128 || x < 0 || page >= 8 || page < 0)
        return;

    SSD1306_Write(true, 0xB0 + page);

    for (i = 0; i < 8; i++)
    {
        if ((x + i) >= 128)
            break;

        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F));

        // ✅ Simple test patterns để check orientation
        unsigned char pattern = 0;

        if (c >= 'A' && c <= 'Z')
        {
            // Letters: vertical line pattern
            pattern = 0xFF; // Full vertical line
        }
        else if (c >= '0' && c <= '9')
        {
            // Numbers: different pattern
            switch (c)
            {
            case '0':
                pattern = 0x7E;
                break; // Outline
            case '1':
                pattern = 0x18;
                break; // Thin line
            case '2':
                pattern = 0xF0;
                break; // Top half
            case '3':
                pattern = 0x0F;
                break; // Bottom half
            default:
                pattern = 0x55;
                break; // Dotted
            }
        }
        else if (c == ' ')
        {
            pattern = 0x00; // Empty for space
        }
        else if (c == ':')
        {
            pattern = 0x24; // Two dots
        }
        else if (c == ',')
        {
            pattern = 0x02; // Bottom dot
        }
        else
        {
            pattern = 0x81; // Corner dots for other chars
        }

        SSD1306_Write(false, pattern);
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
        case 103: /* UP arrow - scroll lên */
            scroll_offset -= 8;
            if (scroll_offset < 0)
                scroll_offset = 0;
            if (!auto_scroll)
                display_scrolled_text();
            printk(KERN_INFO "Scroll UP: offset = %d\n", scroll_offset);
            break;

        case 108: /* DOWN arrow - scroll xuống */
            scroll_offset += 8;
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

        case 1: /* ESC */
            scroll_offset = 0;
            display_scrolled_text();
            printk(KERN_INFO "Scroll RESET\n");
            break;

        case 105: /* LEFT arrow - faster */
            scroll_speed -= 50;
            if (scroll_speed < 50)
                scroll_speed = 50;
            printk(KERN_INFO "Scroll speed: %d ms (faster)\n", scroll_speed);
            break;

        case 106: /* RIGHT arrow - slower */
            scroll_speed += 50;
            if (scroll_speed > 500)
                scroll_speed = 500;
            printk(KERN_INFO "Scroll speed: %d ms (slower)\n", scroll_speed);
            break;

        case 16: /* Q - test different patterns */
            printk(KERN_INFO "Testing display patterns...\n");

            // Test pattern: draw lines on different pages
            oled_clear_screen();

            // Page 0: horizontal line pattern
            SSD1306_Write(true, 0xB0 + 0);
            SSD1306_Write(true, 0x00);
            SSD1306_Write(true, 0x10);
            for (int i = 0; i < 128; i++)
            {
                SSD1306_Write(false, 0xFF); // Full line
            }

            // Page 2: dotted pattern
            SSD1306_Write(true, 0xB0 + 2);
            SSD1306_Write(true, 0x00);
            SSD1306_Write(true, 0x10);
            for (int i = 0; i < 128; i++)
            {
                SSD1306_Write(false, i % 2 ? 0x55 : 0xAA); // Checkerboard
            }

            // Page 4: vertical line test
            SSD1306_Write(true, 0xB0 + 4);
            SSD1306_Write(true, 0x00);
            SSD1306_Write(true, 0x10);
            for (int i = 0; i < 128; i++)
            {
                SSD1306_Write(false, i % 8 == 0 ? 0xFF : 0x00); // Vertical lines every 8 pixels
            }

            printk(KERN_INFO "Pattern test completed. Check display orientation.\n");
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

    printk(KERN_INFO "=== SCROLL TEXT MODULE (PATTERN TEST) ===\n");
    printk(KERN_INFO "Team: %s\n", display_text);
    printk(KERN_INFO "Text length: %d characters\n", text_length);
    printk(KERN_INFO "Using simple patterns instead of fonts for orientation test\n");

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
    printk(KERN_INFO "Press Q to test display patterns and check orientation\n");
    printk(KERN_INFO "Controls: SPACE=toggle, ESC=reset, UP/DOWN=manual, LEFT/RIGHT=speed, Q=pattern test\n");
    return 0;
}

static void __exit scroll_module_exit(void)
{
    module_active = false;

    unregister_keyboard_notifier(&keyboard_notifier_block);
    cancel_delayed_work_sync(&scroll_work);
    destroy_workqueue(scroll_wq);

    /* Reset display state */
    SSD1306_Write(true, 0x20); // Set addressing mode
    SSD1306_Write(true, 0x00); // Horizontal mode
    oled_clear_screen();

    printk(KERN_INFO "Scroll module unloaded successfully!\n");
}

module_init(scroll_module_init);
module_exit(scroll_module_exit);