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

// Text hiển thị thông tin nhóm
static char display_text[] = "NHOM 3 LOP L01: TO QUANG VIEN, BUI DUC KHANH, NGUYEN THI HONG NGAN, THAN NHAN CHINH    ";
static int scroll_offset = 0;
static int text_length;
static int scroll_speed = 150; // ms delay

// Timer cho auto scroll
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;
static bool auto_scroll = true;
static bool module_active = true;

// Clear toàn bộ màn hình OLED
void oled_clear_screen(void)
{
    int page, col;

    for (page = 0; page < 8; page++)
    {
        // Set page address
        SSD1306_Write(true, 0xB0 + page);
        // Set column address to start
        SSD1306_Write(true, 0x00); // Lower column
        SSD1306_Write(true, 0x10); // Higher column

        // Clear all columns in this page
        for (col = 0; col < 128; col++)
        {
            SSD1306_Write(false, 0x00);
        }
    }
}

// Hiển thị một ký tự tại vị trí x, page
void draw_char_at_position(int x, int page, char c)
{
    int font_index = char_to_index(c);
    int i;

    if (x >= 128 || x < 0 || page >= 8 || page < 0)
        return;

    // Set page address
    SSD1306_Write(true, 0xB0 + page);

    // Vẽ 8 cột cho ký tự
    for (i = 0; i < 8; i++)
    {
        if ((x + i) >= 128)
            break;

        // Set column address
        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));        // Lower column
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F)); // Higher column

        // Gửi font data
        SSD1306_Write(false, font_8x8[font_index][i]);
    }
}

// Hiển thị text với scroll offset
void display_scrolled_text(void)
{
    int char_pos = 0;
    int display_x = 0;
    int page;
    int start_char_index;

    // Clear screen trước
    oled_clear_screen();

    // Tính vị trí ký tự bắt đầu dựa trên scroll offset
    start_char_index = scroll_offset / 8; // Mỗi ký tự rộng 8 pixel
    int pixel_offset = scroll_offset % 8;

    // Hiển thị text trên 2 hàng (page 2-3 và page 5-6)
    for (page = 2; page <= 3; page++)
    {
        display_x = -pixel_offset; // Bắt đầu với offset để tạo hiệu ứng scroll mượt

        for (char_pos = start_char_index; char_pos < text_length && display_x < 128; char_pos++)
        {
            if (display_x >= -8)
            { // Chỉ vẽ nếu ký tự có thể nhìn thấy
                draw_char_at_position(display_x, page, display_text[char_pos]);
            }
            display_x += 8; // Mỗi ký tự cách nhau 8 pixel
        }
    }
}

// Timer handler cho auto scroll
static void scroll_work_handler(struct work_struct *work)
{
    if (!module_active)
        return;

    if (auto_scroll)
    {
        scroll_offset++;

        // Reset khi scroll hết text
        if (scroll_offset >= (text_length * 8 + 128))
        {
            scroll_offset = 0;
        }

        display_scrolled_text();
    }

    // Lên lịch cho lần scroll tiếp theo
    if (module_active)
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
    }
}

// Keyboard handler để điều khiển scroll
static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code == KBD_KEYCODE && param->down)
    {
        switch (param->value)
        {
        case 103: // UP arrow - scroll ngược lại
            scroll_offset -= 8;
            if (scroll_offset < 0)
                scroll_offset = 0;
            if (!auto_scroll)
                display_scrolled_text();
            printk(KERN_INFO "Scroll UP: offset = %d\n", scroll_offset);
            break;

        case 108: // DOWN arrow - scroll tiến lên
            scroll_offset += 8;
            if (scroll_offset >= (text_length * 8 + 128))
            {
                scroll_offset = text_length * 8 + 127;
            }
            if (!auto_scroll)
                display_scrolled_text();
            printk(KERN_INFO "Scroll DOWN: offset = %d\n", scroll_offset);
            break;

        case 57: // SPACE - toggle auto scroll
            auto_scroll = !auto_scroll;
            printk(KERN_INFO "Auto scroll: %s\n", auto_scroll ? "ON" : "OFF");
            if (auto_scroll && module_active)
            {
                queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
            }
            break;

        case 1: // ESC - reset position
            scroll_offset = 0;
            display_scrolled_text();
            printk(KERN_INFO "Scroll RESET\n");
            break;

        case 105: // LEFT arrow - giảm tốc độ
            scroll_speed += 50;
            if (scroll_speed > 500)
                scroll_speed = 500;
            printk(KERN_INFO "Scroll speed: %d ms\n", scroll_speed);
            break;

        case 106: // RIGHT arrow - tăng tốc độ
            scroll_speed -= 50;
            if (scroll_speed < 50)
                scroll_speed = 50;
            printk(KERN_INFO "Scroll speed: %d ms\n", scroll_speed);
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
    printk(KERN_INFO "Controls:\n");
    printk(KERN_INFO "  UP/DOWN: Manual scroll\n");
    printk(KERN_INFO "  LEFT/RIGHT: Speed control\n");
    printk(KERN_INFO "  SPACE: Toggle auto scroll\n");
    printk(KERN_INFO "  ESC: Reset position\n");

    // Tạo workqueue
    scroll_wq = create_singlethread_workqueue("scroll_workqueue");
    if (!scroll_wq)
    {
        printk(KERN_ERR "Failed to create workqueue\n");
        return -ENOMEM;
    }

    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);

    // Đăng ký keyboard notifier
    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret)
    {
        printk(KERN_ERR "Failed to register keyboard notifier\n");
        destroy_workqueue(scroll_wq);
        return ret;
    }

    // Hiển thị text ban đầu
    display_scrolled_text();

    // Bắt đầu auto scroll sau 2 giây
    queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(2000));

    printk(KERN_INFO "Scroll module loaded successfully!\n");
    return 0;
}

static void __exit scroll_module_exit(void)
{
    module_active = false;

    // Hủy đăng ký keyboard notifier
    unregister_keyboard_notifier(&keyboard_notifier_block);

    // Dừng và hủy workqueue
    cancel_delayed_work_sync(&scroll_work);
    destroy_workqueue(scroll_wq);

    // Clear màn hình khi thoát
    oled_clear_screen();

    printk(KERN_INFO "Scroll module unloaded successfully!\n");
}

module_init(scroll_module_init);
module_exit(scroll_module_exit);