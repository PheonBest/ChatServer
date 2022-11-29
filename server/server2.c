#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <sqlite3.h>

#include "server2.h"
#include "client2.h"
#include "../protocol/protocol.h"
#include "../log/log.h"
#include "db-utils.h"

sqlite3 *db;

static void init(void)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err < 0)
    {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif
    log_trace("Connecting to local db ...");

    char *err_msg = 0;

    int rc = sqlite3_open(DB_FILENAME, &db);

    if (rc != SQLITE_OK)
    {

        log_fatal("Cannot open database: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);

        return;
    }

    // construct_db_architecture(db);

    log_trace("Successfuly connected to local db");
}

static void end(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}

// FUNCTIONS
static bool room_exists(const char *room_name, char **rooms, int nb_rooms)
{
    int i;
    for (i = 0; i < nb_rooms; i++)
    {
        if (strcmp(rooms[i], room_name) == 0)
        {
            return true;
        }
    }
    return false;
}

static Client *get_client(const char *client_name, Client *clients, int actual)
{
    int i;
    for (i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, client_name) == 0)
        {
            return &(clients[i]);
        }
    }

    return NULL;
}

static void clear_clients(Client *clients, int actual)
{
    int i = 0;
    for (i = 0; i < actual; i++)
    {
        closesocket(clients[i].sock);
    }
}

static void remove_client(Client *clients, int to_remove, int *actual)
{
    /* we remove the client in tcsock, "Veuillez entrer un pseudo :");he array */
    memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
    /* number client - 1 */
    (*actual)--;
}

static void send_broadcast_message(Client *clients, Client *client, Message *m, int actual)
{
    int i = 0;
    char message[BUF_SIZE];
    message[0] = 0;
    for (i = 0; i < actual; i++)
    {
        /* we don't send message to the sender */
        if (client->sock != clients[i].sock)
        {
            m->dest = clients[i].name;
            char *s1 = messageToStr(m);
            printf("BROADCAST MESSAGE SENT after MANIPULATION: %s\n", s1);
            write_client(clients[i].sock, s1);
            free(s1);
        }
    }
}

static int init_connection(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    int opt = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt()");
        exit(EXIT_FAILURE);
    }

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
    {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
    {
        perror("listen()");
        exit(errno);
    }
    return sock;
}

static void end_connection(int sock)
{
    printf("end of socket");
    closesocket(sock);
}

static int read_client(SOCKET sock, char *buffer)
{
    int n = 0;

    if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
    {
        perror("recv()");
        /* if recv error we disonnect the client */
        n = 0;
    }

    buffer[n] = 0;

    return n;
}

