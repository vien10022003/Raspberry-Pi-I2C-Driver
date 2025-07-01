// #include <linux/module.h>
// #include <linux/kernel.h>
// #include <linux/init.h>
// #include <linux/keyboard.h>
// #include <linux/notifier.h>
// #include <linux/workqueue.h>
// #include <linux/delay.h>
// #include <linux/string.h>
// #include <linux/slab.h>
// #include <linux/mutex.h>
// #include "font_chars.h"

// MODULE_LICENSE("GPL");
// MODULE_AUTHOR("Group 3 L01 - Hybrid Scroll");
// MODULE_DESCRIPTION("Hybrid horizontal and vertical scroll text display");
// MODULE_VERSION("2.0");

// extern void SSD1306_Write(bool is_cmd, unsigned char data);

// /* OLED display parameters */
// #define OLED_WIDTH 128
// #define OLED_HEIGHT 64
// #define CHAR_WIDTH 8
// #define CHAR_HEIGHT 8
// #define MAX_CHARS_PER_LINE 16
// #define MAX_LINES 8

// /* Text buffer for multiple lines */
// static char text_buffer[MAX_LINES][MAX_CHARS_PER_LINE + 1];
// static int total_lines = 0;

// /* Scroll control variables */
// static int horizontal_offset = 0;  // Horizontal scroll position
// static int vertical_offset = 0;    // Vertical scroll position (which line to start from)
// static int scroll_speed = 500;     // Speed in milliseconds - increased for stability
// static bool auto_scroll_h = true;  // Auto horizontal scroll
// static bool auto_scroll_v = false; // Auto vertical scroll
// static bool module_active = true;
// static bool display_updating = false; // Flag to prevent concurrent display updates

// /* Mutex for thread safety */
// static DEFINE_MUTEX(display_mutex);

// /* Timer for auto scroll */
// static struct workqueue_struct *scroll_wq;
// static struct delayed_work scroll_work;

// /* Pre-computed transposed fonts for horizontal display */
// static uint8_t transposed_font[40][8];

// /* Longest line length for horizontal scroll reset */
// static int max_line_length = 0;

// /* Function to initialize default text */
// static void init_default_text(void)
// {
//     strcpy(text_buffer[0], "NHOM 3 LOP L01");
//     strcpy(text_buffer[1], "TO QUANG VIEN");
//     strcpy(text_buffer[2], "BUI DUC KHANH");
//     strcpy(text_buffer[3], "NGUYEN THI HONG");
//     strcpy(text_buffer[4], "THAN NHAN CHINH");
//     strcpy(text_buffer[5], "HYBRID SCROLL");
//     strcpy(text_buffer[6], "HORIZONTAL+VERTICAL");
//     strcpy(text_buffer[7], "DEMO MODULE");

//     total_lines = 8;

//     /* Calculate max_line_length once */
//     max_line_length = 0;
//     for (int i = 0; i < total_lines; i++)
//     {
//         int len = strlen(text_buffer[i]);
//         if (len > max_line_length)
//             max_line_length = len;
//     }
// }

// /* Pre-compute transposed fonts for optimized horizontal scroll */
// static void precompute_transposed_fonts(void)
// {
//     int char_idx, i, j;

//     printk(KERN_INFO "HybridScroll: Pre-computing transposed fonts...\n");

//     for (char_idx = 0; char_idx < 40; char_idx++)
//     {
//         /* Transposed font for horizontal display */
//         for (i = 0; i < 8; i++)
//         {
//             transposed_font[char_idx][i] = 0;
//             for (j = 0; j < 8; j++)
//             {
//                 if (font_8x8[char_idx][j] & (1 << (7 - i)))
//                 {
//                     transposed_font[char_idx][i] |= (1 << j);
//                 }
//             }
//         }
//     }

//     printk(KERN_INFO "HybridScroll: Font pre-computation completed!\n");
// }

// /* Optimized clear screen function - clear only necessary pages */
// static void oled_clear_screen_fast(void)
// {
//     int page, col;

//     for (page = 0; page < 8; page++)
//     {
//         SSD1306_Write(true, 0xB0 + page); // Set page address
//         SSD1306_Write(true, 0x00);        // Set lower column address
//         SSD1306_Write(true, 0x10);        // Set higher column address

