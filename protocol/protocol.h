#ifndef __PROTOCOL_H__
#define __PROTOCOL_H__

#define SILENT_CLIENT_LOG true

// MUST BE GREATER THAN 0 (because atoi returns 0 if it can't convert a char * to a number)
#define USER_ANONYMOUS "anonymous"
#define BROADCAST "broadcast"

#define MESSAGE_COMMAND_BROADCAST 1
#define MESSAGE_COMMAND_C2C 2
#define MESSAGE_COMMAND_CREATE_GROUP 3
#define MESSAGE_COMMAND_EDIT_GROUP 4
#define MESSAGE_COMMAND_DELETE_GROUP 5
#define MESSAGE_COMMAND_C2G 6
#define MESSAGE_COMMAND_GET_CLIENTS 7
#define MESSAGE_COMMAND_CREATE_POLL 8
#define MESSAGE_COMMAND_EDIT_POLL 9
#define MESSAGE_COMMAND_DELETE_POLL 10
#define MESSAGE_COMMAND_GET_POLL 11
#define MESSAGE_COMMAND_USER_EXISTS 12
#define MESSAGE_COMMAND_GROUP_EXISTS 13
#define MESSAGE_COMMAND_JOIN_GROUP 14
#define MESSAGE_COMMAND_ADD_USER_TO_GROUP 15
#define MESSAGE_COMMAND_REMOVE_FROM_USER_TO_GROUP 16

#define MESSAGE_COMMAND_SEND_USERNAME 17
#define MESSAGE_COMMAND_LOGIN 18
#define MESSAGE_COMMAND_SIGN_IN 19
#define MESSAGE_COMMAND_REQUEST_REGISTER 20
#define MESSAGE_COMMAND_INIT_CONNECTION 21


#define MESSAGE_COMMAND_ADD_TO_GROUP 1
#define MESSAGE_COMMAND_REMOVE_FROM_GROUP 2
#define MESSAGE_COMMAND_GET_ROOM_USERS 3

#define MESSAGE_SUCCESS 30
#define MESSAGE_ERROR 31

// MUST BE GREATER THAN 0 (because atoi returns 0 if it can't convert a char * to a number)
#define MESSAGE_RICHTEXT_STR 1
#define MESSAGE_RICHTEXT_REACTION 2
#define MESSAGE_RICHTEXT_URL 3
#define MESSAGE_RICHTEXT_IMG 4
#define MESSAGE_RICHTEXT_FILE 5

#define MESSAGE_RICHTEXT_PASSWORD_LENGTH 30
#define MESSAGE_RICHTEXT_ROOM_LENGTH 30
#define MESSAGE_RICHTEXT_USERNAME_LENGTH 30
#define MESSAGE_RICHTEXT_STR_LENGTH 1024^(3)
#define MESSAGE_RICHTEXT_IMAGE_LENGTH 5*1024^(6)
#define MESSAGE_RICHTEXT_FILE_LENGTH 5*1024^(6)
#define MESSAGE_HEADER_LENGTH 100

#define PORT 8009

#define KNRM  "\x1B[0m"
#define KRED  "\x1B[31m"
#define KGRN  "\x1B[32m"
#define KYEL  "\x1B[33m"
#define KBLU  "\x1B[34m"
#define KMAG  "\x1B[35m"
#define KCYN  "\x1B[36m"
#define KWHT  "\x1B[37m"

#define SEPARATOR '~'
#define SECOND_SEPARATOR ','

typedef struct
{
   int type; // NOT NULL
   char * message; // NOT NULL
}RichTextMessage;

/**
   dest can be null if command = MESSAGE_COMMAND_BROADCAST
*/
typedef struct
{
   int command; // NOT NULL
   char * sender;
   char * dest;
   RichTextMessage rtm;
}Message;

char * messageToStr(const Message *m);

Message *strToMessage(const char *str);

#endif