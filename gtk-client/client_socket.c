/* *****************************************************************************
 * Author:         Xigang Wang
 * Email:          wangxigang2014@gmail.com
 * Last modfified: 2015-05-10 15:23
 * Filename:       client_socket.c
 * Note:           任何人可以任意复制代码并运用这些文档，当然包括你的商业用途, 但请遵循GPL
 * *****************************************************************************/
#include "lin.h"
#include "client_socket.h"

#define MAX_BUF				1024 * 2

int sockfd;

extern GtkWidget *g_treeview;
extern GData *widget_chat_dlg;// 聊天窗口集, 用于获取已经建立的聊天窗口
extern GArray *g_array_user; // 储存用户信息的全局变量

extern int socket_ret;// socket 连接 server 返回值
gboolean thread_quit;
extern char my_id[15];

// 同步所用
extern int gnum;
extern pthread_mutex_t mutex; // 互斥量
extern pthread_cond_t cond; // 条件变量

typedef struct
{
	char *file_path; // 文件路径
	unsigned long file_size; // 文件大小
	unsigned int user_index; // 发来消息的 id 在用户信息数组中的位置
	int sock;
} s_send_info; // 文件传送需要的结构体, 发送和接收文件 thread 中使用


int init_socket(const char *servip, uint uport)
{
	// 建立 TCP 连接
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)

	{
		perror("socket");
		return -1;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(uport);
	inet_pton(AF_INET, servip, &server_addr.sin_addr);

	// 连接服务器
	if ((connect(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)))
			!= 0)
	{
		perror("connect");
		return -1;
	}

	// 设置 thread 属性, 设置为分离线程
	pthread_t recv_thread;
	pthread_attr_t thread_attr;
	int ret;

	ret = pthread_attr_init(&thread_attr);
	if (ret != 0)
	{
		perror("init attribute failed");
		exit(EXIT_FAILURE);
	}

	// 分离线程
	ret = pthread_attr_setdetachstate(&thread_attr, PTHREAD_CREATE_DETACHED);
	if (ret != 0)
	{
		perror("setting detached attribute failed");
		exit(EXIT_FAILURE);
	}

	// 接收消息线程, 不断循环阻塞, 接收消息
	ret = pthread_create(&recv_thread, &thread_attr, &recv_msg_thread, NULL);
	if (ret != 0)
	{
		perror("thread create error");
		exit(EXIT_FAILURE);
	}

	(void) pthread_attr_destroy(&thread_attr);

	return 0;
}

// 发送消息函数, 由 gtk 框架中的 chat_single_dlg/chat_group_dlg 调用
int send_msg(const char *text, int size)
{
	int n = send(sockfd, text, size, 0);
	if (n > 0)
		printf("send: %s, %d byte", text, n);
	else
		printf("send: %s error,the erro cause is %d:%s\n", text, errno,
				strerror(errno));
	return n;
}

