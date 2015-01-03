#include "libaa.h"

int aa_g_data = 1, aa_g_bss;
int aa_sth(int x) {
	return x + aa_g_data + (aa_g_bss++);
}

