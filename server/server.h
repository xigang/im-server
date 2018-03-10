
#ifndef _LINPOP_I_H
#define _LINPOP_I_H 1

#include <netinet/in.h>
#include <glib.h>

#define SERVER_PORT		6000
#define MAX_BUF 		1024 
#define THREAD_COUNT 200 

// 自定义协议格式, 用于拆包和组包
#define PROTOCOL_ONLINE					"1"
#define PROTOCOL_RECV_MSG				"2"
#define PROTOCOL_RECV_GROUP_MSG			"3"
#define PROTOCOL_RECV_FILE				"4"
#define PROTOCOL_LOGIN_AGAIN			"5"
#define PROTOCOL_ID_NOT_EXIST			"6"

pthread_t thread_online;
pthread_attr_t thread_attr;

enum
{
	PARENT_ITEM, CHILD_ITEM
};

typedef struct
{
	int item_type;
	char id[15]; // user id
	char name[20]; // user nickname
	struct in_addr ip_addr; // user ip address at in_addr format
	int face; // user face num
	char online; // user is online
} s_user_info;

typedef struct
{
	char id[15];
	int new_fd;
} sock_item; // 将已经连接的 socket 和 id 号关联起来, 便于从 socket 查询主关键字 id

extern GArray *array_sock;
extern s_user_info user_list[13];

void* thread_proc(void *arg);

void message_parser(char *buffer, int sock, int nread);
void *send_online_list(void *arg);
void client_login(char *buffer, int sock, int nread);
void msg_proc(char *buffer, int sock, int nread);
void recv_group_msg_func(char *buffer, int sock, int nread);
void recv_file_func(char *buffer, int sock, int nread);

#endif
