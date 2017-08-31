
#include "head.h"

int main(int argc, char **argv)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	struct sockaddr_in svaddr;
	socklen_t len = sizeof(svaddr);
	bzero(&svaddr, len);

	svaddr.sin_family = AF_INET;
	svaddr.sin_port = htons(atoi(argv[2]));
	svaddr.sin_addr.s_addr = inet_addr(argv[1]);

	if(connect(fd, (struct sockaddr *)&svaddr, len) == -1)
	{
		perror("connect() failed");
		exit(0);
	}

	int lcd = lcd_fd();

	struct fb_var_screeninfo vinfo;
	char *fbmemy = init_lcd(lcd, &vinfo);

	struct image_info *image_info = calloc(1, sizeof(image_info));

	unsigned char *msg = malloc(BUFSIZE);
	int nread, total = 0;
	unsigned char *rgb_buf;
	while(1)
	{	
		total = 0;
		while(1)
		{
			nread = read(fd, msg+total, BUFSIZE-total);
			total += nread;
			printf("total-read: %d\n", total);
			if(total == BUFSIZE)
				break;
		}

		rgb_buf = decompress_jpeg(msg, BUFSIZE, image_info);

		write_lcd(fbmemy, &vinfo, rgb_buf, image_info);
	}
}
