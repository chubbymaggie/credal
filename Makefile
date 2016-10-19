INC_DIR = include
ELF_SRC_DIR = src
TESTSUITES_DIR = testsuites

CC = gcc
CXX = g++
OPT = -g
INC = -I$(INC_DIR)

#DDEBUG = -DLOG_RESULT
#DDEBUG = -DLOG_STATE
DDEBUG = -DDEBUG -DLOG_STATE
CFLAGS = ${OPT} ${INC} ${DDEBUG}
LDFLAGS = -lelf -ldisasm

.PHONY: all reverse test clean
all: main

main: $(ELF_SRC_DIR)/main.c $(ELF_SRC_DIR)/process_core.o $(ELF_SRC_DIR)/access_memory.o $(ELF_SRC_DIR)/common.o $(ELF_SRC_DIR)/process_binary.o $(ELF_SRC_DIR)/disassemble.o
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS) 

$(ELF_SRC_DIR)/process_core.o: $(ELF_SRC_DIR)/process_core.c
	${CC} ${CFLAGS} -c $< -o $@ ${LDFLAGS} 

$(ELF_SRC_DIR)/access_memory.o: $(ELF_SRC_DIR)/access_memory.c
	${CC} ${CFLAGS} -c $< -o $@ ${LDFLAGS} 

$(ELF_SRC_DIR)/common.o: $(ELF_SRC_DIR)/common.c
	${CC} ${CFLAGS} -c $< -o $@ ${LDFLAGS} 

$(ELF_SRC_DIR)/process_binary.o: $(ELF_SRC_DIR)/process_binary.c
	${CC} ${CFLAGS} -c $< -o $@ ${LDFLAGS} 

$(ELF_SRC_DIR)/disassemble.o: $(ELF_SRC_DIR)/disassemble.c
	${CC} ${CFLAGS} -c $< -o $@ ${LDFLAGS} 

test:
	./main $(TESTSUITES_DIR)/simple/core $(TESTSUITES_DIR)/simple/ $(TESTSUITES_DIR)/simple > $(TESTSUITES_DIR)/simple/result
	./main $(TESTSUITES_DIR)/password/core $(TESTSUITES_DIR)/password/ $(TESTSUITES_DIR)/password/ > $(TESTSUITES_DIR)/password/result

clean:
	rm -f $(ELF_SRC_DIR)/*.o
	rm -f main
	> $(TESTSUITES_DIR)/password/result
	> $(TESTSUITES_DIR)/simple/result