//         for (col = 0; col < 128; col++)
//         {
//             SSD1306_Write(false, 0x00);
//         }

//         /* Add small delay to prevent I2C bus overload */
//         udelay(10);
//     }
// }

// /* Draw character at position using pre-computed transposed font */
// static void draw_char_horizontal(int x, int page, char c)
// {
//     int font_index, i;

//     if (x >= 128 || x < 0 || page >= 8 || page < 0)
//         return;

//     font_index = char_to_index(c);
//     if (font_index < 0 || font_index >= 40)
//         return;

//     SSD1306_Write(true, 0xB0 + page); // Set page address

//     for (i = 0; i < 8; i++)
//     {
//         if ((x + i) >= 128)
//             break;

//         /* Set column address */
//         SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));
//         SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F));

//         /* Send pre-computed transposed data */
//         SSD1306_Write(false, transposed_font[font_index][i]);
//     }

//     /* Small delay to prevent I2C congestion */
//     udelay(5);
// }

// /* Thread-safe display function with reduced overhead */
// static void display_hybrid_scroll_safe(void)
// {
//     int char_pos, display_x;
//     int current_line, display_line;

//     if (!mutex_trylock(&display_mutex))
//     {
//         /* If mutex is locked, skip this update to prevent blocking */
//         return;
//     }

//     if (display_updating)
//     {
//         mutex_unlock(&display_mutex);
//         return;
//     }

//     display_updating = true;

//     /* Only clear screen when necessary (not every frame) */
//     oled_clear_screen_fast();

//     /* Display lines with vertical offset */
//     for (display_line = 0; display_line < MAX_LINES; display_line++)
//     {
//         current_line = (vertical_offset + display_line) % total_lines;

//         if (current_line >= total_lines)
//             continue;

//         /* Horizontal scroll for current line */
//         display_x = -horizontal_offset;

//         for (char_pos = 0; char_pos < strlen(text_buffer[current_line]) && display_x < 128; char_pos++)
//         {
//             if (display_x + 8 > 0) /* Only draw if character is visible */
//             {
//                 draw_char_horizontal(display_x, display_line, text_buffer[current_line][char_pos]);
//             }
//             display_x += 8;
//         }
//     }

//     display_updating = false;
//     mutex_unlock(&display_mutex);
// }

// /* Timer work handler for auto scroll - with reduced frequency */
// static void scroll_work_handler(struct work_struct *work)
// {
//     if (!module_active)
//         return;

//     if (auto_scroll_h)
//     {
//         horizontal_offset += 1; /* Reduced scroll speed for stability */

//         /* Reset when scrolled past the longest line */
//         if (horizontal_offset >= (max_line_length * 8 + 128))
//         {
//             horizontal_offset = 0;
//         }
//     }

//     if (auto_scroll_v)
//     {
//         vertical_offset++;
//         if (vertical_offset >= total_lines)
//         {
//             vertical_offset = 0;
//         }
//     }

//     /* Only update display if auto scroll is active */
//     if (auto_scroll_h || auto_scroll_v)
//     {
//         display_hybrid_scroll_safe();
//     }

//     if (module_active)
//     {
//         queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
//     }
// }

// /* Enhanced keyboard event handler with better error handling */
// static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
// {
//     struct keyboard_notifier_param *param = _param;

//     if (code != KBD_KEYCODE || !param->down || !module_active)
//         return NOTIFY_OK;

//     /* Log all key presses for debugging */
//     printk(KERN_DEBUG "HybridScroll: Key pressed, code = %d\n", param->value);

//     switch (param->value)
//     {
//     case 103: /* UP arrow - vertical scroll up */
//         vertical_offset--;
//         if (vertical_offset < 0)
//             vertical_offset = total_lines - 1;
//         /* Use safe display function to prevent system hang */
//         display_hybrid_scroll_safe();
//         printk(KERN_INFO "HybridScroll: Vertical UP, offset = %d\n", vertical_offset);
//         break;

//     case 108: /* DOWN arrow - vertical scroll down */
//         vertical_offset++;
//         if (vertical_offset >= total_lines)
//             vertical_offset = 0;
//         display_hybrid_scroll_safe();
//         printk(KERN_INFO "HybridScroll: Vertical DOWN, offset = %d\n", vertical_offset);
//         break;