static void write_client(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

static void send_message_from_server_to_socket(Message *m, SOCKET dest)
{
    char *str = messageToStr(m);
    printf("MESSAGE SENT FROM SERVER TO SOCKET: %s\n", str);
    printf("socket = %d", dest);
    write_client(dest, str);
    free(str);
}

static void send_message_from_server_to_client(Message *m, Client *dest)
{
    char *str = messageToStr(m);
    printf("MESSAGE SENT FROM SERVER TO CLIENT: %s\n", str);
    write_client(dest->sock, str);
    free(str);
}

static void send_str_from_server_to_socket(char *str, SOCKET dest)
{
    RichTextMessage r1 = {1, str};
    Message m1 = {1, "SERVER", "anonymous", r1};
    char *s1 = messageToStr(&m1);
    write_client(dest, s1);
    free(s1);
}

static void send_str_from_server_to_client(char *str, Client *dest)
{
    RichTextMessage r1 = {1, str};
    Message m1 = {1, "SERVER", dest->name, r1};
    char *s1 = messageToStr(&m1);
    write_client(dest->sock, s1);
    free(s1);
}

static void send_room_message(Client *sender, Message *m, Client *clients, int actual, char **rooms, int nb_rooms)
{
    printf("Sending message ...");
    int i;
    // Check that the room exists
    if (room_exists(m->dest, rooms, nb_rooms))
    {
        // Send the message to all the users with the room
        m->dest = clients[i].name;
        // format msg -> SENDER : msg

        // Copy room name to a variable
        char *room_name = malloc(sizeof(m->dest));
        strcpy(room_name, m->dest);

        // Add message to history to DB here

        int j;
        for (j = 0; j < actual; j++)
        {
            if (strcmp(clients[j].room, room_name) == 0)
            {
                m->dest = clients[j].room;
                send_message(sender, m, clients, actual);
            }
        }

        free(room_name);
    }

    send_str_from_server_to_client("The room doesn't exist ...", sender);
}

static void send_message(Client *sender, Message *msg, Client *clients, int actual)
{
    printf("Sending message ...");
    int i;
    SOCKET dest_socket = -1;

    for (i = 0; i < actual; i++)
    {
        if (strcmp(clients[i].name, msg->dest) == 0)
        {
            dest_socket = clients[i].sock;
        }
    }

    if (dest_socket == -1)
    {
        send_str_from_server_to_client("Receiver is not reachable, he might not be connected ...", sender);
    }
    else
    {
        // format msg -> SENDER : msg
        char *toSend = messageToStr(msg);
        if (toSend == NULL)
        {
            // fprintf(stderr, "Could not convert the already converted message to a string\n");
            send_str_from_server_to_client("Your message is too long!", sender);
        }
        else
        {
            write_client(dest_socket, toSend);
            free(toSend);
        }
    }
}

static void send_server_infos(Client *dest, Client *clients, int actual)
{
    char infos[1024];
    int i;

    if (actual > 1)
    {
        strcpy(infos, "Il y a %d client(s) connecté(s) : ");
        for (i = 0; i < actual; i++)
        {
            strcat(infos, clients[i].name);

            if (i != actual - 1)
            {
                strcat(infos, ", ");
            }
        }
        sprintf(infos, infos, actual);
    }
    else
    {
        strcpy(infos, "Vous êtes le seul connecté 🥲");
    }

    send_str_from_server_to_client(infos, dest);
}
// FUNCTIONS

static void app(void)
{
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    /* the index for the array */
    int actual = 0;
    int max = sock;
    /* an array for all clients */
    Client clients[MAX_CLIENTS];
    char *rooms[MAX_ROOMS];
    int nb_rooms = 0;

    fd_set rdfs;

    while (1)
    {
        int i = 0;
        FD_ZERO(&rdfs);

        /* add STDIN_FILENO */
        FD_SET(STDIN_FILENO, &rdfs);

        /* add the connection socket */
        FD_SET(sock, &rdfs);

        /* add socket of each client */
        for (i = 0; i < actual; i++)
        {
            FD_SET(clients[i].sock, &rdfs);
        }

        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        /* something from standard input : i.e keyboard */
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            printf("Erreur d'input ...");
            /* stop process when type on keyboard */
            break;
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            /* new client */
            SOCKADDR_IN csin = {0};
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
            if (csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }

            /* after connecting the client sends its name */
            if (read_client(csock, buffer) == -1)
            {
                continue;
            }

            /* The authentification is made in 2 steps:
             * The first step is that the new client sends its username
             * If the username is valid:
             *    If the user already exists:
             *       send MESSAGE_SUCCESS
             *       Then the user sends MESSAGE_COMMAND_LOGIN back
             *    Else:
             *       send MESSAGE_COMMAND_REQUEST_REGISTER
             *       Then the user sends MESSAGE_COMMAND_SIGN_IN back
             * Else:
             *    MESSAGE_ERROR
             */

            printf("New connection: %d\n", sock);

            // what is the new maximum fd
            max = csock > max ? csock : max;
            FD_SET(csock, &rdfs);

            // Connect user as USER_ANONYMOUS
            Client client = {csock};
            strncpy(client.name, USER_ANONYMOUS, BUF_SIZE - 1);

            clients[actual] = client;
            actual++;

            continue;
        }
        else
        {
            int i = 0;
            for (i = 0; i < actual; i++)
            {
                /* a client is talking */
                if (FD_ISSET(clients[i].sock, &rdfs))
                {
                    Client client = clients[i];
                    int c = read_client(clients[i].sock, buffer);
                    /* client disconnected */
                    if (c == 0)
                    {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual);

                        strncpy(buffer, client.name, BUF_SIZE - 1);
                        strncat(buffer, " s'est déconnecté !", BUF_SIZE - strlen(buffer) - 1);
                        RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, buffer};
                        Message m = {MESSAGE_COMMAND_BROADCAST, "SERVER", NULL, r1};
                        send_broadcast_message(clients, &client, &m, actual);
                    }
                    else
                    {
                        printf("RAW MESSAGE RECEIVED: %s\n", buffer);
                        Message *m = strToMessage(buffer);
                        if (m == NULL)
                        {
                            // fprintf(stderr, "Could not convert the received str to a message\n");
                            send_str_from_server_to_client("Your message is too long!", &client);
                        }
                        else
                        {
                            char password[MESSAGE_RICHTEXT_PASSWORD_LENGTH];
                            char *context;
                            char *token;
                            switch (m->command)
                            {
                            case MESSAGE_COMMAND_SEND_USERNAME:
                                if (username_already_used(db, m->sender))
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Veuillez renseigner votre mot de passe pour vous connecter"};
                                    Message m1 = {MESSAGE_SUCCESS, "SERVER", " ", r1};
                                    send_message_from_server_to_socket(&m1, clients[i].sock);
                                }
                                else
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Veuillez renseigner un mot de passe pour vous enregistrer (sans espace et sans 'tilde')"};
                                    Message m1 = {MESSAGE_COMMAND_REQUEST_REGISTER, "SERVER", " ", r1};
                                    send_message_from_server_to_socket(&m1, clients[i].sock);
                                }
                                break;
                            case MESSAGE_COMMAND_LOGIN:
                                token = strtok_r(m->rtm.message, (char[2]){SECOND_SEPARATOR, '\0'}, &context);

                                if (token == NULL)
                                {
                                    RichTextMessage rInvalidMessage = {MESSAGE_RICHTEXT_STR, "Votre pseudonyme et mot de passe ne doivent pas être vides !"};
                                    Message mInvalidMessage = {MESSAGE_SUCCESS, "SERVER", " ", rInvalidMessage};
                                    send_message_from_server_to_socket(&mInvalidMessage, clients[i].sock);
                                }
                                else
                                {
                                    strcpy(password, token);

                                    bool userExists = username_already_used(db, m->sender);
                                    // Check a second time that the user does exist
                                    if (!userExists)
                                    {
                                        char *bufferToSend = "L'utilisateur %s n'existe pas";
                                        char formattedBuffer[128];
                                        sprintf(formattedBuffer, bufferToSend, m->sender);

                                        RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, formattedBuffer};
                                        Message m1 = {MESSAGE_ERROR, "SERVER", " ", r1};
                                        send_message_from_server_to_socket(&m1, clients[i].sock);
                                    }
                                    else
                                    {
                                        // Check it's the right password
                                        if (login(db, m->sender, password))
                                        {
                                            strncpy(client.name, m->sender, MESSAGE_RICHTEXT_USERNAME_LENGTH - 1);
                                            send_server_infos(&client, clients, actual);

                                            RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Vous êtes maintenant connecté !"};
                                            Message m1 = {MESSAGE_SUCCESS, "SERVER", client.name, r1};
                                            send_message_from_server_to_client(&m1, &client);

                                            strncpy(buffer, client.name, BUF_SIZE - 1);
                                            strncat(buffer, " s'est connecté !", BUF_SIZE - strlen(buffer) - 1);
                                            RichTextMessage r2 = {MESSAGE_RICHTEXT_STR, buffer};
                                            Message m2 = {MESSAGE_COMMAND_BROADCAST, "SERVER", "", r2};
                                            send_broadcast_message(clients, &client, &m2, actual);
                                            continue;
                                        }
                                        else
                                        {
                                            RichTextMessage rInvalid = {MESSAGE_RICHTEXT_STR, "Impossible d'écrire dans la base de données pour le moment, veuillez réessayer plus tard"};
                                            Message mInvalid = {MESSAGE_ERROR, "SERVER", " ", rInvalid};
                                            send_message_from_server_to_socket(&mInvalid, clients[i].sock);
                                        }
                                        break;
                                    }
                                }
                                break;
                            case MESSAGE_COMMAND_SIGN_IN:

                                token = strtok_r(m->rtm.message, (char[2]){SECOND_SEPARATOR, '\0'}, &context);

                                if (token == NULL)
                                {
                                    RichTextMessage rInvalidMessage = {MESSAGE_RICHTEXT_STR, "Votre pseudonyme et mot de passe ne doivent pas être vides !"};
                                    Message mInvalidMessage = {MESSAGE_SUCCESS, "SERVER", " ", rInvalidMessage};
                                    send_message_from_server_to_socket(&mInvalidMessage, clients[i].sock);
                                }
                                else
                                {
                                    strcpy(password, token);

                                    bool userExists = username_already_used(db, m->sender);
                                    // Check a second time that the user doesn't exists
                                    if (m->command == MESSAGE_COMMAND_SIGN_IN && userExists)
                                    {
                                        char *bufferToSend = "L'utilisateur %s existe déjà";
                                        char formattedBuffer[128];
                                        sprintf(formattedBuffer, bufferToSend, m->sender);

                                        RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, formattedBuffer};
                                        Message m1 = {MESSAGE_ERROR, "SERVER", " ", r1};
                                        send_message_from_server_to_socket(&m1, clients[i].sock);
                                    }
                                    else
                                    {
                                        // Save the user in DB here
                                        if (register_user(db, m->sender, password))
                                        {
                                            strncpy(client.name, m->sender, MESSAGE_RICHTEXT_USERNAME_LENGTH - 1);
                                            send_server_infos(&client, clients, actual);

                                            RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Vous êtes maintenant connecté !"};
                                            Message m1 = {MESSAGE_SUCCESS, "SERVER", client.name, r1};
                                            send_message_from_server_to_client(&m1, &client);

                                            strncpy(buffer, client.name, BUF_SIZE - 1);
                                            strncat(buffer, " vient d'arriver sur le serveur !", BUF_SIZE - strlen(buffer) - 1);
                                            RichTextMessage r2 = {MESSAGE_RICHTEXT_STR, buffer};
                                            Message m2 = {MESSAGE_COMMAND_BROADCAST, "SERVER", "", r2};
                                            send_broadcast_message(clients, &client, &m2, actual);
                                            continue;
                                        }
                                        else
                                        {
                                            RichTextMessage rInvalid = {MESSAGE_RICHTEXT_STR, "Impossible d'écrire dans la base de données pour le moment, veuillez réessayer plus tard"};
                                            Message mInvalid = {MESSAGE_ERROR, "SERVER", " ", rInvalid};
                                            send_message_from_server_to_socket(&mInvalid, clients[i].sock);
                                        }
                                        break;
                                    }
                                }
                                break;
                            case MESSAGE_COMMAND_BROADCAST:
                                send_broadcast_message(clients, &client, m, actual);
                                break;
                            case MESSAGE_COMMAND_C2C:
                                char *room_id = has_private_room(db, client.name, clients->name);
                                log_debug("generated room_id -> %s", room_id);

                                if (room_id == NULL)
                                {
                                    strcat(room_id, client.name);
                                    strcat(room_id, ",");
                                    strcat(room_id, clients->name);

                                    create_room(db, room_id);
                                    add_user_to_room(db, room_id, client.name);
                                    add_user_to_room(db, room_id, clients->name);
                                }

                                send_message(&client, m, clients, actual);
                                save_conversation(db, client.name, room_id, m->rtm.message);
                                break;
                            case MESSAGE_COMMAND_CREATE_GROUP:

                                RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Le groupe a été crée !"};
                                Message m1 = {MESSAGE_ERROR, "SERVER", client.name, r1};
                                send_message_from_server_to_client(&m1, &client);

                                rooms[nb_rooms++] = m->rtm.message;

                                // Create group in DB here
                                break;
                            case MESSAGE_COMMAND_DELETE_GROUP:
                                RichTextMessage r2 = {MESSAGE_RICHTEXT_STR, "Le groupe a été supprimé !"};
                                Message m2 = {MESSAGE_ERROR, "SERVER", client.name, r2};
                                int j;
                                for (j = 0; j < actual; j++)
                                {
                                    if (strcmp(clients[j].room, m2.rtm.message) == 0)
                                    {
                                        // Reset user's room
                                        memset(clients[j].room, 0, MESSAGE_RICHTEXT_USERNAME_LENGTH);

                                        // Send notification message of the deletion to all users
                                        m2.dest = clients[j].name;
                                        send_message_from_server_to_socket(&m2, clients[j].sock);
                                    }
                                }

                                delete_room(db, clients[j].room);
                                break;
                            case MESSAGE_COMMAND_C2G:
                                send_room_message(&client, m, clients, actual, rooms, nb_rooms);
                                break;
                            case MESSAGE_COMMAND_GET_CLIENTS:
                                send_server_infos(&client, clients, actual);
                                break;
                            case MESSAGE_COMMAND_CREATE_POLL:
                                break;
                            case MESSAGE_COMMAND_EDIT_POLL:
                                break;
                            case MESSAGE_COMMAND_DELETE_POLL:
                                break;
                            case MESSAGE_COMMAND_GET_POLL:
                                break;
                            case MESSAGE_COMMAND_ADD_USER_TO_GROUP:
                                if (strcmp(m->sender, m->rtm.message) == 0)
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Vous ne pouvez pas vous ajouter vous-même à un groupe"};
                                    Message m1 = {MESSAGE_ERROR, "SERVER", client.name, r1};
                                    send_message_from_server_to_client(&m1, &client);
                                    continue;
                                }
                                Client *specifiedUser = get_client(m->rtm.message, clients, actual);
                                if (specifiedUser != NULL)
                                {
                                    specifiedUser->room[0] = '\0';
                                    strncat(specifiedUser->room, buffer, MESSAGE_RICHTEXT_USERNAME_LENGTH - 1);

                                    // add user to group in DB here

                                    // Send notification message to the specified user
                                    char *bufferToSend = "Vous avez été ajouté au groupe %s par %s";
                                    char *formattedBuffer;
                                    sprintf(formattedBuffer, bufferToSend, clients[i].room, clients[i].name);
                                    RichTextMessage r2 = {MESSAGE_RICHTEXT_STR, formattedBuffer};
                                    Message m2 = {MESSAGE_SUCCESS, "SERVER", specifiedUser->name, r2};
                                    send_message_from_server_to_client(&m2, specifiedUser);

                                    // Send notification message to the user
                                    char *bufferToSend2 = "L'utilisateur %s a été ajouté au groupe %s";
                                    char *formattedBuffer2;
                                    sprintf(formattedBuffer2, bufferToSend2, clients[i].name, clients[i].room);
                                    RichTextMessage r3 = {MESSAGE_RICHTEXT_STR, formattedBuffer2};
                                    Message m3 = {MESSAGE_SUCCESS, "SERVER", client.name, r3};
                                    send_message_from_server_to_client(&m3, &client);
                                }
                                else
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "L'utilisateur ne fait pas parie du groupe"};
                                    Message m2 = {MESSAGE_ERROR, "SERVER", client.name, r1};
                                    send_message_from_server_to_client(&m2, &client);
                                }
                                break;
                            case MESSAGE_COMMAND_REMOVE_FROM_USER_TO_GROUP:
                                if (strcmp(m->sender, m->rtm.message) == 0)
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Vous ne pouvez pas vous supprimer du groupe ! A la place, veuillez supprimer le groupe"};
                                    Message m1 = {MESSAGE_ERROR, "SERVER", client.name, r1};
                                    send_message_from_server_to_client(&m1, &client);
                                    continue;
                                }
                                Client *specifiedUser2 = get_client(m->rtm.message, clients, actual);
                                if (specifiedUser2 != NULL)
                                {
                                    remove_user_from_room(db, specifiedUser2->room, specifiedUser2->name);

                                    // Reset user's room
                                    memset(specifiedUser2->room, 0, MESSAGE_RICHTEXT_USERNAME_LENGTH);

                                    // Send notification message to the specified user
                                    char *bufferToSend = "Vous avez été supprimé du groupe %s par %s";
                                    char *formattedBuffer;
                                    sprintf(formattedBuffer, bufferToSend, clients[i].room, clients[i].name);
                                    RichTextMessage r2 = {MESSAGE_RICHTEXT_STR, formattedBuffer};
                                    Message m2 = {MESSAGE_SUCCESS, "SERVER", specifiedUser2->name, r2};
                                    send_message_from_server_to_client(&m2, specifiedUser2);

                                    // Send notification message to the user
                                    char *bufferToSend2 = "L'utilisateur %s a été supprimé du groupe %s";
                                    char *formattedBuffer2;
                                    sprintf(formattedBuffer2, bufferToSend2, clients[i].name, clients[i].room);
                                    RichTextMessage r3 = {MESSAGE_RICHTEXT_STR, formattedBuffer2};
                                    Message m3 = {MESSAGE_SUCCESS, "SERVER", client.name, r3};
                                    send_message_from_server_to_client(&m3, &client);
                                }
                                else
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "L'utilisateur ne fait pas partie du groupe"};
                                    Message m2 = {MESSAGE_ERROR, "SERVER", client.name, r1};
                                    send_message_from_server_to_client(&m2, &client);
                                }
                                break;
                            case MESSAGE_COMMAND_USER_EXISTS:
                                if (strcmp(m->sender, m->rtm.message) == 0)
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Vous ne pouvez pas envoyer de messages privés à vous-même !"};
                                    Message m = {MESSAGE_ERROR, "SERVER", client.name, r1};
                                    send_message_from_server_to_client(&m, &client);
                                    continue;
                                }
                                if (username_already_used(db, client.name))
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "L'utilisateur existe"};
                                    Message m = {MESSAGE_SUCCESS, "SERVER", client.name, r1};
                                    send_message_from_server_to_client(&m, &client);
                                }
                                else
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "L'utilisateur n'existe pas"};
                                    Message m = {MESSAGE_ERROR, "SERVER", client.name, r1};
                                    send_message_from_server_to_client(&m, &client);
                                }
                                break;
                            case MESSAGE_COMMAND_EDIT_GROUP:
                                // NO BREAK
                            case MESSAGE_COMMAND_JOIN_GROUP:
                                client.room[0] = '\0';
                                strncat(client.room, buffer, MESSAGE_RICHTEXT_USERNAME_LENGTH - 1);
                                // NO BREAK
                            case MESSAGE_COMMAND_GROUP_EXISTS:
                                int i;
                                // Check that the room exists
                                bool roomFound = false;
                                for (i = 0; i < nb_rooms; ++i)
                                {
                                    if (strcmp(rooms[i], m->dest) == 0)
                                    {
                                        RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Le groupe de disucssion existe"};
                                        Message m = {MESSAGE_SUCCESS, "SERVER", client.name, r1};
                                        send_message_from_server_to_client(&m, &client);

                                        roomFound = true;
                                        break;
                                    }
                                }
                                if (!roomFound)
                                {
                                    RichTextMessage r1 = {MESSAGE_RICHTEXT_STR, "Le groupe de disucussion n'existe pas"};
                                    Message m = {MESSAGE_ERROR, "SERVER", client.name, r1};
                                    send_message_from_server_to_client(&m, &client);
                                }
                                break;
                            default:
                                break;
                            }
                        }
                        free(m);
                    }
                    break;
                }
            }
        }
    }

    sqlite3_close(db);
    clear_clients(clients, actual);
    end_connection(sock);
}

int main(int argc, char **argv)
{
    init();

    app();

    end();

    return EXIT_SUCCESS;
}