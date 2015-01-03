#include "ldlib.h"

#define SHOW_VAR(x) cprintf(#x " = %d [0x%08x]\n", (int)x, x)
extern int lda_di_num;
#define LD_NEEDED_MAX		6

typedef struct {
	const char* path;
	uint32_t base;
	uint32_t max_addr;
	int needed_num;
	Elf32_Word needed[LD_NEEDED_MAX];
	// DynInfo* needed_di[LD_NEEDED_MAX];  // this field is not necessary.
	uint32_t* hash;
	const char* str;
	Elf32_Sym* sym;
	int symigot, symn;
	uint32_t* got;
	int gotln, gotgn;
	Elf32_Rel* rel_dyn;
	int rel_dyn_num;
	Elf32_Rel* rel_plt;
	int rel_plt_num;
	uint32_t* gotplt;
} DynInfo;

// ===== Standard Functions =====
// elf_hash: standard function from ABI Spec
unsigned long elf_hash(const unsigned char *name);
// ===== Routines =====
// ld_hello: called after bootstrap to check if it works
void ld_hello();
// ld_load_libs: called to load all libs needed. `dyn` is from executable.
void ld_load_libs(const Elf32_Dyn* mdyn, uint32_t ld_base);
// ld_reloc: reloc main & libs, rely on array / linked list in `ld_load_libs`
void ld_reloc();
// ===== Utilities =====
// ld_lib_new: alloc and init DynInfo, get ready for a new lib
DynInfo* ld_lib_new(const char* path);
// ld_lib_load: load a lib. Do NOT rely on DynInfo here!
const Elf32_Dyn* ld_lib_load(const char* path, uint32_t *base_store, uint32_t *max_addr_store);
// ld_lib_parse: parse DYNAMIC segment.
void ld_lib_parse(DynInfo* di, const Elf32_Dyn* dyn, uint32_t base, uint32_t max_addr);
// ld_resolve_got
uint32_t ld_resolve_got(DynInfo* di, Elf32_Sym* sym, uint32_t* gote);
// ld_resolve_plt
uint32_t ld_resolve_plt(DynInfo* di, Elf32_Rel* rel);
// ld_handle_got
uint32_t ld_handle_got(int idx, uint32_t ra_stub);
// ld_handle_plt
uint32_t ld_handle_plt(int idx, uint32_t ra_stub);
// ld_hash_lookup: standard lookup in hash table
uint32_t ld_hash_lookup(DynInfo* di, Elf32_Sym* origin, const char* ostr);
// ld_sym_lookup: find symbol value in all libs (excluding main)
Elf32_Addr ld_sym_lookup(Elf32_Sym* origin, const char* ostr);
// ld_unique_add: if unique, add; if not, return 0.
bool ld_unique_add(const char* path);

uint32_t ld_main(Elf32_auxv_t* auxv) {
	// Step 0: read auvx
	uint32_t entry, base;
	struct proghdr* ph = NULL;
	int phnum;
	for (; auxv -> a_type != ELF_AT_NULL; auxv++) {
		switch (auxv -> a_type) {
		case ELF_AT_ENTRY:
			entry = auxv -> a_un.a_val;
			break;
		case ELF_AT_BASE:
			base = auxv -> a_un.a_val;
			break;
		case ELF_AT_PHDR:
			ph = (struct proghdr*)(auxv -> a_un.a_val);
			break;
		case ELF_AT_PHNUM:
			phnum = (int)(auxv -> a_un.a_val);
			break;
		}
	}

	// Step 1: bootstrap
	extern uint32_t _DYNAMIC[];
	const Elf32_Dyn* dynamic = (const Elf32_Dyn*)(((uint32_t)_DYNAMIC) + base);
	uint32_t* got = NULL;
	int gotln = -1, symn = -1, symigot = -1;
	--dynamic;
	while ((++dynamic)->d_tag != DT_NULL) {
		switch (dynamic->d_tag) {
		case DT_PLTGOT:
			got = (uint32_t*)(dynamic->d_un.d_ptr + base);
			break;
		case DT_MIPS_LOCAL_GOTNO:
			gotln = dynamic->d_un.d_val;
			break;
		case DT_MIPS_SYMTABNO:
			symn = dynamic->d_un.d_val;
			break;
		case DT_MIPS_GOTSYM:
			symigot = dynamic->d_un.d_val;
			break;
		}
	}
	int gotgn = symn - symigot;
	int i;
	for (i = 0; i < gotln + gotgn; i++)
		got[i] += base;
	ld_hello();
	SHOW_VAR(lda_di_num);

	// Step 2: load libs
	dynamic = NULL;
	for (i = 0; i < phnum; i++)
		if (ph[i].p_type == ELF_PT_DYNAMIC) {
			dynamic = (const Elf32_Dyn*)ph[i].p_va;
			break;
		}
	ld_load_libs(dynamic, base);
	cprintf("Load OK!\n");
	// Step 3: reloc
	ld_reloc();
	cprintf("Reloc OK!\n");

	// Step 4: jump to entry, fix sp, a0 ~ a3 (argc, argv)
	cprintf("Jumping to main...\n");
    return entry;
}

