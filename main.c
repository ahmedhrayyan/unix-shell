#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>

#define MAX_LINE 80 /* The maximum length command */
#define TRUE 1
#define FALSE 0
typedef int bool;

int read_command(int *argc, char *argv[]);
void deallocate_args(int argc, char *argv[]);
char *trim(char *s);

int main(void)
{
    bool should_wait = FALSE;

    while (TRUE)
    {
        printf("shell> ");
        int argc;
        char *argv[MAX_LINE / 2 + 1];
        if (read_command(&argc, argv) < 0) /* Continue incase of reading errors */
            continue;

        if (strcmp(argv[0], "exit") == 0) /* exit shell */
            break;

        pid_t child_pid = fork();

        if (child_pid < 0)
        {
            printf("Fork failed\n");
            exit(1);
        }
        if (child_pid == 0)
        {
            /* Child proccess */
            if (execvp(argv[0], argv) == -1)
            {
                perror("execvp");
                exit(1);
            }
            exit(0);
        }

        wait(NULL);

        // free memory space allocated for argv
        deallocate_args(argc, argv);
    }
    return 0;
}

int read_command(int *argc, char *argv[])
{
    *argc = 0;

    char command_line[MAX_LINE];
    fgets(command_line, MAX_LINE, stdin);

    if (command_line[strlen(command_line) - 1] != '\n')
    {
        printf("Exceeded allowed command length\n");
        // flush the standard input
        while (getchar() != '\n')
            ;
        return -1;
    }

    /* Trim whitespace from command_line */
    strcpy(command_line, trim(command_line));

    if (strlen(command_line) == 0)
        return -1;

    char cur_word[MAX_LINE] = "";
    for (int i = 0; i <= strlen(command_line); i++)
    {
        if (command_line[i] == ' ' || command_line[i] == '\0')
        {
            if (command_line[i - 1] == ' ') /* Ignore if the previous char was space */
                continue;
            *argc += 1;
            argv[*argc - 1] = (char *)malloc(MAX_LINE * sizeof(cur_word));
            strcpy(argv[*argc - 1], cur_word);
            strcpy(cur_word, ""); /* reset cur_word */
        }
        else
            strncat(cur_word, &command_line[i], 1);
    }

    argv[*argc] = NULL; /* Append NULL to the end of argv (required by execvp) */

    return 0;
};

void deallocate_args(int argc, char *argv[])
{
    for (int i = 0; i < argc; i++)
        free(argv[i]);
}

/* ref: https://stackoverflow.com/a/1431206/10272966 */
char *ltrim(char *s)
{
    while (isspace(*s))
        s++;
    return s;
}

char *rtrim(char *s)
{
    char *back = s + strlen(s);
    while (isspace(*--back))
        ;
    *(back + 1) = '\0';
    return s;
}

char *trim(char *s)
{
    return rtrim(ltrim(s));
}