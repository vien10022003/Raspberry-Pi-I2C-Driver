#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include "font_chars.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 3 L01 - Hybrid Scroll");
MODULE_DESCRIPTION("Hybrid horizontal and vertical scroll text display");
MODULE_VERSION("1.0");

extern void SSD1306_Write(bool is_cmd, unsigned char data);

/* OLED display parameters */
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8
#define MAX_CHARS_PER_LINE 16
#define MAX_LINES 8

/* Text buffer for multiple lines */
static char text_buffer[MAX_LINES][MAX_CHARS_PER_LINE + 1];
static int total_lines = 0;

/* Scroll control variables */
static int horizontal_offset = 0;  // Horizontal scroll position
static int vertical_offset = 0;    // Vertical scroll position (which line to start from)
static int scroll_speed = 150;     // Speed in milliseconds
static bool auto_scroll_h = true;  // Auto horizontal scroll
static bool auto_scroll_v = false; // Auto vertical scroll
static bool module_active = true;

/* Timer for auto scroll */
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;

/* Pre-computed transposed fonts for horizontal display */
static uint8_t transposed_font[40][8];

/* Function to initialize default text */
static void init_default_text(void)
{
    strcpy(text_buffer[0], "NHOM 3 LOP L01");
    strcpy(text_buffer[1], "TO QUANG VIEN");
    strcpy(text_buffer[2], "BUI DUC KHANH");
    strcpy(text_buffer[3], "NGUYEN THI HONG");
    strcpy(text_buffer[4], "THAN NHAN CHINH");
    strcpy(text_buffer[5], "HYBRID SCROLL");
    strcpy(text_buffer[6], "HORIZONTAL+VERTICAL");
    strcpy(text_buffer[7], "DEMO MODULE");

    total_lines = 8;
}

/* Pre-compute transposed fonts for optimized horizontal scroll */
static void precompute_transposed_fonts(void)
{
    int char_idx, i, j;

    printk(KERN_INFO "HybridScroll: Pre-computing transposed fonts...\n");

    for (char_idx = 0; char_idx < 40; char_idx++)
    {
        for (i = 0; i < 8; i++)
        {
            transposed_font[char_idx][i] = 0;
            for (j = 0; j < 8; j++)
            {
                if (font_8x8[char_idx][j] & (1 << (7 - i)))
                {
                    transposed_font[char_idx][i] |= (1 << j);
                }
            }
        }
    }

    printk(KERN_INFO "HybridScroll: Font transpose completed!\n");
}

/* Clear entire OLED screen */
static void oled_clear_screen(void)
{
    int page, col;

    for (page = 0; page < 8; page++)
    {
        SSD1306_Write(true, 0xB0 + page); // Set page address
        SSD1306_Write(true, 0x00);        // Set lower column address
        SSD1306_Write(true, 0x10);        // Set higher column address

        for (col = 0; col < 128; col++)
        {
            SSD1306_Write(false, 0x00);
        }
    }
}

/* Draw character at position using pre-computed transposed font */
static void draw_char_horizontal(int x, int page, char c)
{
    int font_index, i;

    if (x >= 128 || x < 0 || page >= 8 || page < 0)
        return;

    font_index = char_to_index(c);

    SSD1306_Write(true, 0xB0 + page); // Set page address

    for (i = 0; i < 8; i++)
    {
        if ((x + i) >= 128)
            break;

        /* Set column address */
        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F));

        /* Send pre-computed transposed data */
        SSD1306_Write(false, transposed_font[font_index][i]);
    }
}

/* Draw character vertically (rotated 90 degrees) */
static void draw_char_vertical(int x, int y, char c)
{
    int font_index, i, j;
    int page = y / 8;
    unsigned char rotated_data[8] = {0};

    if (x >= 128 || x < 0 || page >= 8 || page < 0)
        return;

    font_index = char_to_index(c);

    /* Transform font data for vertical display */
    for (i = 0; i < 8; i++)
    {
        for (j = 0; j < 8; j++)
        {
            if (font_8x8[font_index][j] & (1 << (7 - i)))
            {
                rotated_data[i] |= (1 << j);
            }
        }
    }

    /* Write each column */
    for (i = 0; i < 8; i++)
    {
        SSD1306_Write(true, 0xB0 + page);
        SSD1306_Write(true, 0x00 | ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 | (((x + i) >> 4) & 0x0F));
        SSD1306_Write(false, rotated_data[i]);
    }
}

/* Display hybrid scroll: horizontal scroll + vertical selection */
static void display_hybrid_scroll(void)
{
    int char_pos, display_x;
    int current_line, display_line;

    oled_clear_screen();

    /* Display lines with vertical offset */
    for (display_line = 0; display_line < MAX_LINES && display_line < total_lines; display_line++)
    {
        current_line = (vertical_offset + display_line) % total_lines;

        if (current_line >= total_lines)
            continue;

        /* Horizontal scroll for current line */
        display_x = -horizontal_offset;

        for (char_pos = 0; char_pos < strlen(text_buffer[current_line]) && display_x < 128; char_pos++)
        {
            if (display_x + 8 > 0) /* Only draw if character is visible */
            {
                if (auto_scroll_h)
                {
                    /* Horizontal scroll mode - use horizontal font */
                    draw_char_horizontal(display_x, display_line, text_buffer[current_line][char_pos]);
                }
                else
                {
                    /* Vertical mode - use vertical font */
                    draw_char_vertical(display_x, display_line * CHAR_HEIGHT, text_buffer[current_line][char_pos]);
                }
            }
            display_x += 8;
        }
    }
}

