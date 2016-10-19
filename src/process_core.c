#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <err.h>
#include <fcntl.h>
#include <sysexits.h>
#include <stdint.h>
#include <errno.h>
#include "elf_core.h"
#include "access_memory.h"

#ifdef DEBUG
// translate Elf_kind to meaningful output 
void print_elf_type(Elf_Kind ek){
    switch (ek){
        case ELF_K_AR:
            fprintf(stdout, "DEBUG: Archive\n");
            break;
        case ELF_K_ELF:
            fprintf(stdout, "DEBUG: ELF Object\n");
            break;
        case ELF_K_NONE:
            fprintf(stdout, "DEBUG: Data\n");
            break;
        default:
            fprintf(stderr, "DEBUG: unrecognized\n");
    }
}
#endif

// destroy core_note_info structure inside core_info
int destroy_note_info(core_note_info* note_info){
    if (note_info->core_file.file_info){
    	free(note_info->core_file.file_info);
    	note_info->core_file.file_info = NULL;		
    }
    if (note_info->core_thread.threads_status){
        free(note_info->core_thread.threads_status);
        note_info->core_thread.threads_status = NULL;
    }	
    return 0;
}

// destroy elf_core_info structure
int destroy_core_info(elf_core_info * core_info){
    if (core_info && core_info->note_info){
    	destroy_note_info(core_info->note_info);
    }
    if (core_info && core_info->phdr){
    	free(core_info->phdr);
    	core_info->phdr = NULL;
    }
    if (core_info){
    	free(core_info);	
    	core_info = NULL;
    }
    return 0;
}

// process all the note entries
int process_note_info(elf_core_info * core_info, char* note_data, unsigned int size){
    size_t thread_num = 0;
    size_t nt_file_num = 0;

    prstatus_t n_prstatus;
    char *note_start = note_data; 
    char *note_end = note_data + size; 	
    Elf32_Nhdr n_entry;
    int reg_num;

    // check if nt_file exists and count the number of threads in this process
    while (note_data < note_end){
    	memcpy(&n_entry, note_data, sizeof(Elf32_Nhdr));
        note_data += sizeof(Elf32_Nhdr);
        note_data += align_power(n_entry.n_namesz, 2);

    	if(n_entry.n_type == NT_PRSTATUS)
    	    thread_num++;

    	if(n_entry.n_type == NT_FILE)
    	    memcpy(&nt_file_num, note_data, sizeof(unsigned long));
    	
    	note_data += align_power(n_entry.n_descsz, 2);
    }

    // prepare the core_info memory space
    if ((core_info->note_info = (core_note_info*)malloc(sizeof(core_note_info))) == NULL){
    	return -1;
    }

    /* prepare contents that are filled into the elf_core_info structure */

    //prepare the process information
    core_info->note_info->core_process.exist = 0;	

    //prepare the nt file entries
    core_info->note_info->core_file.nt_file_num = 0;

    if (!nt_file_num) 
        core_info->note_info->core_file.file_info = NULL;
    else 
        if ((core_info->note_info->core_file.file_info = 
            (nt_file_info*)malloc(nt_file_num * sizeof(nt_file_info))) != NULL){
    	    core_info->note_info->core_file.nt_file_num = nt_file_num;
        }
    	
    // prepare the threads entries
    core_info->note_info->core_thread.thread_num = 0;
    if (!thread_num)
    	core_info->note_info->core_thread.threads_status = NULL; 
    else
        if ((core_info->note_info->core_thread.threads_status = 
    		(prstatus_t *)malloc( thread_num* sizeof(struct elf_prstatus))) != NULL)	
            core_info->note_info->core_thread.thread_num = thread_num; 

    note_data = note_start; 

    unsigned int thread_index = 0; 

    while (note_data < note_end){
        memcpy(&n_entry, note_data, sizeof(Elf32_Nhdr));
        note_data += sizeof(Elf32_Nhdr);
        note_data += align_power(n_entry.n_namesz, 2);

    	if (n_entry.n_type == NT_PRPSINFO){
    	    core_info->note_info->core_process.exist = 1;
    	    memcpy(&core_info->note_info->core_process.process_info, note_data, sizeof(struct elf_prpsinfo));
    	}

        if (n_entry.n_type == NT_PRSTATUS && core_info->note_info->core_thread.thread_num > 0){
            memcpy(&core_info->note_info->core_thread.threads_status[thread_index], note_data, sizeof(struct elf_prstatus));
#ifdef DEBUG	
    	    int reg_num = 0;
    	    fprintf(stdout, "DEBUG: Info of No.%d thread\n", thread_index + 1);
    	    fprintf(stdout, "DEBUG: The number of pending signal is 0x%x\n", 
                    core_info->note_info->core_thread.threads_status[thread_index].pr_info.si_signo);
    	    for (reg_num = 0; reg_num < ELF_NGREG; reg_num++){
                fprintf(stdout, "DEBUG: Register value - 0x%lx\n", 
                        (unsigned long)core_info->note_info->core_thread.threads_status[thread_index].pr_reg[reg_num]);
            }
#endif
    	    thread_index++; 
        }

        if (n_entry.n_type == NT_FILE && core_info->note_info->core_file.nt_file_num >0 ){
            unsigned int i;
            unsigned int index = 0;
            unsigned int fn=0, page_size = 0;
            unsigned int start, end, pos;

            memcpy(&fn, note_data + index, sizeof(unsigned int));
    	    index += sizeof(unsigned int);
    	    memcpy(&page_size, note_data + index, sizeof(unsigned int));
            index += sizeof(unsigned int);

            for (i=0; i<fn; i++){
                memcpy(&start, note_data + index, sizeof(unsigned int));
                index += 4; // 32bit
                memcpy(&end, note_data + index, sizeof(unsigned int));
                index += 4; // 32bit
                memcpy(&pos, note_data + index, sizeof(unsigned int));
                index += 4; // 32bit
    			
    		    core_info->note_info->core_file.file_info[i].start = start;
    		    core_info->note_info->core_file.file_info[i].end = end;
    		    core_info->note_info->core_file.file_info[i].pos = pos;
            }
            for (i=0; i<fn; i++){
    	        strncpy(core_info->note_info->core_file.file_info[i].name, note_data + index, FILE_NAME_SIZE);
                index += strlen(note_data + index) + 1;
            }
#ifdef DEBUG
    	    for (i=0; i<fn; i++)
                fprintf(stdout, "DEBUG: One mapped file name is %s, start from 0x%x, end at 0x%x, position is 0x%x\n",
                        core_info->note_info->core_file.file_info[i].name,
                        core_info->note_info->core_file.file_info[i].start,
                        core_info->note_info->core_file.file_info[i].end,
                        core_info->note_info->core_file.file_info[i].pos);
#endif		
        }
        note_data += align_power(n_entry.n_descsz, 2);
    }
    return 0;
}

