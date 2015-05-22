#ifndef MYSQLCONNECT_H_
#define MYSQLCONNECT_H_

#include "main.h"

MYSQL * GetConnect();
int updateOnlinestatus(MYSQL *connect, char * employeeid,
		const char * statustype);
int updateIp(MYSQL *connect, char * employeeid, const char * ip);
char *findAChatPath(MYSQL *connect, char *id1, char *id2);
char *findGroChatPath(MYSQL *connect, char *groupId);
int findsockfd(MYSQL *connect, char *id1);
int insertAChatPath(MYSQL *connect, char *id1, char *id2, char *path);
int insertGroChatPath(MYSQL *connect, char *group_id, char *path);
int findIdInGro(MYSQL *connect, char *group_id, char *id);
char *findNamebyId(MYSQL *connect, char *id);
char *findIpbyId(MYSQL *connect, char *id);
int idIsExist(MYSQL *connect, char *id);
int allKnoUsers(MYSQL *connect, s_user_info *userInfo);

#endif /* MYSQLCONNECT_H_ */
