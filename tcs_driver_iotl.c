// Nguyễn Mạnh Dũng - 22146287
// Nguyễn Lê Duy - 22146285
#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "tcs34725"
#define CLASS_NAME "tcs34725"
#define DEVICE_NAME "tcs34725"

#define TCS34725_ADDR 0x29

//define dia chi thanh ghi
#define tcs_enable       			0x00
#define tcs_atime         			0x01
#define tcs_control       			0x0F
#define tcs_ID       				0x12
#define clear 						0x14
#define tcs_red_low_data        	0x16
#define tcs_green_low_data        	0x18
#define tcs_blue_low_data         	0x1A


//define gia tri su dung
#define command_bit       			0x80  //Bit theo yêu cầu data 

#define red 						0
#define green 						1
#define blue 						2

#define atime_24ms       			0xF6
#define atime_101ms       			0xD5
#define atime_154ms       			0xC0
#define atime_700ms       			0x00

#define gain_x1       				0x00
#define gain_x4       				0x01
#define gain_x16       				0x10
#define gain_x60       				0x11

// IOCTL commands tạo các lệnh để dùng trên user space
#define TCS34725_IOCTL_MAGIC 'm'     // số 'magic' để phân biệt các lệnh IOCTL cho thiết bị này với các lệnh IOCTL của các thiết bị khác.
#define TCS34725_IOCTL_READ_RED _IOR(TCS34725_IOCTL_MAGIC, 1, int)   
#define TCS34725_IOCTL_READ_GREEN _IOR(TCS34725_IOCTL_MAGIC, 2, int)
#define TCS34725_IOCTL_READ_BLUE _IOR(TCS34725_IOCTL_MAGIC, 3, int)

//khai bao bien toan cuc sài trong probe là chính
static struct i2c_client *tcs34725_client;
static struct class* tcs34725_class = NULL;
static struct device* tcs34725_device = NULL;
static int major_number;

//--------------------------------------------------------Ham an de tinh toan gia tri------------------------------------------------
//Ham doc gia tri r g b
static uint16_t read_16(struct i2c_client *client, int reg) {
    uint16_t low, high, data;

    low = i2c_smbus_read_byte_data(client, command_bit | reg);
    if (low < 0) {
        printk(KERN_ERR "Failed to read16 lown");
        return -EIO;
    }

    high = i2c_smbus_read_byte_data(client, command_bit | (reg + 1));
    if (high < 0) {
        printk(KERN_ERR "Failed to read16 highn");
        return -EIO;
    }

    data = (high << 8) | (low);

    return (uint16_t) data;
}
//Ham chuyen gia  tri sang dang 0-255
static int tcs34725_read_color(struct i2c_client *client, int axis) {
    uint16_t c, r, g, b;
    int color_data[3];

    // Doc gia tri clear (mac dinh)
    c = read_16(client, clear);

    // Đọc các giá trị màu từ cảm biến
    r = read_16(client, tcs_red_low_data);
    g = read_16(client, tcs_green_low_data);
    b = read_16(client, tcs_blue_low_data);

    if (c != 0) {
        color_data[0] = (int) r * 255 / c;
        color_data[1] = (int) g * 255 / c;
        color_data[2] = (int) b * 255 / c;
    } else {
        color_data[0] = color_data[1] = color_data[2] = 0;
    }

    return color_data[axis];
}

//-----------------------------------------------Các File đưa lên user space-------------------------
static int tcs34725_open(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725 device openedn");
    // Các bước khởi tạo thêm nếu cần
    return 0;
}

static int tcs34725_release(struct inode *inodep, struct file *filep)
{
    printk(KERN_INFO "TCS34725 device closedn");
    // Các bước dọn dẹp thêm nếu cần
    return 0;
}

static long tcs34725_ioctl(struct file *file, unsigned int cmd, unsigned long arg)
{
    int data;
    int err;

    printk(KERN_INFO "tcs34725_ioctl called with cmd=%u\n", cmd);

    if (!tcs34725_client) {
        printk(KERN_ERR "tcs34725_client is not initialized\n");
        return -ENODEV;
    }

    switch (cmd) {
        case TCS34725_IOCTL_READ_RED:
            printk(KERN_INFO "Reading RED\n");
            data = tcs34725_read_color(tcs34725_client, red);
            break;
        case TCS34725_IOCTL_READ_GREEN:
            printk(KERN_INFO "Reading GREEN\n");
            data = tcs34725_read_color(tcs34725_client, green);
            break;
        case TCS34725_IOCTL_READ_BLUE:
            printk(KERN_INFO "Reading BLUE\n");
            data = tcs34725_read_color(tcs34725_client, blue);
            break;
        default:
            printk(KERN_ERR "Invalid ioctl command\n");
            return -ENOTTY;
    }

    if (data < 0) {
        printk(KERN_ERR "Error reading color data\n");
        return data;
    }

    err = copy_to_user((int __user *)arg, &data, sizeof(data));
    if (err) {
        printk(KERN_ERR "Failed to copy data to user space\n");
        return -EFAULT;
    }

    return 0;
}


