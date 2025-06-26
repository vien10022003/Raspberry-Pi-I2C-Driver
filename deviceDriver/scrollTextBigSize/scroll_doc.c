#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/string.h>
#include "../scrollText/font_chars.h" // ✅ Dùng chung font với scrollText

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 3 L01");
MODULE_DESCRIPTION("BIG SIZE Scroll text display for team members");
MODULE_VERSION("2.0");

extern void SSD1306_Write(bool is_cmd, unsigned char data);

/* Text hiển thị thông tin nhóm */
static char display_text[] = "NHOM 3 LOP L01: TO QUANG VIEN, BUI DUC KHANH, NGUYEN THI HONG NGAN, THAN NHAN CHINH    ";
static int scroll_offset = 0;
static int text_length;
static int scroll_speed = 200; // ✅ Chậm hơn vì chữ to

/* Timer cho auto scroll */
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;
static bool auto_scroll = true;
static bool module_active = true;

/* ✅ PRE-COMPUTED BIG FONTS (16x16 scaled từ 8x8) */
static uint8_t big_transposed_font[40][16][2]; // [char][col][page_part]

/* ✅ PRE-COMPUTE BIG TRANSPOSE - scale font 8x8 thành 16x16 */
static void precompute_big_transposed_fonts(void)
{
    int char_idx, i, j, bit_row, bit_col;

    printk(KERN_INFO "Pre-computing BIG transposed fonts (16x16)...\n");

    for (char_idx = 0; char_idx < 40; char_idx++)
    {
        // Initialize
        for (i = 0; i < 16; i++)
        {
            big_transposed_font[char_idx][i][0] = 0;
            big_transposed_font[char_idx][i][1] = 0;
        }

        // Scale 8x8 font to 16x16
        for (bit_row = 0; bit_row < 8; bit_row++)
        {
            for (bit_col = 0; bit_col < 8; bit_col++)
            {
                if (font_8x8[char_idx][bit_row] & (1 << (7 - bit_col)))
                {
                    // Mỗi pixel thành 2x2 pixels
                    // Column scaling: bit_col*2 và bit_col*2+1
                    // Row scaling: bit_row*2 và bit_row*2+1

                    int scaled_col_1 = bit_col * 2;
                    int scaled_col_2 = bit_col * 2 + 1;
                    int scaled_row_1 = bit_row * 2;
                    int scaled_row_2 = bit_row * 2 + 1;

                    // Set 4 pixels (2x2) for each original pixel
                    if (scaled_row_1 < 8)
                    {
                        big_transposed_font[char_idx][scaled_col_1][0] |= (1 << scaled_row_1);
                        big_transposed_font[char_idx][scaled_col_2][0] |= (1 << scaled_row_1);
                    }
                    if (scaled_row_2 < 8)
                    {
                        big_transposed_font[char_idx][scaled_col_1][0] |= (1 << scaled_row_2);
                        big_transposed_font[char_idx][scaled_col_2][0] |= (1 << scaled_row_2);
                    }

                    // Second page (rows 8-15)
                    if (scaled_row_1 >= 8)
                    {
                        big_transposed_font[char_idx][scaled_col_1][1] |= (1 << (scaled_row_1 - 8));
                        big_transposed_font[char_idx][scaled_col_2][1] |= (1 << (scaled_row_1 - 8));
                    }
                    if (scaled_row_2 >= 8)
                    {
                        big_transposed_font[char_idx][scaled_col_1][1] |= (1 << (scaled_row_2 - 8));
                        big_transposed_font[char_idx][scaled_col_2][1] |= (1 << (scaled_row_2 - 8));
                    }
                }
            }
        }
    }

    printk(KERN_INFO "BIG font transpose completed!\n");
}

/* ✅ Clear specific page */
void oled_clear_page(int page)
{
    int col;

    SSD1306_Write(true, 0xB0 + page);
    SSD1306_Write(true, 0x00);
    SSD1306_Write(true, 0x10);

    for (col = 0; col < 128; col++)
    {
        SSD1306_Write(false, 0x00);
    }
}

