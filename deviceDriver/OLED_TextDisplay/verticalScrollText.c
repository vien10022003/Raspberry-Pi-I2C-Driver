#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h>
#include "font_chars.h"

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Vertical scrolling text display on OLED");
MODULE_VERSION("1.0");

// External function from the OLED driver
extern void SSD1306_Write(bool is_cmd, unsigned char data);

// Text to scroll
static char *scroll_text = "HELLO WORLD";
module_param(scroll_text, charp, 0644);
MODULE_PARM_DESC(scroll_text, "Text to scroll vertically");

// Workqueue for animation
static struct workqueue_struct *scroll_wq;
static struct delayed_work scroll_work;
static bool stop_scrolling = false;

// OLED display parameters
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define CHAR_WIDTH 8
#define CHAR_HEIGHT 8
#define MAX_CHARS_PER_LINE 16 // 128/8 = 16 characters per line
#define MAX_LINES 8           // 64/8 = 8 lines on the display

// Text buffer to store multiple lines
static char text_buffer[MAX_LINES][MAX_CHARS_PER_LINE + 1]; // +1 for null terminator

// Function to clear the OLED display
static void oled_clear(void)
{
    int i, j;
    printk(KERN_INFO "VerticalScroll: Clearing display\n");

    for (i = 0; i < 8; i++) // 8 pages
    {
        SSD1306_Write(true, 0xB0 + i); // Set page address
        SSD1306_Write(true, 0x00);     // Set lower column address
        SSD1306_Write(true, 0x10);     // Set higher column address

        for (j = 0; j < 128; j++) // 128 columns
        {
            SSD1306_Write(false, 0x00); // Clear each pixel
        }
    }
}

// Function to display a single character at position (x, y)
static void display_char(char c, int x, int y)
{
    int i, j;
    int font_index;
    unsigned char rotated_data[8] = {0}; // For storing rotated character data

    // Get the font index for the character
    if (c >= 'A' && c <= 'Z')
        font_index = c - 'A'; // A-Z are at indices 0-25
    else if (c >= '0' && c <= '9')
        font_index = c - '0' + 26; // 0-9 are at indices 26-35
    else if (c == ' ')
        font_index = 36; // Space is at index 36
    else
        return; // Unsupported character

    // Calculate which page to start at
    int page = y / 8;

    // Transform font data for vertical display (90 degree rotation)
    for (i = 0; i < 8; i++)
    { // For each column in output
        for (j = 0; j < 8; j++)
        { // For each bit in the column
            // Check if corresponding bit is set in original font
            // This transformation maps horizontal rows to vertical columns
            if (font_8x8[font_index][j] & (1 << (7 - i)))
            {
                // Set the corresponding bit in our column data
                rotated_data[i] |= (1 << j);
            }
        }
    }

    // Write each column of the rotated character to display
    for (i = 0; i < 8; i++)
    {
        // Set the page address
        SSD1306_Write(true, 0xB0 + page);

        // Set column address for this column
        SSD1306_Write(true, 0x00 | ((x + i) & 0x0F));
        SSD1306_Write(true, 0x10 | (((x + i) >> 4) & 0x0F));

        // Write the data for this column
        SSD1306_Write(false, rotated_data[i]);
    }
}

// Function to display a string starting at position (x, y)
static void display_string(const char *str, int x, int y)
{
    int i;
    int curr_x = x;

    for (i = 0; str[i] != '\0'; i++)
    {
        display_char(str[i], curr_x, y);
        curr_x += CHAR_WIDTH; // Move to the next character position

        // Wrap to the next line if we reach the end of the display
        if (curr_x >= OLED_WIDTH)
        {
            curr_x = 0;
            y += CHAR_HEIGHT;

            // If we've gone past the bottom of the display, stop
            if (y >= OLED_HEIGHT)
                break;
        }
    }
}

// Function to clear the text buffer
static void clear_text_buffer(void)
{
    int i;
    for (i = 0; i < MAX_LINES; i++)
    {
        memset(text_buffer[i], 0, MAX_CHARS_PER_LINE + 1);
    }
}

// Function to set a line in the text buffer
static void set_line(int line_num, const char *text)
{
    if (line_num < 0 || line_num >= MAX_LINES)
        return;

    strncpy(text_buffer[line_num], text, MAX_CHARS_PER_LINE);
    text_buffer[line_num][MAX_CHARS_PER_LINE] = '\0'; // Ensure null termination
}

// Function to display the entire text buffer on the OLED
static void display_text_buffer(void)
{
    int i;

    // Clear the display first
    oled_clear();

    // Display each line at the appropriate Y position
    for (i = 0; i < MAX_LINES; i++)
    {
        display_string(text_buffer[i], 0, i * CHAR_HEIGHT);
    }
}

// Work queue handler function (stub for now)
static void scroll_work_handler(struct work_struct *work)
{
    // Will implement scrolling logic in next phase
    if (!stop_scrolling)
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(100));
    }
}

static int __init vertical_scroll_init(void)
{
    printk(KERN_INFO "VerticalScroll: Module loaded successfully\n");
    printk(KERN_INFO "VerticalScroll: Text to display: %s\n", scroll_text);

    // Clear the text buffer
    clear_text_buffer();

    // Populate the buffer with 8 lines of text
    set_line(0, "LINE 1");
    set_line(1, "LINE 2");
    set_line(2, "LINE 3");
    set_line(3, "LINE 4");
    set_line(4, "LINE 5");
    set_line(5, "LINE 6");
    set_line(6, "LINE 7");
    set_line(7, "LINE 8");

    // Display all lines on the OLED
    display_text_buffer();

    printk(KERN_INFO "VerticalScroll: Text buffer displayed\n");

    // Initialize workqueue for future animation
    scroll_wq = create_singlethread_workqueue("scroll_workqueue");
    if (!scroll_wq)
    {
        printk(KERN_ERR "VerticalScroll: Failed to create workqueue\n");
        return -ENOMEM;
    }

    // Initialize the delayed work
    INIT_DELAYED_WORK(&scroll_work, scroll_work_handler);

    printk(KERN_INFO "VerticalScroll: OLED I2C initialized\n");
    return 0;
}

static void __exit vertical_scroll_exit(void)
{
    // Set flag to stop scrolling
    stop_scrolling = true;

    // Cleanup workqueue
    if (scroll_wq)
    {
        cancel_delayed_work_sync(&scroll_work);
        destroy_workqueue(scroll_wq);
    }

    // Clear the display when unloading
    oled_clear();

    printk(KERN_INFO "VerticalScroll: Module unloaded successfully\n");
}

module_init(vertical_scroll_init);
module_exit(vertical_scroll_exit);