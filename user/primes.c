#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

void generate_number(int start, int stop, int pipe_write) {
    for (int i = start; i <= stop; i++) {
        write(pipe_write, &i, sizeof(i));
    }
}

void filter_primes(int pipe_read) {
    int prime;
    int number = -1;

    read(pipe_read, &prime, sizeof(prime));
    if (prime) {
        printf("prime %d\n", prime);
    }

    int new_pipe[2];
    pipe(new_pipe);
    while (1) {
        int n = read(pipe_read, &number, sizeof(number));
        if (n <= 0) {
            break;
        }
        if (number % prime != 0) {
            write(new_pipe[1], &number, sizeof(number));
        }
    }

    if (fork() == 0) {
        // Child process continues filtering
        close(pipe_read);
        close(new_pipe[1]); // Close the write end of the new pipe
        filter_primes(new_pipe[0]);
        close(new_pipe[0]);
    } else {
        // Parent process filters numbers and writes to new pipe
        close(new_pipe[0]); // Close the read end of the new pipe
        close(new_pipe[1]); // Close the write end of the new pipe
        close(pipe_read);   // Close the read end of the current pipe
        wait(0);            // Wait for the child process to finish
    }
}

int main(int argc, char *argv[]) {
    int min_num = 2, max_num = 35;
    int initial_pipe[2];
    pipe(initial_pipe);

    if (fork() == 0) {
        // Child process
        close(initial_pipe[1]); // Close write end of first pipe
        filter_primes(initial_pipe[0]);
    } else {
        // Parent process
        close(initial_pipe[0]); // Close read end of first pipe
        generate_number(min_num, max_num, initial_pipe[1]);
        close(initial_pipe[1]);
        wait(0);
    }

    exit(0);
}