// 接收消息 thread
void *recv_msg_thread(void *arg)
{
	char recv_buf[MAX_BUF], *precv;
	char pro_buf[2];
	int recv_len;

	while (!thread_quit)
	{
		bzero(recv_buf, MAX_BUF);
		recv_len = recv(sockfd, recv_buf, MAX_BUF, 0);
		if (recv_len < 0)
		{
			perror("server terminated prematurely");
		}
		else if (recv_len == 0)
		{
			gdk_threads_enter();
			show_info_msg_box(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					"sever shutdown!");
			linpop_quit(NULL, NULL);
			gdk_threads_leave();

			printf("sever shutdown!\n");
			exit(-1);
		}

		bzero(pro_buf, 2);
		strncpy(pro_buf, recv_buf, 1);

		// 刷新在线用户树列表
		if (strcmp(pro_buf, PROTOCOL_ONLINE) == 0)
		{
			// 刷新在线用户
			precv = recv_buf;
			precv += 1;

			s_user_info *p_online = (s_user_info *) precv;
			int u_size = (recv_len - 1) / sizeof(s_user_info);

			if (g_array_user != NULL && g_array_user->len != 0)
				g_array_remove_range(g_array_user, 0, g_array_user->len);
			else
				g_array_user = g_array_new(FALSE, TRUE, sizeof(s_user_info));

			int i;

			// 先添加总群信息
			s_user_info s;
			memset(&s, 0, sizeof(s_user_info));
			s.item_type = PARENT_ITEM;
			strcpy(s.id, "All");
			strcpy(s.name, "Group");
			s.online = 1;
			g_array_append_val(g_array_user, s);

			s.item_type = CHILD_ITEM;
			memset(s.id, 0, sizeof(s.id));
			strcpy(s.id, "公司总群");
			g_array_append_val(g_array_user, s);

			for (i = 0; i < u_size; i++)
			{
				s = p_online[i];
				if (s.item_type == CHILD_ITEM)
					g_array_append_val(g_array_user, s);
			}

			// 各个部门下的信息
			for (i = 0; i < u_size; i++)
			{
				s_user_info s;
				s = p_online[i];
				g_array_append_val(g_array_user, p_online[i]);

				if (s.item_type == PARENT_ITEM)
				{
					s.item_type = CHILD_ITEM;
					g_array_append_val(g_array_user, s);
				}

			}

			pthread_mutex_lock(&mutex); /*获取互斥锁*/

			// 等待主窗口创建完成再初始化 tree view
			while (gnum == 0)
			{
				pthread_cond_wait(&cond, &mutex);
			}

			setup_tree_view_model(g_treeview, g_array_user, my_id);

			pthread_mutex_unlock(&mutex); /*释放互斥锁*/
		}
		// 重复登录
		else if (strcmp(pro_buf, PROTOCOL_LOGIN_AGAIN) == 0)
		{
			socket_ret = -2; // 传回 -2 表示重复登录
			gdk_threads_enter();
			show_info_msg_box(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					"您已经登录了，不能重复登录！");
			linpop_quit(NULL, NULL);
			gdk_threads_leave();
			thread_quit = 1;
		}
		// id 不存在
		else if (strcmp(pro_buf, PROTOCOL_ID_NOT_EXIST) == 0)
		{
			socket_ret = -3; // 传回 -3 表示 id/pwd 错误
			gdk_threads_enter();
			show_info_msg_box(GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,
					"您的ID不存在！");
			linpop_quit(NULL, NULL);
			gdk_threads_leave();
			thread_quit = 1;
		}
		// 收到单对单的消息
		else if (strcmp(pro_buf, PROTOCOL_RECV_MSG) == 0)
		{
			// 获取 id
			char id_buf[15];

			precv = recv_buf;
			precv += strlen(PROTOCOL_RECV_MSG);

			// 获取要转发的 id, 弹出对应对话框, 显示收到的消息
			strncpy(id_buf, precv, 15);

			precv += sizeof(my_id);

			int msg_len = recv_len - strlen(PROTOCOL_RECV_MSG) - sizeof(id_buf);
			char *msg_buf = malloc(msg_len);
			memcpy(msg_buf, precv, msg_len);

			// 查找 id 对应的 face num
			int i;
			for (i = 0; i < g_array_user->len; i++)
			{
				if (strcmp(g_array_index(g_array_user, s_user_info, i).id,
						id_buf) == 0)
					break;
			}

			int face_num = 0;
			if (i < g_array_user->len)
				face_num = g_array_index(g_array_user, s_user_info, i).face;

			gdk_threads_enter(); //在需要与图形窗口交互的时候加

			// 开始接收消息
			receive_text_msg(id_buf, id_buf, face_num, FALSE, msg_buf, msg_len);

			gdk_threads_leave(); //搭配上面的

			// 关键之处, gdk_threads_enter 和 gdk_threads_leave, 否则在线程中创建窗口会崩溃
		}
		// 收到群组消息
		else if (strcmp(pro_buf, PROTOCOL_RECV_GROUP_MSG) == 0)
		{
			// 获取 群 id, 哪个 id 发送来的消息
			char id_buf[15], id_from[15];

			precv = recv_buf;
			precv += strlen(PROTOCOL_RECV_GROUP_MSG);

			// 获取要转发的 id, 弹出对应对话框, 显示收到的消息
			strncpy(id_buf, precv, 15);
			precv += sizeof(my_id);

			strncpy(id_from, precv, 15);
			precv += sizeof(my_id);

			int msg_len = recv_len - strlen(PROTOCOL_RECV_GROUP_MSG) - 2
					* sizeof(id_buf);
			char *msg_buf = malloc(msg_len);
			memcpy(msg_buf, precv, msg_len);

			int i;
			for (i = 0; i < g_array_user->len; i++)
			{
				if (strcmp(g_array_index(g_array_user, s_user_info, i).id,
						id_buf) == 0)
					break;
			}

			int face_num = 0;
			if (i < g_array_user->len)
				face_num = g_array_index(g_array_user, s_user_info, i).face;

			gdk_threads_enter(); //在需要与图形窗口交互的时候加
			// 开始接收消息
			receive_text_msg(id_buf, id_from, face_num, TRUE, msg_buf, msg_len);
			gdk_threads_leave(); //搭配上面的

		}
		// 请求发送文件消息
		else if (strcmp(pro_buf, PROTOCOL_RECV_FILE) == 0)
		{
			process_file_trans(recv_buf, recv_len);
		}
	}
	return 0;
}

