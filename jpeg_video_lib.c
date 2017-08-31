////////////////////////////////////////////////////////////////////
//
//  Copyright(C), 2013-2016, GEC Tech. Co., Ltd.
//
//  File name: GPLE/ch06/6.4/v4l2_jpeg_videostream.c  
//
//  Author: Vincent Lin (林世霖)  微信公众号：秘籍酷
//
//  Date: 2016-3
//  
//  Description: 使用V4L2机制捕获摄像头的jpeg数据，并用jpeg库解码这些
//               数据，将它们显示到LCD上
//
//  GitHub: github.com/vincent040   Bug Report: 2437231462@qq.com
//
////////////////////////////////////////////////////////////////////

#include "video.h"

char *shooting(char *jpegdata, int size, struct image_info *image_info)
{	
	struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpegdata, size);

	int ret = jpeg_read_header(&cinfo, true);
	if(ret != 1)
	{
		fprintf(stderr, "jpeg_read_header failed: %s\n", strerror(errno));
		exit(1);
	}

	jpeg_start_decompress(&cinfo);

	/////////  在解码jpeg数据的同时，顺便将图像的尺寸信息保留下来
	if(image_info == NULL)
	{
		printf("malloc image_info failed\n");
	}	

	image_info->width = cinfo.output_width;
	image_info->height = cinfo.output_height;
	image_info->pixel_size = cinfo.output_components;
	/////////

	int row_stride = image_info->width * image_info->pixel_size;

	unsigned long rgb_size;
	unsigned char *rgb_buffer;
	rgb_size = image_info->width * image_info->height * image_info->pixel_size;

	rgb_buffer = calloc(1, rgb_size);

	int line = 0;
	while(cinfo.output_scanline < cinfo.output_height)
	{
		unsigned char *buffer_array[1];
		buffer_array[0] = rgb_buffer + (cinfo.output_scanline) * row_stride;
		jpeg_read_scanlines(&cinfo, buffer_array, 1);
	}

	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return rgb_buffer;
}

void show_camfmt(struct v4l2_format *fmt)
{
	printf("camera width : %d \n", fmt->fmt.pix.width);
	printf("camera height: %d \n", fmt->fmt.pix.height);

	switch(fmt->fmt.pix.pixelformat)
	{
	case V4L2_PIX_FMT_JPEG:
		printf("camera pixelformat: V4L2_PIX_FMT_JPEG\n");
		break;
	case V4L2_PIX_FMT_YUYV:
		printf("camera pixelformat: V4L2_PIX_FMT_YUYV\n");
	}
}

char *lcd_mem(int lcd, struct fb_var_screeninfo *lcdinfo)
{
	// 申请一块适当跟LCD尺寸一样大小的显存
	unsigned char *fb_mem = mmap(NULL, lcdinfo->xres * lcdinfo->yres * lcdinfo->bits_per_pixel/8,
				    PROT_READ | PROT_WRITE, MAP_SHARED, lcd, 0);

	// 将屏幕刷成黑色
	unsigned long black = 0x0;
	unsigned int i;
	for(i=0; i<lcdinfo->xres * lcdinfo->yres; i++)
	{
		memcpy(fb_mem+i, &black, sizeof(unsigned long));
	}

	return fb_mem;
}

int camera(int cam_fd, struct fb_var_screeninfo lcdinfo)
{
	struct v4l2_fmtdesc *a = calloc(1, sizeof(*a));
	a->index = 0;
	a->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	int ret;
	while((ret=ioctl(cam_fd, VIDIOC_ENUM_FMT, a)) == 0)
	{
		a->index++;
		printf("pixelformat: \"%c%c%c%c\"\n",
				(a->pixelformat >> 0) & 0XFF,
				(a->pixelformat >> 8) & 0XFF,
				(a->pixelformat >>16) & 0XFF,
				(a->pixelformat >>24) & 0XFF);

		printf("description: %s\n", a->description);
	}

	// 获取摄像头设备的功能参数(这一步可以不要)
	struct v4l2_capability cap;
	ioctl(cam_fd, VIDIOC_QUERYCAP, &cap);

	// 获取摄像头当前的采集格式
	struct v4l2_format *fmt = calloc(1, sizeof(*fmt));
	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;

	ioctl(cam_fd, VIDIOC_G_FMT, fmt);
	show_camfmt(fmt);

	// 配置摄像头的采集格式
	bzero(fmt, sizeof(*fmt));
	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	fmt->fmt.pix.width = lcdinfo.xres;
	fmt->fmt.pix.height = lcdinfo.yres;
	fmt->fmt.pix.pixelformat = V4L2_PIX_FMT_JPEG;
	fmt->fmt.pix.field = V4L2_FIELD_INTERLACED;
	ioctl(cam_fd, VIDIOC_S_FMT, fmt);

	// 再次获取摄像头当前的采集格式
	bzero(fmt, sizeof(*fmt));
	fmt->type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl (cam_fd, VIDIOC_G_FMT, fmt);

	if(fmt->fmt.pix.pixelformat != V4L2_PIX_FMT_JPEG)
	{
		printf("\nthe pixel format is NOT JPEG.\n\n");
		exit(1);
	}

	// 设置即将要申请的摄像头缓存的参数
	int nbuf = 4;

	struct v4l2_requestbuffers reqbuf;
	bzero(&reqbuf, sizeof (reqbuf));
	reqbuf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	reqbuf.memory = V4L2_MEMORY_MMAP;
	reqbuf.count = nbuf;

	// 使用该参数reqbuf来申请缓存
	ioctl(cam_fd, VIDIOC_REQBUFS, &reqbuf);

	return nbuf;
}

void video_catchdata(int nbuf, int cam_fd, unsigned char *start[nbuf], int length[nbuf])
{
	// 根据刚刚设置的reqbuf.count的值，来定义相应数量的struct v4l2_buffer
	// 每一个struct v4l2_buffer对应内核摄像头驱动中的一个缓存
	struct v4l2_buffer buffer[nbuf];

	int i;
	for(i=0; i<nbuf; i++)
	{
		bzero(&buffer[i], sizeof(buffer[i]));
		buffer[i].type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
		buffer[i].memory = V4L2_MEMORY_MMAP;
		buffer[i].index = i;
		ioctl(cam_fd, VIDIOC_QUERYBUF, &buffer[i]);

		length[i] = buffer[i].length;
		start[i] = mmap(NULL, buffer[i].length,	PROT_READ | PROT_WRITE,
				  MAP_SHARED,	cam_fd, buffer[i].m.offset);

		ioctl(cam_fd , VIDIOC_QBUF, &buffer[i]);
	}

	// 启动摄像头数据采集
	enum v4l2_buf_type vtype= V4L2_BUF_TYPE_VIDEO_CAPTURE;
	ioctl(cam_fd, VIDIOC_STREAMON, &vtype);
}