static struct file_operations fops = {
    .open = tcs34725_open,
    .unlocked_ioctl = tcs34725_ioctl,
    .release = tcs34725_release,   
};
//---------------------------------------------------Ham de Init cam bien TCS34725-----------------------------------------
static int setupEnable(struct i2c_client *client, int value)
{
    int ret = i2c_smbus_write_byte_data(client, command_bit | tcs_enable, value);
    if (ret < 0) {
        printk(KERN_ERR "TCS34725 enable failedn");
        return ret;
    }
    printk(KERN_INFO "TCS34725 enable set to %dn", value);
    return 0;
}

static int setupAtime(struct i2c_client *client, int value)
{
    int ret = i2c_smbus_write_byte_data(client, command_bit | tcs_atime, value);
    if (ret < 0) {
        printk(KERN_ERR "TCS34725 atime failedn");
        return ret;
    }
    printk(KERN_INFO "TCS34725 atime set to %dn", value);
    return 0;
}

static int setupGain(struct i2c_client *client, int value)
{
    int ret = i2c_smbus_write_byte_data(client, command_bit | tcs_control, value);
    if (ret < 0) {
        printk(KERN_ERR "TCS34725 gain failedn");
        return ret;
    }
    printk(KERN_INFO "TCS34725 gain set to %dn", value);
    return 0;
}

//----------------------------------------Ham bat dau chuong trinh duoi kernel------------------------------
static int tcs34725_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int result;

    printk(KERN_INFO "tcs34725_probe: Startn");

    // Setup Enable
    result = setupEnable(client, 0x1B);
    if (result < 0) {
        printk(KERN_ERR "Failed to setup enablen");
        return result;
    }
    printk(KERN_INFO "setupEnable successfuln");

    // Setup ATIME
    result = setupAtime(client, atime_24ms);
    if (result < 0) {
        printk(KERN_ERR "Failed to setup atimen");
        return result;
    }
    printk(KERN_INFO "setupAtime successfuln");

    // Setup Gain
    result = setupGain(client, gain_x4);
    if (result < 0) {
        printk(KERN_ERR "Failed to setup gainn");
        return result;
    }
    printk(KERN_INFO "setupGain successfuln");

    // Assign the I2C client
    tcs34725_client = client;

    // Register char device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Failed to register a major numbern");
        return major_number;
    }
    printk(KERN_INFO "register_chrdev successful, major_number = %dn", major_number);

    // Create device class
    tcs34725_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(tcs34725_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to register device classn");
        return PTR_ERR(tcs34725_class);
    }
    printk(KERN_INFO "class_create successfuln");

    // Create device
    tcs34725_device = device_create(tcs34725_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(tcs34725_device)) {
        class_destroy(tcs34725_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Failed to create the devicen");
        return PTR_ERR(tcs34725_device);
    }
    printk(KERN_INFO "device_create successfuln");

    printk(KERN_INFO "TCS34725 driver installedn");
    return 0;
}



static void tcs34725_remove(struct i2c_client *client)
{
    device_destroy(tcs34725_class, MKDEV(major_number, 0));
    class_unregister(tcs34725_class);
    class_destroy(tcs34725_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "TCS34725 driver removed\n");
}

////--------------------------------------------------------------Ham mac dinh----------------------------------------------
static const struct of_device_id tcs34725_of_match[] = {
    { .compatible = "taos,tcs34725" },
    { },
};
MODULE_DEVICE_TABLE(of, tcs34725_of_match);

static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(tcs34725_of_match),
    },
    .probe  = tcs34725_probe,
    .remove = tcs34725_remove,
};

static int __init tcs34725_init(void)
{
    printk(KERN_INFO "Initializing TCS34725 drivern");
    return i2c_add_driver(&tcs34725_driver);
}

static void __exit tcs34725_exit(void)
{
    printk(KERN_INFO "Exiting TCS34725 drivern");
    i2c_del_driver(&tcs34725_driver);
}

module_init(tcs34725_init);
module_exit(tcs34725_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("TCS34725 Sensor Driver");




