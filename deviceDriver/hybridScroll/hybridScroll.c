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
#include <linux/atomic.h>
#include "font_chars.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Group 3 L01 - Hybrid Scroll");
MODULE_DESCRIPTION("Hybrid horizontal and vertical scroll text display - Minimal Interrupt");
MODULE_VERSION("5.0");

extern void SSD1306_Write(bool is_cmd, unsigned char data);

/* OLED display parameters */
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8
#define MAX_CHARS_PER_LINE 25
#define MAX_LINES 12

/* Key event structure for deferred processing */
struct key_event
{
    int keycode;
    struct work_struct work;
};

/* Text buffer for multiple lines */
static char text_buffer[MAX_LINES][MAX_CHARS_PER_LINE + 1];
static int total_lines = 0;

/* Scroll control variables - sử dụng atomic */
static atomic_t horizontal_offset = ATOMIC_INIT(0);
static atomic_t vertical_offset = ATOMIC_INIT(0);
static int scroll_speed = 2000;
static atomic_t auto_scroll_h = ATOMIC_INIT(0); // 0 = OFF, 1 = ON
static atomic_t auto_scroll_v = ATOMIC_INIT(0);
static atomic_t module_active = ATOMIC_INIT(1);
static atomic_t display_busy = ATOMIC_INIT(0);

/* Workqueues - tách biệt keyboard, display và scroll */
static struct workqueue_struct *scroll_wq;
static struct workqueue_struct *display_wq;
static struct workqueue_struct *keyboard_wq; // NEW: riêng cho keyboard events
static struct delayed_work scroll_work;
static struct work_struct display_work;

/* Pre-computed transposed fonts */
static uint8_t transposed_font[40][8];
static int max_line_length = 0;

/* Display update flag */
static atomic_t need_display_update = ATOMIC_INIT(0);

/* Function to initialize default text */
static void init_default_text(void)
{
    strcpy(text_buffer[0], "NHOM 3 LOP L01");
    strcpy(text_buffer[1], "");
    strcpy(text_buffer[2], "TO QUANG VIEN");
    strcpy(text_buffer[3], "CT060146");
    strcpy(text_buffer[4], "BUI DUC KHANH");
    strcpy(text_buffer[5], "CT060119");
    strcpy(text_buffer[6], "NGUYEN THI HONG NGAN");
    strcpy(text_buffer[7], "CT060229");
    strcpy(text_buffer[8], "THAN NHAN CHINH");
    strcpy(text_buffer[9], "CT060205");
    strcpy(text_buffer[10], "");
    strcpy(text_buffer[11], "HYBRID SCROLL V5.0");

    total_lines = 12;
    max_line_length = 0;

    for (int i = 0; i < total_lines; i++)
    {
        int len = strlen(text_buffer[i]);
        if (len > max_line_length)
            max_line_length = len;
    }
}

/* Pre-compute fonts */
static void precompute_transposed_fonts(void)
{
    int char_idx, i, j;

    printk(KERN_INFO "HybridScroll: Pre-computing fonts...\n");

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

    printk(KERN_INFO "HybridScroll: Font computation completed!\n");
}

/* Safe screen clear */
static void oled_clear_screen_safe(void)
{
    int page, col;

    for (page = 0; page < 8; page++)
    {
        if (!atomic_read(&module_active))
            return;

        SSD1306_Write(true, 0xB0 + page);
        SSD1306_Write(true, 0x00);
        SSD1306_Write(true, 0x10);

        for (col = 0; col < 128; col++)
        {
            SSD1306_Write(false, 0x00);
            if (col % 64 == 0)
            {
                udelay(50);
            }
        }
        udelay(100);
    }
}

/* Safe character drawing */
static void draw_char_safe(int x, int page, char c)
{
    int font_index, i;

    if (x >= 128 || x < 0 || page >= 8 || page < 0)
        return;

    if (!atomic_read(&module_active))
        return;

    font_index = char_to_index(c);
    if (font_index < 0 || font_index >= 40)
        return;

    SSD1306_Write(true, 0xB0 + page);

    for (i = 0; i < 8; i++)
    {
        if ((x + i) >= 128 || !atomic_read(&module_active))
            break;

        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F));
        SSD1306_Write(false, transposed_font[font_index][i]);
    }
    udelay(20);
}

