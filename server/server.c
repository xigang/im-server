#include "server.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

void message_parser(char *buffer, int sock, int nread)
{
	// Dtermine the MSG type
	char msg_type = buffer[0];

	switch (msg_type)
	{
	case '1':
		client_login(buffer, sock, nread);
		break;
	case '2':
		msg_proc(buffer, sock, nread);
		break;
	case '3':
		recv_group_msg_func(buffer, sock, nread);
		break;
	case '4':
		recv_file_func(buffer, sock, nread);
		break;
	default:
		printf("Unknown Message Type! \n");
		printf("buffer is %s", buffer);
	}

}

void client_login(char *recv_buf, int new_fd, int nread)
{
	char pro_buf[2];
	char id_buf[15];
	char *pbuff;
	int i;

	// 开始处理每个新连接上的数据收发
	memset(pro_buf, 0, sizeof(pro_buf));
	memset(id_buf, 0, sizeof(id_buf));

	// 获取 user ip
	struct sockaddr_in user_addr;
	socklen_t namelen = sizeof(struct sockaddr_in);
	getpeername(new_fd, (struct sockaddr *) &user_addr, &namelen);

	// 获取 id
	pbuff = recv_buf;
	pbuff += strlen(PROTOCOL_ONLINE);
	strcpy(id_buf, pbuff);

	for (i = 0; i < sizeof(user_list) / sizeof(s_user_info); i++)
		if (strcmp(user_list[i].id, id_buf) == 0)
			break;

	if (i < sizeof(user_list) / sizeof(s_user_info))
	{
		// 已经登录, 不能重复登录
		if (user_list[i].online != 0)
			send(new_fd, PROTOCOL_LOGIN_AGAIN, strlen(PROTOCOL_LOGIN_AGAIN), 0);

		// 将新连接的 socket 连同 id 加入 array_sock
		sock_item new_item;
		new_item.new_fd = new_fd;
		strcpy(new_item.id, id_buf);
		g_array_append_val(array_sock, new_item);

		// 设置要回传的数据
		user_list[i].online = 1;
		user_list[i].ip_addr = user_addr.sin_addr;

		// 开启线程, 发送当前上线用户
		int thread_ret = pthread_create(&thread_online, &thread_attr,
				&send_online_list, NULL);
		if (thread_ret != 0)
		{
			perror("server log:thread create error");
			exit(EXIT_FAILURE);
		}
	}
	// 用户 id 不存在
	else
	{
		send(new_fd, PROTOCOL_ID_NOT_EXIST, strlen(PROTOCOL_ID_NOT_EXIST), 0);
	}
}

void msg_proc(char *recv_buf, int new_fd, int nread)
{
	char id_buf[15];
	char *pbuff;
	int forward_fd;// 要转发的 socket
	int i;

	// 获取 id
	pbuff = recv_buf;
	pbuff += strlen(PROTOCOL_RECV_MSG);

	strncpy(id_buf, pbuff, 15);
	for (i = 0; i < array_sock->len; i++)
	{
		sock_item item = g_array_index(array_sock, sock_item, i);
		if (strcmp(id_buf, item.id) == 0)
		{
			strcpy(id_buf, item.id);
			forward_fd = item.new_fd; // 待转发的 socket
			break;
		}
	}

	if (i < array_sock->len)
	{
		memset(id_buf, 0, sizeof(id_buf));
		for (i = 0; i < array_sock->len; ++i)
		{
			sock_item item = g_array_index(array_sock, sock_item, i);
			if (new_fd == item.new_fd)
			{
				strcpy(id_buf, item.id);
				break;
			}
		}
		strncpy(pbuff, id_buf, 15);
		send(forward_fd, recv_buf, nread, 0);
	}
	else
	{
		/* Not Online */
	}
}

