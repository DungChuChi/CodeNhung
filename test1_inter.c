#include <linux/init.h>         // Thư viện khởi tạo module Linux
#include <linux/module.h>       // Thư viện cần thiết để tạo module
#include <linux/i2c.h>          // Thư viện hỗ trợ giao tiếp I2C
#include <linux/delay.h>        // Thư viện hỗ trợ các hàm delay

#define DRIVER_NAME "tcs34725_driver"   // Định nghĩa tên driver

#define TCS34725_ADDR 0x29              // Địa chỉ I2C của TCS34725
#define TCS34725_REG_ENABLE 0x80        // Thanh ghi ENABLE của TCS34725
#define TCS34725_REG_ATIME 0x81         // Thanh ghi ATIME (integration time)
#define TCS34725_REG_CONTROL 0x8F       // Thanh ghi CONTROL
#define TCS34725_REG_CDATAL 0x94        // Thanh ghi chứa dữ liệu kênh C (clear)

#define TCS34725_ENABLE_AEN 0x02        // Giá trị để enable ADC
#define TCS34725_ENABLE_PON 0x01        // Giá trị để power on thiết bị

#define TCS34725_INTEGRATIONTIME_2_4MS 0xFF // Giá trị thời gian tích hợp 2.4ms

static struct i2c_client *tcs34725_client; // Khai báo biến client I2C

static int tcs34725_read_data(struct i2c_client *client)
{
    u8 buf[8];       // Buffer để lưu dữ liệu đọc từ cảm biến
    u16 c, r, g, b;  // Biến để lưu giá trị màu

    // Đọc dữ liệu màu từ cảm biến
    if (i2c_smbus_read_i2c_block_data(client, TCS34725_REG_CDATAL | 0x80, sizeof(buf), buf) < 0) {
        printk(KERN_ERR "Failed to read color data\n"); // Thông báo lỗi nếu đọc thất bại
        return -EIO; // Trả về mã lỗi
    }

    // Kết hợp các byte cao và thấp để tạo thành giá trị 16-bit
    c = (buf[1] << 8) | buf[0]; // Giá trị kênh Clear
    r = (buf[3] << 8) | buf[2]; // Giá trị kênh Red
    g = (buf[5] << 8) | buf[4]; // Giá trị kênh Green
    b = (buf[7] << 8) | buf[6]; // Giá trị kênh Blue

    // In dữ liệu màu ra log kernel
    printk(KERN_INFO "Color Data: C=%d, R=%d, G=%d, B=%d\n", c, r, g, b);

    return 0; // Trả về 0 nếu đọc thành công
}

static int tcs34725_probe(struct i2c_client *client, const struct i2c_device_id *id)
{
    int ret;

    // Bật nguồn thiết bị và bật ADC
    ret = i2c_smbus_write_byte_data(client, TCS34725_REG_ENABLE, TCS34725_ENABLE_PON);
    if (ret < 0) {
        printk(KERN_ERR "Failed to power on TCS34725\n"); // Thông báo lỗi nếu thất bại
        return ret; // Trả về mã lỗi
    }
    msleep(3); // Chờ 3ms cho thiết bị bật nguồn

    ret = i2c_smbus_write_byte_data(client, TCS34725_REG_ENABLE, TCS34725_ENABLE_PON | TCS34725_ENABLE_AEN);
    if (ret < 0) {
        printk(KERN_ERR "Failed to enable ADC for TCS34725\n"); // Thông báo lỗi nếu thất bại
        return ret; // Trả về mã lỗi
    }

    // Thiết lập thời gian tích hợp
    ret = i2c_smbus_write_byte_data(client, TCS34725_REG_ATIME, TCS34725_INTEGRATIONTIME_2_4MS);
    if (ret < 0) {
        printk(KERN_ERR "Failed to set integration time\n"); // Thông báo lỗi nếu thất bại
        return ret; // Trả về mã lỗi
    }

    // Thiết lập gain (1x gain)
    ret = i2c_smbus_write_byte_data(client, TCS34725_REG_CONTROL, 0x00);
    if (ret < 0) {
        printk(KERN_ERR "Failed to set control register\n"); // Thông báo lỗi nếu thất bại
        return ret; // Trả về mã lỗi
    }

    tcs34725_client = client; // Gán client I2C vào biến toàn cục

    // Đọc dữ liệu từ cảm biến TCS34725
    ret = tcs34725_read_data(client);
    if (ret < 0) {
        return ret; // Trả về mã lỗi nếu đọc dữ liệu thất bại
    }

    printk(KERN_INFO "TCS34725 driver installed\n"); // Thông báo cài đặt driver thành công

    return 0; // Trả về 0 nếu probe thành công
}

static void tcs34725_remove(struct i2c_client *client)
{
    printk(KERN_INFO "TCS34725 driver removed\n"); // Thông báo gỡ bỏ driver
}

static const struct i2c_device_id tcs34725_id[] = {
    { "tcs34725", 0 }, // Định danh thiết bị I2C
    { }
};
MODULE_DEVICE_TABLE(i2c, tcs34725_id); // Tạo bảng thiết bị I2C

static struct i2c_driver tcs34725_driver = {
    .driver = {
        .name   = DRIVER_NAME, // Tên driver
        .owner  = THIS_MODULE, // Chủ sở hữu là module hiện tại
    },
    .probe      = tcs34725_probe, // Hàm probe
    .remove     = tcs34725_remove, // Hàm remove
    .id_table   = tcs34725_id,     // Bảng định danh thiết bị
};

/////// 
static int __init tcs34725_init(void)
{
    printk(KERN_INFO "Initializing TCS34725 driver\n"); // Thông báo khởi tạo driver
    return i2c_add_driver(&tcs34725_driver); // Thêm driver vào hệ thống I2C
}

static void __exit tcs34725_exit(void)
{
    printk(KERN_INFO "Exiting TCS34725 driver\n"); // Thông báo gỡ bỏ driver
    i2c_del_driver(&tcs34725_driver); // Gỡ bỏ driver khỏi hệ thống I2C
}

module_init(tcs34725_init); // Đăng ký hàm khởi tạo module
module_exit(tcs34725_exit); // Đăng ký hàm gỡ bỏ module

MODULE_AUTHOR("Your Name"); // Tác giả của module
MODULE_DESCRIPTION("TCS34725 I2C Client Driver"); // Mô tả ngắn gọn về module
MODULE_LICENSE("GPL"); // Giấy phép GPL