// process note segment in program header table
int process_note_segment(Elf* elf, elf_core_info* core_info){
    unsigned int i; 
    unsigned long start, size; 
    size_t phdr_num = 0; 
    GElf_Phdr phdr; 

    if (elf_getphdrnum(elf, &phdr_num) != 0){
         fprintf(stderr, "Cannot get the number of program header %s\n", elf_errmsg(-1));
         return -1;
    }

    for (i=0; i<phdr_num; i++){
        if (gelf_getphdr(elf, i, &phdr) != &phdr){
    		fprintf(stderr, "Cannot get program header %s\n", elf_errmsg(-1)); 
    	    return -1;
        }

        if (phdr.p_type == PT_NOTE){
    	    start = phdr.p_offset;
            size = phdr.p_filesz;
    		char *note_data = (char*)malloc(size);
    		if (!note_data){
#ifdef DEBUG
    		    fprintf(stderr, "Error when allocating new memory %s\n", strerror(errno));
#endif
    			return -1;
    		}
    		if (get_data_from_core(start, size, note_data) < -1){
#ifdef DEBUG
      		    fprintf(stderr, "Error when reading contents from the core file\n");
#endif
    			free(note_data);
    			return -1; 
    		}
    		process_note_info(core_info, note_data, size);
    		free(note_data);
    		break;

        }
    }
    return 0; 
} 

// get all the segments from core dump
int process_segment(Elf* elf, elf_core_info* core_info){
    size_t phdr_num = 0;
    GElf_Phdr phdr;
    unsigned int i;
    
    // get the number of program headers
    if (elf_getphdrnum(elf , &phdr_num) != 0){
    	fprintf(stderr, "Cannot get the number of program heder %s\n", elf_errmsg(-1)); 
    	return -1;
    }
#ifdef DEBUG
    fprintf(stdout, "DEBUG: The number of segment in the core file is %d\n", phdr_num);
#endif
    core_info->phdr_num = phdr_num;

    // store the headers into newly allocated memory space
    core_info->phdr = NULL;	
    if ((core_info->phdr = (GElf_Phdr *)malloc(phdr_num * sizeof(GElf_Phdr))) == NULL){
        fprintf(stderr, "Cannot allocate memory for program header\n");
    	return -1;
    }
    memset(core_info->phdr, 0, phdr_num * sizeof(GElf_Phdr));	
    for (i=0; i< phdr_num; i++){
        if (gelf_getphdr(elf, i, &phdr) != &phdr){
#ifdef DEBUG
    		fprintf(stderr, "Cannot get program header %s\n", elf_errmsg(-1));			
#endif
    		return -1;
        }
    	memcpy(&core_info->phdr[i], &phdr, sizeof(GElf_Phdr));
    }
    return 0;
}

// parse coredump 
elf_core_info* parse_core(char * core_path){
    int fd;
    Elf *elf;
    elf_core_info* core_info = NULL;

    if (elf_version(EV_CURRENT) == EV_NONE){
    	fprintf(stderr, "Not Compitable version of ELF core file\n");
    	return NULL;
    }

    if ((fd = open(core_path, O_RDONLY , 0)) < 0){
        fprintf(stderr, "Error When Open ELF core file: %s\n", strerror(errno));
    	return NULL;
    }
    
    if ((elf = elf_begin(fd, ELF_C_READ, NULL)) == NULL){
        fprintf(stderr, "Error When Initilize the ELF object\n");	
        close(fd);
        return NULL;
    } 
#ifdef DEBUG
    print_elf_type(elf_kind(elf));
#endif
    core_info = (elf_core_info*)malloc(sizeof(elf_core_info));

    if (core_info == NULL){
    	fprintf(stderr, "Error When Memory Allocation\n");
    	elf_end(elf);
        close(fd);
        return NULL;
    }	

    memset(core_info, 0, sizeof(elf_core_info));
    core_info->phdr = NULL;
    core_info->note_info = NULL;

#ifdef LOG_STATE
    fprintf(stdout, "STATE: Parsing Core File: %s\n", core_path);
#endif	

    if (process_segment(elf, core_info) < 0){
#ifdef DEBUG
        fprintf(stderr, "the segments are not correctly parsed\n");
#endif
    	elf_end(elf);
        close(fd);
    	destroy_core_info(core_info);	
    	return NULL;
    }
    
    process_note_segment(elf, core_info);

    elf_end(elf); 
    close(fd);
    return core_info;
}
