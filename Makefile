OS=$(shell uname -s)
INCLUDE='./include'

CXX=g++
CXXFLAGS=-g -std=c++0x -I$(INCLUDE)
CC=gcc
CFLAGS=-g -I$(INCLUDE)

COMMON_LIB=common_lib.a
MINI_GOOGLE_MASTER=mini_google_master
MINI_GOOGLE_SLAVE=mini_google_worker
UISHELL=uishell

cchighlight=\033[0;31m
ccend=\033[0m

# compiling cpp files
.cpp.obj:
	${CXX} ${CXXFLAGS} -c $(.SOURCE)

# compiling c files
.c.obj:
	${CC} ${CFLAGS} -c $(.SOURCE)

COMMON_LIB_INCLUDES= \
	include/svr_base.h \
	include/svr_thrd.h \
	include/io_event.h \
	include/http_event.h \
	include/rpc_log.h \
	include/rpc_lock.h \
	include/rpc_net.h \
	include/rpc_http.h \
	include/rpc_common.h \
	include/mini_google_common.h \
	include/file_mngr.h

COMMON_LIB_OBJS= \
	src/ezxml.o \
	src/svr_base.o \
	src/svr_thrd.o \
	src/io_event.o \
	src/http_event.o \
	src/accept_event.o \
	src/rpc_net.o \
	src/rpc_http.o \
	src/rpc_common.o \
	src/basic_proto.o \
	src/mini_google_common.o \
	src/file_mngr.o

MINI_GOOGLE_MASTER_OBJS= \
	src/mini_google_master_svr.o \
	src/lookup_table.o \
	src/invert_table.o \
	src/main_master.o

MINI_GOOGLE_SLAVE_OBJS=\
	src/mini_google_slave_svr.o \
	src/task_consumer.o \
	src/main_slave.o

UISHELL_OBJS=\
	src/uishell.o \
	src/main_uishell.o

# compiling all
all: $(COMMON_LIB) $(UISHELL) $(MINI_GOOGLE_SLAVE) $(MINI_GOOGLE_MASTER)
	mkdir -p output/bin
	cp $(UISHELL) output/bin
	cp $(MINI_GOOGLE_SLAVE) output/bin
	cp $(MINI_GOOGLE_MASTER) output/bin
	cp $(UISHELL) output/
	cp $(MINI_GOOGLE_SLAVE) output/
	cp $(MINI_GOOGLE_MASTER) output/
	cp $(UISHELL) output/bin
	@echo -e "$(cchighlight)finish compiling$(ccend)"

# compiling common_lib
$(COMMON_LIB): $(COMMON_LIB_OBJS)
	mkdir -p output/include
	mkdir -p output/lib
	$(foreach file, $(COMMON_LIB_INCLUDES), cp $(file) output/include;)
	ar rcs $(COMMON_LIB) $(COMMON_LIB_OBJS)
	cp $(COMMON_LIB) output/lib
	@echo -e "$(cchighlight)successfully compiling $(COMMON_LIB)$(ccend)"

# compiling master server
$(MINI_GOOGLE_MASTER): $(COMMON_LIB) $(MINI_GOOGLE_MASTER_OBJS)
ifeq ($(OS),Linux)
	$(CXX) $(CXXFLAGS) -o $(MINI_GOOGLE_MASTER) -Xlinker "-(" $(COMMON_LIB) $(MINI_GOOGLE_MASTER_OBJS) -Xlinker "-)" -pthread -lcrypto -lssl 
else
	$(CXX) $(CXXFLAGS) -pthread -lcrypto -o $(MINI_GOOGLE_MASTER) -Xlinker $(COMMON_LIB) $(MINI_GOOGLE_MASTER_OBJS)
endif
	@echo -e "$(cchighlight)successfully compiling $(MINI_GOOGLE_MASTER)$(ccend)"

# compiling slave server
$(MINI_GOOGLE_SLAVE): $(COMMON_LIB) $(MINI_GOOGLE_SLAVE_OBJS)
ifeq ($(OS),Linux)
	$(CXX) $(CXXFLAGS) -o $(MINI_GOOGLE_SLAVE) -Xlinker "-(" $(COMMON_LIB) $(MINI_GOOGLE_SLAVE_OBJS) -Xlinker "-)" -pthread -lcrypto -lssl
else
	$(CXX) $(CXXFLAGS) -pthread -lcrypto -o $(MINI_GOOGLE_SLAVE) -Xlinker $(COMMON_LIB) $(MINI_GOOGLE_SLAVE_OBJS)
endif
	@echo -e "$(cchighlight)successfully compiling $(MINI_GOOGLE_SLAVE)$(ccend)"

$(UISHELL): $(COMMON_LIB) $(UISHELL_OBJS)
ifeq ($(OS),Linux)
	$(CXX) $(CXXFLAGS) -o $(UISHELL) -Xlinker "-(" $(COMMON_LIB) $(UISHELL_OBJS) -Xlinker "-)" -pthread -lcrypto -lssl
else
	$(CXX) $(CXXFLAGS) -pthread -lcrypto -o $(UISHELL) -Xlinker $(COMMON_LIB) $(UISHELL_OBJS)
endif
	@echo -e "$(cchighlight)successfully compiling $(UISHELL)$(ccend)"

.PHONY: clean
clean:
	rm -f src/*.o
	rm -f *.o
	rm -f $(COMMON_LIB)
	rm -f $(MINI_GOOGLE_MASTER)
	rm -f $(MINI_GOOGLE_SLAVE)
	rm -f $(UISHELL)
	rm -rf output
