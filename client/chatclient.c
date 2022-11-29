#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include "client2.h"
#include "../protocol/protocol.h"
#include "../log/log.h"

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
}

static void end(void)
{
#ifdef WIN32
   WSACleanup();
#endif
}

static void display_menu()
{
   printf("------- MENU -------\n");
   printf("%d) Broadcast\n", MESSAGE_COMMAND_BROADCAST);
   printf("%d) Message Privé\n", MESSAGE_COMMAND_C2C);
   printf("%d) Créer un groupe de discussion\n", MESSAGE_COMMAND_CREATE_GROUP);
   printf("%d) Editer un groupe de discussion\n", MESSAGE_COMMAND_EDIT_GROUP);
   printf("%d) Supprimer un groupe de discussion\n", MESSAGE_COMMAND_DELETE_GROUP);
   printf("%d) Entrer dans un groupe de discussion\n", MESSAGE_COMMAND_C2G);
   printf("%d) Afficher les utilisateurs connectés\n", MESSAGE_COMMAND_GET_CLIENTS);
   printf("------- MENU -------\n");
}

static void display_submenu()
{
   printf("------- SUB-MENU -------\n");
   printf("%d) Envoyer du texte\n", MESSAGE_RICHTEXT_STR);
   printf("%d) Envoyer une réaction\n", MESSAGE_RICHTEXT_REACTION);
   printf("%d) Envoyer une url sous forme de QR-Code\n", MESSAGE_RICHTEXT_URL);
   printf("%d) Envoyer une image\n", MESSAGE_RICHTEXT_IMG);
   printf("%d) Envoyer un fichier\n", MESSAGE_RICHTEXT_FILE);
   printf("Enter 'q' to leave the current menu\n");
   printf("------- SUB-MENU -------\n");
}

static void display_room_submenu()
{
   printf("------- SUB-MENU -------\n");
   printf("%d) Ajouter un utilisateur au groupe\n", MESSAGE_COMMAND_ADD_TO_GROUP);
   printf("%d) Enlever un utilisateur au groupe\n", MESSAGE_COMMAND_REMOVE_FROM_GROUP);
   printf("%d) Enlever un utilisateur au groupe\n", MESSAGE_COMMAND_GET_ROOM_USERS);
   printf("Enter 'q' to leave the current menu\n");
   printf("------- SUB-MENU -------\n");
}

static int init_connection(const char *address)
{
   SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
   SOCKADDR_IN sin = {0};
   struct hostent *hostinfo;

   if (sock == INVALID_SOCKET)
   {
      perror("socket()");
      exit(errno);
   }

   hostinfo = gethostbyname(address);
   if (hostinfo == NULL)
   {
      log_error("Unknown host %s.\n", address);
      exit(EXIT_FAILURE);
   }

   sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
   sin.sin_port = htons(PORT);
   sin.sin_family = AF_INET;

   if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
   {
      perror("connect()");
      exit(errno);
   }

   return sock;
}

static void end_connection(int sock)
{
   closesocket(sock);
}

static int read_server(SOCKET sock, char *buffer)
{
   int n = 0;

   if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
   {
      perror("recv()");
      exit(errno);
   }

   buffer[n] = 0;

   return n;
}

static void write_server(SOCKET sock, char *dest, const char *buffer, int menu, int submenu, char * username)
{
   RichTextMessage r1 = {submenu, buffer};
   Message m = {menu, username, dest, r1};

   char *str = messageToStr(&m);
   if (str == NULL)
   {
      log_error("Your message is too long!\n");
      return;
   }

   //test(sock, "TEST BORDEL DE BORDEL");
   log_trace("Connecting to local db ...");
   log_trace("str sent -> %s", str);
   log_trace("socket -> %d", sock);

   int n;
   if ((n = send(sock, str, strlen(str), 0)) < 0)
   {
      perror("send()");
      free(str);
      exit(errno);
   }
   else
   {
      log_trace("MESSAGE SENT: %s (%d)\n", str, n);
      free(str);
   }
}

bool isStrValid(char * str, char * fieldName) {
   if (strchr(str, SEPARATOR) != NULL) {
      log_error("Votre %s ne doit pas contenir le caractère %c !", fieldName, SEPARATOR);
      return false;
   }
   if (strchr(str, SECOND_SEPARATOR) != NULL) {
      log_error("Votre %s ne doit pas contenir le caractère %s !", fieldName, SECOND_SEPARATOR);
      return false;
   }
   return true;
}

