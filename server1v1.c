#include "head.h"
#include "video.h"

int main(int argc, char **argv)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);

	int lcd = lcd_fd();

	struct fb_var_screeninfo vinfo;
	char *fbmemy = init_lcd(lcd, &vinfo);

	struct image_info *image_info = calloc(1, sizeof(image_info));

	// 打开摄像头设备文件
	int cam_fd = open("/dev/video7",O_RDWR); //video7这个路径不是一定的，根据情况而定

	int nbuf = camera(cam_fd, vinfo);
	
	unsigned char *start[nbuf];
	int length[nbuf];
	video_catchdata(nbuf, cam_fd, start, length);
	printf("%d\n", __LINE__);

	struct v4l2_buffer v4lbuf;
	bzero(&v4lbuf, sizeof(v4lbuf));
	v4lbuf.type  = V4L2_BUF_TYPE_VIDEO_CAPTURE;
	v4lbuf.memory= V4L2_MEMORY_MMAP;

	unsigned char *rgb_buf;
	int i = 0;

	struct sockaddr_in svaddr;
	socklen_t len = sizeof(svaddr);
	bzero(&svaddr, len);
	svaddr.sin_family = AF_INET;
	svaddr.sin_port = htons(atoi(argv[1]));
	svaddr.sin_addr.s_addr = htonl(INADDR_ANY);

	int on = 1;
	setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

	struct sockaddr_in cliaddr;
	len = sizeof(cliaddr);
	bzero(&cliaddr, len);

	if(bind(fd, (struct sockaddr *)&svaddr, len) == -1)
	{
		perror("bind() failed");
		exit(0);
	}

	listen(fd, 3);

	char *msg = malloc(BUFSIZE);
	int nread, total = 0;
	int connfd = accept(fd, (struct sockaddr *)&cliaddr, &len);
	if(connfd == -1)
	{
		perror("accept() failed");
		exit(0);
	}
	printf("welcome: %s:%hu\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
	while(1)
	{
		
		// 从队列中取出填满数据的缓存
		v4lbuf.index = i%nbuf;
		ioctl(cam_fd , VIDIOC_DQBUF, &v4lbuf); // VIDIOC_DQBUF在摄像头没数据的时候会阻塞
		rgb_buf = shooting(start[i%nbuf], length[i%nbuf], image_info);
		printf("%d\n", __LINE__);
		write_lcd(fbmemy, &vinfo, rgb_buf, image_info, 0, 0);
		printf("%d\n", __LINE__);

		total = 0;
		while(1)
		{
			nread = write(connfd, start[i%nbuf]+total, BUFSIZE-total);
			total += nread;
			printf("total-write: %d\n", total);
			if(total == BUFSIZE)
				break;

		}
			printf("%d\n", __LINE__);
	 	// 将已经读取过数据的缓存块重新置入队列中 
		v4lbuf.index = i%nbuf;
		ioctl(cam_fd , VIDIOC_QBUF, &v4lbuf);

		i++;	
	}
}

