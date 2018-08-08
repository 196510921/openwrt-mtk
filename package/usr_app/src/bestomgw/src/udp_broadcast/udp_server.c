
#include <stdarg.h>    
#include <sys/socket.h>    
#include <netinet/in.h>       
#include <netdb.h>    
#include <arpa/inet.h>    
#include <net/if.h>      
#include <sys/time.h>
#include <string.h>
#include <stdio.h>
#include <stdbool.h>
#include <sys/types.h>  
#include <net/route.h>  

#include "udp_server.h"

#define BC_BUF_MAX_LEN (1024)
#define BROADCAST_SERVER_PORT (4930)
static int running = true;
static pthread_t  looping;
static int  sockfd = -1 ;

static int broadcast_srv_init(void)
{
    int ret = -1 ;   
    struct sockaddr_in addr;
    int opt = 1;    
        
    bzero(&addr,sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;        
    addr.sin_port = htons(BROADCAST_SERVER_PORT);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);    
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);    
    if ( sockfd < 0 ){
        printf(" broadcast socket create fail!\n");
        goto exit;
    }

    if (bind (sockfd, (struct sockaddr*)&addr, sizeof (addr)) == -1){            
        printf("bind error\n");            
        goto exit;    
    }    

    ret = setsockopt(sockfd, SOL_SOCKET, SO_BROADCAST, (char *)&opt, sizeof(opt));      
    if ( ret == -1){
        printf("setsockopt error !\n" ) ;
        goto  exit ; 
    }       
    return ret; 
exit:
    if ( sockfd != -1 ){
        close( sockfd ) ; 
    }

    return ret; 
}
static void* Broadcast_looping(void* param)
{
    struct sockaddr_in addrcli = {0};    
    socklen_t addrlen = sizeof(addrcli);    
    ssize_t size = 0 ;
    char in_buf[BC_BUF_MAX_LEN] = {0};
    char out_buf[BC_BUF_MAX_LEN] = {0};
    fd_set watchset;

    addrcli.sin_family = AF_INET;
    addrcli.sin_addr.s_addr = htonl(INADDR_ANY);
    addrcli.sin_port = htons(BROADCAST_SERVER_PORT);

    char response[512]={0};
    while(running)
    {
        FD_ZERO(&watchset);
        FD_SET(sockfd,&watchset);     
		switch(select(sockfd+1,&watchset,NULL,NULL,NULL))
        {
            case -1:
            	printf("select error\n");
                break;
            case 0:
                break;
            default:
            {
                memset(in_buf,0,BC_BUF_MAX_LEN);
                memset(out_buf,0,BC_BUF_MAX_LEN);
                size  = recvfrom(sockfd, in_buf ,BC_BUF_MAX_LEN, 0,(struct sockaddr*)&addrcli, &addrlen); 
                if( size < 1 )
                {
                    continue;
                }
                printf("[recv broadcast msg]:%s\n",in_buf);
                if (broadcast_msg_proc(in_buf,out_buf,&size, sockfd) != 0)
                    continue;
                
                printf("[send broadcast msg]:%s\n",out_buf);
                sendto (sockfd, out_buf,size, 0,(struct sockaddr*)&addrcli,addrlen);
                memset(response,0,512);
                break;
            }
        }
    }
    return NULL;
}

int BroadcastModuleInit(void *param)
{

    broadcast_srv_init();
    running = true;
    if(pthread_create(&looping, NULL, Broadcast_looping, 0) != 0){
        printf("mqtt thread create failed\n");
        return -1;
    }
    printf("[ BroadcastModuleInit success! ]\n");
    return 0;
}
int BroadcastModuleExit(void *param)
{
    running = false;
    pthread_cancel(looping);
    pthread_join(looping,NULL);
    if(sockfd != -1){
    close(sockfd);
        sockfd = -1;
    }
    printf("[ Broadcast exited! ]\n");	
    return 0;
}

int main(int argc, char* argv[])
{
    BroadcastModuleInit(NULL);
    pthread_join(looping,NULL);
    //BroadcastModuleExit(NULL);
    return 0;
}
