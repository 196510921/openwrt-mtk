/*
 * common.h
 *
 *  Created on: 2014年3月3日
 *      Author: Admin
 */

#ifndef COMMON_H_
#define COMMON_H_

typedef void (*exitFunc)(void *);
struct exit_func_t{
	exitFunc func;
	void *args;
};

extern void init_common();
extern void register_exit_handler(exitFunc,void *);

#endif /* COMMON_H_ */
