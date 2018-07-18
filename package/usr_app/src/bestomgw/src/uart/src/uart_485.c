#include <fcntl.h>     //文件控制定义
#include <stdio.h>     //标准输入输出定义
#include <stdlib.h>     //标准函数库定义
#include <unistd.h>    //Unix标准函数定义 
#include <errno.h>     //错误好定义
#include <termios.h>   //POSIX终端控制定义
#include <sys/ioctl.h>   //ioctl函数定义
#include <string.h>     //字符操作
#include <sys/types.h>  
#include <sys/stat.h> 
#include "uart_485.h"
#include "m1_common_log.h"

struct termios newtio, oldtio;

//配置串口
/* 参数说明：fd 设备文件描述符，nspeed 波特率，nbits 数据位数（7位或8位），
            parity 奇偶校验位（'n'或'N'为无校验位，'o'或'O'为偶校验，'e'或'E'奇校验），
        nstop 停止位（1位或2位）
  成功返回1，失败返回-1。 
*/
int set_com_opt( int fd, int nspeed, int nbits, char parity, int nstop )
{
//打印配置信息
  printf("set_com_opt - speed:%d,bits:%d,parity:%c,stop:%d\n", \
        nspeed, nbits, parity, nstop );
    
  //保存并测试现在有串口参数设置，在这里如果串口号等出错，会有相关的出错信息 
  if( tcgetattr( fd, &oldtio ) != 0 )
  {
    perror( "SetupSerial 1" );
    return -1;
  }
 
    //修改输出模式，原始数据输出
  bzero( &newtio, sizeof( newtio ));
  newtio.c_cflag &=~(OPOST);
 
  //屏蔽其他标志位
  newtio.c_cflag |= (CLOCAL | CREAD );
  newtio.c_cflag &= ~CSIZE;
  
    //设置数据位
  switch( nbits )
  {
  case 7:
    newtio.c_cflag |= CS7;
    break;
  case 8:
    newtio.c_cflag |= CS8;
    break;
  default:
    perror("Unsupported date bit!\n");
    return -1;
  }
  
  //设置校验位
  switch( parity )
  {
  case 'n':
  case 'N':  //无奇偶校验位
    newtio.c_cflag &= ~PARENB;
    newtio.c_iflag &= ~INPCK;
    break;
  case 'o':
  case 'O':  //设置为奇校验
    newtio.c_cflag |= ( PARODD | PARENB );
    newtio.c_iflag |= ( INPCK | ISTRIP );
    break;
  case 'e':
  case 'E':  //设置为偶校验
    newtio.c_iflag |= ( INPCK |ISTRIP );
    newtio.c_cflag |= PARENB;
    newtio.c_cflag &= ~PARODD;
    break;
  default:
    perror("unsupported parity\n");
    return -1;
  }
 
  //设置停止位
  switch( nstop ) 
  {
  case 1: 
    newtio.c_cflag &= ~CSTOPB;
    break;
  case 2:
    newtio.c_cflag |= CSTOPB;
    break;
  default :
    perror("Unsupported stop bit\n");
    return -1;
  }
 
  //设置波特率
  switch( nspeed )
  {
  case 2400:
    cfsetispeed( &newtio, B2400 );
    cfsetospeed( &newtio, B2400 );
    break;
  case 4800:
    cfsetispeed( &newtio, B4800 );
    cfsetospeed( &newtio, B4800 );
    break;  
  case 9600:
    cfsetispeed( &newtio, B9600 );
    cfsetospeed( &newtio, B9600 );
    break;  
  case 115200:
    cfsetispeed( &newtio, B115200 );
    cfsetospeed( &newtio, B115200 );
    break;
  case 460800:
    cfsetispeed( &newtio, B460800 );
    cfsetospeed( &newtio, B460800 );
    break;
  default:  
    cfsetispeed( &newtio, B9600 );
    cfsetospeed( &newtio, B9600 );
    break;
  }
 
  //设置等待时间和最小接收字符
  newtio.c_cc[VTIME] = 0;  
  newtio.c_cc[VMIN] = 0;   
//VTIME=0，VMIN=0，不管能否读取到数据，read都会立即返回。
 
//输入模式
  newtio.c_lflag &= ~(ICANON|ECHO|ECHOE|ISIG);
//设置数据流控制
  newtio.c_iflag &= ~(IXON|IXOFF|IXANY); //使用软件流控制
//如果发生数据溢出，接收数据，但是不再读取 刷新收到的数据但是不读
  tcflush( fd, TCIFLUSH ); 
//激活配置 (将修改后的termios数据设置到串口中）
  if( tcsetattr( fd, TCSANOW, &newtio ) != 0 )
  {
    printf("serial set error!\n");
    perror( "serial set error!" );
    return -1;
  }
 
  printf( "serial set ok!\n" );
  return 1;
}
 
//打开串口并返回串口设备文件描述
int open_com_dev( char *dev_name )
{
  int fd;
 
  if(( fd = open( dev_name, O_RDWR|O_NOCTTY|O_NDELAY)) == -1 )
  {
 
    perror("open\n");
    printf("Can't open Serial %s Port!\n", dev_name );
    return -1;
  }
 
  printf("open %s ok!\n", dev_name );
 
  if(fcntl(fd,F_SETFL,0)<0)
  {
    printf("fcntl failed!\n");
  }
  return fd;
}
 

/*初始化串口*/
int uart_485_init(char* uart)
{
  int fd_s = open_com_dev(uart);
  if( fd_s < 0 )
  {
    printf( "open UART device error! %s\n", uart );
  }
  else
  {
    set_com_opt(fd_s, 9600,8,'n',1);
  }

  return fd_s;
}

/*串口写*/
int uart_write(uart_data_t data)
{
  int n = 0;
  int i = 0;

  printf("uart_write:");
  for(i = 0; i < data.len; i++)
  {
    printf("%x ",data.d[i]);
  }
  printf("\n");
  n = write(data.uartFd, data.d, data.len);
  if(n < 0)
  {
    perror ("write err");
    return -1;
  }
  printf("write len:%d\n",n);

  return n;
}
/*串口读*/
int uart_read(uart_data_t* data)
{
  int n = 0;
  int i = 0;

  n = read(data->uartFd, data->d, 512);
  if(n < 0)
  {
    perror ("write err");
    return -1;
  }
  data->len = n;
  printf("nread:%d read data : ",n);
  for(i = 0; i < n; i++)
    printf("%02x ",data->d[i]);
  printf("read end\n");

  return n;
}