// 处理文件发送与接收
void process_file_trans(char *recv_buf, int recv_len)
{
	char *precv;
	char id_buf[15];

	precv = recv_buf;
	precv += strlen(PROTOCOL_RECV_FILE);

	strncpy(id_buf, precv, 15);
	precv += sizeof(my_id);

	int msg_len = recv_len - strlen(PROTOCOL_RECV_FILE) - sizeof(id_buf);
	char *msg_buf = malloc(msg_len);
	memcpy(msg_buf, precv, msg_len);

	// 查找 id 对应的用户信息
	int i;
	for (i = 0; i < g_array_user->len; i++)
	{
		if (strcmp(g_array_index(g_array_user, s_user_info, i).id, id_buf) == 0)
			break;
	}

	s_send_info send_info;
	if (i < g_array_user->len)
		send_info.user_index = i;

	gdk_threads_enter();

	int index = 0;
	char *suffix = str_suffix_addition(msg_buf, &index);

	// 发送方收到接收方的应答, 接收方方同意接收
	if (strcmp(suffix, "YES") == 0)
	{
		send_info.file_path = g_strndup(msg_buf, index);

		struct stat stat_buf;
		stat(send_info.file_path, &stat_buf); // 获取文件大小
		send_info.file_size = stat_buf.st_size;

		// 设置 thread 属性, 设置为分离线程
		pthread_t recv_thread;
		pthread_attr_t thread_attr;
		int ret;

		ret = pthread_attr_init(&thread_attr);
		if (ret != 0)
		{
			perror("init attribute failed");
			exit(EXIT_FAILURE);
		}
		// 分离线程
		ret
				= pthread_attr_setdetachstate(&thread_attr,
						PTHREAD_CREATE_DETACHED);
		if (ret != 0)
		{
			perror("setting detached attribute failed");
			exit(EXIT_FAILURE);
		}

		// 开启发送文件线程, TCP 方式传输, 由于 UDP 实在是太不可靠, 文件无法发送完整
		ret = pthread_create(&recv_thread, &thread_attr, &send_file_thread,
				&send_info);
		if (ret != 0)
		{
			perror("thread create error");
			exit(EXIT_FAILURE);
		}
		(void) pthread_attr_destroy(&thread_attr);

	}

	// 发送方收到接收方的应答, 接收方方拒绝接收

	else if (strcmp(suffix, "NO") == 0)
	{
		show_info_msg_box(GTK_MESSAGE_INFO, GTK_BUTTONS_OK,
				"receiver refuse accept the file");
	}

	// 接收方, 给与发送方应答, suffix 代表文件大小

	else
	{
		gchar *file_name = g_strdup(str_suffix_filename(g_strndup(msg_buf,
				index))); // 从路径提取的文件名
		char *info = g_strdup_printf(
				"do you want to receive file '%s' from %s", file_name, id_buf);

		if (GTK_RESPONSE_YES == show_info_msg_box(GTK_MESSAGE_QUESTION,
				GTK_BUTTONS_YES_NO, info))
		{
			GtkWidget *dialog;
			gchar *file_path;
			dialog = gtk_file_chooser_dialog_new("Save File As ...", NULL,
					GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_CANCEL,
					GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
					NULL);
			gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER (dialog),
					g_get_home_dir());
			gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER (dialog),
					file_name);

			gint result = gtk_dialog_run(GTK_DIALOG (dialog));
			if (result == GTK_RESPONSE_ACCEPT)
			{
				file_path = gtk_file_chooser_get_filename(
						GTK_FILE_CHOOSER (dialog));
				gtk_widget_destroy(dialog);

				send_info.file_path = file_path;
				send_info.file_size = atol(suffix);

				// 由于接收方发送给发送方确认消息 YES 后, 发送方马上会 connect 接收方
				// 但接收方需要建立服务, listen 还没完成, 此时发送方已经 connect 导致文件传送时
				// 连接失败, 故把原 recv_file_thread 线程中关于建立监听 socket 的步骤放到这,
				// 建立后再开启线程, 再发送回 YES 确认消息, 这样就不会出问题了

				// 建立监听 socket
				struct sockaddr_in server_addr;
				bzero(&server_addr, sizeof(server_addr));
				server_addr.sin_family = AF_INET;
				server_addr.sin_addr.s_addr = htons(INADDR_ANY);
				server_addr.sin_port = htons(FILE_TRANS_PORT);

				int server_socket = socket(PF_INET, SOCK_STREAM, 0);
				if (server_socket < 0)
				{
					perror("socket");
					return;
				}

				// 设置socket属性，端口可以重用
				int opt = SO_REUSEADDR;
				setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &opt,
						sizeof(opt));

				if (bind(server_socket, (struct sockaddr*) &server_addr,
						sizeof(server_addr)))
				{
					perror("bind");
					return;
				}

				//server_socket用于监听
				if (listen(server_socket, 10))
				{
					printf("listen");
					return;
				}

				// 传递给线程的 结构体变量 send_info
				send_info.sock = server_socket;

				// 设置 thread 属性, 设置为分离线程
				pthread_t recv_thread;
				pthread_attr_t thread_attr;
				int ret;

				ret = pthread_attr_init(&thread_attr);
				if (ret != 0)
				{
					perror("init attribute failed");
					exit(EXIT_FAILURE);
				}
				// 分离线程
				ret = pthread_attr_setdetachstate(&thread_attr,
						PTHREAD_CREATE_DETACHED);
				if (ret != 0)
				{
					perror("setting detached attribute failed");
					exit(EXIT_FAILURE);
				}

				// 开启接收文件线程, TCP 方式传输, 由于 UDP 实在是太不可靠, 文件无法发送完整
				ret = pthread_create(&recv_thread, &thread_attr,
						&recv_file_thread, &send_info);
				if (ret != 0)
				{
					perror("thread create error");
					exit(EXIT_FAILURE);
				}
				(void) pthread_attr_destroy(&thread_attr);

				// 必须得等待 recv_file_thread 线程中建立好监听后才能回送确认信息,
				// 否则对方 connect, 而此时接收方还未建立好监听, 导致文件传输一开始就无法建立连接

				// 发送给服务器消息, 通过服务器转发
				char *send_buf;
				int send_size;
				send_buf = recv_buf;
				send_buf = send_buf + index + strlen(PROTOCOL_RECV_FILE)
						+ sizeof(my_id);
				send_size = strlen(PROTOCOL_RECV_FILE) + sizeof(my_id) + index
						+ sizeof("-YES");
				memcpy(send_buf, "-YES", sizeof("-YES"));

				// 回发接收确认给发送方, 建立 TCP Socket 进行通信
				send_msg(recv_buf, send_size);
			}
			else
			{
				gtk_widget_destroy(dialog);

				// 回传拒绝接收消息给发送方
				char *send_buf;
				int send_size;
				send_buf = recv_buf;
				send_buf = send_buf + index + strlen(PROTOCOL_RECV_FILE)
						+ sizeof(my_id);
				send_size = strlen(PROTOCOL_RECV_FILE) + sizeof(my_id) + index
						+ sizeof("-NO");
				memcpy(send_buf, "-NO", sizeof("-NO"));

				// 回发取消接收给发送方, 拒绝接收文件
				send_msg(recv_buf, send_size);
			}
		}
		else
		{
			// 回传拒绝接收消息给发送方
			char *send_buf;
			int send_size;
			send_buf = recv_buf;
			send_buf = send_buf + index + strlen(PROTOCOL_RECV_FILE)
					+ sizeof(my_id);
			send_size = strlen(PROTOCOL_RECV_FILE) + sizeof(my_id) + index
					+ sizeof("-NO");
			memcpy(send_buf, "-NO", sizeof("-NO"));

			// 回发取消接收给发送方, 拒绝接收文件
			send_msg(recv_buf, send_size);
		}
	}
	gdk_threads_leave();
}

