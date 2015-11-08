OS=$(shell uname -s)
INCLUDE='./include'

IDLFILE=conf/idl2.xml

CXX=g++
CXXFLAGS=-g -I$(INCLUDE)
CC=gcc
CFLAGS=-g -I$(INCLUDE)

COMMON_LIB=common_lib.a
TINY_GOOGLE=mini_google

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
	include/rpc_common.h

COMMON_LIB_OBJS= \
	src/ezxml.o \
	src/svr_base.o \
	src/svr_thrd.o \
	src/io_event.o \
	src/http_event.o \
	src/accept_event.o \
	src/rpc_net.o \
	src/rpc_http.o \
	src/rpc_common.o

TINY_GOOGLE_OBJS= \
	src/mini_google_svr.o \
	main_master.o

# compiling all
all: $(COMMON_LIB) $(TINY_GOOGLE)
	@echo -e "$(cchighlight)finish compiling$(ccend)"

# compiling common_lib
$(COMMON_LIB): $(COMMON_LIB_OBJS)
	mkdir -p output/include
	mkdir -p output/lib
	$(foreach file, $(COMMON_LIB_INCLUDES), cp $(file) output/include;)
	ar rcs $(COMMON_LIB) $(COMMON_LIB_OBJS)
	cp $(COMMON_LIB) output/lib
	@echo -e "$(cchighlight)successfully compiling $(COMMON_LIB)$(ccend)"

# compiling directory_server
$(TINY_GOOGLE): $(COMMON_LIB) $(TINY_GOOGLE_OBJS)
	echo $(OS)
ifeq ($(OS),Linux)
	$(CXX) $(CXXFLAGS) -lpthread -o $(TINY_GOOGLE) -Xlinker "-(" $(COMMON_LIB) $(TINY_GOOGLE_OBJS) -Xlinker "-)"
else
	$(CXX) $(CXXFLAGS) -lpthread -o $(TINY_GOOGLE) -Xlinker $(COMMON_LIB) $(TINY_GOOGLE_OBJS)
endif
	@echo -e "$(cchighlight)successfully compiling $(TINY_GOOGLE)$(ccend)"

.PHONY: clean
clean:
	rm -f src/*.o
	rm -f *.o
	rm -f $(COMMON_LIB)
	rm -f $(TINY_GOOGLE)
	rm -rf output
