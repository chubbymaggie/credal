#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include "elf_core.h"
#include "elf_binary.h"
#include "access_memory.h"

//get the value by the name of register
int value_of_register(char * reg, Elf32_Addr* value, struct elf_prstatus thread){
	int match = 0;
	if (strcmp(reg, "eax") == 0){
		*value = thread.pr_reg[EAX];
		match = 1;
		goto out;
	}
    if (strcmp(reg, "ebx") == 0){
        *value = thread.pr_reg[EBX];
        match = 1;
        goto out;
    }
    if (strcmp(reg, "ecx") == 0){
        *value = thread.pr_reg[ECX];
        match = 1;
        goto out;
    }
    if (strcmp(reg, "edx") == 0){
        *value = thread.pr_reg[EDX];
        match = 1;
        goto out;
    }
    if (strcmp(reg, "esi") == 0){
        *value = thread.pr_reg[ESI];
        match = 1;
        goto out;
    }
    if (strcmp(reg, "edi") == 0){
        *value = thread.pr_reg[EDI];
        match = 1;
        goto out;
    }
    if (strcmp(reg, "ebp") == 0){
        *value = thread.pr_reg[EBP];
        match = 1;
        goto out;
    }
    if (strcmp(reg, "esp") == 0){
        *value = thread.pr_reg[UESP];
        match = 1;
        goto out;
    }
out: 
	return match; 
}

// determine the segment this address exists.
// if -1, then this address does not exist in any segment. Illegal access!
int address_segment(elf_core_info* core_info, Elf32_Addr address){
	int segment = -1;
	int i = 0;
	for (i=0; i< core_info->phdr_num; i++){
		if (core_info->phdr[i].p_type & PT_LOAD){
			Elf32_Addr mstart = (Elf32_Addr) core_info->phdr[i].p_vaddr;
			Elf32_Word msize = (Elf32_Word) core_info->phdr[i].p_memsz;
			if (address >= mstart && address < mstart + msize){
				segment = i;
				break;
			}
		}
	}
	return segment; 
}

// get the offset of memory in file based on its address
off_t get_offset_from_address(elf_core_info* core_info, Elf32_Addr address){
	off_t offset; 
	int segment; 
	if ((segment = address_segment(core_info, address))<0){
		return ME_NMAP;
	}
	if (! (Elf32_Word)core_info->phdr[segment].p_memsz)
		return ME_NMEM;

	// this area is not really mapped into the address space
	if (core_info->phdr[segment].p_memsz != core_info->phdr[segment].p_filesz){
#ifdef DEBUG
		fprintf(stdout, "DEBUG: The memsize is %u, and the file size is %u\n",
                (unsigned int)core_info->phdr[segment].p_memsz,
                (unsigned int)core_info->phdr[segment].p_filesz);
#endif 
		return ME_NDUMP;
	}
	offset = (Elf32_Off)core_info->phdr[segment].p_offset + address - (Elf32_Addr)core_info->phdr[segment].p_vaddr; 
	return offset; 
}

// get data from core dump
int get_data_from_core(long int start, long int size, char *note_data){
    int fd;
    if ((fd = open(core_path, O_RDONLY , 0)) < 0){
#ifdef DEBUG
    	fprintf(stderr, "Core file open error %s\n", strerror(errno));
#endif
    	return -1;
    }
    if (lseek(fd, start, SEEK_SET) < 0){
    	fprintf(stderr, "Core file lseek error %s\n", strerror(errno));
    	close(fd);
    	return -1;
    }
    if (read(fd, note_data, size) < 0){
    	fprintf(stderr, "Core file open error %s\n", strerror(errno));
    	close(fd);
    	return -1;
    }
    close(fd);
    return 0;
}

// determine if the address is executable
int address_executable(elf_core_info* core_info, Elf32_Addr address){
	int segment; 
	if ((segment = address_segment(core_info, address)) < 0)
		return 0;
	return (core_info->phdr[segment].p_flags & PF_X) ? 1 : 0;
}

// determine if the address is executable
int address_writable(elf_core_info* core_info, Elf32_Addr address){
    int segment;
    if ((segment = address_segment(core_info, address)) < 0)
        return 0;
    return (core_info->phdr[segment].p_flags & PF_W) ? 1 : 0;
}

// get the memory from the file recorded by the NT_FILE information. 
int get_data_from_specified_file(elf_core_info* core_info, elf_binary_info* bin_info,  Elf32_Addr address, char * buf, size_t buf_size){
	int data_obtained = 0;
	int file_num =-1;
	int phdr_num = -1;
	int i, fd;
	char *file_path;
	Elf32_Addr offset;
	Elf32_Addr reduce = 0;
	individual_binary_info* target_file = 0;

	for(i = 0; i<bin_info->bin_lib_num; i++){
		if (bin_info->binary_info_set[i].parsed)
    		if ((address >= bin_info->binary_info_set[i].base_address) && (address < bin_info->binary_info_set[i].end_address)){
			    file_num = i;
			    break;
		    } 
	}
	if (file_num == -1)
		goto out;

	target_file = &bin_info->binary_info_set[file_num];
	file_path = target_file->bin_name; 

	if (target_file->phdr[0].p_vaddr < target_file->base_address)
		reduce = target_file -> base_address;	

	for(i=0; i<target_file->phdr_num; i++){
		if ((address-reduce)>=target_file->phdr[i].p_vaddr && (address-reduce)<(target_file->phdr[i].p_vaddr+target_file->phdr[i].p_memsz)){
			phdr_num = i; 
			break;
		}
	}
	if (phdr_num == -1)
		goto out;

#ifdef DEBUG
	fprintf(stdout, "DEBUG: The file mapped to address %u is %s\n", address, file_path);	
#endif
	offset = (address-reduce) - target_file->phdr[phdr_num].p_vaddr +  target_file->phdr[phdr_num].p_offset;
	if ((fd = open(file_path, O_RDONLY, 0)) < 0){
#ifdef DEBUG
		fprintf(stderr, "Core file open error %s\n", strerror(errno));
#endif
		return -1;
	}
	if (lseek(fd, offset, SEEK_SET) < 0){
		fprintf(stderr, "Core file lseek error %s\n", strerror(errno));
		close(fd);
		return -1;
	}
    if (read(fd, buf, buf_size) < 0){
		fprintf(stderr, "Core file open error %s\n", strerror(errno));
		close(fd);
		return -1;
	}
	close(fd);
	return 0;
out: 
	return data_obtained;
}
