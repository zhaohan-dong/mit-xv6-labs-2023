#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int main(int argc, char *argv[]) {
    int sleep_period;

    if (argc > 2) {
        fprintf(2, "Usage: sleep(time_ticks)\n");
        exit(1);
    }

    sleep_period = atoi(argv[1]);
    sleep(sleep_period);
    exit(0);
}