#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <string.h>

#define DEV_LCD "/dev/lcd1602"
#define SYS_LCD	   "/sys/bus/i2c/devices/i2c-1/1-0027/sys_lcd"

#define SEND_CMD	1
#define SEND_DATA	3

//unsigned char data1[]="ABCDEFGHIJKLMNOPQ";
//unsigned char data1[]="Hello!Zhang Rui! ";
unsigned char data1[]="Hi!Zhang XiaoMan!";
unsigned char data2[]="Have a happy day!";
//unsigned char data2[]="1234567890abcdefg";

unsigned int lcd_wr_string(int fd , unsigned char x , unsigned y , char *string  );
unsigned int lcd_set_position(int fd , unsigned char x , unsigned char y);
void lcd_init(int fd );

int main(int argc, char * argv[])
{
	int fd;
	unsigned int i=0 ;
	int err = 0 ;
	fd = open(DEV_LCD , O_RDWR);
	if(fd < 0){
		printf("open file %s error\n",DEV_LCD);
	}

	printf("sending data \n");
	sleep(1);
	lcd_wr_string(fd , 0 , 0  , data1 );
	lcd_wr_string(fd , 0 , 1  , data2 );
	if(err<0){
		printf("error- %d send data %d \n",err, i);
		return 1 ;
	}
		
	sleep(1);

	do{
//		err = ioctl(fd , SEND_CMD , 0x0c);
		err = ioctl(fd , SEND_CMD , 0x18);
		usleep(500*1000);
	}while(i++<70);
	close(fd);
	
	return 0;
}

void lcd_init(int fd )
{	
	int err ;
	usleep(100*1000);
	err = ioctl(fd , SEND_CMD , 0x33);
	usleep(100*1000);
	err = ioctl(fd , SEND_CMD , 0x32);
	usleep(100*1000);
	err = ioctl(fd , SEND_CMD , 0x28);
	usleep(100*1000);
	err = ioctl(fd , SEND_CMD , 0x0C);
//	usleep(100*1000);
//	err = ioctl(fd , SEND_CMD , 0x1c);
	usleep(100*1000);
	err = ioctl(fd , SEND_CMD , 0x01);

/*	
	err = ioctl(fd , SEND_CMD , 0x02);
	err = ioctl(fd , SEND_CMD , 0x28);
	err = ioctl(fd , SEND_CMD , 0x0e);
	err = ioctl(fd , SEND_CMD , 0x06);
	err = ioctl(fd , SEND_CMD , 0x01);
*/
/*
	err = ioctl(fd , SEND_CMD , 0x20);
	err = ioctl(fd , SEND_CMD , 0x20);
	err = ioctl(fd , SEND_CMD , 0x20);
	err = ioctl(fd , SEND_CMD , 0x28);
	err = ioctl(fd , SEND_CMD , 0x0C);
	err = ioctl(fd , SEND_CMD , 0x01);
	err = ioctl(fd , SEND_CMD , 0x06);
	err = ioctl(fd , SEND_CMD , 0x0C);
*/
	if(err<0){
		printf("error send data\n");
	}
}
unsigned int lcd_set_position(int fd , unsigned char x , unsigned char y)
{
	unsigned char addr = ( y==0)?(0x80+x):(0xc0+x);
	unsigned int err ;
	err = ioctl(fd , SEND_CMD , addr);
	if( err<0 ){
		printf("error- %d send addr %d \n",err, addr);
		return -1 ;
	}
	return 0 ;
}

unsigned int lcd_wr_string(int fd , unsigned char x , unsigned y , char *string  )
{
	int err ;
	err = lcd_set_position(fd , x , y );
	if(err==-1)
		printf("error \n");
	while(*string){
		err = ioctl(fd , SEND_DATA , *string++ );
		if( err<0 ){
			printf("error- %d send data %c \n",err, *string);
			return -1 ;
		}

	}
	return 0 ;
}