static void app(const char *address)
{
   State state = VALIDATING_USERNAME;
   int menu = -1;
   int submenu = -1;
   char username[MESSAGE_RICHTEXT_USERNAME_LENGTH] = " ";   // init char array
   char dest[MESSAGE_RICHTEXT_USERNAME_LENGTH] = " ";       // init char array
   char room[MESSAGE_RICHTEXT_ROOM_LENGTH] = " ";       // init char array
   char password[MESSAGE_RICHTEXT_PASSWORD_LENGTH] = " ";   // init char array

   SOCKET sock = init_connection(address);
   write_server(sock, dest, " ", MESSAGE_COMMAND_INIT_CONNECTION, submenu, username);
   
   char buffer[BUF_SIZE];

   fd_set rdfs;

   printf("------------ NextChat ------------\n");
   printf("Veuillez renseigner un nom d'utilisateur (sans espace ni 'tilde')\n");

   while (1)
   {
      FD_ZERO(&rdfs);

      /* add STDIN_FILENO */
      FD_SET(STDIN_FILENO, &rdfs);

      /* add the socket */
      FD_SET(sock, &rdfs);

      if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
      {
         perror("select()");
         exit(errno);
      }

      /* something from standard input : i.e keyboard */
      if (FD_ISSET(STDIN_FILENO, &rdfs))
      {
         fgets(buffer, BUF_SIZE - 1, stdin);
         {
            char *p = NULL;
            p = strstr(buffer, "\n");
            if (p != NULL)
            {
               *p = 0;
            }
            else
            {
               /* fclean */
               buffer[BUF_SIZE - 1] = 0;
            }
         }

         if (strcmp(buffer, "q") == 0 && menu != -1)
         {
            if (submenu == -1)
            {
               printf("\nVous avez quitté le menu %d\n", menu);
               menu = -1;
               display_menu();
               continue;
            }
            else
            {
               printf("\nVous avez quitté le sous-menu %d\n", submenu);
               submenu = -1;
               display_submenu();
               continue;
            }
         }

         // The program is waiting for a user input to send right away to the server
         if (state != IDLE)
         {
            switch (state)
            {
            case VALIDATING_USERNAME:
               username[0] = '\0';
               strncat(username, buffer, MESSAGE_RICHTEXT_PASSWORD_LENGTH - 1);

               log_trace("buffered u %s\n, state = %d", buffer, state);
               if (!isStrValid(password, "pseudonyme")) {
                  continue;
               }

               write_server(sock, dest, username, MESSAGE_COMMAND_SEND_USERNAME, submenu, username);
               continue;

            case VALIDATING_PASSWORD:
               // LOG IN
               char password[MESSAGE_RICHTEXT_PASSWORD_LENGTH];
               password[0] = '\0';
               strncat(password, buffer, MESSAGE_RICHTEXT_PASSWORD_LENGTH - 1);

               if (!isStrValid(password, "mot de passe")) {
                  continue;
               }

               write_server(sock, dest, password, MESSAGE_COMMAND_LOGIN, submenu, username);
               continue;

            case VALIDATING_CONFIRM_PASSWORD_TO_REGISTER:
               char confirmedPassword[MESSAGE_RICHTEXT_PASSWORD_LENGTH];
               confirmedPassword[0] = '\0';
               strncat(confirmedPassword, buffer, MESSAGE_RICHTEXT_PASSWORD_LENGTH - 1);
               
               if (strcmp("undo", confirmedPassword) == 0)
               {
                  printf("Veuillez renseigner un mot de passe pour vous enregistrer (sans espace et sans 'tilde')\n");
                  state = VALIDATING_PASSWORD_TO_REGISTER;
               }
               else if (strcmp(password, confirmedPassword) == 0)
               {
                  log_trace("confirmed passwd is %s\n", confirmedPassword);
                  // ENCRYPT THE PASSWORD HERE
                  log_trace("Data sent to server ...\n");
                  write_server(sock, dest, confirmedPassword, MESSAGE_COMMAND_SIGN_IN, submenu, username);
               }
               else
               {
                  printf("Les deux mots de passe ne correspondent pas.\n");
                  printf("Veuillez réecrire votre mot de passe pour le confirmer (ou entrez 'undo' pour changer le mot de passe initial)\n");
               }
               break;
            case VALIDATING_PASSWORD_TO_REGISTER:
               // REGISTER IN
               password[0] = '\0';
               strncat(password, buffer, MESSAGE_RICHTEXT_PASSWORD_LENGTH - 1);

               if (!isStrValid(password, "mot de passe")) {
                  continue;
               }

               state = VALIDATING_CONFIRM_PASSWORD_TO_REGISTER;

               printf("Veuillez réecrire votre mot de passe pour le confirmer (ou entrez 'undo' pour changer le mot de passe initial)\n");
               continue;
            case VALIDATING_USER_EXISTS:
               dest[0] = '\0';
               strncat(dest, buffer, MESSAGE_RICHTEXT_USERNAME_LENGTH - 1);
               write_server(sock, dest, buffer, MESSAGE_COMMAND_USER_EXISTS, submenu, username);
               continue;
            case VALIDATING_GROUP_TO_EDIT:
               room[0] = '\0';
               strncat(room, buffer, MESSAGE_RICHTEXT_USERNAME_LENGTH - 1);
               write_server(sock, dest, buffer, MESSAGE_COMMAND_EDIT_GROUP, submenu, username);
               continue;
            case VALIDATING_GROUP_TO_JOIN:
               room[0] = '\0';
               strncat(room, buffer, MESSAGE_RICHTEXT_USERNAME_LENGTH - 1);
               write_server(sock, dest, buffer, MESSAGE_COMMAND_JOIN_GROUP, submenu, username);
               continue;
            case VALIDATING_GROUP_EXISTS:
               room[0] = '\0';
               strncat(room, buffer, MESSAGE_RICHTEXT_USERNAME_LENGTH - 1);
               write_server(sock, dest, buffer, MESSAGE_COMMAND_GROUP_EXISTS, submenu, username);
               continue;
            default:
               break;
            }
         }
         else if (menu == -1)
         {
            int tmp_menu;
            if ((tmp_menu = atoi(buffer)) == 0 || tmp_menu < 1 || tmp_menu > 7)
            {
               printf("\n\nEntrez un numéro de menu valide !\n\n");
               display_menu();
               continue;
            }

            switch (tmp_menu)
            {
            case MESSAGE_COMMAND_GET_CLIENTS:
               write_server(sock, dest, " ", tmp_menu, submenu, username);
               break;
            case MESSAGE_COMMAND_BROADCAST:
               menu = tmp_menu;
               printf("\nVous envoyez maintenant vos messages à tout le monde !\n\n");
               display_submenu();
               break;
            case MESSAGE_COMMAND_C2C:
               menu = tmp_menu;
               printf("\nA qui voulez-vous envoyer des messages privés ?\n\n");
               state = VALIDATING_USER_EXISTS;
               break;

            case MESSAGE_COMMAND_C2G:
               menu = tmp_menu;
               printf("\nQuel groupe de discussion souhaitez vous utiliser ?\n\n");
               state = VALIDATING_GROUP_EXISTS;
               break;
            case MESSAGE_COMMAND_CREATE_GROUP:
               menu = tmp_menu;
               printf("\nQuel est le nom du groupe de discussion que vous souhaitez créer ?\n\n");
               state = VALIDATING_GROUP_TO_JOIN;
               break;
            case MESSAGE_COMMAND_EDIT_GROUP:
               menu = tmp_menu;
               printf("\nQuel est le nom du groupe de discussion que vous souhaitez éditer ?\n\n");
               state = VALIDATING_GROUP_TO_EDIT;
               break;
            }
         }
         else if (submenu == -1)
         {
            int tmp_submenu;
            if ((tmp_submenu = atoi(buffer)) == 0 || tmp_submenu < 1 || tmp_submenu > 5)
            {
               printf("\n\nPlease enter a valid submenu number !\n\n");
               display_submenu();
               continue;
            }
            submenu = tmp_submenu;
            printf("You entered submenu %d\n", submenu);
         }
         else
         {
            write_server(sock, dest, buffer, menu, submenu, username);
         }
      }
      else if (FD_ISSET(sock, &rdfs))
      {
         int n = read_server(sock, buffer);
         /* server down */
         if (n == 0)
         {
            printf("Server disconnected !\n");
            break;
         }
         else
         {
            Message *m = strToMessage(buffer);
            if (m == NULL)
            {
               printf("The received message is too long!\n"); // Should never happen as the server checks the messages before sending them
            }
            else
            {
               switch (state)
               {
               case VALIDATING_USERNAME:
                  log_trace("command = %d\n", m->command);
                  if (m->command == MESSAGE_SUCCESS)
                  {
                     state = VALIDATING_PASSWORD;
                  }
                  else if (m->command == MESSAGE_COMMAND_REQUEST_REGISTER)
                  {
                     state = VALIDATING_PASSWORD_TO_REGISTER;
                  }
                  else if (m->command == MESSAGE_ERROR)
                  {
                     // The username isn't valid
                  }
                  break;
               case VALIDATING_PASSWORD:
                  if (m->command == MESSAGE_SUCCESS)
                  {
                     state = IDLE;
                     display_menu();
                  }
                  else if (m->command == MESSAGE_ERROR)
                  {
                     //state = VALIDATING_PASSWORD;
                  }
                  break;
               case VALIDATING_CONFIRM_PASSWORD_TO_REGISTER:
                  if (m->command == MESSAGE_SUCCESS)
                  {
                     state = IDLE;
                     display_menu();
                  }
                  else if (m->command == MESSAGE_ERROR)
                  {
                     // The password isn't valid
                  }
               case VALIDATING_USER_EXISTS:
                  if (m->command == MESSAGE_SUCCESS)
                  {
                     if (menu == MESSAGE_COMMAND_C2C)
                     {
                        state = IDLE;
                        display_submenu();
                        continue;
                     }
                     else if (menu == MESSAGE_COMMAND_ADD_TO_GROUP)
                     {
                        state = IDLE;
                        display_room_submenu();
                        continue;
                     }
                     else if (menu == MESSAGE_COMMAND_REMOVE_FROM_GROUP)
                     {
                        state = IDLE;
                        display_room_submenu();
                        continue;
                     }
                  }
                  else if (m->command == MESSAGE_ERROR)
                  {
                     if (menu == MESSAGE_COMMAND_C2C)
                     {
                        // free(dest);
                        // dest = NULL;
                     }
                  }
                  break;
               case VALIDATING_GROUP_EXISTS:
                  if (m->command == MESSAGE_SUCCESS)
                  {

                     if (menu == MESSAGE_COMMAND_DELETE_GROUP)
                     {
                        display_menu();
                        state = IDLE;
                     }
                     else if (menu == MESSAGE_COMMAND_CREATE_GROUP)
                     {
                        display_room_submenu();
                        state = IDLE;
                     }

                     continue;
                  }
                  else if (m->command == MESSAGE_ERROR)
                  {
                     // free(dest);
                     // dest = NULL;
                  }
                  break;
               }

               if (m->command == MESSAGE_COMMAND_C2C)
               {
               }
               switch (m->rtm.type)
               {
               case MESSAGE_RICHTEXT_STR:
                  // fflush(stdout);
                  if (m->command == MESSAGE_COMMAND_BROADCAST)
                     printf("%s[BROADCAST] %s", KRED, KNRM);
                  else if (m->command == MESSAGE_COMMAND_C2C)
                     printf("%s[DM] %s", KBLU, KNRM);
                     
                  printf("%s: %s\n", m->sender, m->rtm.message);
                  break;
               case MESSAGE_RICHTEXT_REACTION:
                  break;
               case MESSAGE_RICHTEXT_URL:
                  break;
               case MESSAGE_RICHTEXT_IMG:
                  break;
               case MESSAGE_RICHTEXT_FILE:
                  break;
               default:
                  break;
               }
            }
         }
         log_trace("RAW MESSAGE: %s\n", buffer);
      }
   }
   end_connection(sock);
}

int main(int argc, char **argv)
{
   log_set_quiet(SILENT_CLIENT_LOG);

   /**/
   if (argc < 1)
   {
      printf("Usage : %s\n", argv[0]);
      return EXIT_FAILURE;
   }

   init();
   
   app(argv[1]);

   end();

   return EXIT_SUCCESS;
}
