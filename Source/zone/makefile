APP=zone
SF=../common/EQNetwork.o \
   ../common/timer.o  ../common/packet_dump.o ../common/packet_functions.o \
   ../common/unix.o ../common/packet_dump_file.o ../common/Mutex.o ../common/MiscFunctions.o \
   zone.o entity.o mob.o client.o client_process.o npc.o net.o spawn2.o attack.o hate_list.o \
   ../common/serverinfo.o ../common/moremath.o worldserver.o spells.o spawngroup.o loottables.o \
   faction.o Map.o PlayerCorpse.o petitions.o MobAI.o Object.o ../common/eqtime.o \
   groups.o ../common/classes.o ../common/races.o ../common/TCPConnection.o forage.o \
   ../common/crc32.o ../common/guilds.o ../common/md5.o ../common/dbcore.o \
   ../common/dbasync.o zonedbasync.o parser.o beacon.o\
   ../common/EMuShareMem.o ../common/EQEMuError.o ../common/ptimer.o \
   .obj/debug.o .obj/database.o .obj/Item.o .obj/misc.o \
   doors.o command.o


CC=gcc
LINKER=gcc
MYSQL_FLAGS=`mysql_config --cflags`
MYSQL_LIB=`mysql_config --libs`
DFLAGS=-DEQDEBUG=5 -DNO_PIDLOG -DSHAREMEM -DSPELL_EFFECT_SPAM -DFIELD_ITEMS
WFLAGS=-Wall -Wuninitialized -Wwrite-strings -Wcast-qual  -Wstrict-prototypes -Wno-deprecated  -Wcomment -Wcast-align
COPTS=$(WFLAGS) -O2 -ggdb -pthread -pipe -D_GNU_SOURCE -DINVERSEXY -DFX -DZONE $(DFLAGS) $(MYSQL_FLAGS)
LINKOPTS=-rdynamic -L. -lstdc++ -ldl $(MYSQL_LIB)

all: $(APP)


$(APP): $(SF)
	$(LINKER) $(COPTS) $(OBJS) $^ $(LINKOPTS) -o $@


import_raw_items: import_raw_items.o
	$(LINKER) $(COPTS) $^ $(LINKOPTS) -o $@

clean:
	rm -f $(SF) $(APP) import_raw_items.o import_raw_items

%.o: %.cpp
	$(CC) -c $(COPTS) $< -o $@

.obj/debug.o: ../common/debug.cpp ../common/debug.h
	mkdir -p .obj
	$(CC) $(COPTS) -c ../common/debug.cpp -o .obj/debug.o

.obj/database.o: ../common/database.cpp ../common/database.h
	mkdir -p .obj
	$(CC) $(COPTS) -c ../common/database.cpp -o .obj/database.o

.obj/Item.o: ../common/Item.cpp ../common/Item.h
	mkdir -p .obj
	$(CC) $(COPTS) -c ../common/Item.cpp -o .obj/Item.o

.obj/misc.o: ../common/misc.cpp ../common/misc.h
	mkdir -p .obj
	$(CC) $(COPTS) -c ../common/misc.cpp -o .obj/misc.o
