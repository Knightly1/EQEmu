APP=world
SF=../common/EQNetwork.o \
   ../common/timer.o ../common/packet_dump.o ../common/races.o ../common/unix.o \
   ../common/Mutex.o ../common/MiscFunctions.o ../common/serverinfo.o ../common/packet_functions.o \
   ../common/classes.o ../common/TCPConnection.o ../common/eqtime.o ../common/crc32.o \
   client.o net.o console.o zoneserver.o LoginServer.o ../common/md5.o ../common/dbcore.o \
   ../common/dbasync.o ../common/EMuShareMem.o ../common/EQEMuError.o ../common/misc.o \
   .obj/debug.o .obj/database.o .obj/Item.o

CC=gcc
LINKER=gcc
DFLAGS=-DDEBUG=5 -DSHAREMEM -DCATCH_CRASH -DNO_PIDLOG -DFIELD_ITEMS -DIRC
WFLAGS=-Wall -Wuninitialized -Wwrite-strings -Wcast-qual  -Wstrict-prototypes -Wcomment -Wcast-align -Wno-deprecated
COPTS=$(WFLAGS) -g -march=i686 -O -pthread -pipe -DFX -D_GNU_SOURCE -DINVERSEXY -DWORLD $(DFLAGS) -I/usr/include/mysql -I/usr/mysql/include
LINKOPTS=-rdynamic -L. -lstdc++ -lm -lz -L/usr/lib/mysql -L/usr/mysql/lib -lmysqlclient -ldl
all: $(APP)

$(APP): $(SF)
	$(LINKER) $(COPTS) $(OBJS) $^ $(LINKOPTS) -o $@

clean:
	rm -f $(SF) $(APP)

%.o: %.cpp
	$(CC) -c $(COPTS) $< -o $@
	

.obj/debug.o: ../common/debug.cpp ../common/debug.h
	mkdir -p .obj
	$(CC) $(COPTS) -c ../common/debug.cpp  -o .obj/debug.o


.obj/database.o: ../common/database.cpp ../common/database.h
	mkdir -p .obj
	$(CC) $(COPTS) -c ../common/database.cpp  -o .obj/database.o

.obj/Item.o: ../common/Item.cpp ../common/Item.h
	mkdir -p .obj
	$(CC) $(COPTS) -c ../common/Item.cpp  -o .obj/Item.o


