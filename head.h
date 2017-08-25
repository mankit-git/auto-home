#ifndef __HEAD_H_
#define __HEAD_H_

#include <stdio.h>
#include <pthread.h>
#include <strings.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include "kernel_list.h"
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <fcntl.h>


#define TEST_MAGIC 'x' 

#define LED(x) _IO(TEST_MAGIC, (x))

struct ledctl
{
	char l0[10];
	char l1[10];
	char l2[10];
	char l3[10];
};

struct beepctl
{
	char beepon[10];
	char beepoff[10];
};

#endif




