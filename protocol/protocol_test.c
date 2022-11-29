#include <stdio.h>
#include "protocol.h"
#include <stdlib.h>

int main(void)
{
    printf("protocol_test started ...\n");

    RichTextMessage r1 = {MESSAGE_COMMAND_BROADCAST, "Ping"};
    Message m1 = {MESSAGE_RICHTEXT_URL, "Client1", "Client2", r1};
    char * s1 = messageToStr(&m1);
    printf("s1: %s\n", s1);
    Message * m2 = strToMessage(s1);
    printf("command: %d\n", m2->command);
    printf("sender: %s\n", m2->sender);
    printf("dest: %s\n", m2->dest);
    printf("rtm.type: %d\n", m2->rtm.type);
    printf("rtm.message: %s\n", m2->rtm.message);
    char * s2 = messageToStr(m2);
    printf("s2: %s\n", s2);

    free(s1);
    free(s2);
    free(m2->dest);
    free(m2->rtm.message);
    free(m2);

    RichTextMessage r3 = {MESSAGE_COMMAND_C2C, "Pong"};
    Message m3 = {MESSAGE_COMMAND_C2C, "Client2", "Client1", r3};
    char * s3 = messageToStr(&m3);
    printf("s3: %s\n", s3);
    Message * m4 = strToMessage(s3);
    char * s4 = messageToStr(m4);
    printf("s4: %s\n", s4);
    free(s3);
    free(s4);
    free(m4->dest);
    free(m4->rtm.message);
    free(m4);

    char * s5 = "2~ ~Jules~1~Salut~";
    Message * m5 = strToMessage(s5);
    char * s6 = messageToStr(m5);
    printf("s6: %s\n", s6);
    free(m5->dest);
    free(m5->rtm.message);
    free (m5);
    free(s6);

    printf("protocol_test ended ...\n");
    return 0;
}