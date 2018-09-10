/*
 * iobuffer.c
 * 仿造Mina2的IoBuffer实现的iobuffer
 * 主要用于socket读写
 *  Created on: 2014年3月24日
 *  Author: Dylan
 */

#include "iobuffer.h"
#include <stdio.h>
//#include <asm/byteorder.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>

#define  IS_BIG_ENDIAN     (1 == htons(1))
#define  IS_LITTLE_ENDIAN  (!IS_BIG_ENDIAN)

static void host_to_network(void *source, int32_t size);
static void network_to_host(void *source, int32_t size);

static int32_t _iobuffer_put_basic(iobuffer_t * iobuf,void *buf,int32_t size);
static int32_t _iobuffer_get_basic(iobuffer_t * iobuf,void *buf,int32_t size);
static int32_t _iobuffer_put(iobuffer_t *iobuf,char *buf,int32_t size,int32_t strict);
static int32_t _iobuffer_get(iobuffer_t *iobuf,char *buf,int32_t size,int32_t strict);

extern iobuffer_t *iobuffer_create(int32_t initSize){
	iobuffer_t *obj = (iobuffer_t *)malloc(sizeof(iobuffer_t));
	obj->buf = (char *)malloc(initSize);
	memset(obj->buf,0,initSize);
	obj->capacity = initSize;
	obj->initSize = initSize;
	obj->position=0;
	obj->increment = initSize;
	obj->limit=0;
	obj->autoExpand = 1;
	obj->autoShrink = 1;
	obj->minSize = initSize;
	obj->maxSize = -1;
	return obj;
}

extern void iobuffer_destroy(iobuffer_t *iobuf){
	if(iobuf == NULL) return;
	if(iobuf->buf!=NULL) {
		free(iobuf->buf);
		iobuf->buf = NULL;
	}
	free(iobuf);
}

extern void iobuffer_set_auto_expend(iobuffer_t *iobuf,int flag){
	if(iobuf == NULL) return;
	iobuf->autoExpand = flag;
}
extern void iobuffer_set_auto_shrink(iobuffer_t *iobuf,int flag){
	if(iobuf == NULL) return;
	iobuf->autoShrink = flag;
}
extern void iobuffer_set_increment(iobuffer_t *iobuf,int32_t inc){
	if (iobuf == NULL) return;
	iobuf->increment = inc;
}
static void _iobuffer_do_shrink(iobuffer_t * iobuf,int32_t newsize){
	if(newsize < iobuf->minSize){
		newsize = iobuf->minSize;
	}
	iobuf->buf = (char *) realloc(iobuf->buf, newsize);
	iobuf->capacity = newsize;
}
extern void iobuffer_flip(iobuffer_t * iobuf){
	if(iobuf == NULL) return;
	iobuf->limit = iobuf->position;
	iobuf->position = 0;

	if (iobuf->autoShrink) {
		if ((iobuf->capacity - iobuf->limit) > iobuf->minSize) {
			_iobuffer_do_shrink(iobuf,iobuf->limit+iobuf->minSize);
		}
	}
}
extern void iobuffer_compact(iobuffer_t * iobuf){
	if(iobuf == NULL) return;
	int32_t remaining = iobuf->limit - iobuf->position;
	if(remaining > 0){
		memmove(iobuf->buf,iobuf->buf+iobuf->position,remaining);
	}
	iobuf->position = remaining;
	iobuf->limit = iobuf->capacity;
}

extern int32_t iobuffer_remaining(iobuffer_t *iobuf){
	if(iobuf == NULL) return 0;
	return iobuf->limit - iobuf->position;
}

extern void iobuffer_clear(iobuffer_t *iobuf){
	if(iobuf == NULL) return;
	iobuf->limit=iobuf->position=0;
	if (iobuf->autoShrink) {
		_iobuffer_do_shrink(iobuf, iobuf->minSize);
	}
}

extern int32_t iobuffer_set_position(iobuffer_t *iobuf,int32_t pos){
	if(iobuf == NULL) return 0;
	if(pos < 0) pos = 0;
	if(pos > iobuf->capacity) pos = iobuf->capacity;

	iobuf->position=pos;
	if (iobuf->position > iobuf->limit) {
		iobuf->limit = iobuf->position;
	}
	return pos;
}
extern int32_t iobuffer_get_position(iobuffer_t *iobuf){
	if(iobuf == NULL) return 0;
	return iobuf->position;
}
extern int32_t iobuffer_set_limit(iobuffer_t *iobuf,int32_t pos){
	if (iobuf == NULL)
		return 0;
	if(pos < 0) pos = 0;
	if(pos > iobuf->capacity) pos = iobuf->capacity;

	iobuf->limit = pos;
	if (iobuf->position > iobuf->limit) {
		iobuf->position = iobuf->limit;
	}
	return pos;
}
extern int32_t iobuffer_get_limit(iobuffer_t *iobuf){
	if (iobuf == NULL)
		return 0;
	return iobuf->limit;
}

