// ======================================================================== //
// author:  ixty                                                       2018 //
// project: mandibule                                                       //
// licence: beerware                                                        //
// ======================================================================== //

// small example of code that we inject into a remote process

#include <signal.h>
#include <stdio.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

int main(int ac, char **av, char **env) {
    printf("# oh hai from pid %d\n", getpid());
    for (int i = 0; i < ac; i++)
        printf("# arg[%d]: %s\n", i, av[i]);

    for (int i = 0; i < 3; i++) {
        printf("# :)\n");
        fflush(stdout);
        sleep(1);
    }

    printf("# bye!\n");
    return 44;
}
