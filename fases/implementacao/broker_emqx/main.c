#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

void execute_script(const char *script_name) {
    #ifdef _WIN32
    char command[256];
    snprintf(command, sizeof(command), "bash %s", script_name);
    system(command);
    #else
    char command[256];
    snprintf(command, sizeof(command), "./%s", script_name);
    system(command);
    #endif
}

int main() {
    const char *script_name = "install_emqx.sh";

    #ifndef _WIN32
    char chmod_command[256];
    snprintf(chmod_command, sizeof(chmod_command), "chmod +x %s", script_name);
    system(chmod_command);
    #endif

    execute_script(script_name);

    return 0;
}
