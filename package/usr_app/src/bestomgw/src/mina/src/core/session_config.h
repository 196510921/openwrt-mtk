/*
 * session_config.h
 *
 *  Created on: 2014年3月25日
 *      Author: Admin
 */

#ifndef SESSION_CONFIG_H_
#define SESSION_CONFIG_H_

typedef struct mina_session_config_t{
	int readBufferSize;
	int minReadBufferSize;
	int maxReadBufferSize;
	int writeBufferSize;
	int minWriteBufferSize;
	int maxWriteBufferSize;

	int tcpNoDelay;
	int sendBufSize;
	int revBufSize;

	int readerIdleTime;//time in sec
	int writerIdleTime;//time in sec
}mina_session_config_t;

extern mina_session_config_t *mina_session_config_create();
extern void mina_session_config_destroy(mina_session_config_t *);

#endif /* SESSION_CONFIG_H_ */
