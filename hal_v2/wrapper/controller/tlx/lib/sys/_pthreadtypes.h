#ifndef _SYS__PTHREADTYPES_H_
#define _SYS__PTHREADTYPES_H_

#include <stdint.h>
#include <sys/types.h>

typedef uint32_t pthread_t;

typedef struct pthread_attr {
	void *stack;
	uint32_t details[2];
} pthread_attr_t;

typedef uint32_t pthread_mutex_t;

typedef struct pthread_mutexattr {
	unsigned char type: 2;
	bool initialized: 1;
} pthread_mutexattr_t;

typedef uint32_t pthread_cond_t;

typedef struct pthread_condattr {
	clock_t clock;
} pthread_condattr_t;

typedef uint32_t pthread_key_t;

typedef struct pthread_once {
	bool flag;
} pthread_once_t;

#endif /* _SYS__PTHREADTYPES_H_ */