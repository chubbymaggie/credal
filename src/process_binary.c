#include <libgen.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include "elf_binary.h"

// destroy all the individual binary info structure
int destroy_binary_set(elf_binary_info * bin_info){
	int i; 
	for (i=0; i< bin_info->bin_lib_num; i++){
		if (bin_info->binary_info_set[i].phdr){
			free(bin_info->binary_info_set[i].phdr);
			bin_info->binary_info_set[i].phdr=NULL;
		}
	}
	free(bin_info->binary_info_set);
	bin_info->binary_info_set = NULL;
	return 0;
}

// destroy elf_binary_info structure
int destroy_bin_info(elf_binary_info * bin_info){
	if (bin_info && bin_info->binary_info_set){
		destroy_binary_set(bin_info);
	}
	if (bin_info){
		free(bin_info);
		bin_info = NULL;
	}
	return 0;
}

// process all the binary files mapped by the core file,
// including the binary and the dynamic libraries
int count_bin_file_num(core_nt_file_info nt_file_info){
	int num = 0;
	int i = 0;
	char *prev_name, *next_name;
	prev_name = basename(nt_file_info.file_info[0].name);

	if (strlen(prev_name) > 0) num++;
	for (i=1; i< nt_file_info.nt_file_num; i++){
		next_name = basename(nt_file_info.file_info[i].name);
		if (!strlen(next_name)) continue;
		if (!strcmp(prev_name, next_name)){		
			continue;
        } else {
			num++;
			prev_name = next_name;
		}		
	}
#ifdef DEBUG
	fprintf(stdout, "DEBUG: The number of different binary files is %d\n", num);
#endif
	return num; 
}

// get program header of binary file
int get_header_from_binary(char * path, individual_binary_info* bin_info){
	int fd;
	GElf_Phdr phdr;  	
    Elf* elf;
	size_t phdr_num = 0;
	bin_info->phdr_num = 0;
	bin_info->phdr = NULL;
	int i;
	
    if (elf_version(EV_CURRENT) == EV_NONE){
        fprintf(stderr, "Not Compitable version of ELF core file\n");
        return 0;
	}
                                      
    if ((fd = open(path, O_RDONLY , 0)) < 0){
        fprintf(stderr, "Error When Open ELF core file: %s\n", strerror(errno));
        return 0;
    }
        
    if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL){
        fprintf(stderr, "Error When Initilize the ELF object\n");
        close(fd);
        return 0;
    } 
#ifdef DEBUG
	print_elf_type(elf_kind(elf));                             
#endif          
	 if (elf_getphdrnum(elf, &phdr_num) != 0){
        fprintf(stderr, "Cannot get the number of program heder %s\n", elf_errmsg(-1));
        return 0;
	}
		
	if ((bin_info->phdr = (GElf_Phdr*)malloc(phdr_num * sizeof(GElf_Phdr))) == NULL){
        fprintf(stderr, "Cannot allocate memory for program header\n");
        return -1;
    }

    memset(bin_info->phdr, 0, phdr_num * sizeof(GElf_Phdr));
    for (i=0; i< phdr_num; i++){
        if (gelf_getphdr(elf, i, &phdr) != &phdr){
#ifdef DEBUG
            fprintf(stderr, "Cannot get program header %s\n", elf_errmsg(-1));
#endif
            continue;
        }
        memcpy(&bin_info->phdr[i], &phdr, sizeof(GElf_Phdr));
	}
	bin_info->phdr_num = phdr_num; 
	return 1;
}

// process one binary file
int process_one_bin_file(char* bin_name, individual_binary_info* bin_info){
	int success=1;
	char full_path[FILE_NAME_SIZE]; 
#ifdef LOG_STATE
	fprintf(stdout, "STATE: Processing Binary File - %s \n", bin_name);	
#endif
    // check whether it is a binary or library
	memset(full_path, 0, FILE_NAME_SIZE);
	memcpy(full_path, get_bin_path(), strlen(get_bin_path()));
	strcat(full_path, bin_name);
	if (access(full_path, R_OK) == 0)
		goto process_bin;
	
    memset(full_path, 0, FILE_NAME_SIZE);
    memcpy(full_path, get_lib_path(), strlen(get_lib_path()));
    strcat(full_path, bin_name);
    if (access(full_path, R_OK) == 0)
        goto process_bin;

#ifdef DEBUG
	fprintf(stderr, "DEBUG: %s not found\n", bin_name);
#endif
	success = 0;
	goto out; 

process_bin:

#ifdef DEBUG
	fprintf(stdout, "DEBUG: Processing Binary File - %s\n", full_path);
#endif

	if (!get_header_from_binary(full_path, bin_info)){
#ifdef DEBUG
		fprintf(stderr, "DEBUG: The program headers for binary %s is not correctly parsed\n", full_path);
#endif
		bin_info->phdr = NULL;
		bin_info->phdr_num = 0;
		success = 0;
		goto out; 	
	}

#ifdef DEBUG
    fprintf(stdout, "DEBUG: The program headers for binary %s contain %d entries\n",
            full_path, bin_info->phdr_num);
#endif
	memset(bin_info->bin_name, 0, FILE_NAME_SIZE);
    memcpy(bin_info->bin_name, full_path, strlen(full_path));	

out: 
	return success; 
}

