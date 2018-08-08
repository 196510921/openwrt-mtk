/*
 * iobuffer.h
 *
 *  Created on: 2014年3月24日
 *      Author: Dylan
 */

#ifndef IOBUFFER_H_
#define IOBUFFER_H_

#include <stdint.h>

typedef struct iobuffer_t{
	char *buf;
	int32_t initSize;
	int32_t increment;
	int autoExpand;
	int autoShrink;
	int32_t position;
	int32_t limit;
	int32_t capacity;
	int32_t minSize;
	int32_t maxSize;
}iobuffer_t;

extern iobuffer_t *iobuffer_create(int32_t);
extern void iobuffer_destroy(iobuffer_t *);
extern void iobuffer_set_auto_expend(iobuffer_t *,int flag);
extern void iobuffer_set_auto_shrink(iobuffer_t *,int flag);
extern void iobuffer_set_increment(iobuffer_t *,int32_t);

extern void iobuffer_flip(iobuffer_t *);
extern void iobuffer_compact(iobuffer_t *);
extern void iobuffer_clear(iobuffer_t *);
extern int32_t iobuffer_set_position(iobuffer_t *,int32_t);
extern int32_t iobuffer_get_position(iobuffer_t *);
extern int32_t iobuffer_set_limit(iobuffer_t *,int32_t);
extern int32_t iobuffer_get_limit(iobuffer_t *);
extern void iobuffer_skip(iobuffer_t *,int32_t);

extern int32_t iobuffer_remaining(iobuffer_t *);
extern int32_t iobuffer_put_int(iobuffer_t *,int32_t);
extern int32_t iobuffer_get_int(iobuffer_t *,int32_t *);
extern int32_t iobuffer_put_double(iobuffer_t *,double);
extern int32_t iobuffer_get_double(iobuffer_t *,double *);
extern int32_t iobuffer_put_long(iobuffer_t *,int64_t);
extern int32_t iobuffer_get_long(iobuffer_t *,int64_t *);
extern int32_t iobuffer_put(iobuffer_t *,char *,int32_t);
extern int32_t iobuffer_get(iobuffer_t *,char *,int32_t);
extern int32_t iobuffer_put_by_read(iobuffer_t *,int fd,int32_t);
extern int32_t iobuffer_get_and_write(iobuffer_t *,int fd,int32_t);
/**
 * 从src中获取内容写入另一个dest
 * 注意：此函数不支持src和dest为同一个iobuffer的情况
 */
extern int32_t iobuffer_put_iobuffer(iobuffer_t *dest,iobuffer_t *src);


#endif /* IOBUFFER_H_ */
