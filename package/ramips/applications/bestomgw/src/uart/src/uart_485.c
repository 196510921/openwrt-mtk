#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>   
#include <string.h>  
#include <signal.h>
#include <termios.h> 
#include <fcntl.h>
#include <errno.h>
#include <linux/serial.h>
#include <sys/ioctl.h>
#include "uart_485.h"
#include "utils.h"
//#include "ralink_gpio.h"
//static void gpio_test_write(void);
/*初始化ttyS2 485串口*/

int open_rs485_port(int fd,int comport) 
{ 
	char *dev[4]={"/dev/ttyS2","/dev/ttyUSB0","/dev/ttyUSB1","/dev/ttyUSB2"};
 
	if (comport==1)//串口1 
	{
		fd = open( dev[0], O_RDWR|O_NOCTTY|O_NDELAY); 
		if (-1 == fd)
		{ 
			perror("Can't Open Serial Port ttyS2"); 
			return(-1); 
		} 
     } 
     else if(comport==2)//串口2 
     {     
		fd = open(dev[1], O_RDWR|O_NOCTTY|O_NDELAY); //没有设置<span style="font-family: Arial, Helvetica, sans-serif;">O_NONBLOCK非阻塞模式，也可以设置为非阻塞模式，两个模式在下一篇博客中具体说明</span>
 
		if (-1 == fd)
		{ 
			perror("Can't Open Serial Port ttyUSB0"); 
			return(-1); 
		} 
     } 
     else if (comport==3)//串口3 
     { 
		fd = open( dev[2], O_RDWR|O_NOCTTY|O_NDELAY); 
		if (-1 == fd)
		{ 
			perror("Can't Open Serial Port ttyUSB1"); 
			return(-1); 
		} 
     }
     else if (comport==4)//串口3 
     { 
		fd = open( dev[3], O_RDWR|O_NOCTTY|O_NDELAY); 
		if (-1 == fd)
		{ 
			perror("Can't Open Serial Port ttyUSB2"); 
			return(-1); 
		} 
     } 
     /*恢复串口为阻塞状态*/ 
     if(fcntl(fd, F_SETFL, 0)<0) 
     		printf("fcntl failed!\n"); 
     else 
		printf("fcntl=%d\n",fcntl(fd, F_SETFL,0)); 
     /*测试是否为终端设备*/ 
     if(isatty(STDIN_FILENO)==0) 
		printf("standard input is not a terminal device\n"); 
     else 
		printf("isatty success!\n"); 
     printf("fd-open=%d\n",fd); 

     return fd; 
}


int set_opt(int fd,int nSpeed, int nBits, char nEvent, int nStop) 
{ 
     struct termios newtio,oldtio; 
	/*保存测试现有串口参数设置，在这里如果串口号等出错，会有相关的出错信息*/ 
     if( tcgetattr( fd,&oldtio) != 0)
     {  
      	perror("SetupSerial 1");
		printf("tcgetattr( fd,&oldtio) -> %d\n",tcgetattr( fd,&oldtio)); 
      	return -1; 
     }

     bzero( &newtio, sizeof( newtio ) ); 
	/*设置字符大小*/ 
     newtio.c_cflag  |=  CLOCAL | CREAD;  
     newtio.c_cflag &= ~CSIZE;  
	/*设置停止位*/ 
     switch( nBits ) 
     { 
     case 7: 
      newtio.c_cflag |= CS7; 
      break; 
     case 8: 
      newtio.c_cflag |= CS8; 
      break; 
     } 
    /*设置奇偶校验位*/ 
     switch( nEvent ) 
     { 
     case 'o':
     case 'O': //奇数 
      newtio.c_cflag |= PARENB; 
      newtio.c_cflag |= PARODD; 
      newtio.c_iflag |= (INPCK | ISTRIP); 
      break; 
     case 'e':
     case 'E': //偶数 
      newtio.c_iflag |= (INPCK | ISTRIP); 
      newtio.c_cflag |= PARENB; 
      newtio.c_cflag &= ~PARODD; 
      break;
     case 'n':
     case 'N':  //无奇偶校验位 
      newtio.c_cflag &= ~PARENB; 
      break;
     default:
      break;
     } 
     /*设置波特率*/ 
     switch( nSpeed ) 
     { 
        case 2400: 
         cfsetispeed(&newtio, B2400); 
         cfsetospeed(&newtio, B2400); 
         break; 
        case 4800: 
         cfsetispeed(&newtio, B4800); 
         cfsetospeed(&newtio, B4800); 
         break; 
        case 9600: 
         cfsetispeed(&newtio, B9600); 
         cfsetospeed(&newtio, B9600); 
         break; 
        case 115200: 
         cfsetispeed(&newtio, B115200); 
         cfsetospeed(&newtio, B115200); 
         break; 
        case 460800: 
         cfsetispeed(&newtio, B460800); 
         cfsetospeed(&newtio, B460800); 
         break; 
        default: 
         cfsetispeed(&newtio, B9600); 
         cfsetospeed(&newtio, B9600); 
        break; 
     } 
    /*设置停止位*/ 
     if( nStop == 1 ) 
      newtio.c_cflag &=  ~CSTOPB; 
     else if ( nStop == 2 ) 
      newtio.c_cflag |=  CSTOPB; 
    /*设置等待时间和最小接收字符*/ 
     newtio.c_cc[VTIME]  = 0; 
     newtio.c_cc[VMIN] = 0; 
    /*处理未接收字符*/ 
     tcflush(fd,TCIFLUSH); 
    /*激活新配置*/ 
    if((tcsetattr(fd,TCSANOW,&newtio))!=0) 
    { 
      perror("com set error"); 
      return -1; 
    } 
    printf("set done!\n"); 
    
    return 0; 
}

