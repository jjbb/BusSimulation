
#####################################################################
## file        : test makefile for build current dir .c            ##
## author      : jernymy                                           ##
## date-time   : 05/06/2010                                        ##
#####################################################################

#arm-none-linux-gnueabi-gcc
#CC      = arm-none-linux-gnueabi-gcc
CC      = gcc
CPP     = g++
RM      = rm -rf

## debug flag
DBG_ENABLE   = 1

## source file path
SRC_PATH   := ./source

## target exec file name
TARGET     := mfiapp
LIBTARGET  := mfi_Lib_test.so

## get all source files
## $(wildcard dir/*.c)可获得dir下的所有.c文件
SRCS         += $(wildcard $(SRC_PATH)/*.c)       

## all .o based on all .c
OBJS        := $(SRCS:.c=.o)

## need libs, add at here
LIBS := pthread
LIBS += rt

## used headers  file path
INCLUDE_PATH := ./head

## used include librarys file path
LIBRARY_PATH := /lib

## debug for debug info, when use gdb to debug
ifeq (1, ${DBG_ENABLE}) 
	CFLAGS += -D_DEBUG -O0 -g -DDEBUG=1
endif

## get all include path
CFLAGS  += $(foreach dir, $(INCLUDE_PATH), -I$(dir))

## get all library path
LDFLAGS += $(foreach lib, $(LIBRARY_PATH), -L$(lib))

## get all librarys
LDFLAGS += $(foreach lib, $(LIBS), -l$(lib))


all: clean build

build:
	$(CC) -c $(CFLAGS) $(SRCS)
	mv ./*.o $(SRC_PATH)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)
	$(RM) $(OBJS)

clean:
	$(RM) $(OBJS) $(TARGET)

sharelib:
	$(CC) -c $(CFLAGS) $(SRCS)
	mv ./*.o $(SRC_PATH)
	$(CC) -shared -fPCI $(CFLAGS) -o $(LIBTARGET) $(OBJS) $(LDFLAGS)
	$(RM) $(OBJS)
	
usesharelib:
	$(CC) ./$(LIBTARGET) $(CFLAGS) -o $(TARGET) mfi_main.c