//     case 105: /* LEFT arrow - horizontal scroll left */
//         horizontal_offset -= 8;
//         if (horizontal_offset < 0)
//             horizontal_offset = 0;
//         display_hybrid_scroll_safe();
//         printk(KERN_INFO "HybridScroll: Horizontal LEFT, offset = %d\n", horizontal_offset);
//         break;

//     case 106: /* RIGHT arrow - horizontal scroll right */
//         horizontal_offset += 8;
//         if (horizontal_offset >= (max_line_length * 8 + 128))
//             horizontal_offset = 0;
//         display_hybrid_scroll_safe();
//         printk(KERN_INFO "HybridScroll: Horizontal RIGHT, offset = %d\n", horizontal_offset);
//         break;

//     case 57: /* SPACE - toggle horizontal auto scroll */
//         auto_scroll_h = !auto_scroll_h;
//         printk(KERN_INFO "HybridScroll: Horizontal auto scroll: %s\n", auto_scroll_h ? "ON" : "OFF");
//         /* Force display update when toggling auto scroll */
//         if (!auto_scroll_h)
//         {
//             display_hybrid_scroll_safe();
//         }
//         break;

//     case 44: /* Z - toggle vertical auto scroll */
//         auto_scroll_v = !auto_scroll_v;
//         printk(KERN_INFO "HybridScroll: Vertical auto scroll: %s\n", auto_scroll_v ? "ON" : "OFF");
//         break;

//     case 1: /* ESC - reset all scroll */
//         horizontal_offset = 0;
//         vertical_offset = 0;
//         display_hybrid_scroll_safe();
//         printk(KERN_INFO "HybridScroll: All scroll RESET\n");
//         break;

//     case 16: /* Q - demo mode */
//         printk(KERN_INFO "HybridScroll: Demo mode activated\n");
//         auto_scroll_h = true;
//         auto_scroll_v = true;
//         horizontal_offset = 0;
//         vertical_offset = 0;
//         display_hybrid_scroll_safe();
//         break;

//     case 25: /* P - pause all auto scroll */
//         auto_scroll_h = false;
//         auto_scroll_v = false;
//         printk(KERN_INFO "HybridScroll: All auto scroll PAUSED\n");
//         /* Force display update when pausing */
//         display_hybrid_scroll_safe();
//         break;

//     default:
//         /* Log unknown key codes for debugging */
//         printk(KERN_DEBUG "HybridScroll: Unknown key code: %d\n", param->value);
//         break;
//     }

//     return NOTIFY_OK;
// }

// static struct notifier_block keyboard_notifier_block = {
//     .notifier_call = keyboard_notify,
// };

// /* Module initialization with better error handling */
// static int __init hybrid_scroll_init(void)
// {
//     int ret;

//     printk(KERN_INFO "HybridScroll: Module loading (v2.0 - Improved)...\n");

//     /* Initialize mutex */
//     mutex_init(&display_mutex);

//     /* Initialize text buffer */
//     init_default_text();

//     /* Pre-compute fonts for optimization */
//     precompute_transposed_fonts();

//     /* Create workqueue for auto scroll */
//     scroll_wq = create_singlethread_workqueue("hybrid_scroll_wq");
//     if (!scroll_wq)
//     {
//         printk(KERN_ERR "HybridScroll: Failed to create workqueue\n");
//         return -ENOMEM;
//     }

//     /* Initialize delayed work */
//     INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);

//     /* Register keyboard notifier first */
//     ret = register_keyboard_notifier(&keyboard_notifier_block);
//     if (ret)
//     {
//         printk(KERN_ERR "HybridScroll: Failed to register keyboard notifier\n");
//         destroy_workqueue(scroll_wq);
//         return ret;
//     }

//     /* Clear screen and display initial content with delay */
//     msleep(100); /* Allow system to stabilize */
//     oled_clear_screen_fast();
//     msleep(50);
//     display_hybrid_scroll_safe();

//     /* Start auto scroll timer with initial delay */
//     queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed * 2));

