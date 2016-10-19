char *core_path;
char *bin_path; 
char *lib_path; 

void set_core_path(char * path){
    core_path = path;
}

void set_bin_path(char * path){
    bin_path = path;
}

void set_lib_path(char * path){
    lib_path = path;
}

char *get_core_path(void){
    return core_path;
}

char *get_bin_path(void){
    return bin_path;
}

char *get_lib_path(void){
    return lib_path;
}
