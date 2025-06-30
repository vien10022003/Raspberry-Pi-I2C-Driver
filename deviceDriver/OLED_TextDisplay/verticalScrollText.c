#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/delay.h>
#include <linux/workqueue.h>
#include <linux/moduleparam.h>
#include <linux/sysfs.h>
#include <linux/device.h>
#include <linux/slab.h> // Add this for kmalloc/kfree
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
#define MAX_TEXT_LENGTH 64    // Maximum text length per line for storage

// Text buffer to store multiple lines
static char text_buffer[MAX_LINES][MAX_TEXT_LENGTH + 1]; // +1 for null terminator

// Scrolling control variables
static int scroll_direction = 0;  // 0: stopped, 1: down, -1: up
static int scroll_speed = 1;      // Lines to scroll per update
static int scroll_interval = 500; // ms between scroll updates
static int virtual_position = 0;  // Virtual position in the text buffer
static int total_lines = 0;       // Total number of lines in the buffer
static bool scrolling_enabled = false;
static int horizontal_offset = 0; // Horizontal offset for selected line

// For sysfs
static struct kobject *scroll_kobj;

// Forward declarations for sysfs handlers
static ssize_t scroll_direction_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t scroll_direction_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t scroll_speed_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t scroll_speed_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t scroll_interval_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t scroll_interval_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t scroll_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t scroll_enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t display_text_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t display_text_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t manual_scroll_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t manual_scroll_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);
static ssize_t horizontal_shift_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf);
static ssize_t horizontal_shift_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count);

// Define sysfs attributes
static struct kobj_attribute scroll_direction_attribute =
    __ATTR(direction, 0664, scroll_direction_show, scroll_direction_store);
static struct kobj_attribute scroll_speed_attribute =
    __ATTR(speed, 0664, scroll_speed_show, scroll_speed_store);
static struct kobj_attribute scroll_interval_attribute =
    __ATTR(interval, 0664, scroll_interval_show, scroll_interval_store);
static struct kobj_attribute scroll_enable_attribute =
    __ATTR(enable, 0664, scroll_enable_show, scroll_enable_store);
static struct kobj_attribute display_text_attribute =
    __ATTR(text, 0664, display_text_show, display_text_store);
static struct kobj_attribute manual_scroll_attribute =
    __ATTR(scroll, 0664, manual_scroll_show, manual_scroll_store);
static struct kobj_attribute horizontal_shift_attribute =
    __ATTR(horizontal_shift, 0664, horizontal_shift_show, horizontal_shift_store);

// Attribute group
static struct attribute *attrs[] = {
    &scroll_direction_attribute.attr,
    &scroll_speed_attribute.attr,
    &scroll_interval_attribute.attr,
    &scroll_enable_attribute.attr,
    &display_text_attribute.attr,
    &manual_scroll_attribute.attr,
    &horizontal_shift_attribute.attr, // Add the new attribute
    NULL,
};

static struct attribute_group attr_group = {
    .attrs = attrs,
};

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

// Function to display a single character at position (x, y) with inversion option
static void display_char_with_inversion(char c, int x, int y, bool inverted)
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

        // If inverted, flip all bits
        if (inverted)
            rotated_data[i] = ~rotated_data[i];
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

// Function to display a string starting at position (x, y) with inversion option
static void display_string_with_inversion(const char *str, int x, int y, bool inverted)
{
    int i;
    int curr_x = x;

    for (i = 0; str[i] != '\0'; i++)
    {
        display_char_with_inversion(str[i], curr_x, y, inverted);
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

// Function to display a string with horizontal offset
static void display_string_with_offset(const char *str, int x, int y, bool inverted, int offset)
{
    int i;
    int curr_x = x;
    int str_len = strlen(str);

    // Skip characters based on offset
    for (i = offset; i < str_len && str[i] != '\0'; i++)
    {
        // Check if we've reached the end of the display width
        if (curr_x + CHAR_WIDTH > OLED_WIDTH)
            break;

        display_char_with_inversion(str[i], curr_x, y, inverted);
        curr_x += CHAR_WIDTH; // Move to the next character position
    }
}

// Original display_char function now calls the new function with inverted=false
static void display_char(char c, int x, int y)
{
    display_char_with_inversion(c, x, y, false);
}

// Original display_string function now calls the new function with inverted=false
static void display_string(const char *str, int x, int y)
{
    display_string_with_inversion(str, x, y, false);
}

// Function to clear the text buffer
static void clear_text_buffer(void)
{
    int i;
    for (i = 0; i < MAX_LINES; i++)
    {
        memset(text_buffer[i], 0, MAX_TEXT_LENGTH + 1);
    }
}

// Function to set a line in the text buffer
static void set_line(int line_num, const char *text)
{
    if (line_num < 0 || line_num >= MAX_LINES)
        return;

    strncpy(text_buffer[line_num], text, MAX_TEXT_LENGTH);
    text_buffer[line_num][MAX_TEXT_LENGTH] = '\0'; // Ensure null termination
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

// Update the display based on current virtual position with first line highlighted
static void update_display_from_position(void)
{
    int i;
    oled_clear();

    // Display MAX_LINES lines starting from virtual_position
    for (i = 0; i < MAX_LINES; i++)
    {
        int buffer_line = (virtual_position + i) % total_lines;

        // First line (selected line) is displayed inverted with horizontal offset
        if (i == 0)
        {
            display_string_with_offset(text_buffer[buffer_line], 0, i * CHAR_HEIGHT, true, horizontal_offset);
            printk(KERN_INFO "VerticalScroll: Selected line %d: %s (offset: %d)\n",
                   buffer_line, text_buffer[buffer_line], horizontal_offset);
        }
        else
        {
            display_string(text_buffer[buffer_line], 0, i * CHAR_HEIGHT);
        }
    }
}

// Work queue handler function for scrolling
static void scroll_work_handler(struct work_struct *work)
{
    if (scrolling_enabled && scroll_direction != 0)
    {
        // Update virtual position based on direction
        virtual_position += scroll_direction * scroll_speed;

        // Handle wraparound
        if (virtual_position < 0)
        {
            virtual_position = total_lines - ((-virtual_position) % total_lines);
        }
        virtual_position %= total_lines;

        // Update the display with the new position
        update_display_from_position();

        // Re-queue the work
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_interval));
    }
}

// Sysfs show/store implementations
static ssize_t scroll_direction_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", scroll_direction);
}