Elf32_Addr file_start_address(core_nt_file_info nt_file_info, char * name, Elf32_Addr min){
    Elf32_Addr min_start = min;
    int i;
    for (i=0; i<nt_file_info.nt_file_num; i++){
        if(!strcmp(name, basename(nt_file_info.file_info[i].name)))
            if(nt_file_info.file_info[i].start < min_start)
                min_start = nt_file_info.file_info[i].start;
    }
    return min_start;
}

Elf32_Addr file_end_address(core_nt_file_info nt_file_info, char *name){
	Elf32_Addr max_end = 0;
	int i;
	for (i=0; i < nt_file_info.nt_file_num; i++){
		if(!strcmp(name, basename(nt_file_info.file_info[i].name)))
			if(nt_file_info.file_info[i].end > max_end)
				max_end = nt_file_info.file_info[i].end;
	}
	return max_end;
}

// process all the binary files
int process_bin_files(core_nt_file_info nt_file_info,individual_binary_info* binary_info_set){
	int i = 0;
	int bin_num = 0;
	char *prev_name, *next_name;
	prev_name = basename(nt_file_info.file_info[0].name);

	if (strlen(prev_name)>0){
		if (process_one_bin_file(prev_name, &binary_info_set[bin_num])){
			binary_info_set[bin_num].parsed = 1;
			binary_info_set[bin_num].base_address = file_start_address(nt_file_info, prev_name, nt_file_info.file_info[i].start);
			binary_info_set[bin_num].end_address = file_end_address(nt_file_info, prev_name); 
#ifdef DEBUG
			fprintf(stdout, "DEBUG: The file name is %s. The base address is 0x%x and the end address is 0x%x\n",
            binary_info_set[bin_num].bin_name, 
            binary_info_set[bin_num].base_address,
            binary_info_set[bin_num].end_address);
#endif
		} else{
			binary_info_set[bin_num].parsed = 0;
			binary_info_set[bin_num].phdr_num = 0;	
			binary_info_set[bin_num].phdr = NULL;	
		}
		bin_num++;
	}
	for (i=1; i< nt_file_info.nt_file_num; i++){
		next_name = basename(nt_file_info.file_info[i].name);
		if (!strlen(next_name)) continue;
		if (!strcmp(prev_name, next_name)){		
			continue;
        } else{
			prev_name = next_name;
			if (process_one_bin_file(prev_name, &binary_info_set[bin_num])){
				binary_info_set[bin_num].parsed = 1;
				binary_info_set[bin_num].base_address = file_start_address(nt_file_info, prev_name, nt_file_info.file_info[i].start);
				binary_info_set[bin_num].end_address = file_end_address(nt_file_info, prev_name);
#ifdef DEBUG
      		    fprintf(stdout, "DEBUG: The file name is %s. The base address is 0x%x and the end address is 0x%x\n",
                binary_info_set[bin_num].bin_name,
                binary_info_set[bin_num].base_address,
                binary_info_set[bin_num].end_address);
#endif
			} else{
				binary_info_set[bin_num].parsed = 0;
				binary_info_set[bin_num].phdr_num = 0;	
				binary_info_set[bin_num].phdr = NULL;	
			}
			bin_num++;
		}
	}
	return 0;
}

// parse binary
elf_binary_info* parse_binary(elf_core_info* core_info){
	int bin_num = 0;
#ifdef LOG_STATE
	fprintf(stdout, "STATE: Process Binary Files Mapped into Address Spapce\n");
#endif

	if (!core_info->note_info->core_file.nt_file_num){
#ifdef DEBUG
		fprintf(stdout, "DEBUG: There is no NT file, could not generate binary information\n");
#endif
		return NULL;
	}

	bin_num = count_bin_file_num(core_info->note_info->core_file);
	elf_binary_info* binary_info = (elf_binary_info*)malloc(sizeof(elf_binary_info));

	if (!binary_info){
#ifdef DEBUG
		fprintf(stderr, "ERROR: memory allocation does not work\n");
#endif
		return NULL;	
	}

	binary_info->bin_lib_num = bin_num; 
	binary_info->binary_info_set = (individual_binary_info*)malloc(bin_num * sizeof(individual_binary_info));

	if (!binary_info->binary_info_set){
#ifdef DEBUG
		fprintf(stderr, "ERROR: memory allocation does not work\n");
#endif		
		free(binary_info);
		return NULL;	
	}

	process_bin_files(core_info->note_info->core_file, binary_info->binary_info_set);
	return binary_info;
}
