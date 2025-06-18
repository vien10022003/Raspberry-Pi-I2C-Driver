#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/keyboard.h>
#include <linux/notifier.h>
#include <linux/input.h>

#include <linux/workqueue.h>
#include <linux/delay.h>
#include <linux/atomic.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("To Vien");
MODULE_DESCRIPTION("Kernel module lắng nghe phím bấm");
MODULE_VERSION("1.0");

extern void SSD1306_Write(bool is_cmd, unsigned char data);

//----------------------------------------------------------------------------------

// timer
static struct workqueue_struct *game_wq;
static struct delayed_work game_work;
// Flag để dừng work khi cần thiết
static bool stop_game = false;
volatile bool should_restart_snake = false;

#define MAX_LENGTH 10
#define OLED_WIDTH 128
#define OLED_HEIGHT 64
#define ITEM_SIZE 8

enum Direction
{
    LEFT = 0,
    RIGHT,
    DOWN,
    UP
};

struct game_item
{
    unsigned int x;
    unsigned int y;
};

static struct game_item snake[MAX_LENGTH];
static struct game_item snake_food;
static unsigned int INIT_SIZE = 3;
static unsigned int snake_size = INIT_SIZE; // Kích thước ban đầu của rắn
static atomic_t command = ATOMIC_INIT(2); // 1= LEFT, 2=RIGHT, 3=UP, 4=DOWN, 5=RESET, 0=NONE

void oled_clear(void)
{
    for (int i = 0; i < 8; i++)
    {
        SSD1306_Write(true, 0xB0 + i); // set page
        SSD1306_Write(true, 0x00);     // lower column
        SSD1306_Write(true, 0x10);     // higher column
        for (int j = 0; j < 128; j++)
        {
            SSD1306_Write(false, 0x00); // clear pixel
        }
    }
}

void oled_draw_block(unsigned int x, unsigned int y)
{
    int i;

    if (x >= 128 || y >= 64)
        return; // tránh vẽ ngoài màn hình

    // Tính toán page (SSD1306 có 8 page, mỗi page là 8 pixel theo chiều dọc)
    int page = y / 8;

    // Đặt địa chỉ page
    SSD1306_Write(true, 0xB0 + page);

    for (i = 0; i < 8; i++)
    {
        if ((x + i) >= 128)
            continue; // tránh vẽ ngoài màn hình theo chiều ngang

        // Đặt vị trí cột
        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));        // set lower col
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F)); // set higher col

        // Gửi 1 byte với toàn bộ 8 pixel sáng (vì là block 8x8)
        SSD1306_Write(false, 0xFF);
    }
}

void oled_clear_block(unsigned int x, unsigned int y)
{
    int i;

    if (x >= 128 || y >= 64)
        return; // tránh vẽ ngoài màn hình

    // Tính toán page (mỗi page cao 8 pixel)
    int page = y / 8;

    // Đặt địa chỉ page
    SSD1306_Write(true, 0xB0 + page);

    for (i = 0; i < 8; i++)
    {
        if ((x + i) >= 128)
            continue; // tránh vẽ ngoài màn hình theo chiều ngang

        // Đặt vị trí cột
        SSD1306_Write(true, 0x00 + ((x + i) & 0x0F));        // set lower col
        SSD1306_Write(true, 0x10 + (((x + i) >> 4) & 0x0F)); // set higher col

        // Gửi 1 byte với tất cả pixel tắt
        SSD1306_Write(false, 0x00);
    }
}

void update_snake_position(void)
{
    for (int i = snake_size - 1; i > 0; i--)
        snake[i] = snake[i - 1];

    switch (atomic_read(&command))
    {
    case 1:
        snake[0].x -= ITEM_SIZE;
        break;
    case 2:
        snake[0].x += ITEM_SIZE;
        break;
    case 3:
        snake[0].y -= ITEM_SIZE;
        break;
    case 4:
        snake[0].y += ITEM_SIZE;
        break;
    }
}

#include <linux/random.h>

void spawn_food(void)
{
    unsigned int i;
    do
    {
        get_random_bytes(&snake_food.x, sizeof(snake_food.x));
        get_random_bytes(&snake_food.y, sizeof(snake_food.y));
        snake_food.x %= (OLED_WIDTH - ITEM_SIZE);
        snake_food.y %= (OLED_HEIGHT - ITEM_SIZE);
        snake_food.x -= snake_food.x % ITEM_SIZE;
        snake_food.y -= snake_food.y % ITEM_SIZE;
        for (i = 0; i < snake_size; i++)
        {
            if (snake_food.x == snake[i].x && snake_food.y == snake[i].y)
                break;
        }
    } while (i != snake_size);
}

void init_snake(void)
{
    oled_clear();
    snake_size = INIT_SIZE;          // Kích thước ban đầu của rắn
    atomic_set(&command, 2); // Reset command to RIGHT
    stop_game = false;
    snake[0].x = 0; // Vị trí đầu rắn
    snake[0].y = 0; // Vị trí đầu rắn
    spawn_food();
    oled_draw_block(snake_food.x, snake_food.y);
}

