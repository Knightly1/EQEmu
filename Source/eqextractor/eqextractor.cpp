/*

EQ Extractor, by Father Nitwit 2005

*/
#include "../common/debug.h"
#include <stdio.h>
#include <time.h>
#include "../common/packetfile.h"
#include "Explorers.h"
#include "../common/opcodemgr.h"
#include "../common/files.h"
#include "../common/buildfile.h"
#include "ExtractDB.h"
#include "patches/versions.h"
#include "ExtractCollector.h"
#include "BuildWriterInterface.h"
#include <vector>

using namespace std;
using namespace EQExtractor;

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
	printf("Usage:\neqextract [options] (packet file...)\n"
		"	-v - Enable debug mode\n"
		"Patch Modes:\n"
		"	-P - Auto-detect patch mode based on file internal time stamp (default)\n"
		"	-p [patch name] - Manually specify the patch name, use 'list' for a list\n"
		"Extractors:\n"
		"   -a - extract AAs\n"
		"   -d - extract doors (fuzzy, recommended)\n"
		"   -D - extract doors (strict)\n"
		"   -o - extract world objects & ground spawns (fuzzy, recommended)\n"
		"   -O - extract world objects & ground spawns (strict)\n"
		"   -z - extract zone points\n"
		"   -Z - extract zone header\n"
		"   -t - extract tribute entries\n"
		"   -T - extract tribute descriptions\n"
		"   -b - extract books\n"
		"   -m - extract mob information\n"
		"   -s - extract skill titles\n"
		"   -M - extract task history\n"
		"   -C - print the short name of the zone.\n"
		"	-j [charid] - Replace the specified char with the one from the log.\n"
		"Build file options:\n"
		"   -B [filename] - create a build file for eqbuilder\n"
		"Explorers:\n"
		"   -1 - explore zone headers\n"
		"   -2 - explore objects\n"
		"   -5 - explore unknown\n"
		"   -9 - explore player profile\n"
		"   -c - Print channel message while exploring\n"
		"   -l - Print spawn list with IDs\n"
		"   -U [id] - Explore position updates\n"
		"\n"
		"By Default, text updates are printed. (needs DB)\n"
		"   -i - generate inserts instead of text data. (no DB)\n"
		"   -u - generate updates instead of text data. (needs DB)\n"
	);
}

/*
	This seems like a messed up way to do this, but we cannot create our
	extractor objects until after we know what packet file we are reading
	in case we need to use the time stamp in the packet file to make our
	extractor factory
*/
enum {	//extractor bit field values
	eAA					= 0x00000001,
	eDoor				= 0x00000002,
	eFuzzyDoor			= 0x00000004,
	eObject				= 0x00000008,
	eFuzzyObject		= 0x00000010,
	eZonePoint			= 0x00000020,
	eZoneHeader			= 0x00000040,
	eTribute			= 0x00000080,
	eTributeText		= 0x00000100,
	eBookText			= 0x00000200,
	eTitle				= 0x00000400,
	eSpawn				= 0x00000800,
	eTaskHistory		= 0x00001000,
	eCatalogZone		= 0x00002000,
	eSpawnList			= 0x00004000,
	eCharacter			= 0x00008000
};

