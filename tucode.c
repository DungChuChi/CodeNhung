#include <linux/init.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/uaccess.h>

#define DRIVER_NAME "TCS34725_driver"
#define CLASS_NAME "TCS34725"
#define DEVICE_NAME "TCS34725"

// Địa chỉ thanh ghi
#define ENABLE 0x00
#define ATIME 0x01
#define CONTROL 0x0F
#define TCS34725_CDATAL 0x14
#define TCS34725_CDATAH 0x15
#define TCS34725_RDATAL 0x16
#define TCS34725_RDATAH 0x17
#define TCS34725_GDATAL 0x18
#define TCS34725_GDATAH 0x19
#define TCS34725_BDATAL 0x1A
#define TCS34725_BDATAH 0x1B
#define COMMAND_BIT 0x80

// Lệnh IOCTL
#define IOCTL_MAGIC 'm'
#define READ_R _IOR(IOCTL_MAGIC, 1, int)
#define READ_G _IOR(IOCTL_MAGIC, 2, int)
#define READ_B _IOR(IOCTL_MAGIC, 3, int)

// Biến toàn cục cho client I2C và thiết bị
static struct i2c_client *tcs34725_client;
static struct class *tcs34725_class = NULL;
static struct device *tcs34725_device = NULL;
static int major_number;

// Hàm cấu hình
static int configure_tcs34725(struct i2c_client *client) {
    int ret;

    // Bật nguồn cho thiết bị
    ret = i2c_smbus_write_byte_data(client, COMMAND_BIT | ENABLE, 0x1B);
    if (ret < 0) {
        printk(KERN_ERR "Không thể bật nguồn\n");
        return ret; 
    }

    // Bật ADC
    ret = i2c_smbus_write_byte_data(client, COMMAND_BIT | ENABLE, 0x03);
    if (ret < 0) {
        printk(KERN_ERR "Không thể bật ADC\n");
        return ret;
    }

    // Thiết lập thời gian tích lũy (ví dụ: 24ms)
    ret = i2c_smbus_write_byte_data(client, COMMAND_BIT | ATIME, 0xF6);
    if (ret < 0) {
        printk(KERN_ERR "Không thể thiết lập ATIME\n");
        return ret;
    }

    // Thiết lập độ lợi (ví dụ: 4x)
    ret = i2c_smbus_write_byte_data(client, COMMAND_BIT | CONTROL, 0x01);
    if (ret < 0) {
        printk(KERN_ERR "Không thể thiết lập CONTROL\n");
        return ret;
    }

    printk(KERN_INFO "Cấu hình TCS34725 thành công\n");
    return 0;
}

// Hàm đọc dữ liệu 16-bit từ thanh ghi
static int read_data_16bit(struct i2c_client *client, uint8_t reg_high, uint8_t reg_low) {
    int high, low;

    low = i2c_smbus_read_byte_data(client, COMMAND_BIT | reg_low);
    high = i2c_smbus_read_byte_data(client, COMMAND_BIT | reg_high);
    if (low < 0 || high < 0) {
        return -1;
    }

    return (high << 8) | low;
}

// Hàm lấy giá trị màu RGB
static int get_color(struct i2c_client *client, int color) {
    int r, g, b, c;
    int datacolor[3];

    c = read_data_16bit(client, TCS34725_CDATAH, TCS34725_CDATAL);
    r = read_data_16bit(client, TCS34725_RDATAH, TCS34725_RDATAL);
    g = read_data_16bit(client, TCS34725_GDATAH, TCS34725_GDATAL);
    b = read_data_16bit(client, TCS34725_BDATAH, TCS34725_BDATAL);

    if (c > 0) {
        datacolor[0] = (r * 255) / c;
        datacolor[1] = (g * 255) / c;
        datacolor[2] = (b * 255) / c;
    } else {
        datacolor[0] = datacolor[1] = datacolor[2] = 0;
    }

    return datacolor[color];
}

