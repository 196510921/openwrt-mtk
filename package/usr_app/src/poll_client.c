#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <poll.h>
#include <time.h>
#include <unistd.h>
#include <sys/types.h>

#define HEAD_LEN    3

#define MAXLINE     1024
#define IPADDRESS   "127.0.0.1"
#define SERV_PORT   8787

#define max(a,b) (a > b) ? a : b

uint8_t test_buf[200];

static void handle_connection(int sockfd);
static int test_data_pdu1(void);
static int test_data_pdu2(void);
static int test_data_pdu3(void);
static int test_data_pdu4(void);
static int test_data_pdu5(void);
static int test_data_pdu6(void);

int main(int argc,char *argv[])
{
	int sockfd;
	struct sockaddr_in  servaddr;
     sockfd = socket(AF_INET,SOCK_STREAM,0);
	 bzero(&servaddr,sizeof(servaddr));
	 servaddr.sin_family = AF_INET;
	 servaddr.sin_port = htons(SERV_PORT);
	 inet_pton(AF_INET,IPADDRESS,&servaddr.sin_addr);
	 if(connect(sockfd,(struct sockaddr*)&servaddr,sizeof(servaddr)) == -1){
		perror("connect fialed:");
		exit(1);
	 }else{
	 	printf("connect ok\n");
	 }
	 //处理连接描述符
	 handle_connection(sockfd);
	 return 0;
}

static void handle_connection(int sockfd)
{
	char    sendline[MAXLINE],recvline[MAXLINE];
    int     maxfdp,stdineof;
	struct pollfd pfds[2];
    int n, len;

	//添加连接描述符
	pfds[0].fd = sockfd;
	pfds[0].events = POLLIN;
	//添加标准输入描述符
    pfds[1].fd = STDIN_FILENO;
    pfds[1].events = POLLIN;
    for (; ;)
    {
		poll(pfds,2,-1);

		if (pfds[0].revents & POLLIN)									
		{
			n = read(sockfd,recvline,MAXLINE);
			if (n == 0)
			{
				fprintf(stderr,"client: server is closed.\n");
				close(sockfd);
			}
			fprintf(stderr,"client read/n");
			write(STDOUT_FILENO,recvline,n);
		}
		//测试标准输入是否准备好
		if (pfds[1].revents & POLLIN)
		{
			n = read(STDIN_FILENO,sendline,MAXLINE);
			if (n  == 0)
			{
				shutdown(sockfd,SHUT_WR);
				continue;
			}
			fprintf(stderr,"client write/n");

			//write(sockfd,sendline,n);
			switch(sendline[0]){
					case '1': len = test_data_pdu1(); break;
					case '2': len = test_data_pdu2(); break;
					case '3': len = test_data_pdu3(); break;
					case '4': len = test_data_pdu4(); break;
					case '5': len = test_data_pdu5(); break;
                    case '6': len = test_data_pdu6(); break;
					default: fprintf(stderr,"no type match/n");
			}
			write(sockfd,test_buf,len);
		
		}
	}
}


