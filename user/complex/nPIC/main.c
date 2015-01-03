#include "liba.h"
#include "libb.h"
#include "libc.h"
#include "libca.h"
#include <stdio.h>

#define SHOW_VAR(x) cprintf("SHOW_VAR: " #x " = %d\n", x)

int var_g_data = 12, var_g_bss;
static int var_l_data = 11, var_l_bss;

static int func_f(int n) {
	SHOW_VAR(n);
	SHOW_VAR(var_g_data);
	SHOW_VAR(var_l_bss);
	return n + var_g_data + var_l_bss;
}
int func_g(int n) {
	SHOW_VAR(var_g_bss);
	SHOW_VAR(var_l_data);
	return func_f(n) + var_g_bss + var_l_data;
}

int main(int argc, char* argv[], char* envp[]) {
	cprintf("Before b_simple\n");
	SHOW_VAR(a_g_data);
	var_l_bss = b_simple(a_g_data);
	SHOW_VAR(var_l_bss);
	cprintf("b_simple - a_set_bss\n");
	a_set_bss(var_l_bss);
	SHOW_VAR(a_g_bss);
	SHOW_VAR(var_g_bss);
	cprintf("a_set_bss - c_cmp\n");
	c_cmp(ca_func);
	SHOW_VAR(ca_var);
	cprintf("c_cmp - b_simple\n");
	b_simple(12);
	cprintf("After b_simple\n");
	int ret = func_g(a_g_bss) + ca_var;
	cprintf("EXIT: %d\n", ret);
	cprintf("argc = %d\n", argc);
	int i;
	for (i = 0; i < argc; i++)
		cprintf("argv[%d] = [0x%08x] %s\n", i, argv[i], argv[i]);
	cprintf("argv[argc] = 0x%08x\n", argv[argc]);
	cprintf("envp[0] = 0x%08x\n", envp[0]);
	cprintf("&argv[argc] = 0x%08x\n", &argv[argc]);
	cprintf("envp = 0x%08x\n", envp);
	return ret;
}

