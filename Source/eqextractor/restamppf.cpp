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

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: %s [file.pf] [new stamp|now]\n", argv[0]);
		return(1);
	}
	
	uint32 stamp;
	if(strcmp(argv[2], "now") == 0)
		stamp = time(NULL);
	else
		stamp = strtoul(argv[2], NULL, 10);
	if(stamp == 0) {
		printf("Invalid stamp\n");
		return(1);
	}

	if(!PacketFileWriter::SetPacketStamp(argv[1], stamp))
		return(1);
	
	printf("Stamp successfully changed.\n");
	return(0);
}