static ssize_t scroll_direction_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int new_direction;

    if (sscanf(buf, "%d", &new_direction) != 1)
        return -EINVAL;

    // Validate direction: -1 (up), 0 (stop), 1 (down)
    if (new_direction < -1 || new_direction > 1)
        return -EINVAL;

    scroll_direction = new_direction;

    // Start scrolling work if enabled and direction is not zero
    if (scrolling_enabled && scroll_direction != 0)
    {
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_interval));
    }

    return count;
}

static ssize_t scroll_speed_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", scroll_speed);
}

static ssize_t scroll_speed_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int new_speed;

    if (sscanf(buf, "%d", &new_speed) != 1)
        return -EINVAL;

    // Validate speed (at least 1)
    if (new_speed < 1)
        return -EINVAL;

    scroll_speed = new_speed;
    return count;
}

static ssize_t scroll_interval_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", scroll_interval);
}

static ssize_t scroll_interval_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int new_interval;

    if (sscanf(buf, "%d", &new_interval) != 1)
        return -EINVAL;

    // Validate interval (at least 100ms)
    if (new_interval < 100)
        return -EINVAL;

    scroll_interval = new_interval;
    return count;
}

static ssize_t scroll_enable_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", scrolling_enabled ? 1 : 0);
}

static ssize_t scroll_enable_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int enable;

    if (sscanf(buf, "%d", &enable) != 1)
        return -EINVAL;

    scrolling_enabled = (enable != 0);

    if (scrolling_enabled && scroll_direction != 0)
    {
        // Start scrolling
        queue_delayed_work(scroll_wq, &scroll_work, msecs_to_jiffies(scroll_interval));
    }
    else
    {
        // Stop scrolling
        cancel_delayed_work_sync(&scroll_work);
    }

    return count;
}

static ssize_t display_text_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int i, len = 0;

    for (i = 0; i < total_lines; i++)
    {
        len += sprintf(buf + len, "%s\n", text_buffer[i]);
    }

    return len;
}

static ssize_t display_text_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    char *temp_buf, *line, *pos;
    int line_num = 0;

    // Make a copy of the buffer since strsep modifies it
    temp_buf = kmalloc(count + 1, GFP_KERNEL);
    if (!temp_buf)
        return -ENOMEM;

    memcpy(temp_buf, buf, count);
    temp_buf[count] = '\0';

    // Clear the text buffer
    clear_text_buffer();

    // Parse lines separated by newline
    pos = temp_buf;
    while ((line = strsep(&pos, "\n")) != NULL && line_num < MAX_LINES * 2)
    {
        // Skip empty lines
        if (strlen(line) > 0)
        {
            set_line(line_num, line);
            line_num++;
        }
    }

    total_lines = line_num > 0 ? line_num : 1; // Ensure at least one line
    virtual_position = 0;                      // Reset position to top

    // Update display
    update_display_from_position();

    kfree(temp_buf);
    return count;
}

// Manual scrolling functions
static void scroll_up(void)
{
    virtual_position--;

    // Handle wraparound when scrolling up
    if (virtual_position < 0)
        virtual_position = total_lines - 1;

    // Update the display with the new position
    update_display_from_position();

    printk(KERN_INFO "VerticalScroll: Manually scrolled up to position %d\n", virtual_position);
}