void ld_hello() {
    cprintf("Hello from ld.so\n");
}
// LDA means ld allocation. ucore has no `malloc`, so we must use static allocation.
// `lda_di` also serves as the array of DIs, dynamic allocation should use a linked list.
// Because of these, array usage in `ld_load_libs` and alloc in	`ld_lib_new` is highly coupled.
#define LDA_LIBS_MAX 20
DynInfo lda_di[LDA_LIBS_MAX];
int lda_di_num = 0;
void ld_load_libs(const Elf32_Dyn* mdyn, uint32_t ld_base) {
	/*DynInfo* mdi = */ld_lib_new(NULL);  // mdi = main DynInfo
	int q = 0;
	SHOW_VAR(lda_di_num);
	while (q < lda_di_num) {
		DynInfo* di = &lda_di[q++];
		const Elf32_Dyn* dyn = mdyn;
		uint32_t base = 0x0, max_addr = ld_base;
		if (di->path != NULL) {
			dyn = ld_lib_load(di->path, &base, &max_addr);
		}
		ld_lib_parse(di, dyn, base, max_addr);
		int i;
		for (i = 0; i < di->needed_num; i++) {
			const char* path = di->str + di->needed[i];
			if (ld_unique_add(path)) {
				ld_lib_new(path);
				cprintf("NEW lib: %s\n", path);
			}
		}
	}
	cprintf("# of libs loaded: %d\n", lda_di_num - 1);
}
void ld_reloc() {
	extern uint32_t ld_entry_got[], ld_entry_plt[];
	DynInfo* mdi = lda_di;
	// Step A0: non-PIC main, lazy & link map
	mdi->got[0] = (uint32_t)ld_entry_got;
	mdi->got[1] = 0x0;
	mdi->gotplt[0] = (uint32_t)ld_entry_plt;
	mdi->gotplt[1] = 0x0;
	// Step A1: non-PIC main, rel.dyn (data)
	Elf32_Rel *rel, *rel_end;
	rel = mdi->rel_dyn;
	rel_end = mdi->rel_dyn + mdi->rel_dyn_num;
	for (; rel != rel_end; rel++) {
		Elf32_Sym* sym = mdi->sym + ELF32_R_SYM(rel->r_info);
		if (ELF32_R_TYPE(rel->r_info) == ELF32_R_MIPS_COPY) {
			Elf32_Addr addr = ld_sym_lookup(sym, mdi->str);
			// FIXME: why copy 4 bytes (uint32_t)?
			*(uint32_t*)rel->r_offset = *(uint32_t*)addr;
		}
	}
	// Step A2: non-PIC main, rel.plt (func)
	//* Uncomment this part to bind NOW.
	rel = mdi->rel_plt;
	rel_end = mdi->rel_plt + mdi->rel_plt_num;
	for (; rel != rel_end; rel++) {
		ld_resolve_plt(mdi, rel);
	}
	//*/
	// Step B: PIC libs
	DynInfo* di = lda_di + 1;
	for (; di != lda_di + lda_di_num; di++) {
		// Step B0: fill lazy & link map
		uint32_t *gote = di->got, *gotend;
		gote[0] = (uint32_t)ld_entry_got;
		gote[1] = 0x0;
		// Step B1: reloc local GOT
		gote = di->got + 2;
		gotend = di->got + di->gotln;
		for (; gote != gotend; gote++) {
			*gote += di->base;
		}
		// Step B2: reloc global GOT
		cprintf("Lib: %s\n", di->path);
		gote = di->got + di->gotln;
		gotend = gote + di->gotgn;
		Elf32_Sym* sym = di->sym + di->symigot;
		for (; gote != gotend; gote++, sym++) {
			cprintf("[%2d] <%02x> 0x%08x (0x%08x) %s\n",
				sym->st_shndx, ELF32_ST_TYPE(sym->st_info),
				sym->st_value, *gote,
				di->str + sym->st_name);
			// When can we be lazy? See Figure 5-10, MIPS ABI Supplement.
			if (ELF32_ST_TYPE(sym->st_info) == STT_FUNC) {
				if (sym->st_shndx == SHN_UNDEF) {
					if (sym->st_value != 0x0) {
						*gote = sym->st_value + di->base;
						continue;
					}
				} else {
					if (*gote != sym->st_value) {
						*gote += di->base;
						continue;
					}
				}
			}
			ld_resolve_got(di, sym, gote);
		}
		// Step B3: rel.dyn
		rel = di->rel_dyn;
		rel_end = di->rel_dyn + di->rel_dyn_num;
		for (; rel != rel_end; rel++) {
			if (ELF32_R_TYPE(rel->r_info) == ELF32_R_MIPS_NONE)
				continue;
			assert(ELF32_R_TYPE(rel->r_info) == ELF32_R_MIPS_REL32);
			int symi = ELF32_R_SYM(rel->r_info);
			if (symi == 0) {
				*(uint32_t*)(rel->r_offset + di->base) = 0xffffffff;
				continue;
			}
			assert(symi >= di->symigot);
			// Elf32_Sym* sym = di->sym + ELF32_R_SYM(rel->r_info);
			// assert: symbol not lazy
			uint32_t* gote = di->got + (symi - di->symigot + di->gotln);
			*(uint32_t*)(rel->r_offset + di->base) = *gote;
		}
	}
}

