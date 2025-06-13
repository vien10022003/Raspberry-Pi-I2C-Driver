#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/input.h>

#include <linux/timer.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("To Vien");
MODULE_DESCRIPTION("Kernel module lắng nghe phím bấm");
MODULE_VERSION("1.0");


extern void SSD1306_Write(bool is_cmd, unsigned char data);

//----------------------------------------------------------------------------------

#define MAX_LENGTH 100
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define ITEM_SIZE 4

enum Direction {
    LEFT = 0,
    RIGHT,
    DOWN,
    UP
};

struct game_item {
    unsigned int x;
    unsigned int y;
};

static struct game_item snake[MAX_LENGTH];
static struct game_item snake_food;
static unsigned int snake_size = 4;
static enum Direction snake_dir = RIGHT;

void oled_clear(void) {
    for (int i = 0; i < 8; i++) {
        SSD1306_Write(true, 0xB0 + i); // set page
        SSD1306_Write(true, 0x00);     // lower column
        SSD1306_Write(true, 0x10);     // higher column
        for (int j = 0; j < 128; j++) {
            SSD1306_Write(false, 0x00); // clear pixel
        }
    }
}

void oled_draw_block(unsigned int x, unsigned int y)
{
    int page, i;

    // SSD1306 chia màn hình thành các page 8 pixel chiều cao (0–7)
    // Mỗi page chứa 128 byte ứng với các cột pixel
    page = y / 8;                         // Xác định page
    unsigned char bit_mask = 0x0F << (y % 8); // Vẽ 4 pixel dọc bắt đầu tại y

    for (i = 0; i < 4; i++) {
        if ((x + i) >= 128) continue; // tránh vẽ ngoài màn hình

        // Set vị trí
        SSD1306_Write(true, 0xB0 + page);                  // set page address
        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));      // set lower col
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F)); // set higher col

        // Gửi data
        SSD1306_Write(false, bit_mask); // Vẽ pixel block 4 dòng
    }
}

void update_snake_position(void) {
    for (int i = snake_size - 1; i > 0; i--)
        snake[i] = snake[i - 1];

    switch (snake_dir) {
        case LEFT:  snake[0].x -= ITEM_SIZE; break;
        case RIGHT: snake[0].x += ITEM_SIZE; break;
        case UP:    snake[0].y -= ITEM_SIZE; break;
        case DOWN:  snake[0].y += ITEM_SIZE; break;
    }
}

#include <linux/random.h>

void spawn_food(void) {
    unsigned int i;
    do {
        get_random_bytes(&snake_food.x, sizeof(snake_food.x));
        get_random_bytes(&snake_food.y, sizeof(snake_food.y));
        snake_food.x %= (OLED_WIDTH - ITEM_SIZE);
        snake_food.y %= (OLED_HEIGHT - ITEM_SIZE);
        snake_food.x -= snake_food.x % ITEM_SIZE;
        snake_food.y -= snake_food.y % ITEM_SIZE;
        for (i = 0; i < snake_size; i++) {
            if (snake_food.x == snake[i].x && snake_food.y == snake[i].y)
                break;
        }
    } while (i != snake_size);
}

bool check_collision(void) {
    // chạm tường
    if (snake[0].x >= OLED_WIDTH || snake[0].y >= OLED_HEIGHT)
        return true;

    // chạm thân
    for (int i = 1; i < snake_size; i++) {
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
            return true;
    }

    return false;
}


void game_step(void) {
    update_snake_position();

    if (snake[0].x == snake_food.x && snake[0].y == snake_food.y) {
        if (snake_size < MAX_LENGTH)
            snake_size++;
        spawn_food();
    }

    if (check_collision()) {
        oled_clear();
        // Bạn có thể in ra chữ "Game Over" ở đây
        snake_size = 4;
        snake_dir = RIGHT;
        spawn_food();
    }

    oled_clear();
    for (int i = 0; i < snake_size; i++)
        oled_draw_block(snake[i].x, snake[i].y);
    oled_draw_block(snake_food.x, snake_food.y);
}
static struct timer_list game_timer;
//----------------------------------------------------------------------------------