extern void iobuffer_skip(iobuffer_t *iobuf,int32_t step){
	if (iobuf == NULL) return;
	iobuffer_set_position(iobuf,iobuf->position + step);
}


static int32_t _iobuffer_put_basic(iobuffer_t * iobuf,void *buf,int32_t size){
	host_to_network(buf,size);
	int32_t n = _iobuffer_put(iobuf,buf,size,1);
	if(n < 0){
		network_to_host(buf,size);
	}
	return n;
}

static int32_t _iobuffer_get_basic(iobuffer_t * iobuf,void *buf,int32_t size){
	int32_t n = _iobuffer_get(iobuf,buf,size,1);
	if(n > 0){
		network_to_host(buf,size);
	}
	return n;
}

extern int32_t iobuffer_put_int(iobuffer_t *iobuf,int32_t v){
	if (iobuf == NULL) return -1;
	int32_t size = sizeof(int32_t);
	return _iobuffer_put_basic(iobuf,&v,size);
}

extern int32_t iobuffer_get_int(iobuffer_t *iobuf,int32_t *p){
	if (iobuf == NULL) return -1;
	int32_t size = sizeof(int32_t);
	int32_t stat = _iobuffer_get_basic(iobuf,(void *)p,size);
	return stat;
}

extern int32_t iobuffer_put_double(iobuffer_t *iobuf,double v){
	if (iobuf == NULL) return -1;
	int32_t size = sizeof(double);
	return _iobuffer_put_basic(iobuf,&v,size);
}

extern int32_t iobuffer_get_double(iobuffer_t *iobuf,double *p){
	if (iobuf == NULL) return -1;
	int32_t size = sizeof(double);
	return _iobuffer_get_basic(iobuf,(void *)p,size);
}

extern int32_t iobuffer_put_long(iobuffer_t *iobuf,int64_t v){
	if (iobuf == NULL) return -1;
	int32_t size = sizeof(int64_t);
	return _iobuffer_put_basic(iobuf,&v,size);
}

extern int32_t iobuffer_get_long(iobuffer_t *iobuf,int64_t *p){
	if (iobuf == NULL) return -1;
	int32_t size = sizeof(int64_t);
	return _iobuffer_get_basic(iobuf,p,size);
}

static void _iobuffer_do_expend(iobuffer_t *iobuf ,int32_t require){
	int32_t increment = iobuf->increment;
	increment = require>increment?require:increment;
	int32_t newsize = iobuf->capacity + increment;
	if(iobuf->maxSize > 0 && newsize>iobuf->maxSize){
		newsize = iobuf->maxSize;
	}
	iobuf->buf = (char *)realloc(iobuf->buf,newsize);
	iobuf->capacity = newsize;
}

static int32_t _iobuffer_put(iobuffer_t *iobuf,char *buf,int32_t size,int32_t strict){
	if (iobuf == NULL) return -1;

	int32_t spaceLeft = iobuf->capacity - iobuf->position;
	if(spaceLeft < size){
		if(iobuf->autoExpand){
			_iobuffer_do_expend(iobuf,size);
			spaceLeft = iobuf->capacity - iobuf->position;
		}
	}
	if(spaceLeft < size){
		if(strict){
			return -1;
		}
		size = spaceLeft;
	}
	memcpy(iobuf->buf+iobuf->position,buf,size);
	iobuf->position += size;
	if(iobuf->position > iobuf->limit){
		iobuf->limit = iobuf->position;
	}
	return size;
}

extern int32_t iobuffer_put(iobuffer_t *iobuf,char *buf,int32_t size){
	return _iobuffer_put(iobuf,buf,size,0);
}
static int32_t _iobuffer_get(iobuffer_t *iobuf,char *buf,int32_t size,int32_t strict){
	if (iobuf == NULL) return -1;
	int32_t remaining = iobuf->limit - iobuf->position;
	if(remaining < size){
		if(strict){
			return -1;
		}
		size = remaining;
	}
	memcpy(buf, iobuf->buf + iobuf->position, size);
	iobuf->position += size;
	if (iobuf->position > iobuf->limit) {
		iobuf->limit = iobuf->position;
	}
	return size;
}