bool check_collision(void)
{
    // chạm tường
    if (snake[0].x >= OLED_WIDTH || snake[0].y >= OLED_HEIGHT)
        return true;

    // chạm thân
    for (int i = 1; i < snake_size; i++)
    {
        printk(KERN_INFO "(%u, %u)", snake[i].x, snake[i].y);
        if (snake[0].x == snake[i].x && snake[0].y == snake[i].y)
            return true;
    }
    printk(KERN_INFO "\n");
    return false;
}

static void game_step(void)
{
    update_snake_position();
    struct game_item snake_tail = snake[snake_size - 1]; // Lưu vị trí đuôi rắn để xóa sau

    if (snake[0].x == snake_food.x && snake[0].y == snake_food.y)
    {
        if (snake_size < MAX_LENGTH)
            snake_size++;
        spawn_food();
        oled_draw_block(snake_food.x, snake_food.y);
    }
    else
    {
        // Xóa đuôi rắn
        oled_clear_block(snake_tail.x, snake_tail.y);
    }

    if (check_collision())
    {
        oled_clear();
        stop_game = true;
        printk(KERN_INFO "Game Over! Your score: %u\n", snake_size);
    }

    oled_draw_block(snake[0].x, snake[0].y);
}

// timer
static void game_work_handler(struct work_struct *work)
{
    if (!stop_game)
    {
        if (atomic_read(&command) == 5) // Reset game
        {
            init_snake();
            queue_delayed_work(game_wq, &game_work, msecs_to_jiffies(3000));
            return;
        }
        game_step();
    }
    queue_delayed_work(game_wq, &game_work, msecs_to_jiffies(700));
}
//----------------------------------------------------------------------------------

static int keyboard_notify(struct notifier_block *nblock, unsigned long code, void *_param)
{
    struct keyboard_notifier_param *param = _param;

    if (code == KBD_KEYCODE)
    {
        // Chỉ xử lý khi phím được nhấn xuống (down = 1)
        if (param->down)
        {
            printk(KERN_INFO "Key pressed: scancode = %d, keycode = %d\n",
                   param->value, param->value);
            // In ra ký tự tương ứng nếu là phím chữ cái/số
            if (param->value >= 2 && param->value <= 11)
            {
                // Phím số 1-0
                char num = '0' + (param->value - 1);
                if (param->value == 11)
                    num = '0'; // Phím 0
                printk(KERN_INFO "Number key: %c\n", num);
            }
            else if (param->value >= 16 && param->value <= 25)
            {
                // Phím chữ cái Q-P
                char letter = 'Q' + (param->value - 16);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else if (param->value >= 30 && param->value <= 38)
            {
                // Phím chữ cái A-L
                char letter = 'A' + (param->value - 30);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else if (param->value >= 44 && param->value <= 50)
            {
                // Phím chữ cái Z-M
                char letter = 'Z' + (param->value - 44);
                printk(KERN_INFO "Letter key: %c\n", letter);
            }
            else
            {
                // Các phím đặc biệt
                switch (param->value)
                {
                case 103:
                    if (atomic_read(&command) != 4)
                        atomic_set(&command, 3);
                    break; // up
                case 108:
                    if (atomic_read(&command) != 3)
                        atomic_set(&command, 4);
                    break; // down
                case 105:
                    if (atomic_read(&command) != 2)
                        atomic_set(&command, 1);
                    break; // left
                case 106:
                    if (atomic_read(&command) != 1)
                        atomic_set(&command, 2);
                    break; // right
                case 1:
                    printk(KERN_INFO "Special key: ESC\n");
                    break;
                case 14:
                    printk(KERN_INFO "Special key: BACKSPACE\n");
                    break;
                case 15:
                    printk(KERN_INFO "Special key: TAB\n");
                    break;
                case 28:
                    atomic_set(&command, 5);
                    stop_game = false; // Reset game
                    break;             // ENTER
                case 29:
                    atomic_set(&command, 5);
                    stop_game = false; // Reset game
                    break;             // LEFT CTRL
                case 42:
                    printk(KERN_INFO "Special key: LEFT SHIFT\n");
                    break;
                case 54:
                    printk(KERN_INFO "Special key: RIGHT SHIFT\n");
                    break;
                case 56:
                    printk(KERN_INFO "Special key: LEFT ALT\n");
                    break;
                case 57:
                    printk(KERN_INFO "Special key: SPACE\n");
                    break;
                default:
                    printk(KERN_INFO "Other key: scancode %d\n", param->value);
                    break;
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
    game_wq = create_singlethread_workqueue("game_workqueue");
    INIT_DELAYED_WORK(&game_work, game_work_handler);
    queue_delayed_work(game_wq, &game_work, msecs_to_jiffies(500));
    // Tiếp tục init
    init_snake();

    int ret;

    printk(KERN_INFO "Keyboard driver: Loading module\n");

    // Đăng ký keyboard notifier
    ret = register_keyboard_notifier(&keyboard_notifier_block);
    if (ret)
    {
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
    stop_game = true;
    cancel_delayed_work_sync(&game_work);
    destroy_workqueue(game_wq);

    printk(KERN_INFO "Keyboard driver: Module unloaded successfully\n");
}

module_init(keyboard_driver_init);
module_exit(keyboard_driver_exit);