/* Display work handler */
static void display_work_handler(struct work_struct *work)
{
    int char_pos, display_x;
    int current_line, display_line;
    int h_offset, v_offset;

    if (!atomic_add_unless(&display_busy, 1, 1))
    {
        return;
    }

    if (!atomic_read(&module_active))
    {
        atomic_set(&display_busy, 0);
        return;
    }

    h_offset = atomic_read(&horizontal_offset);
    v_offset = atomic_read(&vertical_offset);

    oled_clear_screen_safe();

    if (!atomic_read(&module_active))
    {
        atomic_set(&display_busy, 0);
        return;
    }

    for (display_line = 0; display_line < MAX_LINES; display_line++)
    {
        if (!atomic_read(&module_active))
            break;

        current_line = (v_offset + display_line) % total_lines;
        if (current_line >= total_lines)
            continue;

        display_x = -h_offset;

        for (char_pos = 0; char_pos < strlen(text_buffer[current_line]) && display_x < 128; char_pos++)
        {
            if (!atomic_read(&module_active))
                break;

            if (display_x + 8 > 0)
            {
                draw_char_safe(display_x, display_line, text_buffer[current_line][char_pos]);
            }
            display_x += 8;
        }
    }

    atomic_set(&need_display_update, 0);
    atomic_set(&display_busy, 0);
}

/* Request display update - NON-BLOCKING */
static void request_display_update(void)
{
    if (atomic_read(&module_active) && display_wq)
    {
        atomic_set(&need_display_update, 1);
        queue_work(display_wq, &display_work);
    }
}

/* Auto scroll work handler */
static void scroll_work_handler(struct work_struct *work)
{
    if (!atomic_read(&module_active))
        return;

    if (atomic_read(&auto_scroll_h))
    {
        int h_offset = atomic_read(&horizontal_offset);
        h_offset += 8;

        if (h_offset >= (max_line_length * 8 + 128))
            h_offset = 0;

        atomic_set(&horizontal_offset, h_offset);
    }

    if (atomic_read(&auto_scroll_v))
    {
        int v_offset = atomic_read(&vertical_offset);
        v_offset++;

        if (v_offset >= total_lines)
            v_offset = 0;

        atomic_set(&vertical_offset, v_offset);
    }

    if (atomic_read(&auto_scroll_h) || atomic_read(&auto_scroll_v))
    {
        request_display_update();
    }

    if (atomic_read(&module_active))
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
    }
}

/* DEFERRED KEY EVENT HANDLER - chạy trong workqueue */
static void key_work_handler(struct work_struct *work)
{
    struct key_event *key_evt = container_of(work, struct key_event, work);
    int keycode = key_evt->keycode;
    int h_offset, v_offset;

    /* Xử lý key event trong workqueue context - an toàn hơn */
    switch (keycode)
    {
    case 1: /* ESC - toggle vertical auto scroll */
        if (atomic_read(&auto_scroll_v))
        {
            atomic_set(&auto_scroll_v, 0);
            printk(KERN_INFO "HybridScroll: Vertical auto scroll OFF\n");
        }
        else
        {
            atomic_set(&auto_scroll_v, 1);
            atomic_set(&vertical_offset, 0);
            printk(KERN_INFO "HybridScroll: Vertical auto scroll ON\n");
        }
        break;

    case 57: /* SPACE - toggle horizontal auto scroll */
        if (atomic_read(&auto_scroll_h))
        {
            atomic_set(&auto_scroll_h, 0);
            printk(KERN_INFO "HybridScroll: Horizontal auto scroll OFF\n");
        }
        else
        {
            atomic_set(&auto_scroll_h, 1);
            atomic_set(&horizontal_offset, 0);
            printk(KERN_INFO "HybridScroll: Horizontal auto scroll ON\n");
        }
        break;

    case 103: /* UP - manual scroll up */
        if (!atomic_read(&auto_scroll_v))
        {
            v_offset = atomic_read(&vertical_offset);
            v_offset--;
            if (v_offset < 0)
                v_offset = total_lines - 1;
            atomic_set(&vertical_offset, v_offset);
            printk(KERN_INFO "HybridScroll: Manual UP to %d\n", v_offset);
        }
        break;

    case 108: /* DOWN - manual scroll down */
        if (!atomic_read(&auto_scroll_v))
        {
            v_offset = atomic_read(&vertical_offset);
            v_offset++;
            if (v_offset >= total_lines)
                v_offset = 0;
            atomic_set(&vertical_offset, v_offset);
            printk(KERN_INFO "HybridScroll: Manual DOWN to %d\n", v_offset);
        }
        break;

    case 105: /* LEFT - manual scroll left */
        if (!atomic_read(&auto_scroll_h))
        {
            h_offset = atomic_read(&horizontal_offset);
            h_offset -= 8;
            if (h_offset < 0)
                h_offset = 0;
            atomic_set(&horizontal_offset, h_offset);
            printk(KERN_INFO "HybridScroll: Manual LEFT to %d\n", h_offset);
        }
        break;

    case 106: /* RIGHT - manual scroll right */
        if (!atomic_read(&auto_scroll_h))
        {
            h_offset = atomic_read(&horizontal_offset);
            h_offset += 8;
            if (h_offset >= (max_line_length * 8 + 128))
                h_offset = 0;
            atomic_set(&horizontal_offset, h_offset);
            printk(KERN_INFO "HybridScroll: Manual RIGHT to %d\n", h_offset);
        }
        break;

    case 16: /* Q - demo mode */
        atomic_set(&auto_scroll_h, 1);
        atomic_set(&auto_scroll_v, 1);
        atomic_set(&horizontal_offset, 0);
        atomic_set(&vertical_offset, 0);
        printk(KERN_INFO "HybridScroll: Demo mode ON\n");
        break;

    case 25: /* P - pause all */
        atomic_set(&auto_scroll_h, 0);
        atomic_set(&auto_scroll_v, 0);
        printk(KERN_INFO "HybridScroll: All auto scroll OFF\n");
        break;

    case 19: /* R - reset */
        atomic_set(&auto_scroll_h, 0);
        atomic_set(&auto_scroll_v, 0);
        atomic_set(&horizontal_offset, 0);
        atomic_set(&vertical_offset, 0);
        printk(KERN_INFO "HybridScroll: Positions reset\n");
        break;
    }

    /* Request display update */
    request_display_update();

    /* Free allocated memory */
    kfree(key_evt);
}

