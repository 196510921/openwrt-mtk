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


void uart_485_test(void)
{
	int rtn             = 0;
	char buf[1024]      = {0};
	/*串口配置*/
	const char* ttyS2   = "/dev/ttyS2";
	const char* ttyUSB0 = "/dev/ttyUSB0";
	const char* ttyUSB1 = "/dev/ttyUSB1";
	const char* ttyUSB2 = "/dev/ttyUSB2";
	int speed           = 9600;
	int nBits           = 8;
	char nEvent         = 'N';
	int nStop           = 1;   
	/*文件描述符*/
	int ttyS2Fd         = 0;
	int ttyUSB0Fd       = 0;
	int ttyUSB1Fd       = 0;
	int ttyUSB2Fd       = 0;

	ttyS2Fd = open_rs485_port(ttyS2, speed, nBits, nEvent, nStop);
	if(ttyS2Fd == 0)
	{
		printf("ttyS2Fd == 0  !!!\n");
	}

	ttyUSB0Fd = open_rs485_port(ttyUSB0, speed, nBits, nEvent, nStop);
	if(ttyS2Fd == 0)
	{
		printf("ttyS2Fd == 0  !!!\n");
	}

	ttyUSB1Fd = open_rs485_port(ttyUSB1, speed, nBits, nEvent, nStop);
	if(ttyS2Fd == 0)
	{
		printf("ttyS2Fd == 0  !!!\n");
	}

	ttyUSB2Fd = open_rs485_port(ttyUSB2, speed, nBits, nEvent, nStop);
	if(ttyS2Fd == 0)
	{
		printf("ttyS2Fd == 0  !!!\n");
	}

	rtn = write(ttyS2Fd, "ttyS2 485 testing !!!\n", sizeof("ttyS2 485 testing !!!\n"));
	if(rtn < 0)
	{
		printf("write to ttyS2 error !!!\n");
	}

	while(1)
	{
		rtn = read(ttyS2Fd, buf, 1024);
		if(rtn < 0)
		{
			printf("read ttyS2 error !!! \n");
		}
		else
		{
			printf("read ttyS2:%s\n",buf);
			memset(buf, 0, 1024);
		}
	}
}





