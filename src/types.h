

#ifndef __TYPES_H__
#define __TYPES_H__

#define BOOL int
#define TRUE 1
#define FALSE 0
#define YES TRUE
#define NO FALSE

#define MIN(a,b) (a<b?a:b)

#define NULL ((void*)0)

#define OSErr int
#define OK 0
#define UNKNOWN_ERR -1

#define ifOK(x)  if(OK == (x))
#define ifErr(x) if(OK != (x))

#define whileOK(x) while(OK == (x))
#define whileErr(x) while(OK != (x))

#define EAGAIN 13

typedef unsigned int size_t;

#endif
