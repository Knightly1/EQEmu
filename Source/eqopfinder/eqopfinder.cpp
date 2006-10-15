/*

EQ Opcode Finder, by Father Nitwit 2005

*/
#include <stdio.h>
#include <time.h>
#include "structmatch.h"
#include "../common/packetfile.h"
#include "../common/opcodemgr.h"
#include "../common/packet_dump.h"
#include "../common/files.h"
#include <vector>
#include "../common/logsys.h"

using namespace std;

#ifdef WIN32
#include "../common/win_getopt.h"
#endif

#define PACKET_BUFFER_SIZE 1024*1024

//second constants to make patch dates easier
#define YEAR_2004 1072935480
#define YEAR_2005 1104492410
#define MONTH 2629743
#define DAY 86400

#define CURRENT_PATCH YEAR_2005 + 2*MONTH
//5/11/05
//#define CURRENT_PATCH YEAR_2005 + 4*MONTH + 10*DAY


void usage() {
	printf("Usage:\neqopfinder (opcodes file) (packet file)\n");
}

int main(int argc, char *argv[]) {
/*
	char opt;
	while((opt=getopt(argc,argv,"dDaoOzZui123tTbsmM"))!=-1) {
		switch (opt) {
		case 'd':
			collects.push_back(tmp = new DoorExtractor(&zone_info));
			extracts.push_back(tmp);
			break;		}
	}
	argc -= optind;
	argv += optind;
*/	
	argv++;
	argc--;	//temp until we need to use getopt
	
	
	if(argc != 2) {
		usage();
		return(1);
	}
	
	
	//load up existing opcodes
	MutableOpcodeManager *opmgr = new RegularOpcodeManager();
	if(!opmgr->LoadOpcodes(argv[0])) {
		printf("Unable to load opcode. We cannot function without opcodes.\n");
		delete opmgr;
		return(1);
	}
	
	//build our matcher object
	StructMatcher matcher(opmgr);
	
	
	//load up our packet file
	PacketFileReader *from;
	from = PacketFileReader::OpenPacketFile(argv[1]);
	if(from == NULL) {
		printf("Error: Unable to open input packet file '%s'\n", argv[1]);
		return(1);
	}
	
	time_t stamp = from->GetStamp();
	
	if(stamp < CURRENT_PATCH) {
		printf("This packet file was created before the current patch level, this isnt gunna work.\n");
		return(1);
	}
	
	//temp space for the current packet:
	bool to_server;
	struct timeval tv;
	uint16 eq_op;
	uint32 packlen;
	unsigned char packet_buf[PACKET_BUFFER_SIZE];
	
	
	//First pass through the data to find the zone info
	packlen = PACKET_BUFFER_SIZE;
	while(from->ReadPacket(eq_op, packlen, packet_buf, to_server, tv)) {
		EmuOpcode emu_op = opmgr->EQToEmu(eq_op);
		SMPacket *p = new SMPacket(eq_op, emu_op, packlen);
		memcpy(p->pBuffer, packet_buf, packlen);
		
		matcher.Process(p);
		
		packlen = PACKET_BUFFER_SIZE;
	}
	matcher.Finish();
	from->CloseFile();
	
	return(0);
}