void uart_485_test(void)
{
  #if 1
	int rtn             = 0;
	int rc              = 0;
	char buf[1024]      = {0};
	/*串口配置*/
	int speed           = 9600;
	int nBits           = 8;
	char nEvent         = 'N';
	int nStop           = 1;   
	/*文件描述符*/
	int ttyS2Fd         = 0;
	int ttyUSB0Fd       = 0;
	int ttyUSB1Fd       = 0;
	int ttyUSB2Fd       = 0;

	ttyS2Fd = open_rs485_port(ttyS2Fd, 1);
	if(ttyS2Fd < 0)
	{
		printf("ttyS2Fd open failed !!!\n");
	}
	rc = set_opt(ttyS2Fd, speed, nBits, nEvent, nStop);
	if(rc < 0)
	{
		printf("ttyS2Fd config failed !!!\n");	
	}

	uart_write(ttyS2Fd, NULL, 1);
  #else
  gpio_test_write();
  //gpio_set_led();
  #endif
}


void uart_write(int comFd, UINT8* data, UINT16 len)
{
  int fd = 0;
  char buf[512] = {0};
  int i = 0;

    //fd = open("/dev/gpio", O_RDONLY);O_RDWR
  #if 1
    fd = open("/dev/gpio", O_RDWR);
    if (fd < 0)
    {
        printf("open /dev/gpio error\n");
        return;
    }
    /* set gpio direction to output */
    if (ioctl(fd, RALINK_GPIO_SET_DIR_OUT, RALINK_GPIO(10)) < 0)
    {
        printf("set gpio direction to output error!\n");
        goto ioctl_err;
    }

    /* write 1 byte */
    for(i = 0; i < 5; i++)
    {
      if (ioctl(fd, RALINK_GPIO_WRITE, 1) < 0)
      {
          printf("write 1 error!\n");
          goto ioctl_err;
      }
      usleep(500000);
      if (ioctl(fd, RALINK_GPIO_WRITE, 0) < 0)
      {
          printf("write 1 error!\n");
          goto ioctl_err;
      }
      usleep(500000);

     }
    #endif
      while(1)
      {

        if (write(comFd, "485 testing", strlen("485 testing")) < 0)
        {
            printf("write 485 testing failed!\n");
            
        }
        usleep(500000);

        // if (read(comFd, buf, 512) < 0)
        // {
        //     printf("write 485 testing failed!\n");
            
        // }
        // else
        // {
        //   printf("read:%x,%x,%x\n",buf[0],buf[1],buf[2]);
        //   printf("read:%s\n",buf);
        // }
        // usleep(500000);

        printf("write onetime finish!\n");

        sleep(1);
      }

    ioctl_err:
        
    close(fd);
}

