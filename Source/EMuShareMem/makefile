# Generated automatically from Makefile.in by configure.
APP=libEMuShareMem.so
SF=MMFMutex.o Doors.o Items.o Spells.o NPCFactionLists.o MMF.o Loot.o ../common/timer.o ../common/unix.o ../common/EMuShareMem.o Opcodes.o

CC=gcc
LD=ld

#For FreeBSD uncomment these, comment the matching ones further down.
#MYSQL_CFLAGS=-I'/usr/local/include/mysql'
#MYSQL_LIBS=-L'/usr/local/lib/mysql' -lmysqlclient -lz -lcrypt -lm
#DFLAGS=-DDEBUG=1 -DSHAREMEM -DCATCH_CRASH -DEMUSHAREMEM -DFREEBSD

#For Linux, and defaults that don't need to be overwritten.
MYSQL_CFLAGS=`mysql_config --cflags`
MYSQL_LIBS=`mysql_config --libs`
DFLAGS=-DDEBUG=1 -DSHAREMEM -DCATCH_CRASH -DEMUSHAREMEM -DAPP_OPCODE_SIZE=2

WFLAGS=-Wall -Wuninitialized -Wwrite-strings -Wcast-qual -Wstrict-prototypes -Wno-deprecated -Wcomment -Wcast-align
COPTS=$(WFLAGS) $(DFLAGS) -g -fPIC -O2 -D_GNU_SOURCE -DINVERSEXY -DFX $(MYSQL_CFLAGS)
LINKOPTS=-L. -lstdc++ $(MYSQL_LIBS)

all: $(APP)

	

$(APP): $(SF)
	$(CC) -g -shared -Wl,-soname,$(APP) -o $(APP) $(SF) $(LINKOPTS)

clean:
	rm -f $(SF) $(APP)

%.o: %.cpp
	$(CC) $(COPTS) -c $< -o $@

.obj/debug.o: ../common/debug.h ../common/debug.cpp
	mkdir -p .obj
	$(CC) $(COPTS) -c ../common/debug.cpp -o .obj/debug.o