/* MINIMAL INTERRUPT CONTEXT KEYBOARD HANDLER */
static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;
    struct key_event *key_evt;

    /* MINIMAL PROCESSING - chỉ check cơ bản */
    if (code != KBD_KEYCODE || !param->down || !atomic_read(&module_active))
        return NOTIFY_OK;

    /* Allocate memory for key event - nhanh */
    key_evt = kmalloc(sizeof(struct key_event), GFP_ATOMIC);
    if (!key_evt)
    {
        /* Nếu không allocate được thì skip, không crash */
        return NOTIFY_OK;
    }

    /* Store keycode và setup work */
    key_evt->keycode = param->value;
    INIT_WORK(&key_evt->work, key_work_handler);

    /* Queue work để xử lý sau - INSTANT RETURN */
    if (keyboard_wq)
    {
        queue_work(keyboard_wq, &key_evt->work);
    }
    else
    {
        kfree(key_evt); /* Cleanup nếu workqueue chưa ready */
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

    printk(KERN_INFO "HybridScroll: Minimal Interrupt v5.0 loading...\n");

    init_default_text();
    precompute_transposed_fonts();

    /* Tạo 3 workqueue riêng biệt */
    scroll_wq = create_singlethread_workqueue("hybrid_scroll_wq");
    if (!scroll_wq)
    {
        printk(KERN_ERR "HybridScroll: Failed to create scroll workqueue\n");
        return -ENOMEM;
    }

    display_wq = create_singlethread_workqueue("hybrid_display_wq");
    if (!display_wq)
    {
        printk(KERN_ERR "HybridScroll: Failed to create display workqueue\n");
        destroy_workqueue(scroll_wq);
        return -ENOMEM;
    }

    /* NEW: Riêng workqueue cho keyboard events */
    keyboard_wq = create_singlethread_workqueue("hybrid_keyboard_wq");
    if (!keyboard_wq)
    {
        printk(KERN_ERR "HybridScroll: Failed to create keyboard workqueue\n");
        destroy_workqueue(scroll_wq);
        destroy_workqueue(display_wq);
        return -ENOMEM;
    }

    /* Initialize work structures */
    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);
    INIT_WORK(&display_work, display_work_handler);

    /* Register keyboard notifier */
    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret)
    {
        printk(KERN_ERR "HybridScroll: Failed to register keyboard notifier\n");
        destroy_workqueue(scroll_wq);
        destroy_workqueue(display_wq);
        destroy_workqueue(keyboard_wq);
        return ret;
    }

    /* Initial display */
    msleep(200);
    request_display_update();
    msleep(100);

    /* Start auto scroll timer */
    queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));

    printk(KERN_INFO "HybridScroll: Minimal Interrupt module loaded!\n");
    printk(KERN_INFO "Controls: ESC=VertAuto, SPACE=HorizAuto, Arrows=Manual, Q=Demo, P=Pause, R=Reset\n");
    printk(KERN_INFO "Interrupt processing: MINIMAL (< 10us per key)\n");

    return 0;
}

/* Module cleanup */
static void __exit hybrid_scroll_exit(void)
{
    printk(KERN_INFO "HybridScroll: Unloading...\n");

    atomic_set(&module_active, 0);

    unregister_keyboard_notifier(&keyboard_notifier_block);

    /* Cleanup tất cả workqueues */
    if (scroll_wq)
    {
        cancel_delayed_work_sync(&scroll_work);
        destroy_workqueue(scroll_wq);
    }

    if (display_wq)
    {
        cancel_work_sync(&display_work);
        destroy_workqueue(display_wq);
    }

    if (keyboard_wq)
    {
        flush_workqueue(keyboard_wq); /* Đợi tất cả key events complete */
        destroy_workqueue(keyboard_wq);
    }

    msleep(200);
    oled_clear_screen_safe();

    printk(KERN_INFO "HybridScroll: Unloaded safely!\n");
}

module_init(hybrid_scroll_init);
module_exit(hybrid_scroll_exit);