#include <stdio.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include "../common/types.h"
#include "../common/packetfile.h"
#include "../common/opcodemgr.h"
#include "../common/files.h"
#include "../common/eqstrmgr.h"
#include "../common/eq_packet_structs.h"

#ifdef WIN32
#include "../common/win_getopt.h"
#endif

using namespace std;

void DumpPacketHex(const uchar* buf, int32 size, int32 cols=16, int32 skip=0);

#define PACKET_BUFFER_SIZE 1024*1024

int main(int argc, char *argv[]) {
	bool quiet = true;
	bool hunt = false;
	
	const char *opfile = OPCODES_FILE;
	EQStringManager *str_mgr = NULL;
	
	char opt;
	while((opt=getopt(argc,argv,"vuo:"))!=-1) {
		switch (opt) {
		case 'v':
			quiet = false;
			break;
		case 'o':
			opfile = optarg;
			break;
		case 'u':
			hunt = true;
			break;
		}
	}
	argc -= optind;
	argv += optind;
	
	if(argc != 1) {
		printf("Usage: pf2text [-o file] [-vu] (input packet file)\n");
		printf("  -v - print ClientUpdate and other annoying packets too\n");
		printf("  -u - unknown hunt mode. Print chat text and unknowns\n");
		printf("  -o [file] - Load opcodes from 'file' instead of %s", OPCODES_FILE);
		return(1);
	}
	
	if(hunt) {
		quiet = false;	//to avoid double lookups
		str_mgr = new EQStringManager();
		if(!str_mgr->LoadStringFile("eqstr_us.txt")) {
			fprintf(stderr, "Unable to load eqstr_us.txt, strings will not resolve.\n");
		}
	}
	
	OpcodeManager *opmgr = new RegularOpcodeManager();
	if(!opmgr->LoadOpcodes(opfile)) {
		printf("Unable to load all opcodes. Names may not be resolved.\n");
	}
	
	PacketFileReader *from;
	
	from = PacketFileReader::OpenPacketFile(argv[0]);
	if(from == NULL) {
		printf("Error: Unable to open input packet file '%s'\n", argv[0]);
		return(1);
	}
	
	uint16 eq_op;
	uint32 packlen;
	struct timeval tim;
	bool to_server;
	unsigned char* packet_buf = new unsigned char[PACKET_BUFFER_SIZE];
	
	//read in each EQ packet, ship it off to the build manager.
	while((packlen = PACKET_BUFFER_SIZE)
	  && from->ReadPacket(eq_op, packlen, packet_buf, to_server, tim)) {
	  	if(hunt) {
			EmuOpcode e = opmgr->EQToEmu(eq_op);
			switch(e) {
			case OP_FormattedMessage: {
				FormattedMessage_Struct *s = (FormattedMessage_Struct *) packet_buf;
//				printf("Format %d: ", s->string_id);
				string eqstr;
				str_mgr->Lookup(s->string_id, eqstr);
				printf("Format '%s': ", eqstr.c_str());
				int len;
				const char *msg = s->message;
				len = strlen(msg);
				while(len > 0) {
					printf("'%s' ", msg);
					msg += len + 1;
					len = strlen(msg);
				}
				printf("\n\n");
				continue;
			}
			case OP_SimpleMessage: {
				SimpleMessage_Struct *s = (SimpleMessage_Struct *) packet_buf;
				string eqstr;
				str_mgr->Lookup(s->string_id, eqstr);
				printf("Simple '%s'\n\n", eqstr.c_str());
				continue;
			}
			case OP_ChannelMessage: {
				ChannelMessage_Struct *s = (ChannelMessage_Struct *) packet_buf;
				printf("Channel: %s->%s: \"%s\"\n\n", s->sender, s->targetname, s->message);
				continue;
			}
			case OP_Unknown:
				break;
			default:
				continue;
			}
	  	}
		if(quiet) {
			EmuOpcode e = opmgr->EQToEmu(eq_op);
			if(e == OP_AnnoyingZoneUnknown || e == OP_ClientUpdate || e == OP_FloatListThing)
				continue;
		}
		printf("%s: [ Opcode: %s (0x%.4x) Size: %d ]\n",
			to_server?"Client->Server":"Server->Client",
			opmgr->EQToName(eq_op),
			eq_op,
			packlen);
		DumpPacketHex(packet_buf, packlen);
		printf("\n");
	}
	
	from->CloseFile();
	delete from;
	
	return(0);
}




void DumpPacketHex(const uchar* buf, int32 size, int32 cols, int32 skip) {
	if (size == 0 || size > 65535)
		return;
	// Output as HEX
	char output[4];
	int j = 0; char* ascii = new char[cols+1]; memset(ascii, 0, cols+1);
	int32 i;
    for(i=skip; i<size; i++)
    {
		if ((i-skip)%cols==0) {
			if (i != skip)
				cout << " | " << ascii << endl;
			cout << setw(4) << setfill(' ') << i-skip << ": ";
			memset(ascii, 0, cols+1);
			j = 0;
		}
		else if ((i-skip)%(cols/2) == 0) {
			cout << "- ";
		}
		sprintf(output, "%02X ", (unsigned char)buf[i]);
		cout << output;

		if (buf[i] >= 32 && buf[i] < 127) {
			ascii[j++] = buf[i];
		}
		else {
			ascii[j++] = '.';
		}
//		cout << setfill(0) << setw(2) << hex << (int)buf[i] << " ";
    }
	int32 k = ((i-skip)-1)%cols;
	if (k < 8)
		cout << "  ";
	for (int32 h = k+1; h < cols; h++) {
		cout << "   ";
	}
	cout << " | " << ascii << endl;
	safe_delete_array(ascii);
}




