// Hàm IOCTL
static long tcs34725_ioctl(struct file *file, unsigned int cmd, unsigned long arg) {
    int data;

    switch (cmd) {
        case READ_R:
            data = get_color(tcs34725_client, 0);
            break;
        case READ_G:
            data = get_color(tcs34725_client, 1);
            break;
        case READ_B:
            data = get_color(tcs34725_client, 2);
            break;
        default:
            return -EINVAL;
    }

    if (copy_to_user((int __user *)arg, &data, sizeof(data))) {
        return -EFAULT;
    }

    return 0;
}

// Hàm mở file
static int tcs34725_open(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Thiết bị TCS34725 đã được mở\n");
    return 0;
}

// Hàm đóng file
static int tcs34725_release(struct inode *inodep, struct file *filep) {
    printk(KERN_INFO "Thiết bị TCS34725 đã được đóng\n");
    return 0;
}

// Cấu trúc file operations
static struct file_operations fops = {
    .open = tcs34725_open,
    .unlocked_ioctl = tcs34725_ioctl,
    .release = tcs34725_release,
};

// Hàm probe
static int tcs34725_probe(struct i2c_client *client, const struct i2c_device_id *id) {
    int ret;

    ret = configure_tcs34725(client);
    if (ret < 0) {
        return ret;
    }

    tcs34725_client = client;

    // Đăng ký char device
    major_number = register_chrdev(0, DEVICE_NAME, &fops);
    if (major_number < 0) {
        printk(KERN_ERR "Không thể đăng ký số chính\n");
        return major_number;
    }

    tcs34725_class = class_create(THIS_MODULE, CLASS_NAME);
    if (IS_ERR(tcs34725_class)) {
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Không thể đăng ký class thiết bị\n");
        return PTR_ERR(tcs34725_class);
    }

    tcs34725_device = device_create(tcs34725_class, NULL, MKDEV(major_number, 0), NULL, DEVICE_NAME);
    if (IS_ERR(tcs34725_device)) {
        class_destroy(tcs34725_class);
        unregister_chrdev(major_number, DEVICE_NAME);
        printk(KERN_ERR "Không thể tạo thiết bị\n");
        return PTR_ERR(tcs34725_device);
    }

    printk(KERN_INFO "Driver TCS34725 đã được cài đặt\n");
    return 0;
}

// Hàm remove
static int tcs34725_remove(struct i2c_client *client) {
    device_destroy(tcs34725_class, MKDEV(major_number, 0));
    class_unregister(tcs34725_class);
    class_destroy(tcs34725_class);
    unregister_chrdev(major_number, DEVICE_NAME);

    printk(KERN_INFO "Driver TCS34725 đã được gỡ bỏ\n");
    return 0;
}

// Bảng match ID
static const struct of_device_id tcs34725_of_match[] = {
    { .compatible = "taos,tcs34725", },
    { },
};
MODULE_DEVICE_TABLE(of, tcs34725_of_match);

// Cấu trúc driver I2C
static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name   = DRIVER_NAME,
        .owner  = THIS_MODULE,
        .of_match_table = of_match_ptr(tcs34725_of_match),
    },
    .probe      = tcs34725_probe,
    .remove     = tcs34725_remove,
};

// Hàm khởi tạo module
static int __init tcs34725_init(void) {
    printk(KERN_INFO "Khởi tạo driver TCS34725\n");
    return i2c_add_driver(&tcs34725_driver);
}

// Hàm thoát module
static void __exit tcs34725_exit(void) {
    printk(KERN_INFO "Thoát driver TCS34725\n");
    i2c_del_driver(&tcs34725_driver);
}

module_init(tcs34725_init);
module_exit(tcs34725_exit);

MODULE_AUTHOR("Your Name");
MODULE_DESCRIPTION("Driver I2C TCS34725 với giao diện IOCTL");
MODULE_LICENSE("GPL");