// 发送文件 thread
void *send_file_thread(void *send_info)
{
	gdk_threads_enter();

	// 文件传输隶属于聊天对话框
	s_user_info uinfo;
	uinfo
			= g_array_index(g_array_user, s_user_info, ((s_send_info *) send_info)->user_index);
	Widgets_Chat *w_chatdlg =
			(Widgets_Chat *) g_datalist_get_data(&widget_chat_dlg, uinfo.id);
	if (w_chatdlg == NULL)
	{
		w_chatdlg = create_chat_dlg(uinfo.id, uinfo.face, FALSE);
		g_datalist_set_data(&widget_chat_dlg, uinfo.id, w_chatdlg);
	}
	else
		gtk_widget_show_all(w_chatdlg->chat_dlg);
	gtk_progress_bar_set_fraction(w_chatdlg->progress, 0);
	gdk_threads_leave();

	// 开启文件发送 socket, 连接接收方 socket, 开始发送文件数据
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket < 0)
	{
		perror("socket");
		return 0;
	}

	struct sockaddr_in server_addr;
	bzero(&server_addr, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr = uinfo.ip_addr;
	server_addr.sin_port = htons(FILE_TRANS_PORT);
	socklen_t server_addr_length = sizeof(server_addr);

	if (connect(client_socket, (struct sockaddr*) &server_addr,
			server_addr_length) < 0)
	{
		perror("connect");
		return 0;
	}

	char buffer[MAX_BUF];
	bzero(buffer, MAX_BUF);

	FILE * fp = fopen(((s_send_info *) send_info)->file_path, "r");
	if (NULL == fp)
	{
		perror("open file");
		return 0;
	}

	unsigned long file_len = ((s_send_info *) send_info)->file_size;
	unsigned long send_add, send_percent;
	send_add = send_percent = (unsigned long) (double) file_len / 100;
	unsigned long has_send = 0;

	bzero(buffer, MAX_BUF);
	int file_block_length = 0;
	while ((file_block_length = fread(buffer, sizeof(char), MAX_BUF, fp)) > 0)
	{
		printf("file_block_length = %d\n", file_block_length);

		if (send(client_socket, buffer, file_block_length, 0) < 0)
		{
			perror("send file");
			break;
		}

		gdk_threads_enter(); //在需要与图形窗口交互的时候加

		has_send += file_block_length;
		if (file_len <= MAX_BUF)
		{
			gtk_progress_bar_set_fraction(w_chatdlg->progress, 1);
			gtk_progress_bar_set_text(w_chatdlg->progress, "%100 Complete");
		}
		else if (has_send >= send_add)
		{
			send_add += send_percent;

			double new_val = (double) has_send / file_len;

			if (new_val <= 1)
				gtk_progress_bar_set_fraction(w_chatdlg->progress, new_val);

			gchar *message = g_strdup_printf("%.0f%% Complete", new_val * 100);
			gtk_progress_bar_set_text(w_chatdlg->progress, message);
		}
		bzero(buffer, MAX_BUF);
		gdk_threads_leave(); //搭配上面的
	}

	fclose(fp);
	close(client_socket);

	printf("file:\t %s transfer finished\n",
			((s_send_info *) send_info)->file_path);

	gdk_threads_enter();
	gtk_progress_bar_set_fraction(w_chatdlg->progress, 1);
	show_info_msg_box(GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "file trans finished!");
	gtk_widget_hide(GTK_WIDGET(w_chatdlg->progress));
	gtk_widget_hide(w_chatdlg->label_file);
	gtk_progress_bar_set_fraction(w_chatdlg->progress, 0);
	gtk_progress_bar_set_text(w_chatdlg->progress, "");
	gdk_threads_leave();

	return 0;
}