/* Clear screen */
void oled_clear_screen(void)
{
    int page;
    for (page = 0; page < 8; page++)
    {
        oled_clear_page(page);
    }
}

/* ✅ Draw BIG character (16x16) spanning 2 pages */
void draw_big_char_at_position(int x, int start_page, char c)
{
    int font_index;
    int i;

    if (x >= 128 || x < 0 || start_page >= 7 || start_page < 0)
        return;

    font_index = char_to_index(c);

    /* ✅ Draw upper part (page 0) */
    SSD1306_Write(true, 0xB0 + start_page);

    for (i = 0; i < 16; i++)
    {
        if ((x + i) >= 128)
            break;

        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F));

        SSD1306_Write(false, big_transposed_font[font_index][i][0]);
    }

    /* ✅ Draw lower part (page 1) */
    SSD1306_Write(true, 0xB0 + start_page + 1);

    for (i = 0; i < 16; i++)
    {
        if ((x + i) >= 128)
            break;

        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F));

        SSD1306_Write(false, big_transposed_font[font_index][i][1]);
    }
}

/* ✅ Display BIG scrolled text */
void display_big_scrolled_text(void)
{
    int char_pos = 0;
    int display_x = 0;

    /* Clear 2 pages for big text */
    // oled_clear_page(1);
    // oled_clear_page(2);

    /* SCROLL NGANG: text di chuyển từ trái sang phải */
    display_x = -scroll_offset;

    /* Hiển thị BIG text trên page 1-2 */
    for (char_pos = 0; char_pos < text_length && display_x < 128; char_pos++)
    {
        if (display_x + 16 > 0) /* Chỉ vẽ nếu ký tự có thể nhìn thấy */
        {
            draw_big_char_at_position(display_x, 1, display_text[char_pos]);
        }
        display_x += 16; /* ✅ Mỗi ký tự BIG cách nhau 16 pixel */
    }
}

/* ✅ Timer handler cho BIG text */
static void scroll_work_handler(struct work_struct *work)
{
    if (!module_active)
        return;

    if (auto_scroll)
    {
        scroll_offset += 2; /* ✅ Scroll chậm hơn vì chữ to */

        /* Reset khi scroll hết text */
        if (scroll_offset >= (text_length * 16 + 128))
        {
            scroll_offset = 0;
        }

        display_big_scrolled_text();
    }

    if (module_active)
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
    }
}

/* ✅ Keyboard handler cho BIG text */
static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code == KBD_KEYCODE && param->down)
    {
        switch (param->value)
        {
        case 103: /* UP arrow - manual scroll up */
            if (!auto_scroll)
            {
                scroll_offset -= 16; /* ✅ Scroll 16 pixel (1 char) */
                if (scroll_offset < 0)
                    scroll_offset = 0;
                display_big_scrolled_text();
            }
            printk(KERN_INFO "BIG Scroll UP: offset = %d\n", scroll_offset);
            break;

        case 108: /* DOWN arrow - manual scroll down */
            if (!auto_scroll)
            {
                scroll_offset += 16; /* ✅ Scroll 16 pixel (1 char) */
                if (scroll_offset >= (text_length * 16))
                    scroll_offset = text_length * 16 - 1;
                display_big_scrolled_text();
            }
            printk(KERN_INFO "BIG Scroll DOWN: offset = %d\n", scroll_offset);
            break;

        case 57: /* SPACE - toggle auto scroll */
            auto_scroll = !auto_scroll;
            printk(KERN_INFO "BIG Auto scroll: %s\n", auto_scroll ? "ON" : "OFF");
            if (auto_scroll && module_active)
            {
                queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
            }
            break;

        case 1: /* ESC - reset scroll */
            scroll_offset = 0;
            display_big_scrolled_text();
            printk(KERN_INFO "BIG Scroll RESET\n");
            break;

        case 105: /* LEFT arrow - increase speed */
            scroll_speed -= 25;
            if (scroll_speed < 100)
                scroll_speed = 100;
            printk(KERN_INFO "BIG Scroll speed: %d ms (faster)\n", scroll_speed);
            break;

        case 106: /* RIGHT arrow - decrease speed */
            scroll_speed += 25;
            if (scroll_speed > 800)
                scroll_speed = 800;
            printk(KERN_INFO "BIG Scroll speed: %d ms (slower)\n", scroll_speed);
            break;

        case 16: /* Q - test BIG characters */
            printk(KERN_INFO "BIG Performance test...\n");
            // oled_clear_page(1);
            // oled_clear_page(2);

            draw_big_char_at_position(0, 1, 'B');
            draw_big_char_at_position(16, 1, 'I');
            draw_big_char_at_position(32, 1, 'G');
            draw_big_char_at_position(48, 1, ' ');
            draw_big_char_at_position(64, 1, '3');

            printk(KERN_INFO "BIG test completed!\n");
            break;
        }
    }

    return NOTIFY_OK;
}

