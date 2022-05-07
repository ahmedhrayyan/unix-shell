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

struct command
{
    int argc;
    char **argv;
};

int read_command(struct command *command, int *should_wait);
void deallocate_command(struct command *command);
char *ltrim(char *s);
char *rtrim(char *s);
char *trim(char *s);

int main(void)
{
    bool prev_should_wait = TRUE;
    while (TRUE)
    {
        printf("shell> ");
        bool should_wait = TRUE;
        struct command command = {.argc = 0, .argv = malloc(MAX_LINE * sizeof(char *))};
        if (read_command(&command, &should_wait) == -1) // handle errors
            continue;

        if (strcmp(command.argv[0], "exit") == 0)
            break;

        if (prev_should_wait == FALSE) // fix wired behaviour when not waiting the children proccess to finish
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

            /* handle redirects or pipes */
            for (int i = 0; i < command.argc; i++)
            {
                if (strcmp(command.argv[i], ">") == 0) // handle redirecting to a file
                {
                    int fd = open(command.argv[i + 1], O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
                    dup2(fd, STDOUT_FILENO);
                    command.argv[i] = NULL; // stop command execution at i
                    break;
                }
                else if (strcmp(command.argv[i], "<") == 0) // handle redirection from a file
                {
                    command.argv[i] = command.argv[i + 1];
                    command.argv[i + 1] = NULL;
                    break;
                }
                else if (strcmp(command.argv[i], "|") == 0) // handle pipes
                {
                    command.argv[i] = NULL; // stop first command execution at i
                    /* build second_command struct
                    Note: the code in the following loop is simillar to the code exists in read_command function */
                    struct command second_command = {.argc = 0, .argv = malloc(MAX_LINE * sizeof(char *))};
                    for (int j = i + 1; j < command.argc; j++)
                    {
                        int arg_index = second_command.argc; // put the new argument to argv end
                        second_command.argv[arg_index] = malloc(sizeof(command.argv[j]));
                        strcpy(second_command.argv[arg_index], command.argv[j]);
                        second_command.argc += 1;
                    }
                    second_command.argv[second_command.argc] = NULL; // required by execvp

                    int fd[2];
                    pipe(fd);
                    pid_t child_pid = fork();
                    if (child_pid < 0)
                    {
                        printf("Fork failed\n");
                        exit(1);
                    }
                    else if (child_pid == 0)
                    {
                        /* -------- child proccess ------- */
                        close(fd[0]);
                        dup2(fd[1], STDOUT_FILENO);
                        // execute the first command in the created child proccess
                        if (execvp(command.argv[0], command.argv) == -1)
                        {
                            perror("execvp");
                            exit(1);
                        }
                        exit(0);
                    }
                    /* --------- Parent ---------- */
                    close(fd[1]);
                    dup2(fd[0], STDIN_FILENO);
                    // execute the second command in the parent proccess
                    if (execvp(second_command.argv[0], second_command.argv) == -1)
                    {
                        perror("execvp");
                        exit(1);
                    }
                    deallocate_command(&second_command);
                    exit(0); // Exit & do not execute further code in child procces
                }
            }
            /* End handle redirects & pipes */

            if (execvp(command.argv[0], command.argv) == -1)
            {
                perror("execvp");
                exit(1);
            }
            exit(0);
        }

        if (should_wait)
            wait(NULL);

        prev_should_wait = should_wait;

        deallocate_command(&command);
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
int read_command(struct command *command, int *should_wait)
{
    char command_line[MAX_LINE];
    fgets(command_line, MAX_LINE, stdin);

    if (command_line[strlen(command_line) - 1] != '\n')
    {
        printf("Exceeded allowed command length\n");
        while (getchar() != '\n') // flush the standard input
            ;
        return -1;
    }

    trim(command_line);

    int command_size = strlen(command_line);

    if (command_line[command_size - 1] == '&')
    {
        *should_wait = FALSE;
        command_line[command_size - 1] = '\0'; // remove & character
        rtrim(command_line);
        command_size = strlen(command_line);
    }

    if (strlen(command_line) == 0)
        return -1;

    command->argc = 0;
    command->argv = (char **)malloc((MAX_LINE / 2 + 1) * sizeof(char *)); // malloc 41 space for argv
    char cur_word[MAX_LINE] = "";
    for (int i = 0; i <= strlen(command_line); i++)
    {
        if (command_line[i] == ' ' || command_line[i] == '\0')
        {
            if (command_line[i - 1] == ' ') // Ignore if the previous char was space
                continue;
            int arg_index = command->argc; // put the new argument to argv end
            command->argv[arg_index] = malloc(sizeof(cur_word));
            strcpy(command->argv[arg_index], cur_word);

            command->argc += 1;
            strcpy(cur_word, ""); // reset cur_word
        }
        else
            strncat(cur_word, &command_line[i], 1);
    }

    command->argv[command->argc] = NULL; // put NULL to the end of argv (required by execvp)
    return 0;                            // no errors
};

void deallocate_command(struct command *command)
{
    for (int i = 0; i < command->argc; i++)
        free(command->argv[i]);
    free(command->argv);
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