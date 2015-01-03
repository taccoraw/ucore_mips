#include "libc.h"
#include "libca.h"

void c_cmp(ca_ftype f) {
	if (f == ca_func)
		ca_var = 9;
	else
		ca_var = 8;
}

