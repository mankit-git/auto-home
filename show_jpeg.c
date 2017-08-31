
#include "jpeg.h"

void read_image_from_file(int fd, unsigned char *jpg_buffer, unsigned long image_size)
{
	int nread = 0;
	int size = image_size;
	unsigned char *buf = jpg_buffer;

	// 循环地将jpeg文件中的所有数据，统统读取到jpg_buffer中
	while(size > 0)
	{
		nread = read(fd, buf, size);
		if(nread == -1)
		{
			printf("read jpg failed \n");
			exit(0);
		}
		size -= nread;
		buf +=nread;
	}
}

void write_lcd(unsigned char *FB, struct fb_var_screeninfo *vinfo, 
				unsigned char *rgb_buffer, struct image_info *imageinfo, int xoffset, int yoffset)
{
	FB += (xoffset + yoffset*vinfo->xres)*vinfo->bits_per_pixel/8;
	int x, y;
	for(y=0; y<imageinfo->height && y<vinfo->yres; y++)
	{
		for(x=0; x<imageinfo->width && x<vinfo->xres; x++)
		{
			int image_offset = x * 3 + y * imageinfo->width * 3;
			int lcd_offset   = x * 4 + y * vinfo->xres * 4;

			memcpy(FB+lcd_offset, rgb_buffer+image_offset+2, 1);
			memcpy(FB+lcd_offset+1, rgb_buffer+image_offset+1, 1);
			memcpy(FB+lcd_offset+2, rgb_buffer+image_offset, 1);
		}
	}
}

char *init_lcd(int lcd, struct fb_var_screeninfo *vinfo)
{
	ioctl(lcd, FBIOGET_VSCREENINFO, vinfo);

	unsigned long bpp = vinfo->bits_per_pixel;
	unsigned char *fbmem = mmap(NULL, vinfo->xres * vinfo->yres * bpp/8,
				    PROT_READ|PROT_WRITE, MAP_SHARED, lcd, 0);
	if(fbmem == MAP_FAILED)
	{
		perror("mmap() failed");
		exit(0);
	}

	return fbmem;
}

char *load_jpg(char *argv, struct image_info *image_info)
{
	// 获取文件的属性
	struct stat file_info;
	stat(argv, &file_info);

	// 打开jpeg文件
	int fd = open(argv, O_RDWR);
	if(fd == -1)
	{
		printf("open the argv failed\n");
	}

	// 根据获取的stat信息中的文件大小，来申请一块恰当的内存，用来存放jpeg编码的数据
	unsigned char *jpg_buffer = calloc(1, file_info.st_size);
	read_image_from_file(fd, jpg_buffer, file_info.st_size);

	/********* 以下代码，就是使用了jpeglib的函数接口，来将jpeg数据解码 *********/
	unsigned char *rgb_buffer = decompress_jpeg(jpg_buffer, file_info.st_size, image_info);
	free(jpg_buffer);
	/****************************************************************************/

	return rgb_buffer;
}

unsigned char *decompress_jpeg(unsigned char *jpg_buffer, int file_size, struct image_info *image_info)
{
struct jpeg_decompress_struct cinfo;
	struct jpeg_error_mgr jerr;

	cinfo.err = jpeg_std_error(&jerr);
	jpeg_create_decompress(&cinfo);

	jpeg_mem_src(&cinfo, jpg_buffer, file_size);

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
	//free(jpg_buffer);

	return rgb_buffer;
}

int lcd_fd(void)
{
	int lcd = open("/dev/fb0", O_RDWR);
	if(lcd == -1)
	{
		printf("open the lcd failed %s", strerror(errno));
		exit(1);
	}
	return lcd;
}