static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;
    
    if (code == KBD_KEYCODE) {
        // Chỉ xử lý khi phím được nhấn xuống (down = 1)
        if (param->down) {
            printk(KERN_INFO "Key pressed: scancode = %d, keycode = %d\n", 
                   param->value, param->value);
            SSD1306_Write(false, 0x00);
            // In ra ký tự tương ứng nếu là phím chữ cái/số
            if (param->value >= 2 && param->value <= 11) {
                // Phím số 1-0
                char num = '0' + (param->value - 1);
                if (param->value == 11) num = '0'; // Phím 0
                printk(KERN_INFO "Number key: %c\n", num);
            }
            else if (param->value >= 16 && param->value <= 25) {
                // Phím chữ cái Q-P
                char letter = 'Q' + (param->value - 16);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else if (param->value >= 30 && param->value <= 38) {
                // Phím chữ cái A-L
                char letter = 'A' + (param->value - 30);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else if (param->value >= 44 && param->value <= 50) {
                // Phím chữ cái Z-M
                char letter = 'Z' + (param->value - 44);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else {
                // Các phím đặc biệt
                switch (param->value) {
                    case 103: if (snake_dir != DOWN) snake_dir = UP; break;
                    case 108: if (snake_dir != UP) snake_dir = DOWN; break;
                    case 105: if (snake_dir != RIGHT) snake_dir = LEFT; break;
                    case 106: if (snake_dir != LEFT) snake_dir = RIGHT; break;
                    case 1:   printk(KERN_INFO "Special key: ESC\n"); break;
                    case 14:  printk(KERN_INFO "Special key: BACKSPACE\n"); break;
                    case 15:  printk(KERN_INFO "Special key: TAB\n"); break;
                    case 28:  printk(KERN_INFO "Special key: ENTER\n"); break;
                    case 29:  printk(KERN_INFO "Special key: LEFT CTRL\n"); break;
                    case 42:  printk(KERN_INFO "Special key: LEFT SHIFT\n"); break;
                    case 54:  printk(KERN_INFO "Special key: RIGHT SHIFT\n"); break;
                    case 56:  printk(KERN_INFO "Special key: LEFT ALT\n"); break;
                    case 57:  printk(KERN_INFO "Special key: SPACE\n"); break;
                    default:  printk(KERN_INFO "Other key: scancode %d\n", param->value); break;
                }
            }
        }
    }
    
    return NOTIFY_OK;
}

static struct notifier_block keyboard_notifier_block = {
    .notifier_call = keyboard_notify,
};

static int __init keyboard_driver_init(void)
{
    // Khởi tạo timer vòng lặp
    timer_setup(&game_timer, game_step, 0);
    mod_timer(&game_timer, jiffies + msecs_to_jiffies(500));
    // Tiếp tục init

    int ret;
    
    printk(KERN_INFO "Keyboard driver: Loading module\n");
    
    // Đăng ký keyboard notifier
    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret) {
        printk(KERN_ERR "Keyboard driver: Failed to register keyboard notifier\n");
        return ret;
    }
    
    printk(KERN_INFO "Keyboard driver: Module loaded successfully\n");
    return 0;
}

static void __exit keyboard_driver_exit(void)
{
    
    printk(KERN_INFO "Keyboard driver: Unloading module\n");
    
    // Hủy đăng ký keyboard notifier
    unregister_keyboard_notifier(&keyboard_notifier_block);

    // Hủy Timer
    del_timer_sync(&game_timer);
    
    printk(KERN_INFO "Keyboard driver: Module unloaded successfully\n");
}

module_init(keyboard_driver_init);
module_exit(keyboard_driver_exit);
