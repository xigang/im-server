/* *****************************************************************************
 * Author:         Xigang Wang
 * Email:          wangxigang2014@gmail.com
 * Last modfified: 2015-05-10 15:21
 * Filename:       server.c
 * Note:           任何人可以任意复制代码并运用这些文档，当然包括你的商业用途, 但请遵循GPL
 * *****************************************************************************/
#include "mysql_connect.h"

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
	MYSQL * conn = NULL;
	char pro_buf[2];
	char id_buf[15];
	char ip[32];
	char *pbuff;
	int j;

	conn = (MYSQL *) GetConnect();
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
	j = idIsExist(conn, id_buf);
	printf("j= %d\n", j);

	if (j == 2)
	{
		send(new_fd, PROTOCOL_LOGIN_AGAIN, strlen(PROTOCOL_LOGIN_AGAIN), 0);
	}
	else if (j == 1)
	{
		// 将新连接的 socket 连同 id 加入 array_sock
		sock_item new_item;
		new_item.new_fd = new_fd;
		strcpy(new_item.id, id_buf);
		g_array_append_val(array_sock, new_item);

		// 设置要回传的数据
		//user_list[i].online = 1;
		printf("%d\n", user_addr.sin_addr.s_addr);
		strcpy(ip, inet_ntoa(user_addr.sin_addr));
		printf("ip=%s\n", ip);
		updateOnlinestatus(conn, id_buf, "1");
		updateIp(conn, id_buf, ip);

		printf(".........begin........\n");
		printf("返回值是：%d\n", allKnoUsers(conn, &user_list));
		printf("........end.......\n");

		mysql_close(conn);

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
	MYSQL *conn = (MYSQL *) GetConnect();

	char *path;
	char *id_buf = (char*) malloc(15 * sizeof(char));
	char *id_sr = (char*) malloc(15 * sizeof(char));
	char sqlstate[100];

	char *username = (char*) malloc(20 * sizeof(char));
	char *buf2 = " ";
	char *buf1 = "\n";
	char cm[100];
	char id[15];
	FILE *fp;

	char *pbuff;
	int forward_fd;// 要转发的 socket
	int i;

	pbuff = recv_buf;

	pbuff += 1;
	strncpy(id_buf, pbuff, 15);
	printf("id_buf=%s\n", id_buf);
	for (i = 0; i < array_sock->len; i++)
	{
		sock_item item = g_array_index(array_sock, sock_item, i);
		if (new_fd == item.new_fd)
		{
			strcpy(id_sr, item.id);
			break;
		}
	}

	path = findAChatPath(conn, id_sr, id_buf);
	if (strlen(path) <= 2)
	{
		printf("path=....\n");

		memset(sqlstate, 0, sizeof(sqlstate));
		strcat(sqlstate, id_sr);
		strcat(sqlstate, "/");
		strcat(sqlstate, id_buf);

		path = sqlstate;
		printf("path=%s\n", path);
		for (i = 0; i < 15; i++)
			id[i] = *id_sr++;
		id[15] = '\0';
		id_sr = id_sr - 15;
		//printf("%s \n",id);
		sprintf(cm, "mkdir %s", id);
		system(cm);

		insertAChatPath(conn, id_sr, id_buf, path);

		insertAChatPath(conn, id_buf, id_sr, path);

	}
	printf("insert sucess!\n");
	printf("id_sr=%s\n", id_sr);

	strcpy(username, findNamebyId(conn, id_sr));
	printf("username= %s\n", username);
	mysql_close(conn);

	if (strlen(recv_buf) > 0)
	{

		//      FILE *fp;
		printf("ready to open the file\n");
		printf("%s \n", path);
		fp = fopen(path, "ab+");
		printf("fp %d\n", (int) fp);

		fwrite(username, sizeof(char), strlen(username), fp);
		fwrite(buf2, sizeof(char), strlen(buf2), fp);
		printf(" just open the file\n");

		time_t timep;
		struct tm *time_cur;
		char *time_str;
		time(&timep);
		time_cur = localtime(&timep);

		time_str = g_strdup_printf("%02d:%02d:%02d\n", time_cur->tm_hour,
				time_cur->tm_min, time_cur->tm_sec);

		fwrite(time_str, sizeof(char), strlen(time_str), fp);
		char *pmsg = pbuff + 15;
		if ((fwrite(pmsg, sizeof(char), nread - 16, fp)) < 0)
		{
			printf("保存信息失败！\n");
		}
		fwrite(buf1, sizeof(char), strlen(buf1), fp);
		fwrite(buf1, sizeof(char), strlen(buf1), fp);
		fclose(fp);

	}

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
		printf("recv_buf=%s,%s,%d\n", recv_buf, recv_buf + 16, nread);
		send(forward_fd, recv_buf, nread, 0);
	}
	else
	{
		/* Not Online */
	}
}

