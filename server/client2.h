#ifndef CLIENT_H
#define CLIENT_H

#include "server-config.h"

typedef struct
{
   SOCKET sock; // NOT NULL
   char name[BUF_SIZE]; // UNIQUE NOT NULL (null before socket authentication)
   bool authenticated; // NOT NULL
   char room[MESSAGE_RICHTEXT_USERNAME_LENGTH]; // NULLABLE
}Client;

#endif /* guard */
