#ifndef __COMMON__
#define __COMMON__

#define align_power(addr,power) \
(addr + (1 << power) - 1) & (-(1 << power))

#define FILE_NAME_SIZE 256
#define ADDRESS_SIZE 32
#define INST_LEN 64

extern char *core_path;
extern char *bin_path;
extern char *lib_path;

#define ME_NMAP -1
#define ME_NMEM -2
#define ME_NDUMP -3 

void set_core_path(char *path);
char *get_core_path(void);

void set_bin_path(char *path);
char *get_bin_path(void);

void set_lib_path(char *path);
char *get_lib_path(void);

#endif