void recv_group_msg_func(char *recv_buf, int new_fd, int nread)
{
	MYSQL *conn = GetConnect();

	char *path;
	char *id_sr = (char*) malloc(15 * sizeof(char));
	char sqlstate[100];
	char *username = (char*) malloc(20 * sizeof(char));
	char *buf2 = " ";
	char *buf1 = "\n";
	char cm[100];
	FILE *fp;

	char id_buf[15];
	char *pbuff;
	int i;
	int forward_fd;// 要转发的 socket

	// 获取要转发的群 id
	pbuff = recv_buf;
	pbuff += strlen(PROTOCOL_RECV_GROUP_MSG);
	strncpy(id_buf, pbuff, 15);
	char *msg = pbuff + 15;
	strncpy(id_sr, msg, 15);
	printf("群 id：%s\n", id_buf);
	printf("消息：%s %s %s 共 %d 个字节\n", recv_buf, recv_buf + 16, recv_buf + 31,
			nread);
	path = findGroChatPath(conn, id_buf);
	if (strlen(path) <= 2)
	{
		memset(sqlstate, 0, sizeof(sqlstate));

		if (access("group", 0) != 0)
		{
			sprintf(cm, "mkdir %s", "group");
			system(cm);
		}

		strcat(sqlstate, "group");
		strcat(sqlstate, "/");
		strcat(sqlstate, id_buf);

		path = sqlstate;
		printf("path=%s\n", path);

		insertGroChatPath(conn, id_buf, path);

	}

	strcpy(username, findNamebyId(conn, id_sr));
	printf("username= %s\n", username);
	mysql_close(conn);

	if (strlen(recv_buf) > 0)
	{
		printf("ready to open the file\n");
		printf("%s \n", path);

		fp = fopen(path, "ab+");
		printf("fp %d\n", (int) fp);

		fwrite(username, sizeof(char), strlen(username), fp);
		fwrite(buf2, sizeof(char), strlen(buf2), fp);
		printf(" just open the file\n");

		time_t timep;
		struct tm *time_cur;
		char *time_str;
		time(&timep);
		time_cur = localtime(&timep);

		time_str = g_strdup_printf("%02d:%02d:%02d\n", time_cur->tm_hour,
				time_cur->tm_min, time_cur->tm_sec);

		fwrite(time_str, sizeof(char), strlen(time_str), fp);
		char *pmsg = pbuff + 30;
		if ((fwrite(pmsg, sizeof(char), nread - 31, fp)) < 0)
		{
			printf("保存信息失败！\n");
		}
		fwrite(buf1, sizeof(char), strlen(buf1), fp);
		fwrite(buf1, sizeof(char), strlen(buf1), fp);
		fclose(fp);

	}

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
	MYSQL *conn = GetConnect();

	char id_buf[15];
	char *path;
	char *id_sr = (char*) malloc(15 * sizeof(char));
	char sqlstate[100];
	char *username = (char*) malloc(20 * sizeof(char));
	char *buf2 = " ";
	char *buf1 = "\n";
	char cm[100];
	FILE *fp;
	char *pbuff;
	int i;
	int forward_fd;// 要转发的 socket

	for (i = 0; i < array_sock->len; i++)
	{
		sock_item item = g_array_index(array_sock, sock_item, i);
		if (new_fd == item.new_fd)
		{
			strcpy(id_sr, item.id);
			break;
		}
	}
	// 获取 id
	pbuff = recv_buf;
	pbuff += strlen(PROTOCOL_RECV_FILE);

	// 获取要转发的 id, 查询其响应的 socket
	strncpy(id_buf, pbuff, 15);
	path = findAChatPath(conn, id_sr, id_buf);
	if (strlen(path) <= 2)
	{
		printf("path=....\n");

		memset(sqlstate, 0, sizeof(sqlstate));
		strcat(sqlstate, id_sr);
		strcat(sqlstate, "/");
		strcat(sqlstate, id_buf);

		path = sqlstate;
		printf("path=%s\n", path);

		sprintf(cm, "mkdir %s", id_sr);
		system(cm);

		insertAChatPath(conn, id_sr, id_buf, path);

		insertAChatPath(conn, id_buf, id_sr, path);

	}
	printf("insert sucess!\n");
	printf("id_sr=%s\n", id_sr);

	strcpy(username, findNamebyId(conn, id_sr));
	printf("username= %s\n", username);
	mysql_close(conn);

	if (strlen(recv_buf) > 0)
	{
		printf("ready to open the file\n");
		printf("%s \n", path);
		fp = fopen(path, "ab+");
		printf("fp %d\n", (int) fp);

		fwrite(username, sizeof(char), strlen(username), fp);
		fwrite(buf2, sizeof(char), strlen(buf2), fp);
		printf(" just open the file\n");

		time_t timep;
		struct tm *time_cur;
		char *time_str;
		time(&timep);
		time_cur = localtime(&timep);

		time_str = g_strdup_printf("%02d:%02d:%02d\n", time_cur->tm_hour,
				time_cur->tm_min, time_cur->tm_sec);

		fwrite(time_str, sizeof(char), strlen(time_str), fp);
		char *pmsg = pbuff + 15;
		if ((fwrite(pmsg, sizeof(char), nread - 16, fp)) < 0)
		{
			printf("保存信息失败！\n");
		}
		fwrite(buf1, sizeof(char), strlen(buf1), fp);
		fwrite(buf1, sizeof(char), strlen(buf1), fp);
		fclose(fp);

	}

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
		if (len <= 0)
			printf(
					"server log:%d:send: %s error online,the erro cause is %d:%s\n",
					sockfd, user_list[n].id, errno, strerror(errno));
	}
	return 0;
}

