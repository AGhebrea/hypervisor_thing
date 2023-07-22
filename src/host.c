#include<sys/mman.h>
#include<sys/stat.h>
#include<fcntl.h>
#include<unistd.h>
#include<string.h>
#include<stdlib.h>
#include<stdint.h>
#include<stdio.h>
#include<errno.h>
#include<elf.h>

/*
* TODO(
*	=> cleanup, refactor, whatever.
* 	=> make this automatically determine
*	the entry point.
*	=> make calls that guest does work
*	as intended. We will need to patch code.
*	but that is fine.
*	=> do the hypervisor thing that actually
*	started this project.
* )
*/

typedef int (*func_ptr_t)(void);

uint64_t lookup_entry_point_offset(uint8_t *);

int main(int argc, char **argv)
{
	int fd, retcode;
	struct stat st;
	uint8_t *guest_mem;
	uint64_t entry_offset;
	void *entry_address;
	uint8_t *magic_address;
	func_ptr_t func;

	if(argc < 2){
		printf("\nusage: <%s> <program_path>", argv[0]);
		exit(0);
	}
	if((fd = open(argv[1], O_RDWR)) < 0){
		printf("\nopen");
		exit(errno);
	}
	if(fstat(fd, &st) < 0){
		printf("\nstat");
		exit(errno);
	}
	guest_mem = mmap(NULL, st.st_size, PROT_EXEC|PROT_READ, MAP_PRIVATE, fd, 0);
	if(guest_mem == MAP_FAILED){
		printf("\nmmap");
		exit(errno);
	}
	if(mprotect(guest_mem, st.st_size, PROT_EXEC|PROT_READ)){
		printf("\nmprotect");
		exit(errno);
	}
	close(fd);
	printf("\n[HOST] guest mapped successfully.");

	entry_offset = lookup_entry_point_offset(guest_mem);
	entry_address = (uint8_t *)((uint64_t )entry_offset + (uint64_t)guest_mem);

	printf("\nprocess pid:		[%d]", getpid());
	printf("\nguest_mem:		[0x%lx]", guest_mem);
	printf("\nentry_offset:		[0x%lx]", entry_offset);
	printf("\nentry_address:	\t[0x%lx]", entry_address);
	printf("\n\n");

	func = (func_ptr_t) entry_address;

	retcode = (*func)();

	printf("\nRETCODE:[%d]", retcode);

	return 0;
}

Elf64_Addr lookup_entry_point_offset(uint8_t *mem)
{
	int i, j;
	char *strtab;

	Elf64_Ehdr *ehdr;
	Elf64_Phdr *phdr;
	Elf64_Shdr *shdr;
	Elf64_Sym *symtab;

	uint64_t main_sym, _start_sym, text_offset;
	char *shdr_name_str;

	text_offset = -1;

	ehdr = (Elf64_Ehdr *)mem;
	phdr = (Elf64_Phdr *)(mem + ehdr->e_phoff);
	shdr = (Elf64_Shdr *)(mem + ehdr->e_shoff);

	shdr_name_str = (char *)&mem[shdr[ehdr->e_shstrndx].sh_offset];
	
	// see man elf.5
	for(i = 0; i < ehdr->e_shnum; ++i){
		if(shdr[i].sh_type == SHT_SYMTAB){
			strtab = (char *)&mem[shdr[shdr[i].sh_link].sh_offset];
			symtab = (Elf64_Sym *)&mem[shdr[i].sh_offset];
			for(j = 0; j < shdr[i].sh_size/sizeof(Elf64_Sym); ++j){
				if(strcmp(&strtab[symtab->st_name], "main") == 0){
					main_sym = (uint64_t)(symtab->st_value);
					printf("\nfound main at:[0x%llx]", main_sym);
				}else if(strcmp(&strtab[symtab->st_name], "_start") == 0){
					_start_sym = (uint64_t)(symtab->st_value);
					printf("\nfound _start at:[0x%llx]", _start_sym);
				}
				symtab++;
			}
		}else{
			// printf("\nsection:[%s]", &shdr_name_str[shdr[i].sh_name]);
			if(!strcmp(".text", &shdr_name_str[shdr[i].sh_name])) {
				text_offset = shdr[i].sh_offset;
				printf("\nfound .text offset at:[0x%llx]", text_offset);
			}
		}
	}

	if(ehdr->e_shstrndx == SHN_UNDEF){
		printf("\nGood luck. TODO: implement section string table undefined");
		exit(1);
	}

	if(text_offset == -1){
		printf("\nNo .text section found. exiting.");
		exit(1);
	}

	// doing it in this way is kind of a hack. If I knew better i would do
	// it in the way the dynamic linker (or loader or interpretor or whatever) 
	// does this. maybe some other day
	return (text_offset + main_sym - _start_sym);
}