// 接收文件 thread
void *recv_file_thread(void *send_info)
{
	gdk_threads_enter();
	// 文件传输隶属于聊天对话框
	s_user_info uinfo;
	uinfo
			= g_array_index(g_array_user, s_user_info, ((s_send_info *) send_info)->user_index);
	Widgets_Chat *w_chatdlg =
			(Widgets_Chat *) g_datalist_get_data(&widget_chat_dlg, uinfo.id);
	if (w_chatdlg == NULL)
	{
		w_chatdlg = create_chat_dlg(uinfo.id, uinfo.face, FALSE);
		g_datalist_set_data(&widget_chat_dlg, uinfo.id, w_chatdlg);
	}
	else
		gtk_widget_show_all(w_chatdlg->chat_dlg);
	gtk_progress_bar_set_fraction(w_chatdlg->progress, 0);

	gtk_label_set_text(GTK_LABEL(w_chatdlg->label_file),
			((s_send_info *) send_info)->file_path);
	gtk_widget_show(GTK_WIDGET(w_chatdlg->progress));
	gtk_widget_show(w_chatdlg->label_file);
	gdk_threads_leave();

	struct sockaddr_in client_addr;
	socklen_t len = sizeof(client_addr);

	int new_server_socket = accept(((s_send_info *) send_info)->sock,
			(struct sockaddr*) &client_addr, &len);
	if (new_server_socket < 0)
	{
		perror("accept");
		return 0;
	}

	char buffer[MAX_BUF];
	bzero(buffer, MAX_BUF);

	unsigned long file_len = ((s_send_info *) send_info)->file_size;
	unsigned long recv_add, recv_percent;
	recv_add = recv_percent = (unsigned long) (double) file_len / 100;
	unsigned long has_recv = 0;

	FILE * fp = fopen(((s_send_info *) send_info)->file_path, "w");
	if (NULL == fp)
	{
		perror("file");
		return 0;
	}
	else
	{
		bzero(buffer, MAX_BUF);
		int recv_len = 0;
		while ((recv_len = recv(new_server_socket, buffer, MAX_BUF, 0)))
		{
			if (recv_len < 0)
			{
				perror("receive file");
				break;
			}

			int write_length = fwrite(buffer, sizeof(char), recv_len, fp);
			if (write_length < recv_len)
			{
				perror("file write");
				break;
			}
			bzero(buffer, MAX_BUF);

			gdk_threads_enter(); //在需要与图形窗口交互的时候加

			has_recv += write_length;
			if (file_len <= MAX_BUF)
			{
				gtk_progress_bar_set_fraction(w_chatdlg->progress, 1);
				gtk_progress_bar_set_text(w_chatdlg->progress, "%100 Complete");
			}
			else if (has_recv >= recv_add)
			{
				recv_add += recv_percent;

				double new_val = (double) has_recv / file_len;

				if (new_val <= 1)
					gtk_progress_bar_set_fraction(w_chatdlg->progress, new_val);

				gchar *message = g_strdup_printf("%.0f%% Complete", new_val
						* 100);
				gtk_progress_bar_set_text(w_chatdlg->progress, message);
			}
			gdk_threads_leave(); //搭配上面的
		}
		fclose(fp);
	}
	close(new_server_socket);
	close(((s_send_info *) send_info)->sock);

	printf("file:\t %s transfer finished\n",
			((s_send_info *) send_info)->file_path);

	gdk_threads_enter();
	gtk_progress_bar_set_fraction(w_chatdlg->progress, 1);
	show_info_msg_box(GTK_MESSAGE_INFO, GTK_BUTTONS_OK, "file trans finished!");
	gtk_widget_hide(GTK_WIDGET(w_chatdlg->progress));
	gtk_widget_hide(w_chatdlg->label_file);
	gtk_progress_bar_set_fraction(w_chatdlg->progress, 0);
	gtk_progress_bar_set_text(w_chatdlg->progress, "");
	gdk_threads_leave();

	return 0;
}

// 从文件路径字符串中根据符号 “-” 分离出附加信息, 例如文件大小, 是否接收文件等,
// 同时回传文件路径信息(包含文件名)
char *str_suffix_addition(const char *str, int *index)
{
	const char *ptr;
	char *suffix;

	ptr = strrchr(str, '-'); // strrchr 返回 字符 ‘-’ 在字符串中的最后一次出现的位置
	if (ptr && ptr != str)
	{
		suffix = g_strndup(ptr + 1, str + strlen(str) - ptr - 1);
		*index = ptr - str;
	}
	else
	{
		suffix = g_strdup(str);
		*index = 0;
	}

	return suffix;
}

// 从文件路径字符串中分离出文件名, 不包含路径信息, 供接收方用
char *str_suffix_filename(const char *str)
{
	const char *ptr;
	char *suffix;

	ptr = strrchr(str, '/'); // strrchr 返回 字符 ‘-’ 在字符串中的最后一次出现的位置
	if (ptr && ptr != str)
		suffix = g_strndup(ptr + 1, str + strlen(str) - ptr - 1);
	else
		suffix = g_strdup(str);

	return suffix;
}
