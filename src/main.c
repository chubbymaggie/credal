#include <stdio.h>
#include <stdlib.h>
#include "disassemble.h"
#include "access_memory.h"
#include "elf_binary.h"

int main(int argc, char *argv[]){
    if (argc != 4){
        fprintf(stderr, "Help: %s coredump binary_path library_path\n", argv[0]);
        fprintf(stderr, "      Make sure that all the library files are in the directory defined by library_path\n");
        exit(0);
    }
    set_core_path(argv[1]);
    set_bin_path(argv[2]);
    set_lib_path(argv[3]);

    elf_core_info *core_info = parse_core(argv[1]); 
    if(!core_info)
        fprintf(stderr,"The core file is not parsed correctly");
    
    elf_binary_info *binary_info = parse_binary(core_info); 
    if(!binary_info)
        fprintf(stderr,"The binary file is not parsed correctly");
    
    destroy_core_info(core_info);
    destroy_bin_info(binary_info);
}
