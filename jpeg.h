#ifndef __JPEG_H_
#define __JPEG_H_

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

struct image_info
{
	int width;
	int height;
	int pixel_size;
};

int lcd_fd(void);

char *init_lcd(int lcd, struct fb_var_screeninfo *vinfo);

char *load_jpg(char *argv, struct image_info *image_info);

void write_lcd(unsigned char *FB, struct fb_var_screeninfo *vinfo, 
			unsigned char *rgb_buffer, struct image_info *imageinfo, int xoffset, int yoffset);

void read_image_from_file(int fd, unsigned char *jpg_buffer, unsigned long image_size);

unsigned char *decompress_jpeg(unsigned char *jpg_buffer, int file_size, struct image_info *image_info);

#endif 
