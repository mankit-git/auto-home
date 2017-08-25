
#include "head.h"

int led(int x, int flag)
{
	int led_fd,val;
	led_fd = open("/dev/Led",O_RDWR);                //打开设备下的LED，成功返回0
	if(led_fd == -1)
	{
		perror("Can not open /dev/LED\n");
		return 0;
	}

	ioctl(led_fd, LED(x),flag);

	close(led_fd);

	return 0;

}

int beep(int x)
{
	int fd;
	int ret;
	fd = open("/dev/beep", O_RDWR);            //打开设备，成功返回0
	if(fd<0)
	{
		perror("open:");
		return -1;
	}

	//printf("**********************buzzer On*************************\n");
	ret = ioctl(fd, x, 1);                 //BUZZER on
	if(ret < 0)
	{
		perror("ioctl:");
		return -1;
	}

}

char *temp()
{
}

void *routine(void *arg)
{
	pthread_detach(pthread_self());

	char ledon[4][5] = {"TL11", "TL21", "TL31", "TL41"};
	char ledoff[4][5] = {"TL10", "TL20", "TL30", "TL40"};
	char beepctl[2][5] = {"TF10", "TF11"};

	int fd = (int)arg;
	char rev[100];
	while(1)
	{
		bzero(rev, 100);

		read(fd, rev, 100);

		printf("rev msg: %s\n", rev);
	
		char recv[100];
		strtok(rev, "\n");
		strcpy(recv, rev);

		char *ledonmsg = NULL;
		char *ledoffmsg = NULL;
		
		int i;
		for(i=0; i<=3; i++)
		{
			if(strcmp(recv, ledon[i]) == 0)
			{
				led(i, 0);          //灯亮
			}
			if(strcmp(recv, ledoff[i]) == 0)
			{
				led(i, 1);          //灯灭
			}
		}

		if(strcmp(recv, beepctl[0]) == 0)
		{
			beep(1);    //on
		}
		if(strcmp(recv, beepctl[1]) == 0)
		{
			beep(0);   //off
		}
		if(strcmp(recv, "TH") == 0)
		{
			srand(time(NULL));
			int wh, hh;
			wh = rand()%1000;
			hh = rand()%1000;
			char a[10];
			sprintf(a, "ST%dTH%d\n", wh, hh);
			write(fd, a, 10);
		}
	}

}

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

	char msg[100];
	while(1)
	{	
		bzero(msg, 100);

		if(fgets(msg, 100, stdin) == NULL)
			break;

		write(fd, msg, strlen(msg));

		pthread_t tid;
		pthread_create(&tid, NULL, routine, (void *)fd);
	}
}
