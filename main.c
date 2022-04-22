#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>
#define MAX_LINE 80 /* The maximum length command */
#define TRUE 1
#define FALSE 0
typedef int bool;

int read_command(char *command, int *argc, char *argv[]);
void deallocate_args(int argc, char *argv[]);

int main(void)
{
    bool should_wait = FALSE;

    while (TRUE)
    {
        printf("shell> ");
        char command[MAX_LINE] = "";
        int argc = 0;
        char *argv[MAX_LINE / 2 + 1];
        if (read_command(command, &argc, argv) < 0)
        {
            /* Continue incase of reading errors */
            continue;
        }

        // exit shell
        if (strcmp(command, "exit") == 0)
            break;

        // free memory space allocated for argv
        deallocate_args(argc, argv);
    }
    return 0;
}

int read_command(char *command, int *argc, char *argv[])
{
    char command_line[MAX_LINE];
    fgets(command_line, MAX_LINE, stdin);

    /* Return -1 indicating error if user entered nothing (fgets capture new line character) */
    if (command_line[0] == '\n')
        return -1;

    // Return -1 indicating error if the input exceeded allowed length (MAX_LINE)
    if (command_line[strlen(command_line) - 1] != '\n')
    {
        printf("Exceeded allowed command length\n");
        // flush the standard input
        while (getchar() != '\n')
            ;
        return -1;
    }

    /* Remove trainling newline */
    command_line[strlen(command_line) - 1] = '\0';

    /* split input (command_line) by space
    saving first word in command variable and then the rest to argv array */
    char cur_word[MAX_LINE] = "";
    for (int i = 0; i <= strlen(command_line); i++)
    {
        /* incase of space or end of string */
        if (command_line[i] == ' ' || command_line[i] == '\0')
        {
            if (strlen(command) == 0)
                strcpy(command, cur_word);
            else
            {
                *argc += 1;
                argv[*argc - 1] = (char *)malloc(MAX_LINE * sizeof(cur_word));
                strcpy(argv[*argc - 1], cur_word);
            }
            /* reset cur_word */
            strcpy(cur_word, "");
        }
        else
            /* Building cur_word character by character */
            strncat(cur_word, &command_line[i], 1);
    }

    return 0;
};

void deallocate_args(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++)
        free(argv[i]);
}
