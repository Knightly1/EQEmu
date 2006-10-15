#include <stdio.h>
#include <time.h>
#include <iostream>
#include <iomanip>
#include "../common/types.h"
#include "../common/packetfile.h"
#include "../common/opcodemgr.h"
#include "../common/files.h"
#include "../common/eq_packet_structs.h"

using namespace std;

void DumpPacketHex(const uchar* buf, int32 size, int32 cols=16, int32 skip=0);

#define PACKET_BUFFER_SIZE 1024*1024

int main(int argc, char *argv[]) {
	if(argc < 2) {
		printf("Usage: %s (input packet files ...)\n", argv[0]);
		return(1);
	}
	
	OpcodeManager *opmgr = new RegularOpcodeManager();
	if(!opmgr->LoadOpcodes(OPCODES_FILE)) {
		printf("Unable to load opcode. Names may not be resolved.\n");
//		delete opmgr;
//		opmgr = new NullOpcodeManager();
	}
	
	PacketFileReader *from;
	
	argv++;
	argc--;
	while(argc != 0) {
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
		
		//read in each EQ packet
		while((packlen = PACKET_BUFFER_SIZE)
		  && from->ReadPacket(eq_op, packlen, packet_buf, to_server, tim)) {
		  	if(opmgr->EQToEmu(eq_op) == OP_NewZone) {
				if(packlen != sizeof(NewZone_Struct)) {
					printf("Log %s's NewZone packet is the wrong size. got %d expected %d\n", argv[0], packlen, sizeof(NewZone_Struct));
					break;
				}
				const NewZone_Struct *nz = (const NewZone_Struct *) packet_buf;
				printf("Log %s contains zone '%s'\n", argv[0], nz->zone_short_name);
				break;
		  	}
		}
		
		from->CloseFile();
		delete from;
		
		argv++;
		argc--;
	}
	
	return(0);
}




void DumpPacketHex(const uchar* buf, int32 size, int32 cols, int32 skip) {
	if (size == 0 || size > 32565)
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




















