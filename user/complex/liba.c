#include "liba.h"
#include "libaa.h"

int a_g_data = 10, a_g_bss;
static int a_l_data = 9, a_l_bss;
static void a_set_bss_aux_l(int x) {
	a_g_bss = x + 1;
}
void a_set_bss(int x) {
	extern int var_g_bss;
	a_set_bss_aux_l(x);
	a_set_bss_aux_g(x);
	aa_g_bss = aa_g_data + a_l_data;
	var_g_bss = aa_sth(x) + aa_sth(x);
}
void a_set_bss_aux_g(int x) {
	a_l_bss = x - 1;
}

