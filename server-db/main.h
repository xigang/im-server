#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <glib.h>
#include <netinet/in.h>
#include "server.h"
#include <mysql.h>

void* thread_proc(void *arg);

#endif /* MAIN_H_ */
