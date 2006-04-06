APP=build_packets
SF=../common/MiscFunctions.o ../common/misc.o ../PacketCollector/packetfile.o \
   buildfile.o build_packets.o build_patches/current.o \
   build_patches/121504.o 


CC=gcc
LINKER=gcc
DFLAGS=
WFLAGS=-Wall -Wuninitialized -Wwrite-strings -Wcast-qual  -Wstrict-prototypes -Wcomment -Wcast-align -Wno-deprecated

#linux threads (on FreeBSD)
LINKOPTS=-rdynamic -L. -lm -lz -L/usr/local/lib -llthread -llgcc_r -llstdc++ -llsupc++
COPTS=-I/usr/local/include/pthread/linuxthreads $(WFLAGS) -g -O -pipe -D_GNU_SOURCE -DPACKETCOLLECTOR $(DFLAGS)



all: $(APP) pf2text

$(APP): $(SF)
	$(LINKER) $(COPTS) $(OBJS) $^ $(LINKOPTS) -o $@

pf2text: ../PacketCollector/packetfile.o pf2text.o ../common/packet_dump.o
	$(LINKER) $(COPTS) $^ $(LINKOPTS) -o $@

clean:
	rm -f $(SF) $(APP)

%.o: %.cpp
	$(CC) -c $(COPTS) $< -o $@





