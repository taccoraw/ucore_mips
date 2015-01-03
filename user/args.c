#include <stdio.h>
int main(int argc, const char* argv[], const char* envp[]) {
    cprintf("argc = %d\n", argc);
    cprintf("argv[argc] = 0x%08x\n", argv[argc]);
    cprintf("envp[0] = 0x%08x\n", envp[0]);
    cprintf("&argv[argc] = 0x%08x\n", &argv[argc]);
    cprintf("envp = 0x%08x\n", envp);
    return 0;
}

