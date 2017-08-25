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

typedef struct node
{
	int connfd;
	unsigned short id;
	struct list_head list;
}listnode, *linklist;

pthread_mutex_t m;
linklist head = NULL;

linklist init_list(void)
{
	linklist head = malloc(sizeof(listnode));
	if(head != NULL)
	{
		INIT_LIST_HEAD(&head->list);
	}
	return head;
}

linklist newnode(int connfd, unsigned short port)
{
	linklist new = calloc(1, sizeof(listnode));
	if(new !=  NULL)
	{
		new->connfd = connfd;
		new->id = port; 
		INIT_LIST_HEAD(&new->list);
	}
	return new;
}

void read_remove(linklist client)
{
	list_del(&client->list);

	free(client);
}

void show_list(linklist head)
{
	struct list_head *pos;
	linklist cli;

	list_for_each(pos, &head->list)
	{
		cli = list_entry(pos, listnode, list);
		printf("[%d] ID: %hu\n", __LINE__, cli->id);
	}
}

void broadcast(char *msg, linklist sender)
{
	struct list_head *pos;
	linklist cli;

	pthread_mutex_lock(&m);
	list_for_each(pos, &head->list)
	{
		cli = list_entry(pos, listnode, list);
		if(cli->id == sender->id)
			continue;

		write(cli->connfd, msg, strlen(msg));
	}
	pthread_mutex_unlock(&m);
}

void private_talk(char *msg, char *target_id)
{
	struct list_head *pos;
	linklist cli;

	pthread_mutex_lock(&m);
	list_for_each(pos, &head->list)
	{
		cli = list_entry(pos, listnode, list);
		if(cli->id == atoi(target_id))
		{
			write(cli->connfd, msg, strlen(msg));
			break;
		}
	}
	pthread_mutex_unlock(&m);
}

void *routine(void *arg)
{
	pthread_detach(pthread_self());
	linklist client = (linklist)arg;
	int connfd = client->connfd;
	unsigned short id = client->id;

	char msg[100];
	while(1)
	{
		bzero(msg, 100);
		if(read(connfd, msg, 100) == 0)
		{
			read_remove(client);
			break;
		}
		
		printf("recv: %s", msg);

		char *recv_msg = strstr(msg, ":");
		//群发
		if(recv_msg == NULL)
		{
			printf("broadcasting: %s", msg);
			broadcast(msg, client);
		}
		else //私发
		{
			printf("private: %s", msg);
			char *target_id = strtok(msg, ":");
			private_talk(strtok(NULL, ":"), target_id);
		}
	}

	close(connfd);
	pthread_exit(NULL);
}


int main(int argc, char **argv)
{


	int fd = socket(AF_INET, SOCK_STREAM, 0);

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

	pthread_mutex_init(&m, NULL);
	head = init_list();
	char msg[100];
	while(1)
	{
		int connfd = accept(fd, (struct sockaddr *)&cliaddr, &len);
		if(connfd == -1)
		{
			perror("accept() failed");
			exit(0);
		}
		printf("welcome: %s:%hu\n", inet_ntoa(cliaddr.sin_addr), ntohs(cliaddr.sin_port));
	
		pthread_mutex_lock(&m);
		linklist new = newnode(connfd, ntohs(cliaddr.sin_port));
		list_add(&new->list, &head->list);
		pthread_mutex_unlock(&m);

		show_list(head);
		pthread_t tid;
		pthread_create(&tid, NULL, routine, (void *)new);

	}
}

