# Makefile for a RISA Device Driver
#
BD?=zcu102
ifeq ($(BD),zcu102)
CC = g++
CXX = g++
AR = ar
#CC = aarch64-linux-gnu-g++
#AR = aarch64-linux-gnu-ar

GLIBDIR1 = /usr/include/glib-2.0/
GLIBDIR2 = /usr/lib/aarch64-linux-gnu/glib-2.0/include/

BOARD = zcu102
CONNECTAL_DIR = connectal_zcu102
else
#CC = arm-linux-gnueabihf-g++
#AR = arm-linux-gnueabihf-ar
#CC=g++
BOARD = zc706_ubuntu
CONNECTAL_DIR = connectal_zc706
endif

ROOTDIR = ../
INCLUDE = $(ROOTDIR)/include
COMMON = $(ROOTDIR)/common
DM_COMMON = $(ROOTDIR)/devices/common
DM_NOHOST = $(ROOTDIR)/devices/nohost
BOARDDIR = $(BOARD)/jni
LIBDIR = $(PWD)/lib

CFLAGS += \
	-g \
	-Wall -Wsign-compare -Wwrite-strings \
	-D_LARGEFILE64_SOURCE \
	-D_GNU_SOURCE  \
	-D CONFIG_ENABLE_MSG \
	-D CONFIG_ENABLE_DEBUG \
	-D USER_MODE \
	-D USE_PMU \
	-D USE_KTIMER \
	-D USE_NEW_RMW \
	-D NOHOST \
	-D LIBLSM \

CXXFLAGS += \
	$(CFLAGS) \
	-std=c++11 \
	#-D USE_ACP \ # only valid for zc706. mkTop8k_ACP.exe should be used.
	#-D ZYNQ=1 \ # Already defined in ConnectalProjectConfig.h:20 

INCLUDES := \
	-I$(LIBDIR) \
	-I$(INCLUDE) \
	-I$(COMMON)/utils \
	-I$(COMMON)/3rd \
	-I$(DM_COMMON) \
	-I$(LIBDIR)/$(CONNECTAL_DIR) \
	-I$(LIBDIR)/$(CONNECTAL_DIR)/cpp \
	-I$(LIBDIR)/$(BOARDDIR) \
	-I$(LIBDIR)/$(CONNECTAL_DIR)/drivers/zynqportal \
	-I$(LIBDIR)/$(CONNECTAL_DIR)/drivers/portalmem \
	-I$(DM_NOHOST)/ \
	-I$(GLIBDIR1) \
	-I$(GLIBDIR2) \

LIBS := \
	-lm -lpthread -lrt -lssl -lcrypto -lglib-2.0

SRCS := \
	$(LIBDIR)/$(BOARDDIR)/FlashRequest.c \
	$(LIBDIR)/$(BOARDDIR)/FlashIndication.c \
	$(LIBDIR)/$(BOARDDIR)/GeneratedCppCallbacks.cpp \
	$(LIBDIR)/$(BOARDDIR)/MMURequest.c \
	$(LIBDIR)/$(BOARDDIR)/MMUIndication.c \
	$(LIBDIR)/$(BOARDDIR)/MemServerRequest.c \
	$(LIBDIR)/$(BOARDDIR)/MemServerIndication.c \
	$(LIBDIR)/$(CONNECTAL_DIR)/cpp/portal.c \
	$(LIBDIR)/$(CONNECTAL_DIR)/cpp/portalPrintf.c \
	$(LIBDIR)/$(CONNECTAL_DIR)/cpp/transportHardware.c \
	$(LIBDIR)/$(CONNECTAL_DIR)/cpp/poller.cpp \
	$(LIBDIR)/$(CONNECTAL_DIR)/cpp/DmaBuffer.cpp \
	$(LIBDIR)/$(CONNECTAL_DIR)/cpp/dmaManager.c \
	$(LIBDIR)/$(CONNECTAL_DIR)/cpp/platformMemory.cpp \
	$(LIBDIR)/$(CONNECTAL_DIR)/cpp/timer.c \
	$(COMMON)/utils/umemory.c \
	$(COMMON)/utils/uthread.c \
	$(DM_COMMON)/dev_main.c \
	$(DM_COMMON)/dev_params.c \
	$(DM_NOHOST)/dm_nohost.cpp \
	$(DM_NOHOST)/sw_poller.cpp \

ifeq ($(priority), "true")
 SRCS+=$(LIBDIR)/libmemio_priority.cpp\

else

  ifeq ($(non_test), "true")
   SRCS+=$(LIBDIR)/libmemio.cpp\

  else
   SRCS+=$(LIBDIR)/libmemio.cpp\

  endif

endif



OBJS := \
	$(SRCS:.c=.o) $(SRCS:.cpp=.o)


all: nohost.exe

nohost.exe: $(SRCS) libmemio.a *.c
	$(CC) $(INCLUDES) $(CFLAGS) -o $@ *.c libmemio.a $(LIBS) 

libmemio.a: $(OBJS)
	$(AR) r $(@) $(OBJS)

.c.o:
	$(CC) $(INCLUDES) $(CFLAGS) -c $< -o $@

.cpp.o:
	$(CXX) $(INCLUDES) $(CXXFLAGS) -c $< -o $@

clean:
	@$(RM) *.o core *~ libmemio.a nohost.exe
	@$(RM) $(DM_COMMON)/*.o
	@$(RM) $(DM_NOHOST)/*.o
	@$(RM) $(LIBDIR)/$(BOARDDIR)/*.o
	@$(RM) $(LIBDIR)/$(CONNECTAL_DIR)/cpp/*.o
	@$(RM) $(COMMON)/utils/*.o
