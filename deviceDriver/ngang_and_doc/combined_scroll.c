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
MODULE_AUTHOR("Group 3 - Combined Scroll");
MODULE_DESCRIPTION("Combined horizontal and vertical scrolling with keyboard control");
MODULE_VERSION("1.0");

extern void SSD1306_Write(bool is_cmd, unsigned char data);

/* Display parameters */
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8
#define MAX_CHARS_PER_LINE 16
#define MAX_LINES 8

/* Scroll modes */
typedef enum {
    SCROLL_MODE_HORIZONTAL = 0,
    SCROLL_MODE_VERTICAL = 1
} scroll_mode_t;

/* Global variables */
static scroll_mode_t current_mode = SCROLL_MODE_HORIZONTAL;
static bool auto_scroll = true;
static bool module_active = true;
static int scroll_speed = 200; // ms

/* Horizontal scroll variables */
static char horizontal_text[] = "NHOM 3 LOP L01: TO QUANG VIEN, BUI DUC KHANH, NGUYEN THI HONG NGAN, THAN NHAN CHINH    ";
static int horizontal_offset = 0;
static int horizontal_text_length;

/* Vertical scroll variables */
static char vertical_text_buffer[MAX_LINES * 2][MAX_CHARS_PER_LINE + 1]; // Extended buffer for scrolling
static int vertical_position = 0;
static int vertical_total_lines = 0;

/* Work queue */
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;

/* Pre-computed transposed fonts */
static uint8_t transposed_font[40][8];

/* Font preprocessing */
static void precompute_transposed_fonts(void)
{
    int char_idx, i, j;

    for (char_idx = 0; char_idx < 40; char_idx++) {
        for (i = 0; i < 8; i++) {
            transposed_font[char_idx][i] = 0;
            for (j = 0; j < 8; j++) {
                if (font_8x8[char_idx][j] & (1 << (7 - i))) {
                    transposed_font[char_idx][i] |= (1 << j);
                }
            }
        }
    }
}

/* Display functions */
static void oled_clear_screen(void)
{
    int page, col;
    
    for (page = 0; page < 8; page++) {
        SSD1306_Write(true, 0xB0 + page);
        SSD1306_Write(true, 0x00);
        SSD1306_Write(true, 0x10);
        
        for (col = 0; col < 128; col++) {
            SSD1306_Write(false, 0x00);
        }
    }
}

static void oled_clear_page(int page)
{
    int col;
    
    SSD1306_Write(true, 0xB0 + page);
    SSD1306_Write(true, 0x00);
    SSD1306_Write(true, 0x10);
    
    for (col = 0; col < 128; col++) {
        SSD1306_Write(false, 0x00);
    }
}

static int char_to_index(char c)
{
    if (c >= 'A' && c <= 'Z') return c - 'A';
    if (c >= '0' && c <= '9') return c - '0' + 26;
    if (c == ' ') return 36;
    if (c >= 'a' && c <= 'z') return c - 'a'; // Convert to uppercase
    return 36; // Default to space
}

static void draw_char_horizontal(int x, int page, char c)
{
    int font_index, i;
    
    if (x >= 128 || x < 0 || page >= 8 || page < 0) return;
    
    font_index = char_to_index(c);
    SSD1306_Write(true, 0xB0 + page);
    
    for (i = 0; i < 8; i++) {
        if ((x + i) >= 128) break;
        
        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F));
        SSD1306_Write(false, transposed_font[font_index][i]);
    }
}

static void draw_char_vertical(char c, int x, int y, bool inverted)
{
    int i, j, font_index;
    unsigned char rotated_data[8] = {0};
    int page = y / 8;
    
    font_index = char_to_index(c);
    
    // Rotate character for vertical display
    for (i = 0; i < 8; i++) {
        for (j = 0; j < 8; j++) {
            if (font_8x8[font_index][j] & (1 << (7 - i))) {
                rotated_data[i] |= (1 << j);
            }
        }
        if (inverted) rotated_data[i] = ~rotated_data[i];
    }
    
    // Write to display
    for (i = 0; i < 8; i++) {
        SSD1306_Write(true, 0xB0 + page);
        SSD1306_Write(true, 0x00 | ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 | (((x + i) >> 4) & 0x0F));
        SSD1306_Write(false, rotated_data[i]);
    }
}

/* Horizontal scrolling display */
static void display_horizontal_scroll(void)
{
    int char_pos = 0;
    int display_x = 0;
    
    oled_clear_page(0);
    display_x = -horizontal_offset;
    
    for (char_pos = 0; char_pos < horizontal_text_length && display_x < 128; char_pos++) {
        if (display_x + 8 > 0) {
            draw_char_horizontal(display_x, 0, horizontal_text[char_pos]);
        }
        display_x += 8;
    }
}