DynInfo* ld_lib_new(const char* path) {
	assert(lda_di_num < LDA_LIBS_MAX);
	DynInfo* di = &lda_di[lda_di_num++];
	memset(di, 0, sizeof(DynInfo));
	di -> path = path;
	return di;
}
void ld_lib_parse(DynInfo* di, const Elf32_Dyn* dyn, uint32_t base, uint32_t max_addr) {
	di -> base = base;
	di -> max_addr = max_addr;
	di -> needed_num = 0;
	for (; dyn -> d_tag != DT_NULL; dyn++) {
		switch (dyn -> d_tag) {
		case DT_NEEDED:
			di -> needed[di -> needed_num++] = dyn -> d_un.d_val;
			break;
		case DT_HASH:
			di -> hash = (uint32_t*)(dyn -> d_un.d_ptr + base);
			break;
		case DT_STRTAB:
			di -> str = (const char*)(dyn -> d_un.d_ptr + base);
			break;
		case DT_SYMTAB:
			di -> sym = (Elf32_Sym*)(dyn -> d_un.d_ptr + base);
			break;
		case DT_PLTGOT:
			di -> got = (uint32_t*)(dyn -> d_un.d_ptr + base);
			break;
		case DT_REL:
			di -> rel_dyn = (Elf32_Rel*)(dyn -> d_un.d_ptr + base);
			break;
		case DT_RELSZ:
			di -> rel_dyn_num = dyn -> d_un.d_val / sizeof(Elf32_Rel);
			break;
		case DT_MIPS_LOCAL_GOTNO:
			di -> gotln = dyn -> d_un.d_val;
			break;
		case DT_MIPS_SYMTABNO:
			di -> symn = dyn -> d_un.d_val;
			break;
		case DT_MIPS_GOTSYM:
			di -> symigot = dyn -> d_un.d_val;
			break;
		case DT_JMPREL:
			di -> rel_plt = (Elf32_Rel*)(dyn -> d_un.d_ptr + base);
			break;
		case DT_PLTRELSZ:
			di -> rel_plt_num = dyn -> d_un.d_val / sizeof(Elf32_Rel);
			break;
		case DT_MIPS_PLTGOT:
			di -> gotplt = (uint32_t*)(dyn -> d_un.d_ptr + base);
			break;
		}
	}
	// gotln, symn and symigot are mandatory fields, according to ABI
	di -> gotgn = di->symn - di->symigot;
}


