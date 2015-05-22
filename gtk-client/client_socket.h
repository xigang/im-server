#ifndef CLIENT_SOCKET_H_
#define CLIENT_SOCKET_H_

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>

int init_socket(const char *servip, uint uport);
int send_msg(const char *text, int size);
void *recv_msg_thread(void *arg);
void process_file_trans(char *recv_buf, int recv_len);
void *send_file_thread(void *send_info);
void *recv_file_thread(void *send_info);
char *str_suffix_addition(const char *str, int *index);
char *str_suffix_filename(const char *str);

#endif /* CLIENT_SOCKET_H_ */