/* Vertical scrolling display */
static void display_vertical_scroll(void)
{
    int i;
    
    oled_clear_screen();
    
    for (i = 0; i < MAX_LINES; i++) {
        int buffer_line = (vertical_position + i) % vertical_total_lines;
        bool is_selected = (i == 0); // First line is highlighted
        
        // Draw each character in the line
        for (int j = 0; vertical_text_buffer[buffer_line][j] != '\0' && j < MAX_CHARS_PER_LINE; j++) {
            draw_char_vertical(vertical_text_buffer[buffer_line][j], j * CHAR_WIDTH, i * CHAR_HEIGHT, is_selected);
        }
    }
}

/* Work handler for auto-scrolling */
static void scroll_work_handler(struct work_struct *work)
{
    if (!module_active) return;
    
    if (auto_scroll) {
        if (current_mode == SCROLL_MODE_HORIZONTAL) {
            horizontal_offset += 4;
            if (horizontal_offset >= (horizontal_text_length * 8 + 128)) {
                horizontal_offset = 0;
            }
            display_horizontal_scroll();
        } else {
            vertical_position++;
            if (vertical_position >= vertical_total_lines) {
                vertical_position = 0;
            }
            display_vertical_scroll();
        }
    }
    
    if (module_active) {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
    }
}

/* Initialize vertical text buffer */
static void init_vertical_text(void)
{
    const char* lines[] = {
        "LAP TRINH DRIVE",
        "C601",
        "NHOM 3",
        "CAC THANH VIEN:",
        "BUI DUC KHANH",
        "CT060119",
        "NGUYEN THI HONG",
        "NGAN CT060229",
        "TO QUANG VIEN",
        "CT060146",
        "THAN NHAN CHINH",
        "CT060205",
        "HET DANH SACH",
        "THANK YOU",
        "GROUP 3 L01",
        "COMPLETED"
    };
    
    vertical_total_lines = sizeof(lines) / sizeof(lines[0]);
    
    for (int i = 0; i < vertical_total_lines; i++) {
        strncpy(vertical_text_buffer[i], lines[i], MAX_CHARS_PER_LINE);
        vertical_text_buffer[i][MAX_CHARS_PER_LINE] = '\0';
    }
}

/* Keyboard event handler */
static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;
    
    if (code == KBD_KEYCODE && param->down) {
        switch (param->value) {
            case 57: // SPACE - toggle auto scroll
                auto_scroll = !auto_scroll;
                printk(KERN_INFO "CombinedScroll: Auto scroll %s\n", auto_scroll ? "ON" : "OFF");
                if (auto_scroll && module_active) {
                    queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_speed));
                }
                break;
                
            case 50: // M key - toggle mode
                current_mode = (current_mode == SCROLL_MODE_HORIZONTAL) ? SCROLL_MODE_VERTICAL : SCROLL_MODE_HORIZONTAL;
                printk(KERN_INFO "CombinedScroll: Mode switched to %s\n", 
                       current_mode == SCROLL_MODE_HORIZONTAL ? "HORIZONTAL" : "VERTICAL");
                if (current_mode == SCROLL_MODE_HORIZONTAL) {
                    display_horizontal_scroll();
                } else {
                    display_vertical_scroll();
                }
                break;
                
            case 103: // UP arrow
                if (current_mode == SCROLL_MODE_VERTICAL) {
                    vertical_position--;
                    if (vertical_position < 0) vertical_position = vertical_total_lines - 1;
                    display_vertical_scroll();
                } else {
                    if (!auto_scroll) {
                        horizontal_offset -= 8;
                        if (horizontal_offset < 0) horizontal_offset = 0;
                        display_horizontal_scroll();
                    }
                }
                break;
                
            case 108: // DOWN arrow
                if (current_mode == SCROLL_MODE_VERTICAL) {
                    vertical_position++;
                    if (vertical_position >= vertical_total_lines) vertical_position = 0;
                    display_vertical_scroll();
                } else {
                    if (!auto_scroll) {
                        horizontal_offset += 8;
                        if (horizontal_offset >= (horizontal_text_length * 8)) 
                            horizontal_offset = horizontal_text_length * 8 - 1;
                        display_horizontal_scroll();
                    }
                }
                break;
                
            case 105: // LEFT arrow - increase speed
                scroll_speed -= 25;
                if (scroll_speed < 50) scroll_speed = 50;
                printk(KERN_INFO "CombinedScroll: Speed increased to %d ms\n", scroll_speed);
                break;
                
            case 106: // RIGHT arrow - decrease speed
                scroll_speed += 25;
                if (scroll_speed > 500) scroll_speed = 500;
                printk(KERN_INFO "CombinedScroll: Speed decreased to %d ms\n", scroll_speed);
                break;
                
            case 1: // ESC - reset
                if (current_mode == SCROLL_MODE_HORIZONTAL) {
                    horizontal_offset = 0;
                    display_horizontal_scroll();
                } else {
                    vertical_position = 0;
                    display_vertical_scroll();
                }
                printk(KERN_INFO "CombinedScroll: Reset to beginning\n");
                break;
                
            case 16: // Q key - show current status
                printk(KERN_INFO "=== COMBINED SCROLL STATUS ===\n");
                printk(KERN_INFO "Mode: %s\n", current_mode == SCROLL_MODE_HORIZONTAL ? "HORIZONTAL" : "VERTICAL");
                printk(KERN_INFO "Auto scroll: %s\n", auto_scroll ? "ON" : "OFF");
                printk(KERN_INFO "Speed: %d ms\n", scroll_speed);
                if (current_mode == SCROLL_MODE_HORIZONTAL) {
                    printk(KERN_INFO "Horizontal offset: %d\n", horizontal_offset);
                } else {
                    printk(KERN_INFO "Vertical position: %d/%d\n", vertical_position, vertical_total_lines);
                }
                break;
        }
    }
    
    return NOTIFY_OK;
}

