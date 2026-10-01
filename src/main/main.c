#include <stdio.h>
#include <unistd.h>
#include <string.h>
int main(int argc, char** argv){
    if ( strcmp(argv[1], "--version") == 0 ) {
        printf("Systemd-rc Version E0,2\nBy QuasarFoks Community\n");
        return 0;
    } else {
        execlp("/usr/bin/openrc-init", "openrc-init", NULL);
    }
}
