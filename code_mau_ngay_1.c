#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <math.h>  //gcc ... -lwiringPi -lm
#include <i2c/smbus.h> // add "-li2c" when compile

#define I2C_addr  					(0x29)
#define command_bit       			0x80
#define enable_PON					0x01
#define enable_AEN					0x02
#define enable_AIEN					0x10
#define enable_WEN					0x08


//define dia chi thanh ghi
#define tcs_enable       			0x00
#define tcs_atime         			0x01
#define tcs_control       			0x0F
#define tcs_ID       				0x12
#define tcs_clear_low_data        	0x14
#define tcs_red_low_data        	0x16
#define tcs_green_low_data        	0x18
#define tcs_blue_low_data        	0x1A

//Khai bao bien toan cuc
int tcs;
int tcs_1;
uint16_t c;
uint16_t r;
uint16_t g;
uint16_t b;
int R;
int G;
int B;

//Ham write8
void write8(int file, uint8_t reg, uint8_t value) {
    // Write byte operation
    if (i2c_smbus_write_byte_data(file, command_bit | reg, value) < 0) {
        perror("Failed to write byte to the i2c bus_write8");
    }
}
//Ham read8
uint8_t read8(int file, uint8_t reg) {
    int32_t res = i2c_smbus_read_byte_data(file, command_bit | reg);
    if (res < 0) {
        perror("Failed to read byte from the i2c bus_read8");
        exit(1);
    }
    return (uint8_t) res;
}
//Ham read16
uint16_t read16(int file, uint8_t reg) {
    int32_t res = i2c_smbus_read_word_data(file, command_bit | reg);
    if (res < 0) {
        perror("Failed to read word from the i2c bus_read16");
        exit(1);
    }
    return (uint16_t) res;
}

//Init 
void Init()
{
	tcs = open("/dev/i2c-1", O_RDWR);
	if(tcs < 0){
        printf("Can't load I2C driver \n");
        exit(1);
    }
	
	if (ioctl(tcs, I2C_SLAVE, I2C_addr) < 0) {      
		printf("Failed to acquire bus access and/or talk to slave \n");
        exit(1);
    }	
}


void setUpEnable()
{
	write8(tcs,tcs_enable,0x1B);
	
}

void setUpAtime(uint8_t value)
{
	write8(tcs,tcs_atime,value);
}

void setUpGain(uint8_t value)
{
	write8(tcs,tcs_control,value);
}

void read_color()
{
	 // Đọc các giá trị màu từ cảm biến
    /*c = read_value(tcs_clear_low_data);
    r = read_value(tcs_red_low_data);
    g = read_value(tcs_green_low_data);
    b = read_value(tcs_blue_low_data);*/
	c = read16(tcs, tcs_clear_low_data);
    r = read16(tcs, tcs_red_low_data);
    g = read16(tcs, tcs_green_low_data);
    b = read16(tcs, tcs_blue_low_data);		
}

void read_RGB()
{
	read_color();
	if (c==0)
	{
		R=0;G=0;B=0;
	}
	else{
		R = (int)r * 255 / c;
        G = (int)g * 255 / c;
        B = (int)b * 255 / c;
	}
}

int main(void)
{
	Init();
	setUpEnable();
	setUpAtime(0xF6);
	setUpGain(0x00);
	while(1)
	{
		read_RGB();
		
		printf("R: %d, G: %d, B: %d \n", R,G,B);
		
		sleep(1);
	}
	
	return 0;
	
}