#if 0
#define GPIO_DEV  "/dev/gpio"

enum {
  gpio_in,
  gpio_out,
};
enum {
  gpio3100,
  gpio6332,
  gpio9564,
};


int gpio_set_dir(int r, int dir)
{
  int fd, req;
  printf("gpio_set_dir\n");
  fd = open(GPIO_DEV, O_RDONLY);
  if (fd < 0) {
    perror(GPIO_DEV);
    return -1;
  }

  if (r == gpio9564)
      req = RALINK_GPIO9564_SET_DIR_OUT;
    else if (r == gpio6332)
      req = RALINK_GPIO6332_SET_DIR_OUT;
    else
      req = RALINK_GPIO_SET_DIR_OUT;
  
  if (ioctl(fd, req, 0xffffffff) < 0) {
    perror("ioctl");
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}


int gpio_write_int(int r, int value)
{
  int fd, req;

  printf("gpio_write_int\n");
  fd = open(GPIO_DEV, O_RDONLY);
  if (fd < 0) {
    perror(GPIO_DEV);
    return -1;
  }

  if (r == gpio9564)
    req = RALINK_GPIO9564_WRITE;
  else if (r == gpio6332)
    req = RALINK_GPIO6332_WRITE;
  else
    req = RALINK_GPIO_WRITE;
  if (ioctl(fd, req, value) < 0) {
    perror("ioctl");
    close(fd);
    return -1;
  }
  close(fd);
  return 0;
}

static void gpio_test_write(void)
{
  int i = 0;

  gpio_set_dir(gpio9564, gpio_out);
  gpio_set_dir(gpio6332, gpio_out);
  gpio_set_dir(gpio3100, gpio_out);

  //turn off LEDs
  gpio_write_int(gpio9564, 0xffffffff);
  gpio_write_int(gpio6332, 0xffffffff);
  gpio_write_int(gpio3100, 0xffffffff);

  sleep(3);

  //turn on all LEDs
  gpio_write_int(gpio9564, 0);
  gpio_write_int(gpio6332, 0);
  gpio_write_int(gpio3100, 0);

}

void gpio_set_led(void)
{
  int fd;
  ralink_gpio_led_info led;

  led.gpio = 10;
  if (led.gpio < 0 || led.gpio >= RALINK_GPIO_NUMBER) {
    printf("gpio number %d out of range (should be 0 ~ %d)\n", led.gpio, RALINK_GPIO_NUMBER);
    return;
  }
  led.on = 50;
  if (led.on > RALINK_GPIO_LED_INFINITY) {
    printf("on interval %d out of range (should be 0 ~ %d)\n", led.on, RALINK_GPIO_LED_INFINITY);
    return;
  }
  led.off = 50;
  if (led.off > RALINK_GPIO_LED_INFINITY) {
    printf("off interval %d out of range (should be 0 ~ %d)\n", led.off, RALINK_GPIO_LED_INFINITY);
    return;
  }
  led.blinks = 200;
  if (led.blinks > RALINK_GPIO_LED_INFINITY) {
    printf("number of blinking cycles %d out of range (should be 0 ~ %d)\n", led.blinks, RALINK_GPIO_LED_INFINITY);
    return;
  }
  led.rests = 200;
  if (led.rests > RALINK_GPIO_LED_INFINITY) {
    printf("number of resting cycles %d out of range (should be 0 ~ %d)\n", led.rests, RALINK_GPIO_LED_INFINITY);
    return;
  }
  led.times = 2000;
  if (led.times > RALINK_GPIO_LED_INFINITY) {
    printf("times of blinking %d out of range (should be 0 ~ %d)\n", led.times, RALINK_GPIO_LED_INFINITY);
    return;
  }

  fd = open(GPIO_DEV, O_RDONLY);
  if (fd < 0) {
    perror(GPIO_DEV);
    return;
  }
  if (ioctl(fd, RALINK_GPIO_LED_SET, &led) < 0) {
    perror("ioctl");
    close(fd);
    return;
  }
  close(fd);
}

#endif