static void scroll_down(void)
{
    virtual_position++;

    // Handle wraparound when scrolling down
    if (virtual_position >= total_lines)
        virtual_position = 0;

    // Update the display with the new position
    update_display_from_position();

    printk(KERN_INFO "VerticalScroll: Manually scrolled down to position %d\n", virtual_position);
}

// Horizontal shift sysfs handlers
static ssize_t horizontal_shift_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int selected_line = virtual_position % total_lines;
    int max_offset = strlen(text_buffer[selected_line]) - MAX_CHARS_PER_LINE;
    if (max_offset < 0)
        max_offset = 0;

    return sprintf(buf, "Horizontal offset: %d (max: %d)\nLine: %s\nWrite '1' to shift left\n",
                   horizontal_offset, max_offset, text_buffer[selected_line]);
}

static ssize_t horizontal_shift_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    int shift_value;
    int selected_line = virtual_position % total_lines;
    int str_len = strlen(text_buffer[selected_line]);
    int max_offset = str_len - MAX_CHARS_PER_LINE;

    if (max_offset < 0)
        max_offset = 0;

    if (sscanf(buf, "%d", &shift_value) != 1)
        return -EINVAL;

    if (shift_value == 1)
    {
        // Shift left (increase offset)
        horizontal_offset++;

        // Wrap around if we exceed the maximum offset
        if (horizontal_offset > max_offset)
            horizontal_offset = 0;

        // Update the display
        update_display_from_position();

        printk(KERN_INFO "VerticalScroll: Horizontal shift - offset now: %d\n", horizontal_offset);
    }
    else
    {
        return -EINVAL; // Only accept value 1
    }

    return count;
}

// Manual scroll sysfs handlers
static ssize_t manual_scroll_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    int selected_line = virtual_position % total_lines;
    return sprintf(buf, "Selected line: %d - %s\nHorizontal offset: %d\nWrite 'up' or 'down' to scroll one line\n",
                   selected_line, text_buffer[selected_line], horizontal_offset);
}

static ssize_t manual_scroll_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    if (strncmp(buf, "up", 2) == 0)
    {
        horizontal_offset = 0; // Reset horizontal offset when changing lines
        scroll_up();
    }
    else if (strncmp(buf, "down", 4) == 0)
    {
        horizontal_offset = 0; // Reset horizontal offset when changing lines
        scroll_down();
    }
    else
    {
        return -EINVAL; // Invalid command
    }

    return count;
}

static int __init vertical_scroll_init(void)
{
    printk(KERN_INFO "VerticalScroll: Module loaded successfully\n");
    printk(KERN_INFO "VerticalScroll: Text to display: %s\n", scroll_text);

    // Clear the text buffer
    clear_text_buffer();

    // Populate the buffer with 8 lines of text (now with full text)
    set_line(0, "LAP TRINH DRIVER KERNEL");
    set_line(1, "C601 - HOC VIEN KY THUAT MAT MA");
    set_line(2, "NHOM 3 - BAI CUOI KY");
    set_line(3, "CAC THANH VIEN TRONG NHOM GOM:");
    set_line(4, "BUI DUC KHANH - CT060119");
    set_line(5, "NGUYEN THI HONG NGAN - CT060229");
    set_line(6, "TO QUANG VIEN - CT060146");
    set_line(7, "THAN NHAN CHINH - CT060205");

    total_lines = 8; // Initialize total lines

    // Display all lines on the OLED (use update function to show highlighting)
    update_display_from_position();

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

    // Create sysfs entries
    scroll_kobj = kobject_create_and_add("oled_scroll", kernel_kobj);
    if (!scroll_kobj)
    {
        printk(KERN_ERR "VerticalScroll: Failed to create kobject\n");
        destroy_workqueue(scroll_wq);
        return -ENOMEM;
    }

    // Create the files associated with this kobject
    if (sysfs_create_group(scroll_kobj, &attr_group))
    {
        printk(KERN_ERR "VerticalScroll: Failed to create sysfs group\n");
        kobject_put(scroll_kobj);
        destroy_workqueue(scroll_wq);
        return -ENOMEM;
    }

    printk(KERN_INFO "VerticalScroll: OLED I2C initialized with sysfs interface\n");
    return 0;
}

static void __exit vertical_scroll_exit(void)
{
    // Set flag to stop scrolling
    stop_scrolling = true;
    scrolling_enabled = false;

    // Cleanup workqueue
    if (scroll_wq)
    {
        cancel_delayed_work_sync(&scroll_work);
        destroy_workqueue(scroll_wq);
    }

    // Remove sysfs entries
    kobject_put(scroll_kobj);

    // Clear the display when unloading
    oled_clear();

    printk(KERN_INFO "VerticalScroll: Module unloaded successfully\n");
}

module_init(vertical_scroll_init);
module_exit(vertical_scroll_exit);