const Elf32_Dyn* ld_lib_load(const char* path, uint32_t *base_store, uint32_t *max_addr_store) {
	int fd = sys_open(path, O_RDONLY);
	assert(fd >= 0);

	struct elfhdr eh;
	sys_read(fd, &eh, sizeof(eh));
	assert(eh.e_magic == ELF_MAGIC);
	uint32_t base = sys_mmap2_query();
	cprintf("BASE = 0x%08x\n", base);
	if (base_store != NULL)
		*base_store = base;
	if (max_addr_store != NULL)
		*max_addr_store = base;
	const Elf32_Dyn* dyn = NULL;
	int i;
	struct proghdr ph;
	for (i = 0; i < eh.e_phnum; i++) {
		sys_seek(fd, eh.e_phoff + eh.e_phentsize * i, LSEEK_SET);
		sys_read(fd, &ph, sizeof(ph));
		if (ph.p_type == ELF_PT_LOAD) {
			int prot = ((ph.p_flags & ELF_PF_R) ? PROT_READ : 0) |
				((ph.p_flags & ELF_PF_W) ? PROT_WRITE : 0) |
				((ph.p_flags & ELF_PF_X) ? PROT_EXEC: 0);
			uint32_t addr = (ph.p_va + base) & 0xfffff000, len = ph.p_filesz + (ph.p_va & 0xfff),
				offset = ph.p_offset & 0xfffff000;
			void* dst = sys_mmap2((void*)addr, len, prot, fd, offset);
			cprintf("OFF 0x%08x -> MEM 0x%08x, LEN 0x%08x\n", offset, dst, len);
			if ((ph.p_flags & ELF_PF_W) && (len & 0xfff))
				memset((void*)(addr+len), 0, 0x1000-(len & 0xfff));
			uint32_t end = (addr+len+0xfff) & 0xfffff000,
				memend = (base + ph.p_va + ph.p_memsz + 0xfff) & 0xfffff000;
			if (memend > end) {
				cprintf("==========BSS==========\n");
				void* dst = sys_mmap2((void*)end, memend - end, prot, NO_FD, 0x0);
				cprintf("ZERO -> MEM 0x%08x, LEN 0x%08x\n",
					dst, memend - end);
			}
			if (max_addr_store != NULL && memend > *max_addr_store)
				*max_addr_store = memend;
		} else if (ph.p_type == ELF_PT_DYNAMIC) {
			dyn = (const Elf32_Dyn*)(ph.p_va + base);
		}
	}

	sys_close(fd);
	return dyn;
}

uint32_t ld_resolve_got(DynInfo* di, Elf32_Sym* sym, uint32_t* gote) {
	DynInfo* mdi = lda_di;
	assert(di != mdi);
	cprintf("[%s] Resolve: %s\n", di->path, di->str + sym->st_name);
	uint32_t idx = ld_hash_lookup(mdi, sym, di->str);
	if (idx != STN_UNDEF) {
		*gote = mdi->sym[idx].st_value;
	} else if (sym->st_shndx != SHN_UNDEF) {
		*gote += di->base;
	} else {
		Elf32_Addr addr = ld_sym_lookup(sym, di->str);
		*gote = addr;
	}
	return *gote;
}