void recv_group_msg_func(char *recv_buf, int new_fd, int nread)
{
	char id_buf[15];
	char *pbuff;
	int i;
	int forward_fd;// 要转发的 socket

	// 获取要转发的群 id
	pbuff = recv_buf;
	pbuff += strlen(PROTOCOL_RECV_GROUP_MSG);
	strncpy(id_buf, pbuff, 15);

	printf("群 id：%s\n", id_buf);
	printf("消息：%s %s %s 共 %d 个字节\n", recv_buf, recv_buf + 16, recv_buf + 31,
			nread);

	// 查找群 id 在 user_list 中的位置
	for (i = 0; i < sizeof(user_list) / sizeof(s_user_info); i++)
		if (strcmp(user_list[i].id, id_buf) == 0)
			break;

	// 查找群组对应的上线 socket, 对每个 socket 进行转发

	int j, k;
	if (i < sizeof(user_list) / sizeof(s_user_info))
	{
		printf("群 id：%s 找到了\n", id_buf);

		for (j = i + 1; user_list[j].item_type != PARENT_ITEM; j++)
		{
			// 从已经连接的 socket 中找隶属于这个群的用户
			for (k = 0; k < array_sock->len; k++)
			{
				sock_item item = g_array_index(array_sock, sock_item, k);
				if (strcmp(user_list[j].id, item.id) == 0) // 群/部门下的在线用户 id
				{
					if (new_fd != item.new_fd) // 自己就不用往回发送了
					{
						printf("要发送的 id：%s 找到了\n", user_list[j].id);
						forward_fd = item.new_fd; // 待转发的 socket
						break;
					}
				}
			}

			// 找到了
			if (k < array_sock->len)
			{
				// 发送给每一个部门下的用户
				int send_len = send(forward_fd, recv_buf, nread, 0);
				printf("发送消息：%s %s %s 共 %d 个字节\n", recv_buf, recv_buf + 16,
						recv_buf + 31, send_len);
			}
		}
	}
	else if (strcmp(id_buf, "公司总群") == 0) // 总群, 对公司所有用户转发
	{
		printf("群 id：%s 没有找到, 是公司总群\n", id_buf);

		for (k = 0; k < array_sock->len; k++)
		{
			sock_item item = g_array_index(array_sock, sock_item, k);

			if (new_fd != item.new_fd) // 自己就不用往回发送了
			{
				forward_fd = item.new_fd; // 待转发的 socket

				// 发送给已经上线的全部用户
				int send_len = send(forward_fd, recv_buf, nread, 0);
				printf("发送消息：%s %s %s 共 %d 个字节\n", recv_buf, recv_buf + 16,
						recv_buf + 31, send_len);
			}
		}
	}
}

void recv_file_func(char *recv_buf, int new_fd, int nread)
{
	char id_buf[15];
	char *pbuff;
	int i;
	int forward_fd;// 要转发的 socket

	// 获取 id
	pbuff = recv_buf;
	pbuff += strlen(PROTOCOL_RECV_FILE);

	// 获取要转发的 id, 查询其响应的 socket
	strncpy(id_buf, pbuff, 15);
	for (i = 0; i < array_sock->len; i++)
	{
		sock_item item = g_array_index(array_sock, sock_item, i);
		if (strcmp(id_buf, item.id) == 0)
		{
			strcpy(id_buf, item.id);
			forward_fd = item.new_fd; // 待转发的 socket
			break;
		}
	}

	if (i < array_sock->len) // 找到 待转发的 socket
	{
		// 把收到的数据包, 改一下其中的 id, 把原来的接收方 id 变成发送方的 id, 转发给接收方
		memset(id_buf, 0, sizeof(id_buf));
		for (i = 0; i < array_sock->len; i++)
		{
			sock_item item = g_array_index(array_sock, sock_item, i);
			if (new_fd == item.new_fd)
			{
				strcpy(id_buf, item.id);
				break;
			}
		}
	}

	strncpy(pbuff, id_buf, 15);
	send(forward_fd, recv_buf, nread, 0);
}

void *send_online_list(void *arg)
{
	int sockfd;
	int n, len;
	char *send_buf, *psend;
	send_buf = malloc(sizeof(user_list) + strlen(PROTOCOL_ONLINE));
	psend = send_buf;
	char *pro_buf = strdup(PROTOCOL_ONLINE);
	memcpy(send_buf, pro_buf, 1);
	psend += 1;
	memcpy(psend, (char *) user_list, sizeof(user_list));

	// 刷新在线用户列表
	for (n = 0; n < array_sock->len; n++)
	{
		sockfd = g_array_index(array_sock, sock_item, n).new_fd;

		len = send(sockfd, send_buf, sizeof(user_list) + 6, 0);
		if (len > 0)
		{
			int i = 0;
			/*
			 for (i = 0; i < sizeof(user_list) / sizeof(s_user_info); i++)
			 {
			 printf("id : %s\n", user_list[i].id);
			 printf("server log:%d:send: id  %s online\n", sockfd,
			 user_list[i].id);
			 printf("server log:%d:send: face %d online\n", sockfd,
			 user_list[i].face);
			 printf("server log:%d:send: %d online\n", sockfd,
			 user_list[i].ip_addr.s_addr);
			 printf("server log:%d:send: item_type  %d online\n", sockfd,
			 user_list[i].item_type);
			 printf("server log:%d:send: name  %s online\n", sockfd,
			 user_list[i].name);
			 printf("server log:%d:send: online  %d online\n", sockfd,
			 user_list[i].online);

			 printf("\n");
			 }
			 */
		}
		else
			printf(
					"server log:%d:send: %s error online,the erro cause is %d:%s\n",
					sockfd, user_list[n].id, errno, strerror(errno));
	}
}
