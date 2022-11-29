#include "protocol.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

char *removeSeparatorFromString(char *str)
{
    int i, j;
    i = 0;
    while (i < strlen(str))
    {
        if (str[i] == '~')
        {
            for (j = i; j < strlen(str); j++) {
                str[j] = str[j + 1];
            }
        }
        else
            i++;
    }
    return str;
}

/*char *removeSeparatorFromString(char *str) {
    const char sub = '~';
    size_t len = strlen(sub);
    if (len > 0) {
        char *p = str;
        while ((p = strstr(p, sub)) != NULL) {
            memmove(p, p + len, strlen(p + len) + 1);
        }
    }
    return str;
}*/


/* convert contents of message structure to a string of values
   separated by the '~' char. Returns pointer to
   dynamically allocated string containing contents
   of message structure on success, otherwise NULL.
*/
char *messageToStr(const Message *m)
{
    /* get lenght of string required to hold struct values */
    size_t len = 0;

    len = snprintf(NULL, len, "%d%c%s%c%s%c%d%c%s%c", m->command, '~', removeSeparatorFromString(m->sender), '~', removeSeparatorFromString(m->dest), '~', m->rtm.type, '~', removeSeparatorFromString(m->rtm.message), '~');

    /* allocate/validate string to hold all values (+1 to null-terminate) */
    char *apstr = calloc(1, sizeof *apstr * len + 1);
    if (!apstr)
    {
        fprintf(stderr, "%s() error: virtual memory allocation failed.\n", __func__);
        return NULL;
    }

    /* write/validate struct values to apstr */
    if (snprintf(apstr, len + 1, "%d%c%s%c%s%c%d%c%s%c", m->command, '~', removeSeparatorFromString(m->sender), '~', removeSeparatorFromString(m->dest), '~', m->rtm.type, '~', removeSeparatorFromString(m->rtm.message), '~') > len + 1)
    {
        fprintf(stderr, "%s() error: snprintf returned truncated result.\n", __func__);
        return NULL;
    }

    return apstr;
}

/* convert a string of values
   separated by the '~' to a message structure.
   Returns pointer to dynamically allocated Message structure
   containing contents  of message structure on
   success, otherwise NULL.
*/
Message *strToMessage(const char *str)
{
    int i, oldI, j, parameterIndex, sizeLimit;
    i = 0;
    oldI = 0;
    parameterIndex = 0;
    Message *out = malloc(sizeof(Message));
    if (out == NULL) {
        fprintf(stderr, "[strToMessage] Could not allocate %ld bytes\n", sizeof(Message));
        return NULL;
    }

    const int strLen = strlen(str);
    
    while (i < strLen)
    {
        if (str[i] == '~')
        {
            char *arr;
            if (parameterIndex != 4) // RICH TEXT PARAMETER INDEX. Update it if the message struct evolves !
            {
                arr = malloc(sizeof(char) * (i - oldI + 1));
                if (arr == NULL) {
                    fprintf(stderr, "[strToMessage] Could not allocate %ld bytes\n", sizeof(char) * (i - oldI));
                    return NULL;
                }
                for (j = oldI; j < i; ++j)
                {
                    arr[j - oldI] = str[j];
                }
                arr[i - oldI] = '\0';
            }
            switch (parameterIndex)
            {
            case 0:
                if ((out->command = atoi(arr)) == 0)
                {
                    fprintf(stderr, "[strToMessage] Could not convert str's command parameter '%s' to a number ! Return value: %d\n", arr, out->command);
                    free(arr);
                    return NULL;
                }
                free(arr);
                break;
            case 1:
                out->sender = malloc(sizeof arr);
                if (out->sender == NULL) {
                    fprintf(stderr, "[strToMessage] Could not allocate %ld bytes\n", sizeof arr);
                    free(arr);
                    return NULL;
                }
                memcpy(out->sender, arr, sizeof arr);
                free(arr);
                break;
            case 2:
                out->dest = malloc(sizeof arr);
                if (out->dest == NULL) {
                    fprintf(stderr, "[strToMessage] Could not allocate %ld bytes\n", sizeof arr);
                    free(arr);
                    return NULL;
                }
                memcpy(out->dest, arr, sizeof arr);
                free(arr);
                break;
            case 3:
                if ((out->rtm.type = atoi(arr)) == 0)
                {
                    fprintf(stderr, "[strToMessage] Could not convert str's rtm type parameter '%s' to a number ! Return value: %d\n", arr, out->rtm.type);
                    free(arr);
                    return NULL;
                }
                break;
            case 4:
                switch (out->rtm.type)
                {
                case MESSAGE_RICHTEXT_STR:
                    // empty case
                case MESSAGE_RICHTEXT_REACTION:
                    // empty case
                case MESSAGE_RICHTEXT_URL:
                    sizeLimit = MESSAGE_RICHTEXT_STR_LENGTH;
                    break;
                case MESSAGE_RICHTEXT_IMG:
                    sizeLimit = MESSAGE_RICHTEXT_IMAGE_LENGTH;
                    break;
                case MESSAGE_RICHTEXT_FILE:
                    sizeLimit = MESSAGE_RICHTEXT_FILE_LENGTH;
                    break;
                default:
                    sizeLimit = MESSAGE_RICHTEXT_STR_LENGTH;
                    break;
                }
                if (i - oldI > sizeLimit)
                {
                    fprintf(stderr, "[strToMessage] The message's length is %d. However, it must be less than '%d' for the message type %d\n", i - oldI, sizeLimit, out->rtm.type);
                    free(out->dest);
                    return NULL;
                }
                out->rtm.message = malloc(sizeof(char) * (i - oldI + 1));
                if (out->rtm.message == NULL)
                {
                    fprintf(stderr, "[strToMessage] Could not allocate %ld bytes\n", sizeof(char) * (i - oldI));
                    free(out->dest);
                    return NULL;
                }

                for (j = oldI; j < i; ++j)
                {
                    out->rtm.message[j - oldI] = str[j];
                }
                out->rtm.message[i - oldI] = '\0';
                break;
            default:
                fprintf(stderr, "[strToMessage] There's more than 4 parameters in the string to unparse\n");
                free(out->dest);
                free(out->rtm.message);
                return NULL;
            }

            oldI = i + 1; // +1 to skip the char separator
            ++parameterIndex;
        }
        ++i;
    }
    return out;
}