static struct notifier_block keyboard_notifier_block = {
    .notifier_call = keyboard_notify,
};

/* Module initialization */
static int __init combined_scroll_init(void)
{
    int ret;
    
    printk(KERN_INFO "=== COMBINED SCROLL MODULE ===\n");
    printk(KERN_INFO "Group 3 - Horizontal & Vertical Scrolling\n");
    
    horizontal_text_length = strlen(horizontal_text);
    
    // Pre-compute fonts
    precompute_transposed_fonts();
    
    // Initialize vertical text
    init_vertical_text();
    
    // Initialize OLED
    SSD1306_Write(true, 0xAE); // Display OFF
    SSD1306_Write(true, 0xA8); // Set multiplex ratio
    SSD1306_Write(true, 0x3F); // 64 MUX
    SSD1306_Write(true, 0xD3); // Set display offset
    SSD1306_Write(true, 0x00); // No offset
    SSD1306_Write(true, 0x40); // Set display start line
    SSD1306_Write(true, 0xA1); // Segment remap
    SSD1306_Write(true, 0xC8); // COM scan direction
    SSD1306_Write(true, 0xDA); // Set COM pins
    SSD1306_Write(true, 0x12);
    SSD1306_Write(true, 0x81); // Set contrast
    SSD1306_Write(true, 0x8F); // Higher contrast
    SSD1306_Write(true, 0xA4); // Entire display ON
    SSD1306_Write(true, 0xA6); // Normal display
    SSD1306_Write(true, 0xD5); // Set display clock
    SSD1306_Write(true, 0x80); // Default frequency
    SSD1306_Write(true, 0x8D); // Charge pump
    SSD1306_Write(true, 0x14); // Enable charge pump
    SSD1306_Write(true, 0xAF); // Display ON
    
    // Create workqueue
    scroll_wq = create_singlethread_workqueue("combined_scroll_wq");
    if (!scroll_wq) {
        printk(KERN_ERR "CombinedScroll: Cannot create workqueue\n");
        return -ENOMEM;
    }
    
    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);
    
    // Register keyboard notifier
    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret) {
        printk(KERN_ERR "CombinedScroll: Cannot register keyboard notifier\n");
        destroy_workqueue(scroll_wq);
        return ret;
    }
    
    // Initial display (start with horizontal)
    display_horizontal_scroll();
    
    // Start auto scroll
    queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(2000));
    
    printk(KERN_INFO "CombinedScroll: Module loaded successfully!\n");
    printk(KERN_INFO "Controls:\n");
    printk(KERN_INFO "  SPACE = toggle auto scroll\n");
    printk(KERN_INFO "  M = switch mode (horizontal/vertical)\n");
    printk(KERN_INFO "  UP/DOWN = manual scroll\n");
    printk(KERN_INFO "  LEFT/RIGHT = speed control\n");
    printk(KERN_INFO "  ESC = reset\n");
    printk(KERN_INFO "  Q = show status\n");
    
    return 0;
}

/* Module cleanup */
static void __exit combined_scroll_exit(void)
{
    module_active = false;
    
    unregister_keyboard_notifier(&keyboard_notifier_block);
    cancel_delayed_work_sync(&scroll_work);
    destroy_workqueue(scroll_wq);
    
    oled_clear_screen();
    
    printk(KERN_INFO "CombinedScroll: Module unloaded successfully!\n");
}

module_init(combined_scroll_init);
module_exit(combined_scroll_exit);
