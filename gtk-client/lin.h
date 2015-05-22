#ifndef LINPOPII_H_
#define LINPOPII_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <limits.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include "treeview.h"
#include "user_info_set.h"
#include "chat_dlg.h"
#include "linwnd.h"
#include "client_socket.h"

// 自定义协议格式, 用于拆包和组包
#define PROTOCOL_ONLINE					"1"
#define PROTOCOL_RECV_MSG				"2"
#define PROTOCOL_RECV_GROUP_MSG			"3"
#define PROTOCOL_RECV_FILE				"4"
#define PROTOCOL_LOGIN_AGAIN			"5"
#define PROTOCOL_ID_NOT_EXIST			"6"

#define FILE_TRANS_PORT		7000

#define MAX_BUF				1024 * 2

#endif /* LINPOPII_H_ */
