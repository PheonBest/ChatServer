#ifndef DBUTILS_H
#define DBUTILS_H

#include <sqlite3.h>
#include <stdbool.h>

#include "../protocol/protocol.h"

#define DB_FILENAME "mondo.db"
#define DB_INIT_FILENAME "db_init.sql"

#define CONVERSATION_TABLE "conversations"
#define ROOM_TABLE "room"
#define ROOM_USERS_TABLE "room_users"
#define USER_TABLE "user"

void construct_db_architecture(sqlite3 * db);

bool username_already_used(sqlite3 * db, char * username);
bool login(sqlite3 * db, char * username, char * password);

bool register_user(sqlite3 * db, char * username, char * password);

//à voir avec les dépendances en SQL
//static void delete_user(char * username);
Message * get_room_conversation(sqlite3 * db, char * room_id);

void save_conversation(sqlite3 * db, char * sender, char * room_id, char * msg);

char * has_private_room(sqlite3 * db, char * user1, char * user2);

void create_room(sqlite3 * db, char * room_id);
void add_user_to_room(sqlite3 * db, char * room_id, char * username);
bool delete_room(sqlite3 * db, char * room_id);
void remove_user_from_room(sqlite3 * db, char * room_id, char * username);
char ** get_room_users(sqlite3 * db, char * room_id);
char ** get_rooms_of_user(sqlite3 * db, char * username);


#endif