extern int32_t iobuffer_get(iobuffer_t *iobuf,char *buf,int32_t size){
	return _iobuffer_get(iobuf,buf,size,0);
}

extern int32_t iobuffer_put_by_read(iobuffer_t *iobuf,int32_t fd,int32_t size){
	if (iobuf == NULL) return -1;
	int32_t spaceLeft = iobuf->capacity - iobuf->position;
	if(spaceLeft < size){
		if(iobuf->autoExpand){
			_iobuffer_do_expend(iobuf,size);
			spaceLeft = iobuf->capacity - iobuf->position;
		}
	}
	if (spaceLeft < size) {
		size = spaceLeft;
	}
	if (size <= 0)
		return 0;
	size = recv(fd,iobuf->buf + iobuf->position, size, MSG_DONTWAIT);
	if (size == 0) {
		return -1;
	} else {
		if (size < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				return -1;
			}
			return 0;
		} else {
			iobuf->position += size;
			if (iobuf->position > iobuf->limit) {
				iobuf->limit = iobuf->position;
			}
		}
	}
	return size;
}

extern int32_t iobuffer_get_and_write(iobuffer_t *iobuf,int32_t fd,int32_t size){
	if (iobuf == NULL) return -1;
	int32_t remaining = iobuf->limit - iobuf->position;
	if(remaining < size){
		size = remaining;
	}
	size = write(fd, iobuf->buf + iobuf->position, size);
	if (size == 0) {
		return -1;
	} else {
		if (size < 0) {
			if (errno != EINTR && errno != EAGAIN) {
				return -1;
			}
			return 0;
		} else {
			iobuf->position += size;
			if (iobuf->position > iobuf->limit) {
				iobuf->limit = iobuf->position;
			}
		}
	}
	return size;
}

extern int32_t iobuffer_put_iobuffer(iobuffer_t *iobuf,iobuffer_t *src){
	if (iobuf == NULL || src==NULL) return -1;
	if (iobuf == src) return -1;

	int32_t size = iobuffer_remaining(src);

	int32_t spaceLeft = iobuf->capacity - iobuf->position;
	if (spaceLeft < size) {
		if (iobuf->autoExpand) {
			_iobuffer_do_expend(iobuf, size);
			spaceLeft = iobuf->capacity - iobuf->position;
		}
	}
	if (spaceLeft < size) {
		size = spaceLeft;
	}
	if (size <= 0)
		return 0;

	memcpy(iobuf->buf + iobuf->position, src->buf+src->position, size);
	iobuf->position += size;
	if (iobuf->position > iobuf->limit) {
		iobuf->limit = iobuf->position;
	}
	src->position += size;
	if (src->position > src->limit) {
		src->limit = src->position;
	}

	return size;
}

static void byte_swap(void *source, int32_t size) {
	typedef unsigned char TwoBytes[2];
	typedef unsigned char FourBytes[4];
	typedef unsigned char EightBytes[8];
	unsigned char temp;
	if (size == 2) {
		TwoBytes *src = (TwoBytes *) source;
		temp = (*src)[0];
		(*src)[0] = (*src)[1];
		(*src)[1] = temp;
		return;
	}
	if (size == 4) {
		FourBytes *src = (FourBytes *) source;
		temp = (*src)[0];
		(*src)[0] = (*src)[3];
		(*src)[3] = temp;

		temp = (*src)[1];
		(*src)[1] = (*src)[2];
		(*src)[2] = temp;
		return;
	}
	if (size == 8) {
		EightBytes *src = (EightBytes *) source;
		temp = (*src)[0];
		(*src)[0] = (*src)[7];
		(*src)[7] = temp;

		temp = (*src)[1];
		(*src)[1] = (*src)[6];
		(*src)[6] = temp;

		temp = (*src)[2];
		(*src)[2] = (*src)[5];
		(*src)[5] = temp;

		temp = (*src)[3];
		(*src)[3] = (*src)[4];
		(*src)[4] = temp;

		return;
	}
}

static void host_to_network(void *source, int32_t size){
	if(IS_BIG_ENDIAN) return;
	byte_swap(source,size);
}
static void network_to_host(void *source, int32_t size){
	if(IS_BIG_ENDIAN) return;
	byte_swap(source,size);
}
