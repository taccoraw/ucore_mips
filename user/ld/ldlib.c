#include "ldlib.h"

// syscall.c
#define MAX_ARGS            5
uint32_t do_syscall(uint32_t arg0, uint32_t arg1, uint32_t arg2, uint32_t arg3, uint32_t arg4, uint32_t num);
// static functions generate `bal` instructions
int syscall(int num, ...) {
    va_list ap;
    va_start(ap, num);
    uint32_t a[MAX_ARGS];
    int i, ret;
    for (i = 0; i < MAX_ARGS; i ++) {
        a[i] = va_arg(ap, uint32_t);
    }
    va_end(ap);
    ret = do_syscall(a[0], a[1], a[2], a[3], a[4], num);
    return ret;
}
int sys_exit(int error_code) {
    return syscall(SYS_exit, error_code);
}
int sys_putc(int c) {
    return syscall(SYS_putc, c);
}
int sys_open(const char *path, uint32_t open_flags) {
    return syscall(SYS_open, path, open_flags);
}
int sys_close(int fd) {
    return syscall(SYS_close, fd);
}
int sys_read(int fd, void *base, size_t len) {
    return syscall(SYS_read, fd, base, len);
}
int sys_write(int fd, void *base, size_t len) {
    return syscall(SYS_write, fd, base, len);
}
int sys_seek(int fd, off_t pos, int whence) {
    return syscall(SYS_seek, fd, pos, whence);
}
void* sys_mmap2(void *addr, size_t length, int prot, int fd, off_t offset) {
	return (void*)syscall(SYS_mmap, addr, length, prot, fd, offset);
}
uint32_t sys_mmap2_query() {
	return (uint32_t)syscall(SYS_mmap2_query);
}

// string.c
size_t strnlen(const char *s, size_t len) {
    size_t cnt = 0;
    while (cnt < len && *s ++ != '\0') {
        cnt ++;
    }
    return cnt;
}
int strcmp(const char *s1, const char *s2) {
    while (*s1 != '\0' && *s1 == *s2) {
        s1 ++, s2 ++;
    }
    return (int)((unsigned char)*s1 - (unsigned char)*s2);
}
void *memset(void *s, char c, size_t n) {
    char *p = s;
    while (n -- > 0) {
        *p ++ = c;
    }
    return s;
}

