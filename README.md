# credal: A debugging tool to locate vulnerability area from core dump caused by memory corruption.  
# We are now releasing the code step by step. At the current stage, we have only opened the code for 32-bit Linux core dump parsing. Other components will come soon. 

## Prerequirement

    $ sudo apt-get install libelf1 libelf-dev

library to read and write ELF files

    $ sudo apt-get install libdisasm0 libdisasm-dev

disassembler library for x86 code

## Usage

    $ ./main coredump binary_path library_path

### Compile

    $ make

### Test

    $ make test

### Clean

    $ make clean

## Data Structure

### `elf_core_info` data structure:

```
typedef struct nt_file_info_struct{
    Elf32_Addr start, end, pos;
    char name[FILE_NAME_SIZE]; 
}nt_file_info;

typedef struct nt_file_struct{
    size_t nt_file_num;
    nt_file_info *file_info;
}core_nt_file_info;

typedef struct thread_info_struct{
    size_t thread_num;
    struct elf_prstatus *threads_status;
}core_thread_info;

typedef struct process_info_struct{
    int exist; 
    struct elf_prpsinfo process_info; 
}core_process_info; 

typedef struct note_info_struct{
    core_nt_file_info core_file;
    core_process_info core_process;
    core_thread_info  core_thread; 
}core_note_info;

typedef struct core_info_struct{
    size_t phdr_num;
    GElf_Phdr *phdr; 
    core_note_info *note_info;
}elf_core_info; 
```

### `elf_binary_info` data structure:

```
typedef struct individual_binary_info_struct{
    char bin_name[FILE_NAME_SIZE];
    int parsed;
    size_t phdr_num;
    GElf_Phdr *phdr;
    Elf32_Addr base_address;
    Elf32_Addr end_address; 
}individual_binary_info; 

typedef struct elf_binary_info_struct{
    size_t bin_lib_num;
    individual_binary_info *binary_info_set;
}elf_binary_info;
```

## Corresponding APIs

- common.c
  - void set_core_path(char *path);
  - char *get_core_path(void);
  - void set_bin_path(char *path);
  - char *get_bin_path(void);
  - void set_lib_path(char *path);
  - char *get_lib_path(void);
- disassemble.c
  - int disasm_one_inst(char *buf, size_t buf_size, int pos, x86_insn_t *inst);
- access_memory.c
  - int value_of_register(char *reg, Elf32_Addr *value, struct elf_prstatus thread );
  - int address_segment(elf_core_info *core_info, Elf32_Addr address);
  - off_t get_offset_from_address(elf_core_info *core_info, Elf32_Addr address);
  - int get_data_from_core(long int start, long int size, char *note_data);
  - int address_executable(elf_core_info *core_info, Elf32_Addr address);
  - int address_writable(elf_core_info *core_info, Elf32_Addr address);
  - int get_data_from_specified_file(elf_core_info *core_info, elf_binary_info *bin_info,  Elf32_Addr address, char *buf, size_t buf_size);
- elf_binary.c
  - elf_binary_info *parse_binary(elf_core_info *core_info);
  - int destroy_bin_info(elf_binary_info *bin_info);
- elf_core.c
  - int destroy_core_info(elf_core_info*);
  - elf_core_info* parse_core(char*);
  - int process_segment(Elf*, elf_core_info*);
  - int process_note_segment(Elf*, elf_core_info*);
