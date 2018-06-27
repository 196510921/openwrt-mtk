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


void uart_write(UINT8* data, UINT16 len)
{
	int fd = 0;

	fd = open("/dev/gpio", O_RDONLY);
    if (fd < 0)
    {
        perror("/dev/gpio");
        return;
    }
    /* set gpio direction to output */
    // if (ioctl(fd, RALINK_GPIO_SET_DIR_OUT, RALINK_GPIO(TTYS2_485_ENABLE_PIN)) < 0)
    //     goto ioctl_err;

    // /* write 1 byte */
    // if (ioctl(fd, RALINK_GPIO_WRITE, 1) < 0)
    //     goto ioctl_err;

    // if (ioctl(fd, RALINK_GPIO_WRITE, 2) < 0)
    //     goto ioctl_err;
        
    close(fd);
}

void uart_485_test(void)
{
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

	uart_write(NULL, 1);

}



#if 0
/***RS485*****/
/* Driver-specific ioctls: ...\linux-3.10.x\include\uapi\asm-generic\ioctls.h */
#define TIOCGRS485      0x542E
#define TIOCSRS485      0x542F

static struct termios newtios;
static struct termios oldtios;
static int saved_portfd =-1;/*serial port fd */

/* Test GCC version, this structure is consistent in GCC 4.8, thus no need to overwrite */
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 3)

struct my_serial_rs485
{
	unsigned long	                    flags;			       /* RS485 feature flags */
	#define SER_RS485_ENABLED		    (1 << 0)	           /* If enabled */
	#define SER_RS485_RTS_ON_SEND		(1 << 1)	           /* Logical level for RTS pin when sending */
    #define SER_RS485_RTS_AFTER_SEND	(1 << 2)	           /* Logical level for RTS pin after sent*/
    #define SER_RS485_RX_DURING_TX		(1 << 4)
	unsigned long	                    delay_rts_before_send; /* Delay before send (milliseconds) */
	unsigned long	                    delay_rts_after_send;  /* Delay after send (milliseconds) */
	unsigned long	                    padding[5];		       /* Memory is cheap, new structs are a royal PITA .. */
};

#endif

static void reset_tty_atexit(void)
{
	if(saved_portfd != -1)
	{
		tcsetattr(saved_portfd,TCSANOW,&oldtios);
	}
}

/*cheanup signal handler */
static void reset_tty_handler(int signal)
{
	if(saved_portfd != -1)
	{
		tcsetattr(saved_portfd,TCSANOW,&oldtios);
	}
	_exit(EXIT_FAILURE);
}

int open_rs485_port(char *portname,int nSpeed, int nBits, char nEvent, int nStop)
{
	struct sigaction sa;
	int portfd;
#if (__GNUC__ == 4 && __GNUC_MINOR__ == 3)
	struct my_serial_rs485 rs485conf;
	struct my_serial_rs485 rs485conf_bak;
#else
	struct serial_rs485 rs485conf;
	struct serial_rs485 rs485conf_bak;
#endif
	printf("opening serial port:%s\n",portname);
	/*open serial port */
	if((portfd=open(portname,O_RDWR | O_NOCTTY, 0)) < 0 )
	{
   		printf("open serial port %s fail \n ",portname);
   		return portfd;
	}

	/*get serial port parnms,save away */
	tcgetattr(portfd,&newtios);
	memcpy(&oldtios,&newtios,sizeof newtios);
	/* configure new values */
	cfmakeraw(&newtios); /*see man page */
	newtios.c_iflag |=IGNPAR; /*ignore parity on input */
	newtios.c_oflag &= ~(OPOST | ONLCR | OLCUC | OCRNL | ONOCR | ONLRET | OFILL);

    newtios.c_cflag |= CLOCAL;
    newtios.c_cflag |= CREAD;
    switch( nBits )
    {
    case 7:
      newtios.c_cflag |= CS7;
      break;
    case 8:
      newtios.c_cflag |= CS8;
      break;
    }
    switch( nEvent )
    {
    case 'O':
      newtios.c_cflag |= PARENB;
      newtios.c_cflag |= PARODD;
      newtios.c_iflag |= (INPCK | ISTRIP);
      break;
    case 'E':
      newtios.c_iflag |= (INPCK | ISTRIP);
      newtios.c_cflag |= PARENB;
      newtios.c_cflag &= ~PARODD;
      break;
    case 'N':
      newtios.c_cflag &= ~PARENB;
      break;
    }
    switch( nEvent ){
        case 1:
            newtios.c_cflag &= ~CSTOPB;
            break;
        case 2:
            newtios.c_cflag |= CSTOPB;
            break;
    }

	newtios.c_cc[VMIN]=16; /* block until 1 char received */
	newtios.c_cc[VTIME]=1; /*no inter-character timer */

	/* bps */
    switch( nSpeed )
    {
      case 2400:
        cfsetispeed(&newtios, B2400);
        cfsetospeed(&newtios, B2400);
        break;
      case 4800:
        cfsetispeed(&newtios, B4800);
        cfsetospeed(&newtios, B4800);
        break;
      case 9600:
        cfsetispeed(&newtios, B9600);
        cfsetospeed(&newtios, B9600);
        break;
      case 115200:
        cfsetispeed(&newtios, B115200);
        cfsetospeed(&newtios, B115200);
        break;
      default:
        cfsetispeed(&newtios, B9600);
        cfsetospeed(&newtios, B9600);
        break;
    }

	/* register cleanup stuff */
	atexit(reset_tty_atexit);
	memset(&sa,0,sizeof sa);
	sa.sa_handler = reset_tty_handler;
	sigaction(SIGHUP,&sa,NULL);
	sigaction(SIGINT,&sa,NULL);
	sigaction(SIGPIPE,&sa,NULL);
	sigaction(SIGTERM,&sa,NULL);
	/*apply modified termios */
	saved_portfd=portfd;
	tcflush(portfd,TCIFLUSH);
	tcsetattr(portfd,TCSADRAIN,&newtios);


	if (ioctl (portfd, TIOCGRS485, &rs485conf) < 0)
	{
		/* Error handling.*/
		printf("ioctl error:%s\n",strerror(errno));
	}
	/* Enable RS485 mode: */
	rs485conf.flags |= SER_RS485_ENABLED;
	/* Set logical level for RTS pin equal to 1 when sending: */
	rs485conf.flags |= SER_RS485_RTS_ON_SEND;
	//rs485conf.flags &= ~(SER_RS485_RTS_ON_SEND);
	//rs485conf.flags |= SER_RS485_RTS_AFTER_SEND;
	rs485conf.flags &= ~(SER_RS485_RTS_AFTER_SEND);
	/* Set rts delay after send, if needed: */
	rs485conf.delay_rts_after_send = 0x80;
	//rs485conf.delay_rts_before_send = ...;
	//rs485conf.flags | = SER_RS485_RX_DURING_TX;
	if (ioctl (portfd, TIOCSRS485, &rs485conf) < 0)
	{
		/* Error handling.*/
		printf("ioctl error:%s\n",strerror(errno));
	}

	if (ioctl (portfd, TIOCGRS485, &rs485conf_bak) < 0)
	{
		/* Error handling.*/
		printf("ioctl error:%s\n",strerror(errno));
	}
	else
	{
//		printf("rs485conf_bak.flags 0x%x.\n", rs485conf_bak.flags);
//		printf("rs485conf_bak.delay_rts_before_send 0x%x.\n", rs485conf_bak.delay_rts_before_send);
//		printf("rs485conf_bak.delay_rts_after_send 0x%x.\n", rs485conf_bak.delay_rts_after_send);
	}

	return portfd;
}

#endif



