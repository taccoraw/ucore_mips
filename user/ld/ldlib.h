// defs.h
#ifndef NULL
#define NULL ((void *)0)
#endif
#define __noreturn __attribute__((noreturn))
typedef int bool;
typedef char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;
typedef int32_t intptr_t;
typedef uint32_t uintptr_t;
typedef uintptr_t size_t;
typedef intptr_t off_t;
// stdarg.h
typedef __builtin_va_list va_list;
#define va_start(ap, last)              (__builtin_va_start(ap, last))
#define va_arg(ap, type)                (__builtin_va_arg(ap, type))
#define va_end(ap)                      (__builtin_va_end(ap))
// error.h
#define E_PANIC             10
// unistd.h
#define SYS_exit            1
#define SYS_mmap            20
#define SYS_munmap          21
#define SYS_putc            30
#define SYS_open            100
#define SYS_close           101
#define SYS_read            102
#define SYS_write           103
#define SYS_seek            104
#define SYS_mmap2_query     254
#define O_RDONLY            0
#define NO_FD               -0x9527
#define LSEEK_SET           0
#define LSEEK_CUR           1
#define LSEEK_END           2
#define PROT_NONE           0x0
#define PROT_READ           0x1
#define PROT_WRITE          0x2
#define PROT_EXEC           0x4

// elf.h
#define ELF_MAGIC    0x464C457FU
struct elfhdr {
    uint32_t e_magic;
    uint8_t e_elf[12];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
    uint16_t e_shentsize;
    uint16_t e_shnum;
    uint16_t e_shstrndx;
};
struct proghdr {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_va;
    uint32_t p_pa;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
};
#define ELF_PT_LOAD                     1
#define ELF_PT_DYNAMIC                  2
#define ELF_PT_INTERP                   3
#define ELF_PT_PHDR                     6
#define ELF_PF_X                        1
#define ELF_PF_W                        2
#define ELF_PF_R                        4

// auxv
typedef struct
{
  uint32_t a_type;
  union
    {
      uint32_t a_val;
    } a_un;
} Elf32_auxv_t;
#define ELF_AT_NULL     0
#define ELF_AT_PHDR     3
#define ELF_AT_PHENT    4
#define ELF_AT_PHNUM    5
#define ELF_AT_ENTRY    9
#define ELF_AT_BASE     7
// dynamic
typedef uint16_t Elf32_Half;
typedef uint32_t Elf32_Word;
typedef	int32_t  Elf32_Sword;
typedef uint64_t Elf32_Xword;
typedef	int64_t  Elf32_Sxword;
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint16_t Elf32_Section;
typedef Elf32_Half Elf32_Versym;
typedef struct
{
  Elf32_Sword d_tag;
  union
    {
      Elf32_Word d_val;
      Elf32_Addr d_ptr;
    } d_un;
} Elf32_Dyn;
#define DT_NEEDED   1
#define DT_HASH     4
#define DT_STRTAB   5
#define DT_SYMTAB   6
#define DT_STRSZ    10
#define DT_SYMENT   11
#define DT_PLTGOT   3
#define DT_REL      17
#define DT_RELSZ    18
#define DT_RELENT   19
#define DT_MIPS_LOCAL_GOTNO     0x7000000a
#define DT_MIPS_SYMTABNO        0x70000011
#define DT_MIPS_GOTSYM          0x70000013
#define DT_JMPREL   23
#define DT_PLTRELSZ 2
#define DT_MIPS_PLTGOT          0x70000032
#define DT_NULL     0
// reloc
typedef struct
{
  Elf32_Addr	r_offset;
  Elf32_Word	r_info;
} Elf32_Rel;
#define ELF32_R_SYM(val)		((val) >> 8)
#define ELF32_R_TYPE(val)		((val) & 0xff)
#define ELF32_R_MIPS_REL32          0x03
#define ELF32_R_MIPS_JUMP_SLOT      0x7f
#define ELF32_R_MIPS_COPY           0x7e
#define ELF32_R_MIPS_NONE           0x0
// sym
typedef struct
{
  Elf32_Word	st_name;		/* Symbol name (string tbl index) */
  Elf32_Addr	st_value;		/* Symbol value */
  Elf32_Word	st_size;		/* Symbol size */
  unsigned char	st_info;		/* Symbol type and binding */
  unsigned char	st_other;		/* Symbol visibility */
  Elf32_Section	st_shndx;		/* Section index */
} Elf32_Sym;
#define ELF32_ST_BIND(val)		(((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)		((val) & 0xf)
#define STB_LOCAL	0		/* Local symbol */
#define STB_GLOBAL	1		/* Global symbol */
#define STB_WEAK	2		/* Weak symbol */
#define STT_NOTYPE	0		/* Symbol type is unspecified */
#define STT_OBJECT	1		/* Symbol is a data object */
#define STT_FUNC	2		/* Symbol is a code object */
#define STT_SECTION	3		/* Symbol associated with a section */
#define STO_MIPS_PLT	0x8
#define STN_UNDEF	0		/* End of a chain.  */
#define SHN_UNDEF	0		/* Undefined section */

// do_div
#define do_div(n, base) ({                                      \
    unsigned __mod, q;                                          \
    if (base == 10) {                                           \
        q = (n >> 1) + (n >> 2);                                \
        q += q >> 4;                                            \
        q += q >> 8;                                            \
        q += q >> 16;                                           \
        q >>= 3;                                                \
        __mod = n - (((q << 2) + q) << 1);                      \
        q += ((__mod + 6) >> 4);                                \
        __mod = n - (((q << 2) + q) << 1);                      \
        n = q;                                                  \
    } else if (base == 16) {                                    \
        __mod = n & 0xf;                                        \
        n >>= 4;                                                \
    } else if (base == 8) {                                     \
        __mod = n & 0x7;                                        \
        n >>= 3;                                                \
    } else if (base == 2147483648UL) {                          \
        __mod = n & 0x7fffffff;                                 \
        n >>= 31;                                               \
    } else {                                                    \
        __mod = 3;                                              \
        n = 1;                                                  \
    }                                                           \
    __mod;                                                      \
 })

// panic/assert
void __noreturn __panic(const char *file, int line, const char *fmt, ...);
#define panic(...)                                      \
    __panic(__FILE__, __LINE__, __VA_ARGS__)
#define assert(x)                                       \
    do {                                                \
        if (!(x)) {                                     \
            panic("assertion failed: %s", #x);          \
        }                                               \
    } while (0)


int sys_open(const char *path, uint32_t open_flags);
int sys_close(int fd);
int sys_read(int fd, void *base, size_t len);
int sys_write(int fd, void *base, size_t len);
int sys_seek(int fd, off_t pos, int whence);
void* sys_mmap2(void *addr, size_t length, int prot, int fd, off_t offset);
uint32_t sys_mmap2_query();
size_t strnlen(const char *s, size_t len);
int strcmp(const char *s1, const char *s2);
void *memset(void *s, char c, size_t n);
int cprintf(const char *fmt, ...);