//     printk(KERN_INFO "HybridScroll: Module loaded successfully\n");
//     printk(KERN_INFO "HybridScroll: Improved Controls:\n");
//     printk(KERN_INFO "  - Arrow keys: Manual scroll\n");
//     printk(KERN_INFO "  - SPACE: Toggle horizontal auto scroll\n");
//     printk(KERN_INFO "  - Z: Toggle vertical auto scroll\n");
//     printk(KERN_INFO "  - ESC: Reset all scroll\n");
//     printk(KERN_INFO "  - Q: Demo mode (auto scroll both)\n");
//     printk(KERN_INFO "  - P: Pause all auto scroll\n");
//     printk(KERN_INFO "HybridScroll: Scroll speed: %d ms (improved stability)\n", scroll_speed);

//     return 0;
// }

// /* Module cleanup with proper synchronization */
// static void __exit hybrid_scroll_exit(void)
// {
//     printk(KERN_INFO "HybridScroll: Module unloading...\n");

//     module_active = false;

//     /* Unregister keyboard notifier first */
//     unregister_keyboard_notifier(&keyboard_notifier_block);

//     /* Cancel and destroy workqueue */
//     if (scroll_wq)
//     {
//         cancel_delayed_work_sync(&scroll_work);
//         destroy_workqueue(scroll_wq);
//     }

//     /* Wait for any pending display updates */
//     mutex_lock(&display_mutex);

//     /* Clear screen */
//     oled_clear_screen_fast();

//     mutex_unlock(&display_mutex);

//     printk(KERN_INFO "HybridScroll: Module unloaded successfully\n");
// }

// module_init(hybrid_scroll_init);
// module_exit(hybrid_scroll_exit);

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/string.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include "font_chars.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 3 L01 - Hybrid Scroll");
MODULE_DESCRIPTION("Hybrid horizontal and vertical scroll text display");
MODULE_VERSION("3.0");

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
static int scroll_speed = 400;     // Speed in milliseconds
static bool auto_scroll_h = false; // Auto horizontal scroll - mặc định TẮT
static bool auto_scroll_v = false; // Auto vertical scroll - mặc định TẮT
static bool module_active = true;
static bool display_updating = false; // Flag to prevent concurrent display updates

/* Mutex for thread safety */
static DEFINE_MUTEX(display_mutex);

/* Timer for auto scroll */
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;

/* Pre-computed transposed fonts for horizontal display */
static uint8_t transposed_font[40][8];

/* Longest line length for horizontal scroll reset */
static int max_line_length = 0;

/* Function to initialize default text */
static void init_default_text(void)
{
    strcpy(text_buffer[0], "NHOM 3 LOP L01");
    strcpy(text_buffer[1], "TO QUANG VIEN");
    strcpy(text_buffer[2], "BUI DUC KHANH");
    strcpy(text_buffer[3], "NGUYEN THI HONG");
    strcpy(text_buffer[4], "THAN NHAN CHINH");
    strcpy(text_buffer[5], "HYBRID SCROLL V3");
    strcpy(text_buffer[6], "CRASH-FREE DESIGN");
    strcpy(text_buffer[7], "STABLE MODULE");

    total_lines = 8;

    /* Calculate max_line_length once */
    max_line_length = 0;
    for (int i = 0; i < total_lines; i++)
    {
        int len = strlen(text_buffer[i]);
        if (len > max_line_length)
            max_line_length = len;
    }
}

/* Pre-compute transposed fonts for optimized horizontal scroll */
static void precompute_transposed_fonts(void)
{
    int char_idx, i, j;

    printk(KERN_INFO "HybridScroll: Pre-computing transposed fonts...\n");

    for (char_idx = 0; char_idx < 40; char_idx++)
    {
        /* Transposed font for horizontal display */
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

    printk(KERN_INFO "HybridScroll: Font pre-computation completed!\n");
}

/* Optimized clear screen function */
static void oled_clear_screen_fast(void)
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

        /* Add small delay to prevent I2C bus overload */
        udelay(10);
    }
}

/* Draw character at position using pre-computed transposed font */
static void draw_char_horizontal(int x, int page, char c)
{
    int font_index, i;

    if (x >= 128 || x < 0 || page >= 8 || page < 0)
        return;

    font_index = char_to_index(c);
    if (font_index < 0 || font_index >= 40)
        return;

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

    /* Small delay to prevent I2C congestion */
    udelay(5);
}

