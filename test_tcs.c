#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h> // Include errno header

#define DEVICE_PATH "/dev/tcs34725"
#define TCS34725_IOCTL_MAGIC 'm'
#define TCS34725_IOCTL_READ_RED _IOR(TCS34725_IOCTL_MAGIC, 1, int)
#define TCS34725_IOCTL_READ_GREEN _IOR(TCS34725_IOCTL_MAGIC, 2, int)
#define TCS34725_IOCTL_READ_BLUE _IOR(TCS34725_IOCTL_MAGIC, 3, int)

int main()
{
	int fd;
    int data;
	
	fd = open(DEVICE_PATH, O_RDWR);
	if (fd < 0) {
        perror("Failed to open the device");
        return errno;
    }
	
	// Red 
    if (ioctl(fd, TCS34725_IOCTL_READ_RED, &data) < 0) {
        perror("Failed to RED");
        close(fd);
        return errno;
    }
    printf("Red: %d\n", data);
	
	// Green
    if (ioctl(fd, TCS34725_IOCTL_READ_GREEN, &data) < 0) {
        perror("Failed to GREEN");
        close(fd);
        return errno;
    }
    printf("GREEN: %d\n", data);
	
	// Blue
    if (ioctl(fd, TCS34725_IOCTL_READ_BLUE, &data) < 0) {
        perror("Failed to BLUE");
        close(fd);
        return errno;
    }
    printf("BLUE: %d\n", data);
	
	close(fd);
	return 0;
}