/* Timer work handler for auto scroll */
static void scroll_work_handler(struct work_struct *work)
{
    int max_length = 0;
    int i;

    if (!module_active)
        return;

    if (auto_scroll_h)
    {
        horizontal_offset += 2; /* Horizontal scroll speed */

        /* Reset when scrolled past the longest line */
        for (i = 0; i < total_lines; i++)
        {
            int len = strlen(text_buffer[i]);
            if (len > max_length)
                max_length = len;
        }

        if (horizontal_offset >= (max_length * 8 + 128))
        {
            horizontal_offset = 0;
        }
    }

    if (auto_scroll_v)
    {
        vertical_offset++;
        if (vertical_offset >= total_lines)
        {
            vertical_offset = 0;
        }
    }

    display_hybrid_scroll();

    if (module_active)
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
    }
}

/* Keyboard event handler */
static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code == KBD_KEYCODE && param->down)
    {
        switch (param->value)
        {
        case 103: /* UP arrow - vertical scroll up */
            vertical_offset--;
            if (vertical_offset < 0)
                vertical_offset = total_lines - 1;
            display_hybrid_scroll();
            printk(KERN_INFO "HybridScroll: Vertical UP, offset = %d\n", vertical_offset);
            break;

        case 108: /* DOWN arrow - vertical scroll down */
            vertical_offset++;
            if (vertical_offset >= total_lines)
                vertical_offset = 0;
            display_hybrid_scroll();
            printk(KERN_INFO "HybridScroll: Vertical DOWN, offset = %d\n", vertical_offset);
            break;

        case 105: /* LEFT arrow - horizontal scroll left */
            horizontal_offset -= 8;
            if (horizontal_offset < 0)
                horizontal_offset = 0;
            display_hybrid_scroll();
            printk(KERN_INFO "HybridScroll: Horizontal LEFT, offset = %d\n", horizontal_offset);
            break;

        case 106: /* RIGHT arrow - horizontal scroll right */
            horizontal_offset += 8;
            display_hybrid_scroll();
            printk(KERN_INFO "HybridScroll: Horizontal RIGHT, offset = %d\n", horizontal_offset);
            break;

        case 57: /* SPACE - toggle horizontal auto scroll */
            auto_scroll_h = !auto_scroll_h;
            printk(KERN_INFO "HybridScroll: Horizontal auto scroll: %s\n", auto_scroll_h ? "ON" : "OFF");
            break;

        case 44: /* Z - toggle vertical auto scroll */
            auto_scroll_v = !auto_scroll_v;
            printk(KERN_INFO "HybridScroll: Vertical auto scroll: %s\n", auto_scroll_v ? "ON" : "OFF");
            break;

        case 1: /* ESC - reset all scroll */
            horizontal_offset = 0;
            vertical_offset = 0;
            display_hybrid_scroll();
            printk(KERN_INFO "HybridScroll: All scroll RESET\n");
            break;

        case 16: /* Q - demo mode */
            printk(KERN_INFO "HybridScroll: Demo mode activated\n");
            auto_scroll_h = true;
            auto_scroll_v = true;
            horizontal_offset = 0;
            vertical_offset = 0;
            display_hybrid_scroll();
            break;

        case 25: /* P - pause all auto scroll */
            auto_scroll_h = false;
            auto_scroll_v = false;
            printk(KERN_INFO "HybridScroll: All auto scroll PAUSED\n");
            break;
        }
    }

    return NOTIFY_OK;
}

static struct notifier_block keyboard_notifier_block = {
    .notifier_call = keyboard_notify,
};

/* Module initialization */
static int __init hybrid_scroll_init(void)
{
    int ret;

    printk(KERN_INFO "HybridScroll: Module loading...\n");

    /* Initialize text buffer */
    init_default_text();

    /* Pre-compute fonts for optimization */
    precompute_transposed_fonts();

    /* Clear screen and display initial content */
    oled_clear_screen();
    display_hybrid_scroll();

    /* Create workqueue for auto scroll */
    scroll_wq = create_singlethread_workqueue("hybrid_scroll_wq");
    if (!scroll_wq)
    {
        printk(KERN_ERR "HybridScroll: Failed to create workqueue\n");
        return -ENOMEM;
    }

    /* Initialize delayed work */
    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);

    /* Start auto scroll timer */
    queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));

    /* Register keyboard notifier */
    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret)
    {
        printk(KERN_ERR "HybridScroll: Failed to register keyboard notifier\n");
        destroy_workqueue(scroll_wq);
        return ret;
    }

    printk(KERN_INFO "HybridScroll: Module loaded successfully\n");
    printk(KERN_INFO "HybridScroll: Controls:\n");
    printk(KERN_INFO "  - Arrow keys: Manual scroll\n");
    printk(KERN_INFO "  - SPACE: Toggle horizontal auto scroll\n");
    printk(KERN_INFO "  - Z: Toggle vertical auto scroll\n");
    printk(KERN_INFO "  - ESC: Reset all scroll\n");
    printk(KERN_INFO "  - Q: Demo mode (auto scroll both)\n");
    printk(KERN_INFO "  - P: Pause all auto scroll\n");

    return 0;
}

/* Module cleanup */
static void __exit hybrid_scroll_exit(void)
{
    printk(KERN_INFO "HybridScroll: Module unloading...\n");

    module_active = false;

    /* Unregister keyboard notifier */
    unregister_keyboard_notifier(&keyboard_notifier_block);

    /* Cancel and destroy workqueue */
    if (scroll_wq)
    {
        cancel_delayed_work_sync(&scroll_work);
        destroy_workqueue(scroll_wq);
    }

    /* Clear screen */
    oled_clear_screen();

    printk(KERN_INFO "HybridScroll: Module unloaded successfully\n");
}

module_init(hybrid_scroll_init);
module_exit(hybrid_scroll_exit);