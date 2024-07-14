#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    if (argc > 1) {
        fprintf(2, "Error: Usage pingpong\n");
        exit(1);
    }

    char byte = '0';
    int pipe1[2], pipe2[2];

    pipe(pipe1); // Parent to child
    pipe(pipe2); // Child to parent

    int pid = fork();

    if (pid == 0) {
        // Child process
        char tmp_byte;

        close(pipe1[0]); // Close pipe 1 input
        close(pipe2[1]); // CLose pipe 2 output

        read(pipe1[1], &tmp_byte, 1);
        printf("%d: received ping\n", getpid());
        write(pipe2[0], &tmp_byte, 1);

        close(pipe1[1]);
        close(pipe2[0]);
        exit(0);
    } else if (pid > 0) {
        // Parent process
        char tmp_byte;

        close(pipe1[1]);
        close(pipe2[0]);

        write(pipe1[0], &byte, 1);
        wait(0);
        read(pipe2[1], &tmp_byte, 1);
        printf("%d: received pong\n", getpid());

        close(pipe1[0]);
        close(pipe2[1]);
    } else {
        fprintf(2, "Error creating fork. Exiting...");
        exit(1);
    }

    exit(0);
}