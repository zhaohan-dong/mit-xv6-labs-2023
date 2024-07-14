#include "kernel/types.h"
#include "kernel/stat.h"
#include "kernel/param.h"
#include "user/user.h"

#ifndef MAX_LINE
#define MAX_LINE 1024
#endif

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(2, "Too few arguments. xargs require at least one argument");
        exit(1);
    }

    char *args[MAXARG];
    int arg_index = 0;

    for (int i = 1; i < argc; i++) {
        args[arg_index++] = argv[i];
    }

    int n;
    char c;
    char pipe_per_line_buf[MAX_LINE];
    int pipe_per_line_buf_index = 0;

    // Execute the program before xarg, so the read should be with the previous
    // input
    while ((n = read(0, &c, 1)) > 0) {
        if (c == '\n' || c == '\0') {
            pipe_per_line_buf[pipe_per_line_buf_index] = '\0';
            args[arg_index] = pipe_per_line_buf;
            args[arg_index + 1] = 0;
            if (fork() == 0) {
                exec(args[0], args);
                exit(0); // Exit the child process if exec fails
            } else {
                wait(0); // Wait for the child process to finish
            }
            pipe_per_line_buf_index = 0; // Reset buffer index for the next line
        } else {
            pipe_per_line_buf[pipe_per_line_buf_index++] =
                c; // Add character to buffer
        }
    }
    exit(0);
}