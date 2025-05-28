//Fevzi Kagan Becel, 31141
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/wait.h>

int main(int argc, char *argv[]) {
    printf("I'm SHELL process with PID: %d - Main command is: $ man ping | grep -A 7 -m 1 -e '-f' > output.txt \n", getpid());

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        perror("Pipe failed");
        exit(EXIT_FAILURE);
    }

    pid_t man_pid = fork();
    if (man_pid == -1) {
        perror("Man fork failed");
        exit(EXIT_FAILURE);
    }

    if (man_pid == 0) {
        printf("I'm MAN process with PID: %d - My command is: $ man ping \n", getpid());
        // Child process for 'man'
        close(pipe_fd[0]); // Close the read end of the pipe

        // Redirect 'man' output to the write end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        char *man_args[] = {"man", "ping", NULL};
        execvp("man", man_args);
        perror("execvp for 'man' failed");
        exit(EXIT_FAILURE);
    } else {
        pid_t grep_pid = fork();

        if (grep_pid == -1) {
            perror("Grep fork failed");
            exit(EXIT_FAILURE);
        }
        if (grep_pid == 0) {
            printf("I'm GREP process with PID: %d - My command is: $ grep -A 7 -m 1 -e '-f' > output.txt \n", getpid());
            // Child process for 'grep'
            close(pipe_fd[1]); // Close write end of the pipe

            // Redirect 'grep' input from the read end of the pipe
            dup2(pipe_fd[0], STDIN_FILENO);
            close(pipe_fd[0]);

            int output = open("output.txt", O_CREAT | O_WRONLY | O_TRUNC, 0644);
            dup2(output, STDOUT_FILENO);
            close(output);

            char *grep_args[] = {"grep", "-A", "7", "-m", "1", "-e", "-f", NULL};
            execvp("grep", grep_args);
            perror("execvp for 'grep' failed");
            exit(EXIT_FAILURE);
        } else {
            // Parent process
            close(pipe_fd[0]); // Close unused pipe ends
            close(pipe_fd[1]);

            int man_status, grep_status;
            waitpid(man_pid, &man_status, 0);
            waitpid(grep_pid, &grep_status, 0);

            if (WIFEXITED(man_status) && WEXITSTATUS(man_status) != 0) {
                fprintf(stderr, "The 'man' command was not executed successfully\n");
                exit(EXIT_FAILURE);
            }
            if (WIFEXITED(grep_status) && WEXITSTATUS(grep_status) != 0) {
                fprintf(stderr, "The 'grep' command was not executed successfully\n");
                exit(EXIT_FAILURE);
            }
        }
    }

    printf("I'm SHELL process, with PID: %d - execution is completed, you can find the results in output.txt\n", getpid());
    return 0;
}

