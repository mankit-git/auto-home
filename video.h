#ifndef __VIDEO_H_
#define __VIDEO_H_

#include <stdio.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <syslog.h>
#include <errno.h>
#include <linux/input.h>

#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <stdbool.h>

#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ioctl.h>

#include "jpeglib.h"
#include "jpeg.h"

#include <linux/videodev2.h>
#include <setjmp.h>

char *shooting(char *jpegdata, int size, struct image_info *image_info);

void show_camfmt(struct v4l2_format *fmt);

char *lcd_mem(int lcd, struct fb_var_screeninfo *lcdinfo);

int camera(int cam_fd, struct fb_var_screeninfo lcdinfo);

void video_catchdata(int nbuf, int cam_fd, unsigned char *start[nbuf], int length[nbuf]);

#endif 