int main(int argc, char *argv[]) {
	vector<ExtractCollector *> collects;
	vector<ExtractBase *> extracts;
	vector<StructExplorer *> explorers;
	vector<ExtractBase *>::iterator cur,end;
	vector<ExtractCollector *>::iterator curc,endc;
	vector<StructExplorer *>::iterator cure,ende;
	
	bool inserts = false;
	bool updates = false;
	
	bool debug_mode = false;
	
	uint32 enabled_extractors = 0;
	string build_file;
	string patch_name = "auto";
	uint32 extract_char_id = 0;
	
	char opt;
	while((opt=getopt(argc,argv,"dDaoOzZui1234569tTbB:smMcClU:Pp:vj:"))!=-1) {
		switch (opt) {
		//Patch Selection:
		case 'P':
			patch_name = "auto";
			break;
		case 'p':
			if(!strcasecmp(optarg, "list")) {
				ExtractorAbstractFactory::ListExtractorFactories();
				return(0);
			}
			patch_name = optarg;
			break;
		
		case 'v':
			debug_mode = true;
			break;
		
		//Extractor section:
		case 'a':
			enabled_extractors |= eAA;
			break;
		case 'd':
			enabled_extractors |= eFuzzyDoor;
			break;
		case 'D':
			enabled_extractors |= eDoor;
			break;
		case 'o':
			enabled_extractors |= eFuzzyObject;
			break;
		case 'O':
			enabled_extractors |= eObject;
			break;
		case 'z':
			enabled_extractors |= eZonePoint;
			break;
		case 'Z':
			enabled_extractors |= eZoneHeader;
			break;
		case 't':
			enabled_extractors |= eTribute;
			break;
		case 'T':
			enabled_extractors |= eTributeText;
			break;
		case 'b':
			enabled_extractors |= eBookText;
			break;
		case 's':
			enabled_extractors |= eTitle;
			break;
		case 'm':
			enabled_extractors |= eSpawn;
			break;
		case 'M':
			enabled_extractors |= eTaskHistory;
			break;
		case 'C':
			enabled_extractors |= eCatalogZone;
			break;
		case 'l':
			enabled_extractors |= eSpawnList;
			break;
		case 'j':
			enabled_extractors |= eCharacter;
			extract_char_id = atoi(optarg);
			break;
		
		//Build File Stuff:
		case 'B':
			build_file = optarg;
			break;
		
		//Explorers:
		case '1':
			explorers.push_back(new ZoneHeaderExplorer());
			break;
		case 'U':
			explorers.push_back(new ClientUpdateExplorer(atoi(optarg)));
			break;
		case '3':
			explorers.push_back(new SpawnExplorer());
			break;
		case '4':
			explorers.push_back(new ObjectExplorer());
			break;
		case '5':
			explorers.push_back(new UnknownExplorer());
			break;
		case '6':
			explorers.push_back(new SpellExplorer());
			break;
		case '9':
			explorers.push_back(new PlayerProfileExplorer());
			break;
		case 'c':
			explorers.push_back(new MessageExplorer());
			break;
		
		//Modes:
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
	
	if(argc < 1) {
		usage();
		return(1);
	}
	
	
/*	OpcodeManager *opmgr = new RegularOpcodeManager();
	if(!opmgr->LoadOpcodes(OPCODES_FILE)) {
		printf("Unable to load opcode. We cannot function without opcodes.\n");
		delete opmgr;
		return(1);
	}
*/
	
	ExtractorDB * db = NULL;
	if(!inserts) {
		db = new ExtractorDB();
	}
	
	//temp space for the current packet:
	bool to_server;
	struct timeval tv;
	uint16 eq_op;
	uint32 packlen;
	unsigned char *packet_buf = new unsigned char[PACKET_BUFFER_SIZE];
	
	while(argc != 0) {
		PacketFileReader *from;
		
		from = PacketFileReader::OpenPacketFile(argv[0]);
		if(from == NULL) {
			fprintf(stderr, "Error: Unable to open input packet file '%s'\n", argv[0]);
			argc--;
			argv++;
			continue;
		}
		
		time_t stamp = from->GetStamp();
		
		//Get our factory object
		ExtractorAbstractFactory *eaf = NULL;
		if(patch_name.length() == 0 || patch_name == "auto") {
			//auto patch.
			eaf = ExtractorAbstractFactory::GetExtractorFactoryByDate(stamp, 
				(enabled_extractors & eCatalogZone)?argv[0]:NULL);
		} else {
			eaf = ExtractorAbstractFactory::GetExtractorFactoryByName(patch_name.c_str(), 
				(enabled_extractors & eCatalogZone)?argv[0]:NULL);
		}
		if(eaf == NULL) {
			fprintf(stderr, "Error: Unable to find an extractor factory for the packet file. We cannot continue without a factory.\n");
			argc--;
			argv++;
			continue;
		}
		
		//get our zone info extractor
		ExtractCollector *zone_info = eaf->getZoneInfoExtractor();
		
		//now that we actually have a factory, we can build our extractors
		ExtractCollector *tmp;
		if(enabled_extractors & eAA) {
			if(debug_mode) fprintf(stderr, "# Enabling AA extractor\n");
			extracts.push_back(tmp = eaf->newAAExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eDoor) {
			if(debug_mode) fprintf(stderr, "# Enabling door extractor\n");
			extracts.push_back(tmp = eaf->newDoorExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eFuzzyDoor) {
			if(debug_mode) fprintf(stderr, "# Enabling fuzzy door extractor\n");
			extracts.push_back(tmp = eaf->newFuzzyDoorExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eFuzzyObject) {
			if(debug_mode) fprintf(stderr, "# Enabling fuzzy world object extractor\n");
			extracts.push_back(tmp = eaf->newFuzzyObjectExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eObject) {
			if(debug_mode) fprintf(stderr, "# Enabling world object extractor\n");
			extracts.push_back(tmp = eaf->newObjectExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eZonePoint) {
			if(debug_mode) fprintf(stderr, "# Enabling zone point extractor\n");
			extracts.push_back(tmp = eaf->newZonePointExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eZoneHeader) {
			if(debug_mode) fprintf(stderr, "# Enabling zone header extractor\n");
			extracts.push_back(tmp = eaf->newZoneHeaderExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eTribute) {
			if(debug_mode) fprintf(stderr, "# Enabling tribute table extractor\n");
			extracts.push_back(tmp = eaf->newTributeExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eTributeText) {
			if(debug_mode) fprintf(stderr, "# Enabling tribute text extractor\n");
			extracts.push_back(tmp = eaf->newTributeTextExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eBookText) {
			if(debug_mode) fprintf(stderr, "# Enabling book extractor\n");
			extracts.push_back(tmp = eaf->newBookTextExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eTitle) {
			if(debug_mode) fprintf(stderr, "# Enabling skill title extractor\n");
			extracts.push_back(tmp = eaf->newTitleExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eSpawn) {
			if(debug_mode) fprintf(stderr, "# Enabling spawn extractor\n");
			extracts.push_back(tmp = eaf->newSpawnExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eTaskHistory) {
			if(debug_mode) fprintf(stderr, "# Enabling task history extractor\n");
			extracts.push_back(tmp = eaf->newTaskHistoryExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eSpawnList) {
			if(debug_mode) fprintf(stderr, "# Enabling spawn list extractor\n");
			extracts.push_back(tmp = eaf->newSpawnListExtractor());
			collects.push_back(tmp);
		}
		if(enabled_extractors & eCharacter) {
			if(debug_mode) fprintf(stderr, "# Enabling character extractor for char id %d\n", extract_char_id);
			extracts.push_back(tmp = eaf->newCharacterExtractor(extract_char_id));
			collects.push_back(tmp);
		}
		
		if(build_file.length() > 0) {
			PatchBuildFileWriterInterface *build_file_writer;
			build_file_writer = eaf->newBuildFileWriter();
			if(!build_file_writer->OpenFile(build_file.c_str())) {
				fprintf(stderr, "# Error: Unable to open build file '%s'\n", build_file.c_str());
			} else {
				//force this cast just to make everything simpler
				extracts.push_back(build_file_writer);
				if(debug_mode) fprintf(stderr, "# Enabling build file extractor\n");
			}
		}
		
		
		//First pass through the data to find the zone info
		while((packlen = PACKET_BUFFER_SIZE)
		  && from->ReadPacket(eq_op, packlen, packet_buf, to_server, tv)) {
			EmuOpcode emu_op = eaf->opcodeManager->EQToEmu(eq_op);
	//		printf("0x%04x -> %s\n", eq_op, opmgr->
			if(emu_op == OP_Unknown) {
				continue;
			}
			zone_info->GivePacket(emu_op, packet_buf, packlen, to_server);
		}
		
		from->ResetFile();
		//Second pass to process the zone
		end = extracts.end();
		ende = explorers.end();
		//read in each EQ packet, ship it off to the build manager.
		while((packlen = PACKET_BUFFER_SIZE)
		  && from->ReadPacket(eq_op, packlen, packet_buf, to_server, tv)) {
			EmuOpcode emu_op = eaf->opcodeManager->EQToEmu(eq_op);
			if(emu_op == OP_Unknown) {
				//nobody gets unknowns.
				continue;
			}
			
			if(debug_mode) fprintf(stderr, "# Processing opcode %s of length %d\n", eaf->opcodeManager->EmuToName(emu_op), packlen);
			
			//do something with this packet.
			//give it to all the extractors to see if they want it
			cur = extracts.begin();
			for(; cur != end; cur++) {
				(*cur)->GivePacket(emu_op, packet_buf, packlen, to_server);
			}
			//give it to all the explorers to see if they want it too
			cure = explorers.begin();
			for(; cure != ende; cure++) {
				(*cure)->GivePacket(emu_op, packet_buf, packlen, to_server);
			}
		}
		
		from->CloseFile();
		
		//now we have all our data, do something with it.
		curc = collects.begin();
		endc = collects.end();
		for(; curc != endc; curc++) {
			ExtractCollector *e = *curc;
			if(inserts)
				e->GenerateInserts(stdout, false);
			else if(updates)
				e->GenerateUpdates(stdout, db);
			else
				e->GenerateTexts(stdout, db);
		}
		
		
		delete from;
	//	delete opmgr;
		cur = extracts.begin();
		end = extracts.end();
		for(; cur != end; cur++) {
			delete *cur;
		}
		cure = explorers.begin();
		ende = explorers.end();
		for(; cure != ende; cure++) {
			delete *cure;
		}
		
		argc--;
		argv++;
	}
	
	delete[] packet_buf;
	
	return(0);
}





