
APP=zone
SF=../common/EQNetwork.o \
   ../common/timer.o  ../common/packet_dump.o ../common/packet_functions.o \
   ../common/unix.o ../common/packet_dump_file.o ../common/Mutex.o ../common/MiscFunctions.o \
   zone.o entity.o mob.o client.o client_process.o npc.o net.o spawn2.o attack.o hate_list.o \
   ../common/serverinfo.o ../common/moremath.o worldserver.o spells.o spawngroup.o loottables.o \
   faction.o Map.o PlayerCorpse.o petitions.o MobAI.o Object.o ../common/eqtime.o \
   groups.o ../common/classes.o ../common/races.o ../common/TCPConnection.o forage.o \
   ../common/crc32.o ../common/md5.o ../common/dbcore.o \
   ../common/dbasync.o zonedbasync.o parser.o embperl.o embparser.o \
   ../common/EMuShareMem.o ../common/EQEMuError.o client_packet.o \
   .obj/debug.o .obj/database.o .obj/Item.o .obj/misc.o zone_profile.o \
   doors.o command.o beacon.o embxs.o ../common/ptimer.o tribute.o \
   bonuses.o trading.o spdat.o  spell_effects.o aggro.o guilds.o \
   inventory.o client_mods.o tradeskills.o waypoints.o pets.o \
   effects.o AA.o trap.o perl_client.o perl_entity.o perl_mob.o perl_npc.o \
   perl_PlayerCorpse.o perl_groups.o questmgr.o client_logs.o perlparser.o \
   ../common/rdtsc.o ../common/extprofile.o


CC=gcc
LINKER=gcc
MYSQL_FLAGS=-I/usr/include/mysql
MYSQL_LIB=-L/usr/include/mysql -lmysqlclient -lz -lcrypt -lnsl -lm -lc -lnss_files -lnss_dns -lresolv -lc -lnss_files -lnss_dns -lresolv
DFLAGS=-DEQDEBUG=5 -DCATCH_CRASH -DNO_PIDLOG -DSHAREMEM -DSPELL_EFFECT_SPAM -DFIELD_ITEMS
#try commenting out the following three lines to disable embedded perl
PERL_FLAGS=perl -MExtUtils::Embed -e ccopts
PERL_LIB=perl -MExtUtils::Embed -e ldopts
DFLAGS+=-DEMBPERL -DEMBPERL_PLUGIN -DHAS_UNION_SEMUN
WFLAGS=-Wall -Wuninitialized -Wwrite-strings -Wcast-qual  -Wstrict-prototypes -Wno-deprecated  -Wcomment -Wcast-align
COPTS=$(WFLAGS) -O -g -pg -march=i686 -pthread -pipe -D_GNU_SOURCE -DINVERSEXY -DFX -DZONE $(DFLAGS) $(MYSQL_FLAGS)
LINKOPTS=-rdynamic -L. -lstdc++ -ldl $(MYSQL_LIB)

all: $(APP)


$(APP): $(SF)
	$(LINKER) $(COPTS) $(OBJS) $^ $(LINKOPTS) `$(PERL_LIB)` -o $@


import_raw_items: import_raw_items.o
	$(LINKER) $(COPTS) $^ $(LINKOPTS) `$(PERL_LIB)` -o $@

clean:
	rm -f $(SF) $(APP) import_raw_items.o import_raw_items

%.o: %.cpp
	$(CC) -c $(COPTS) `$(PERL_FLAGS)` $< -o $@

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