uint32_t ld_resolve_plt(DynInfo* di, Elf32_Rel* rel) {
	DynInfo* mdi = lda_di;
	assert(di == mdi);
	Elf32_Sym* sym = mdi->sym + ELF32_R_SYM(rel->r_info);
	cprintf("mResolve: %s\n", mdi->str + sym->st_name);
	if (ELF32_R_TYPE(rel->r_info) == ELF32_R_MIPS_JUMP_SLOT) {
		Elf32_Addr addr = ld_sym_lookup(sym, mdi->str);
		*(Elf32_Addr*)rel->r_offset = addr;
		return addr;
	} else {
		cprintf("PLT REL TYPE 0x%08x not supported!\n", ELF32_R_TYPE(rel->r_info));
		assert(0);
		return 0xffffffff;
	}
}
bool ld_check_delay();
uint32_t ld_handle_got(int idx, uint32_t ra_stub) {
	if (!ld_check_delay()) {
		cprintf("Lazy got on ThinPad II.\n");
		uint32_t inst = *(uint32_t*)ra_stub;
		assert((inst & 0xffff0000) == 0x24180000);
		idx = inst & 0xffff;
	} else {
		cprintf("Lazy got in Qemu.\n");
	}
	DynInfo *di = lda_di, *di_end = lda_di + lda_di_num;
	for (; di != di_end; di++) {
		if (di->base <= ra_stub && ra_stub <= di->max_addr)  // <= ra-4 < max_addr
			break;
	}
	assert (di != di_end);
	return ld_resolve_got(di, di->sym + idx, di->got + (idx - di->symigot + di->gotln));
}

uint32_t ld_handle_plt(int idx, uint32_t ra_stub) {
	if (!ld_check_delay()) {
		cprintf("Lazy plt on ThinPad II.\n");
		cprintf("I don't know the idx in delay slot of `jr`.\n");
		assert(0);
	} else {
		cprintf("Lazy plt in Qemu.\n");
	}
	DynInfo* mdi = lda_di;
	return ld_resolve_plt(mdi, mdi->rel_plt + idx);
}

uint32_t ld_hash_lookup(DynInfo* di, Elf32_Sym* origin, const char* ostr) {
	uint32_t bn = di->hash[0], cn = di->hash[1],
		*b = di->hash + 2, *c = b + bn;
	assert(cn == di->symn);
	const char* oname = ostr + origin->st_name;
	uint32_t idx = b[elf_hash((const unsigned char*)oname) % bn];
	for (; idx != STN_UNDEF; idx = c[idx]) {
		Elf32_Sym* sym = di->sym + idx;
		const char* name = di->str + sym->st_name;
		if ((sym->st_shndx != SHN_UNDEF ||
			(ELF32_ST_TYPE(sym->st_info) == STT_FUNC && (sym->st_other & STO_MIPS_PLT))
			) && strcmp(oname, name) == 0)
			return idx;
	}
	return STN_UNDEF;
}
Elf32_Addr ld_sym_lookup(Elf32_Sym* origin, const char* ostr) {
	DynInfo* di = lda_di + 1;  // skip main
	for (; di != lda_di + lda_di_num; di++) {
		uint32_t idx = ld_hash_lookup(di, origin, ostr);
		if (idx != STN_UNDEF)
			return di->sym[idx].st_value + di->base;
	}
	cprintf("Symbol `%s` not found!", ostr + origin->st_name);
	assert(0);
	return 0xffffffff;
}
bool ld_unique_add(const char* path) {
	int i;
	for (i = 1; i < lda_di_num; i++) {
		if (strcmp(path, lda_di[i].path) == 0)
			return 0;
	}
	return 1;
}

unsigned long elf_hash(const unsigned char *name) {
	unsigned long h = 0, g;
	while (*name) {
		h = (h << 4) + *name++;
		if ((g = h & 0xf0000000))
			h ^= g >> 24;
		h &= ~g;
	}
	return h;
}