// printfmt.c
void printnum(void (*putch)(int, void*, int), int fd, void *putdat,
        unsigned long long num, unsigned base, int width, int padc) {
    unsigned long long result = num;
    unsigned mod = do_div(result, base);

    // first recursively print all preceding (more significant) digits
    if (num >= base) {
        printnum(putch, fd, putdat, result, base, width - 1, padc);
    } else {
        // print any needed pad characters before first digit
        while (-- width > 0)
            putch(padc, putdat, fd);
    }
    // then print this (the least significant) digit
    putch("0123456789abcdef"[mod], putdat, fd);
}
unsigned long long getuint(va_list *ap, int lflag) {
    if (lflag >= 2) {
        return va_arg(*ap, unsigned long long);
    }
    else if (lflag) {
        return va_arg(*ap, unsigned long);
    }
    else {
        return va_arg(*ap, unsigned int);
    }
}
long long getint(va_list *ap, int lflag) {
    if (lflag >= 2) {
        return va_arg(*ap, long long);
    }
    else if (lflag) {
        return va_arg(*ap, long);
    }
    else {
        return va_arg(*ap, int);
    }
}
void vprintfmt(void (*putch)(int, void*, int), int fd, void *putdat, const char *fmt, va_list ap) {
    register const char *p;
    register int ch/*, err*/;
    unsigned long long num;
    int base, width, precision, lflag, altflag;

    while (1) {
        while ((ch = *(unsigned char *)fmt ++) != '%') {
            if (ch == '\0') {
                return;
            }
            putch(ch, putdat, fd);
        }

        // Process a %-escape sequence
        char padc = ' ';
        width = precision = -1;
        lflag = altflag = 0;

    reswitch:
        switch (ch = *(unsigned char *)fmt ++) {

        // flag to pad on the right
        case '-':
            padc = '-';
            goto reswitch;

        // flag to pad with 0's instead of spaces
        case '0':
            padc = '0';
            goto reswitch;

        // width field
        case '1' ... '9':
            for (precision = 0; ; ++ fmt) {
                precision = precision * 10 + ch - '0';
                ch = *fmt;
                if (ch < '0' || ch > '9') {
                    break;
                }
            }
            goto process_precision;

        case '*':
            precision = va_arg(ap, int);
            goto process_precision;

        case '.':
            if (width < 0)
                width = 0;
            goto reswitch;

        case '#':
            altflag = 1;
            goto reswitch;

        process_precision:
            if (width < 0)
                width = precision, precision = -1;
            goto reswitch;

        // long flag (doubled for long long)
        case 'l':
            lflag ++;
            goto reswitch;

        // character
        case 'c':
            putch(va_arg(ap, int), putdat, fd);
            break;

        // error message
        /*
        case 'e':
            err = va_arg(ap, int);
            if (err < 0) {
                err = -err;
            }
            if (err > MAXERROR || (p = error_string[err]) == NULL) {
                printfmt(putch, fd, putdat, "error %d", err);
            }
            else {
                printfmt(putch, fd, putdat, "%s", p);
            }
            break;
        */
        // string
        case 's':
            if ((p = va_arg(ap, char *)) == NULL) {
                p = "(null)";
            }
            if (width > 0 && padc != '-') {
                for (width -= strnlen(p, precision); width > 0; width --) {
                    putch(padc, putdat, fd);
                }
            }
            for (; (ch = *p ++) != '\0' && (precision < 0 || -- precision >= 0); width --) {
                if (altflag && (ch < ' ' || ch > '~')) {
                    putch('?', putdat, fd);
                }
                else {
                    putch(ch, putdat, fd);
                }
            }
            for (; width > 0; width --) {
                putch(' ', putdat, fd);
            }
            break;

        // (signed) decimal
        case 'd':
            num = getint(&ap, lflag);
            if ((long long)num < 0) {
                putch('-', putdat, fd);
                num = -(long long)num;
            }
            base = 10;
            goto number;

        // unsigned decimal
        case 'u':
            num = getuint(&ap, lflag);
            base = 10;
            goto number;

        // (unsigned) octal
        case 'o':
            num = getuint(&ap, lflag);
            base = 8;
            goto number;

        // pointer
        case 'p':
            putch('0', putdat, fd);
            putch('x', putdat, fd);
            num = (unsigned long long)(uintptr_t)va_arg(ap, void *);
            base = 16;
            goto number;

        // (unsigned) hexadecimal
        case 'x':
            num = getuint(&ap, lflag);
            base = 16;
        number:
            printnum(putch, fd, putdat, num, base, width, padc);
            break;

        // escaped '%' character
        case '%':
            putch(ch, putdat, fd);
            break;

        // unrecognized escape sequence - just print it literally
        default:
            putch('%', putdat, fd);
            for (fmt --; fmt[-1] != '%'; fmt --)
                /* do nothing */;
            break;
        }
    }
}

// stdio.c
void cputch(int c, int *cnt) {
    sys_putc(c);
    (*cnt) ++;
}
int vcprintf(const char *fmt, va_list ap) {
    int cnt = 0;
    vprintfmt((void*)cputch, NO_FD, &cnt, fmt, ap);
    return cnt;
}
int cprintf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    int cnt = vcprintf(fmt, ap);
    va_end(ap);
    return cnt;
}

// panic/assert
void __panic(const char *file, int line, const char *fmt, ...) {
    // print the 'message'
    va_list ap;
    va_start(ap, fmt);
    cprintf("user panic at %s:%d:\n    ", file, line);
    vcprintf(fmt, ap);
    cprintf("\n");
    va_end(ap);
    sys_exit(-E_PANIC);
    while(1);
}