static struct notifier_block keyboard_notifier_block = {
    .notifier_call = keyboard_notify,
};

/* ✅ BIG SIZE INIT */
static int __init scroll_big_module_init(void)
{
    int ret;

    text_length = strlen(display_text);

    printk(KERN_INFO "=== BIG SIZE SCROLL TEXT MODULE ===\n");
    printk(KERN_INFO "Team: %s\n", display_text);
    printk(KERN_INFO "Text length: %d characters\n", text_length);
    printk(KERN_INFO "BIG Scroll speed: %d ms\n", scroll_speed);

    /* ✅ PRE-COMPUTE BIG FONTS */
    precompute_big_transposed_fonts();

    /* SSD1306 Init */
    SSD1306_Write(true, 0xAE); // Display OFF
    SSD1306_Write(true, 0xA8);
    SSD1306_Write(true, 0x3F);
    SSD1306_Write(true, 0xD3);
    SSD1306_Write(true, 0x00);
    SSD1306_Write(true, 0x40);
    SSD1306_Write(true, 0xA1);
    SSD1306_Write(true, 0xC8);
    SSD1306_Write(true, 0xDA);
    SSD1306_Write(true, 0x12);
    SSD1306_Write(true, 0x81);
    SSD1306_Write(true, 0x9F); // ✅ Higher contrast for big text
    SSD1306_Write(true, 0xA4);
    SSD1306_Write(true, 0xA6);
    SSD1306_Write(true, 0xD5);
    SSD1306_Write(true, 0x80);
    SSD1306_Write(true, 0x8D);
    SSD1306_Write(true, 0x14);
    SSD1306_Write(true, 0xAF); // Display ON

    scroll_wq = create_singlethread_workqueue("big_scroll_workqueue");
    if (!scroll_wq)
    {
        printk(KERN_ERR "Cannot create BIG workqueue\n");
        return -ENOMEM;
    }

    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);

    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret)
    {
        printk(KERN_ERR "Cannot register BIG keyboard notifier\n");
        destroy_workqueue(scroll_wq);
        return ret;
    }

    display_big_scrolled_text();
    queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(2000));

    printk(KERN_INFO "BIG SIZE scroll module loaded successfully!\n");
    printk(KERN_INFO "Controls: SPACE=toggle, ESC=reset, UP/DOWN=manual, LEFT/RIGHT=speed, Q=test\n");
    return 0;
}

static void __exit scroll_big_module_exit(void)
{
    module_active = false;

    unregister_keyboard_notifier(&keyboard_notifier_block);
    cancel_delayed_work_sync(&scroll_work);
    destroy_workqueue(scroll_wq);

    SSD1306_Write(true, 0x20);
    SSD1306_Write(true, 0x00);
    oled_clear_screen();

    printk(KERN_INFO "BIG SIZE scroll module unloaded successfully!\n");
}

module_init(scroll_big_module_init);
module_exit(scroll_big_module_exit);