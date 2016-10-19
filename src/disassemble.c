#include <stdio.h>
#include <libdis.h>

int disasm_one_inst(char *buf, size_t buf_size, int pos, x86_insn_t *inst){
	int disasm = 0;
	size_t size; 
	
	x86_init(opt_none, NULL, NULL);
	size = x86_disasm((unsigned char*)buf, buf_size, 0, pos, inst);
	if(!size){
#ifdef DEBUG
		fprintf(stdout, "DEBUG: Cannot perform disassemble on the buffer\n");
#endif
		disasm = -1;
	}

	x86_cleanup();
	return disasm;
}