static int test_data_pdu1(void)
{

    typedef struct _unit1_data{
     uint16_t param_id;                            //parameter id(reference to selfdefine device paramenter table)
     uint8_t  v_len;                               //parameter length
     uint8_t  val[];                                 //parameter value
    }unit1_data_t;

    typedef struct _pdu1_data{
     uint16_t d_type;                              //device type
     uint8_t d_id[8];
     uint8_t d_len;                                //device data length
    }pdu1_data_t;

    typedef struct _pdu1{
     uint16_t p_type;                              //payload type
     uint16_t p_len;                                //payload length
    }pdu1_t;

    uint8_t* p_write = test_buf;
    uint8_t buf[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t buf_1[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x09};
    uint8_t buf_2[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0A};
    uint8_t buf_3[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x0B};


    memset(test_buf, 200, 0);
    /*pdu*/
    pdu1_t* data = (pdu1_t*)p_write;
    data->p_type = 0x1001;
    p_write += 4;

     /*pdu data 1*/
    pdu1_data_t* p_data = (pdu1_data_t*)p_write;
    memcpy(p_data->d_id, buf, 8);
    p_data->d_type = 0x1001;
    p_write += 11;

    unit1_data_t* u_data = (unit1_data_t*)p_write;
    u_data->param_id = 0x2001;
    *(uint16_t*)u_data->val = 0x1234;
    u_data->v_len    = 0x02;
    p_write += (u_data->v_len + 3);

    unit1_data_t* u_data_1 = (unit1_data_t*)p_write;
    u_data_1->param_id = 0x2002;
    *(uint16_t*)u_data_1->val = 0x1234;
    u_data_1->v_len    = 0x02;
    p_write += (u_data_1->v_len + 3);

    p_data->d_len = u_data->v_len + 3 + u_data_1->v_len + 3;

    /*pdu data 2*/
    pdu1_data_t* p_data_1 = (pdu1_data_t*)p_write;
    memcpy(p_data_1->d_id, buf_1, 8);
    p_data_1->d_type = 0x1002;
    p_write += 11;

    unit1_data_t* u_data_2 = (unit1_data_t*)p_write;
    u_data_2->param_id = 0x2003;
    *(uint16_t*)u_data_2->val = 0x5678;
    u_data_2->v_len    = 0x02;
    p_write += (u_data_2->v_len + 3);

    unit1_data_t* u_data_3 = (unit1_data_t*)p_write;
    u_data_3->param_id = 0x2004;
    *(uint16_t*)u_data_3->val = 0x5678;
    u_data_3->v_len    = 0x02;
    p_write += (u_data_3->v_len + 3);

    p_data_1->d_len = u_data_2->v_len + 3 + u_data_3->v_len + 3;

    data->p_len = p_data->d_len + 11 + p_data_1->d_len + 11;
   

	
    return (data->p_len + 4);
}

static int test_data_pdu2(void)
{

    typedef struct _pdu{
     uint16_t p_type;                              //payload type
     uint16_t p_len;                                //payload length
    }pdu_t;

    uint8_t* p_write = test_buf;

    memset(test_buf, 200, 0);
    /*pdu*/
    pdu_t* data = (pdu_t*)p_write;
    data->p_type = 0x0003;
    p_write += 4;

    *p_write = 0x01;

    data->p_len = 1;
	
    return (data->p_len + 4);
}

static int test_data_pdu3(void)
{

    typedef struct _pdu{
     uint16_t p_type;                              //payload type
     uint16_t p_len;                                //payload length
    }pdu_t;

    uint8_t* p_write = test_buf;

    memset(test_buf, 200, 0);
    /*pdu*/
    pdu_t* data = (pdu_t*)p_write;
    data->p_type = 0x0004;
    p_write += 4;

    *p_write = 0x01;

    data->p_len = 1;
	
    return (data->p_len + 4);
}

static int test_data_pdu4(void)
{

    typedef struct _unit4_data{
     uint16_t param_id;                            //parameter id(reference to selfdefine device paramenter table)
     uint8_t  v_len;                               //parameter length
     uint8_t  val[];                               //parameter value
    }unit4_data_t;

    typedef struct _pdu4_data{
     uint8_t d_id[8];                                //port number
     uint8_t d_len;                                 //device type
    }pdu4_data_t;

    typedef struct _pdu4{
     uint16_t p_type;                              //payload type
     uint16_t p_len;                                //payload length
    }pdu_t;

    uint8_t* p_write = test_buf;
    uint8_t buf[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t buf_1[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x09};
    uint16_t tmp = 0x1234, tmp1 = 0x5678;


    memset(test_buf, 200, 0);
    /*pdu*/
    pdu_t* data = (pdu_t*)p_write;
    data->p_type = 0x0005;
    p_write += 4;

     /*pdu data 1*/
    pdu4_data_t* p_data = (pdu4_data_t*)p_write;
    memcpy(p_data->d_id, buf, 8);
    p_write += 9;

    
    unit4_data_t* u_data = (unit4_data_t*)p_write;
    u_data->param_id = 0x1111;
    memcpy(u_data->val, (uint8_t*)&tmp, 2);
    u_data->v_len = 2;
    p_write += (u_data->v_len + 3);
	

    unit4_data_t* u_data_1 = (unit4_data_t*)p_write;
    u_data_1->param_id = 0x2222;
    memcpy(u_data_1->val, (uint8_t*)&tmp1, 2);
    u_data_1->v_len = 2;
    p_write += (u_data_1->v_len + 3);
    p_data->d_len = u_data->v_len + 3 + u_data_1->v_len + 3;
   
    /*pdu data 2*/
    pdu4_data_t* p_data_1 = (pdu4_data_t*)p_write;
    memcpy(p_data_1->d_id, buf_1, 8);
    p_write += 9;

    unit4_data_t* u_data_2 = (unit4_data_t*)p_write;
    u_data_2->param_id = 0x3333;
    memcpy(u_data_2->val, (uint8_t*)&tmp, 2);
    u_data_2->v_len    = 2;
    p_write += (u_data_2->v_len + 3);

    unit4_data_t* u_data_3 = (unit4_data_t*)p_write;
    u_data_3->param_id = 0x4444;
    memcpy(u_data_3->val, (uint8_t*)&tmp1, 2);
    u_data_3->v_len    = 2;
    p_write += (u_data_3->v_len + 3);

    p_data_1->d_len = u_data_2->v_len + 3 + u_data_3->v_len + 3;

    data->p_len = p_data->d_len + 9 + p_data_1->d_len + 9;
   
	
    return (data->p_len + 4);
}

static int test_data_pdu5(void)
{

 
    typedef struct _pdu5_data{
     uint8_t d_id[8];                                //port number
     uint8_t d_len;                                 //device type
    }pdu5_data_t;

    typedef struct _pdu4{
     uint16_t p_type;                              //payload type
     uint16_t p_len;                                //payload length
    }pdu_t;

    uint8_t* p_write = test_buf;
    uint8_t buf[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t buf_1[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x09};
    uint16_t tmp = 0x1234, tmp1 = 0x5678, tmp2 = 0x2234, tmp3 = 0x5578;


    memset(test_buf, 200, 0);
    /*pdu*/
    pdu_t* data = (pdu_t*)p_write;
    data->p_type = 0x0006;
    p_write += 4;

     /*pdu data 1*/
    pdu5_data_t* p_data = (pdu5_data_t*)p_write;
    memcpy(p_data->d_id, buf, 8);
    p_write += 9;

   	uint16_t* param_id = (uint16_t*)p_write;
   	param_id[0] = tmp;
   	param_id[1] = tmp1;
   	p_write += 4;

   	p_data->d_len = 4;

    /*pdu data 2*/
    pdu5_data_t* p_data_1 = (pdu5_data_t*)p_write;
    memcpy(p_data_1->d_id, buf_1, 8);
    p_write += 9;

    uint16_t* param_id_1 = (uint16_t*)p_write;
   	param_id_1[0] = tmp2;
   	param_id_1[1] = tmp3;
   	p_write += 4;

   	p_data_1->d_len = 4;

    data->p_len = p_data->d_len + 9 + p_data_1->d_len + 9;
   
	
    return (data->p_len + 4);
}

static int test_data_pdu6(void)
{

    typedef struct _pdu4{
     uint16_t p_type;                              //payload type
     uint16_t p_len;                                //payload length
    }pdu_t;

    typedef struct _pdu6_data{
     uint16_t port;
     uint16_t d_type;                              //device type
     uint8_t ap_id[8];
     uint8_t d_id[8];
     uint8_t n_len;
     char name[];
    }pdu6_data_t;

    uint8_t buf[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08};
    uint8_t buf_1[8] = {0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x09};


    uint8_t* p_write = test_buf;

    memset(test_buf, 200, 0);
    /*pdu*/
    pdu_t* data = (pdu_t*)p_write;
    data->p_type = 0x1003;
    p_write += 4;

    pdu6_data_t* p_data = (pdu6_data_t*)p_write;
    p_data->port = 0x6688;
    p_data->d_type = 0xaaaa;
    memcpy(p_data->ap_id, buf, 8);
    memcpy(p_data->d_id, buf_1, 8);
    p_data->n_len = sizeof("device1");
    memcpy(p_data->name, "device1", p_data->n_len);
    p_write += (p_data->n_len + 21);

    pdu6_data_t* p_data_1 = (pdu6_data_t*)p_write;
    p_data_1->port = 0x6688;
    p_data_1->d_type = 0xaaaa;
    memcpy(p_data_1->ap_id, buf, 8);
    memcpy(p_data_1->d_id, buf_1, 8);
    p_data_1->n_len = sizeof("device1");
    memcpy(p_data_1->name, "device1", p_data_1->n_len);
    p_write += (p_data_1->n_len + 21);


    data->p_len = p_data->n_len + 21 + p_data_1->n_len + 21;
    
    return (data->p_len + 4);
}


