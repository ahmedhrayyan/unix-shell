#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>

#define MAX_LINE 80 /* The maximum length command */
#define TRUE 1
#define FALSE 0
typedef int bool;

int read_command(int *argc, char *argv[]);
void deallocate_args(int argc, char *argv[]);
char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *s);

int main(void)
{
    bool prev_should_wait = TRUE;
    while (TRUE)
    {
        printf("shell> ");
        int argc;
        char *argv[MAX_LINE / 2 + 1];
        bool should_wait = read_command(&argc, argv);

        if (should_wait == -1) /* Handle errors */
            continue;

        if (strcmp(argv[0], "exit") == 0)
            break;

        if (prev_should_wait == FALSE)
            wait(NULL);

        pid_t child_pid = fork();

        if (child_pid < 0)
        {
            printf("Fork failed\n");
            exit(1);
        }
        if (child_pid == 0)
        {
            /* ---------- Child proccess --------  */

            /* Handle redirects */
            for (int i = 0; i < argc; i++)
            {
                if (strcmp(argv[i], ">") == 0)
                {
                    int fd = open(argv[i + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                    dup2(fd, STDOUT_FILENO);
                    argv[i] = NULL; /* Stop command execution at i */
                    break;
                }
                else if (strcmp(argv[i], "<") == 0)
                {
                    argv[i] = argv[i + 1];
                    argv[i + 1] = NULL; /* Stop command execution at i + 1 */
                    break;
                }
            }

            if (execvp(argv[0], argv) == -1)
            {
                perror("execvp");
                exit(1);
            }
            exit(0);
        }

        if (should_wait)
            wait(NULL);

        prev_should_wait = should_wait;

        // free memory space allocated for argv
        deallocate_args(argc, argv);
    }
    return 0;
}

/**
 * @brief read command line from stdin & convert it into args array
 * @param argc
 * @param argv
 * @param should_wait
 * @return -1 incase of errors or bool indicating wether it should wait or not
 */
int read_command(int *argc, char *argv[])
{
    *argc = 0;

    char command_line[MAX_LINE];
    fgets(command_line, MAX_LINE, stdin);

    if (command_line[strlen(command_line) - 1] != '\n')
    {
        printf("Exceeded allowed command length\n");
        while (getchar() != '\n') /* flush the standard input */
            ;
        return -1;
    }

    trim(command_line);

    int command_size = strlen(command_line);

    bool should_wait = TRUE;
    if (command_line[command_size - 1] == '&')
    {
        should_wait = FALSE;
        command_line[command_size - 1] = '\0'; /* remove & character */
        rtrim(command_line);
        command_size = strlen(command_line);
    }

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

    return should_wait;
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