/* Thread-safe display function */
static void display_hybrid_scroll_safe(void)
{
    int char_pos, display_x;
    int current_line, display_line;

    if (!mutex_trylock(&display_mutex))
    {
        /* If mutex is locked, skip this update to prevent blocking */
        return;
    }

    if (display_updating)
    {
        mutex_unlock(&display_mutex);
        return;
    }

    display_updating = true;

    /* Clear screen */
    oled_clear_screen_fast();

    /* Display lines with vertical offset */
    for (display_line = 0; display_line < MAX_LINES; display_line++)
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
                draw_char_horizontal(display_x, display_line, text_buffer[current_line][char_pos]);
            }
            display_x += 8;
        }
    }

    display_updating = false;
    mutex_unlock(&display_mutex);
}

/* Timer work handler for auto scroll */
static void scroll_work_handler(struct work_struct *work)
{
    if (!module_active)
        return;

    if (auto_scroll_h)
    {
        horizontal_offset += 1; /* Horizontal scroll speed */

        /* Reset when scrolled past the longest line */
        if (horizontal_offset >= (max_line_length * 8 + 128))
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

    /* Only update display if auto scroll is active */
    if (auto_scroll_h || auto_scroll_v)
    {
        display_hybrid_scroll_safe();
    }

    if (module_active)
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
    }
}

