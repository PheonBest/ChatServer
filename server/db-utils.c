#include <string.h>

#include <stdlib.h>
#include "db-utils.h"
#include "../log/log.h"
#include "server-config.h"
#include "../protocol/protocol.h"

 void construct_db_architecture(sqlite3 * db)
{  
    // load file
    FILE* f;
    char raw_sql[5012];

    f = fopen(DB_INIT_FILENAME, "a+");
 
    if (NULL == f) {
        log_error("Can't open init file for DB ...");
    }
 
    while (fgets(raw_sql, 5012, f) != NULL) {
    }
 
    fclose(f);

    // execute raw sql from file
    char *err_msg = 0;
    int rc = sqlite3_exec(db, raw_sql, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return;
    }

    log_info("DB architecture set with success"); 
}


 bool username_already_used(sqlite3 * db, char * username){
    char *err_msg = 0;
    bool exists = false;
    
    char *sql = "SELECT username from %s where username = '%s';";
    char b[50];
    sprintf(b, sql, USER_TABLE, username);

    sqlite3_stmt * stmt;
    int rc = sqlite3_prepare_v2(db, b, -1, &stmt, NULL);

    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_finalize(stmt);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return exists;
    }

    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        exists = true;
    }

    sqlite3_finalize(stmt);

    return exists;
}

 bool login(sqlite3 * db, char * username, char * password){
    char *err_msg = 0;
    bool exists = false;
    
    char *sql = "SELECT username from %s where username = '%s' and password = '%s';";
    char b[128];

    sprintf(b, sql, USER_TABLE, username, password);

    sqlite3_stmt * stmt;
    int rc = sqlite3_prepare_v2(db, b, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_finalize(stmt);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return exists;
    }

    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        exists = true;
    }

    sqlite3_finalize(stmt);

    return exists;
}

 char * has_private_room(sqlite3 * db, char * username1, char * username2){
    char *err_msg = 0;
    char *room_id;

    char *sql = "SELECT r.room_id FROM %s as r, %s AS r2 WHERE r.username = '%s' AND r2.username = '%s' AND r.room_id = r2.room_id;";
    char b[128];
    sprintf(b, sql, ROOM_USERS_TABLE, ROOM_USERS_TABLE, username1, username2);

    sqlite3_stmt * stmt;
    int rc = sqlite3_prepare_v2(db, b, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_finalize(stmt);
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return NULL;
    }

    
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        log_debug(room_id);
        strcpy(room_id, sqlite3_column_text(stmt, 0));
    }

    sqlite3_finalize(stmt);

    return room_id;
}

 bool register_user(sqlite3 * db, char * username, char * password){
    char *err_msg = 0;
    bool res = false;
    char *sql = "INSERT INTO %s VALUES('%s', '%s');";
    char b[50];
    sprintf(b, sql, USER_TABLE, username, password);

    int rc = sqlite3_exec(db, b, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return res;
    }

    res = true;
    log_info("User %s successfuly added to the mondo DB", username); 

    return res;
}

 void create_room(sqlite3 * db, char * room_name){
    char *err_msg = 0;

    char *sql = "INSERT INTO %s VALUES('%s');";
    char b[50];
    sprintf(b, sql, ROOM_TABLE, room_name);

    int rc = sqlite3_exec(db, b, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return;
    }

    log_info("Room %s successfuly added to the mondo DB", room_name); 
}

 void save_conversation(sqlite3 * db, char * sender, char * room_id, char * msg){
    char *err_msg = 0;

    char *sql = "INSERT INTO %s(room_id, sender_id, message) VALUES('%s', '%s', '%s');";
    char b[1024];
    sprintf(b, sql, CONVERSATION_TABLE, room_id, sender, msg);

    int rc = sqlite3_exec(db, b, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return;
    }

    log_info("Conversation of room %s, from %s with the text %s was successfuly added to the mondo DB", room_id, sender, msg); 
}

 void add_user_to_room(sqlite3 * db, char * room_id, char * username){
    char *err_msg = 0;

    char *sql = "INSERT INTO %s VALUES('%s', '%s');";
    char b[50];
    sprintf(b, sql, ROOM_USERS_TABLE, room_id, username);

    int rc = sqlite3_exec(db, b, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return;
    }

    log_info("%s was successfuly added to %s room", username, room_id); 
}

void remove_user_from_room(sqlite3 * db, char * room_id, char * username){
    char *err_msg = 0;

    char *sql = "DELETE FROM %s WHERE username = '%s' AND room_id = '%s';";
    char b[128];
    sprintf(b, sql, ROOM_USERS_TABLE, username, room_id);

    int rc = sqlite3_exec(db, b, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return;
    }

    log_info("%s was successfuly removed from %s room", username, room_id);
}

bool delete_room(sqlite3 * db, char * room_id){
    char *err_msg = 0;
    bool res = false;

    char *sql = "DELETE FROM %s WHERE name = '%s'";
    char b[128];
    sprintf(b, sql, ROOM_TABLE, room_id);

    int rc = sqlite3_exec(db, b, 0, 0, &err_msg);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return res;
    }

    res = true;
    log_info("%s room was successfuly removed", room_id);
    return res;
}

char ** get_room_users(sqlite3 * db, char * room_id){
    //SELECT username FROM room_users WHERE room_id = '%s'

    char *err_msg = 0;

    char *sql = "SELECT username FROM %s WHERE room_id = '%s'";
    char b[128];
    sprintf(b, sql, ROOM_USERS_TABLE, room_id);

    sqlite3_stmt * stmt;
    int rc = sqlite3_prepare_v2(db, b, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return NULL;
    }

    char ** res = malloc(MESSAGE_RICHTEXT_USERNAME_LENGTH * MAX_CLIENTS);
    int i = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        strcpy(res[i], sqlite3_column_text(stmt, 0));
        i++;
    }

    sqlite3_finalize(stmt);

    return res;
}

Message * get_room_conversation(sqlite3 *db, char * room_id){
    char *err_msg = 0;

    char *sql = "SELECT sender_id, rtm_type, message FROM %s where room_id = '%s';";
    char b[128];
    sprintf(b, sql, CONVERSATION_TABLE, room_id);

    sqlite3_stmt * stmt;
    int rc = sqlite3_prepare_v2(db, b, -1, &stmt, NULL);
    
    if (rc != SQLITE_OK ) {
        
        log_error("SQL error: %s\n", err_msg);
        
        sqlite3_free(err_msg);        
        sqlite3_close(db);
        
        return NULL;
    }

    // 10000 : arbitrary nbr of messages that could be sent in each room
    Message * messages = malloc(sizeof(Message) * 10000);
    int i = 0;
    while ((rc = sqlite3_step(stmt)) == SQLITE_ROW) {
        messages[i].command = MESSAGE_COMMAND_C2C;
        strcpy(messages[i].sender, sqlite3_column_text(stmt, 0));
        strcpy(messages[i].dest, room_id);

        RichTextMessage rtm;
        rtm.type = sqlite3_column_int(stmt, 1);
        strcpy(rtm.message, sqlite3_column_text(stmt, 2));

        messages[i].rtm = rtm;
        i++;
    }

    sqlite3_finalize(stmt);

    return messages;
}