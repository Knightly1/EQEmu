/*

EQ Extractor, by Father Nitwit 2005

*/
#include <stdio.h>
#include <time.h>
#include "packetfile.h"
#include "Extractors.h"
#include "Explorers.h"
#include "../common/opcodemgr.h"
#include "../common/files.h"
#include "ExtractDB.h"
#include <vector>

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

//3/1/05.. not exact date
#define CURRENT_PATCH YEAR_2005 + 2*MONTH


void usage() {
	printf("Usage:\neqextract [options] (packet file)\n");
	printf("   -d - extract doors (fuzzy, recommended)\n");
	printf("   -D - extract doors (strict)\n");
	printf("   -a - extract AAs\n");
	printf("   -o - extract world objects & ground spawns (fuzzy, recommended)\n");
	printf("   -O - extract world objects & ground spawns (strict)\n");
	printf("   -z - extract zone points\n");
	printf("   -Z - extract zone header\n");
	printf("   -t - extract tribute entries\n");
	printf("   -T - extract tribute descriptions\n");
	printf("   -b - extract books\n");
	printf("   -m - extract mob information\n");
	printf("   -s - extract skill titles\n");
	printf("   -M - extract task history\n");
	printf("Explorers:\n");
	printf("   -1 - explore zone headers\n");
	printf("   -2 - explore objects\n");
	printf("\nBy Default, text updates are printed. (needs DB)\n");
	printf("   -i - generate inserts instead of text data. (no DB)\n");
	printf("   -u - generate updates instead of text data. (needs DB)\n");
}

int main(int argc, char *argv[]) {
	vector<ExtractCollector *> collects;
	vector<ExtractBase *> extracts;
	vector<StructExplorer *> explorers;
	vector<ExtractBase *>::iterator cur,end;
	vector<ExtractCollector *>::iterator curc,endc;
	vector<StructExplorer *>::iterator cure,ende;
	
	ZoneInfoExtractor zone_info;
	
	bool inserts = false;
	bool updates = false;
	
	ExtractCollector *tmp;
	char opt;
	while((opt=getopt(argc,argv,"dDaoOzZui123tTbsmM"))!=-1) {
		switch (opt) {
		case 'd':
			collects.push_back(tmp = new DoorExtractor(&zone_info));
			extracts.push_back(tmp);
			break;
		case 'D':
			collects.push_back(tmp = new FuzzyDoorExtractor(&zone_info));
			extracts.push_back(tmp);
			break;
		case 'a':
			collects.push_back(tmp = new AAExtractor());
			extracts.push_back(tmp);
			break;
		case 'o':
			collects.push_back(tmp = new FuzzyObjectExtractor());
			extracts.push_back(tmp);
			break;
		case 'O':
			collects.push_back(tmp = new ObjectExtractor());
			extracts.push_back(tmp);
			break;
		case 'z':
			collects.push_back(tmp = new ZonePointExtractor(&zone_info));
			extracts.push_back(tmp);
			break;
		case 'Z':
			collects.push_back(tmp = new ZoneHeaderExtractor());
			extracts.push_back(tmp);
			break;
		case 't':
			collects.push_back(tmp = new TributeExtractor());
			extracts.push_back(tmp);
			break;
		case 'T':
			collects.push_back(tmp = new TributeTextExtractor());
			extracts.push_back(tmp);
			break;
		case 'b':
			collects.push_back(tmp = new BookTextExtractor());
			extracts.push_back(tmp);
			break;
		case 's':
			collects.push_back(tmp = new TitleExtractor());
			extracts.push_back(tmp);
			break;
		case 'm':
			collects.push_back(tmp = new SpawnExtractor(&zone_info));
			extracts.push_back(tmp);
			break;
		case 'M':
			collects.push_back(tmp = new TaskHistoryExtractor());
			extracts.push_back(tmp);
			break;
		
		case '1':
			explorers.push_back(new ZoneHeaderExplorer());
			break;
		case '2':
			explorers.push_back(new ObjectExplorer());
			break;
		case '3':
			explorers.push_back(new SpawnExplorer());
			break;
		case 'u':
			inserts = false;
			updates = true;
			break;
		case 'i':
			inserts = true;
			updates = false;
			break;
		default:
			printf("Unrecognized argument: '-%c'\n", opt);
			usage();
			return(1);
		}
	}
	argc -= optind;
	argv += optind;
	
	if(argc != 1) {
		usage();
		return(1);
	}
	
	
	OpcodeManager *opmgr = new RegularOpcodeManager();
	if(!opmgr->LoadOpcodes(OPCODES_FILE)) {
		printf("Unable to load opcode. We cannot function without opcodes.\n");
		delete opmgr;
		return(1);
	}
	
	ExtractorDB * db = NULL;
	if(!inserts) {
		db = new ExtractorDB();
	}
	
	
	PacketFileReader *from;
	
	from = PacketFileReader::OpenPacketFile(argv[0]);
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
	while((packlen = PACKET_BUFFER_SIZE)
	  && from->ReadPacket(eq_op, packlen, packet_buf, to_server, tv)) {
		EmuOpcode emu_op = opmgr->EQToEmu(eq_op);
		if(emu_op == OP_Unknown) {
			continue;
		}
		zone_info.GivePacket(emu_op, packet_buf, packlen);
	}
	
	from->ResetFile();
	//Second pass to process the zone
	end = extracts.end();
	ende = explorers.end();
	//read in each EQ packet, ship it off to the build manager.
	while((packlen = PACKET_BUFFER_SIZE)
	  && from->ReadPacket(eq_op, packlen, packet_buf, to_server, tv)) {
		EmuOpcode emu_op = opmgr->EQToEmu(eq_op);
		if(emu_op == OP_Unknown) {
			//nobody gets unknowns.
			continue;
		}
		
		//do something with this packet.
		//give it to all the extractors to see if they want it
		cur = extracts.begin();
		for(; cur != end; cur++) {
			(*cur)->GivePacket(emu_op, packet_buf, packlen);
		}
		//give it to all the explorers to see if they want it too
		cure = explorers.begin();
		for(; cure != ende; cure++) {
			(*cure)->GivePacket(emu_op, packet_buf, packlen);
		}
	}
	
	from->CloseFile();
	
	//now we have all our data, do something with it.
	curc = collects.begin();
	endc = collects.end();
	for(; curc != endc; curc++) {
		if(inserts)
			(*curc)->GenerateInserts(stdout, false);
		else if(updates)
			(*curc)->GenerateUpdates(stdout, db);
		else
			(*curc)->GenerateTexts(stdout, db);
	}
	
	
	delete from;
	delete opmgr;
	cur = extracts.begin();
	for(; cur != end; cur++) {
		delete *cur;
	}
	cure = explorers.begin();
	for(; cure != ende; cure++) {
		delete *cure;
	}
	
	return(0);
}