/* NEW LOGIC: Enhanced keyboard event handler với logic mới */
static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code != KBD_KEYCODE || !param->down || !module_active)
        return NOTIFY_OK;

    /* Log all key presses for debugging */
    printk(KERN_DEBUG "HybridScroll: Key pressed, code = %d\n", param->value);

    switch (param->value)
    {
    case 1: /* ESC - toggle auto scroll dọc */
        auto_scroll_v = !auto_scroll_v;
        printk(KERN_INFO "HybridScroll: Vertical auto scroll: %s\n", auto_scroll_v ? "ON" : "OFF");
        /* Reset vertical position when enabling auto scroll */
        if (auto_scroll_v)
        {
            vertical_offset = 0;
        }
        display_hybrid_scroll_safe();
        break;

    case 57: /* SPACE - toggle auto scroll ngang */
        auto_scroll_h = !auto_scroll_h;
        printk(KERN_INFO "HybridScroll: Horizontal auto scroll: %s\n", auto_scroll_h ? "ON" : "OFF");
        /* Reset horizontal position when enabling auto scroll */
        if (auto_scroll_h)
        {
            horizontal_offset = 0;
        }
        display_hybrid_scroll_safe();
        break;

    case 103: /* UP arrow - manual scroll up (chỉ khi auto scroll dọc TẮT) */
        if (!auto_scroll_v)
        {
            vertical_offset--;
            if (vertical_offset < 0)
                vertical_offset = total_lines - 1;
            display_hybrid_scroll_safe();
            printk(KERN_INFO "HybridScroll: Manual UP, offset = %d\n", vertical_offset);
        }
        else
        {
            printk(KERN_INFO "HybridScroll: UP ignored - auto scroll vertical is ON. Press ESC to disable.\n");
        }
        break;

    case 108: /* DOWN arrow - manual scroll down (chỉ khi auto scroll dọc TẮT) */
        if (!auto_scroll_v)
        {
            vertical_offset++;
            if (vertical_offset >= total_lines)
                vertical_offset = 0;
            display_hybrid_scroll_safe();
            printk(KERN_INFO "HybridScroll: Manual DOWN, offset = %d\n", vertical_offset);
        }
        else
        {
            printk(KERN_INFO "HybridScroll: DOWN ignored - auto scroll vertical is ON. Press ESC to disable.\n");
        }
        break;

    case 105: /* LEFT arrow - manual scroll left (chỉ khi auto scroll ngang TẮT) */
        if (!auto_scroll_h)
        {
            horizontal_offset -= 8;
            if (horizontal_offset < 0)
                horizontal_offset = 0;
            display_hybrid_scroll_safe();
            printk(KERN_INFO "HybridScroll: Manual LEFT, offset = %d\n", horizontal_offset);
        }
        else
        {
            printk(KERN_INFO "HybridScroll: LEFT ignored - auto scroll horizontal is ON. Press SPACE to disable.\n");
        }
        break;

    case 106: /* RIGHT arrow - manual scroll right (chỉ khi auto scroll ngang TẮT) */
        if (!auto_scroll_h)
        {
            horizontal_offset += 8;
            if (horizontal_offset >= (max_line_length * 8 + 128))
                horizontal_offset = 0;
            display_hybrid_scroll_safe();
            printk(KERN_INFO "HybridScroll: Manual RIGHT, offset = %d\n", horizontal_offset);
        }
        else
        {
            printk(KERN_INFO "HybridScroll: RIGHT ignored - auto scroll horizontal is ON. Press SPACE to disable.\n");
        }
        break;

    case 16: /* Q - demo mode (bật cả auto scroll ngang và dọc) */
        printk(KERN_INFO "HybridScroll: Demo mode activated - both auto scrolls ON\n");
        auto_scroll_h = true;
        auto_scroll_v = true;
        horizontal_offset = 0;
        vertical_offset = 0;
        display_hybrid_scroll_safe();
        break;

    case 25: /* P - pause all (tắt cả auto scroll) */
        auto_scroll_h = false;
        auto_scroll_v = false;
        printk(KERN_INFO "HybridScroll: All auto scroll PAUSED - manual control enabled\n");
        display_hybrid_scroll_safe();
        break;

    case 19: /* R - reset positions */
        horizontal_offset = 0;
        vertical_offset = 0;
        display_hybrid_scroll_safe();
        printk(KERN_INFO "HybridScroll: All positions RESET\n");
        break;

    default:
        /* Log unknown key codes for debugging */
        printk(KERN_DEBUG "HybridScroll: Unknown key code: %d\n", param->value);
        break;
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

    printk(KERN_INFO "HybridScroll: Module loading (v3.0 - Crash-Free Design)...\n");

    /* Initialize mutex */
    mutex_init(&display_mutex);

    /* Initialize text buffer */
    init_default_text();

    /* Pre-compute fonts for optimization */
    precompute_transposed_fonts();

    /* Create workqueue for auto scroll */
    scroll_wq = create_singlethread_workqueue("hybrid_scroll_wq");
    if (!scroll_wq)
    {
        printk(KERN_ERR "HybridScroll: Failed to create workqueue\n");
        return -ENOMEM;
    }

    /* Initialize delayed work */
    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);

    /* Register keyboard notifier */
    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret)
    {
        printk(KERN_ERR "HybridScroll: Failed to register keyboard notifier\n");
        destroy_workqueue(scroll_wq);
        return ret;
    }

    /* Clear screen and display initial content with delay */
    msleep(100); /* Allow system to stabilize */
    oled_clear_screen_fast();
    msleep(50);
    display_hybrid_scroll_safe();

    /* Start auto scroll timer */
    queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));

    printk(KERN_INFO "HybridScroll: Module loaded successfully\n");
    printk(KERN_INFO "HybridScroll: NEW CONTROLS (Crash-Free):\n");
    printk(KERN_INFO "  - ESC: Toggle vertical auto scroll ON/OFF\n");
    printk(KERN_INFO "  - SPACE: Toggle horizontal auto scroll ON/OFF\n");
    printk(KERN_INFO "  - Arrow keys: Manual scroll (only when respective auto scroll is OFF)\n");
    printk(KERN_INFO "  - Q: Demo mode (enable both auto scrolls)\n");
    printk(KERN_INFO "  - P: Pause all auto scrolls (enable manual control)\n");
    printk(KERN_INFO "  - R: Reset all positions\n");
    printk(KERN_INFO "HybridScroll: Default: Both auto scrolls OFF - manual control enabled\n");

    return 0;
}

/* Module cleanup */
static void __exit hybrid_scroll_exit(void)
{
    printk(KERN_INFO "HybridScroll: Module unloading...\n");

    module_active = false;

    /* Unregister keyboard notifier first */
    unregister_keyboard_notifier(&keyboard_notifier_block);

    /* Cancel and destroy workqueue */
    if (scroll_wq)
    {
        cancel_delayed_work_sync(&scroll_work);
        destroy_workqueue(scroll_wq);
    }

    /* Wait for any pending display updates */
    mutex_lock(&display_mutex);

    /* Clear screen */
    oled_clear_screen_fast();

    mutex_unlock(&display_mutex);

    printk(KERN_INFO "HybridScroll: Module unloaded successfully\n");
}

module_init(hybrid_scroll_init);
module_exit(hybrid_scroll_exit);