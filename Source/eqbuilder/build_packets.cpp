#include <stdio.h>
#include <time.h>
#include "../PacketCollector/packetfile.h"
#include "buildfile.h"
#include "build_patches/writers.h"

/*

When a new patch is released which affects the packets:
- The date of the patch is MMDDYY
- Copy the eq_opcodes.h and eq_packet_structs.h from before the 
patch into the build_patches directory.
- Rename them to eq_opcodes_MMDDYY.h and eq_packet_structs_MMDDYY.h
- Copy current.cpp to MMDDYY.cpp
- edit MMDDYY.cpp, changing _Current to _MMDDYY
- edit MMDDYY.cpp, changing the ITEM_* defines if they changed
- edit builtfile.h, make a copy of the BuildFileWriter_Current and rename
- Edit eq_packet_structs_MMDDYY.h to use proper paths, which is:
  fix these includes to be relative to ../../common/:
   - eq_old_structs.h
   - types.h
   - eq_constants.h
   - version.h
- Add build_patches/MMDDYY.o to the makefile
- Add a conditional in this file before BuildFileWriter_Current which 
  allocates a new BuildFileWriter_MMDDYY class if the timestamp is before
  the patch date, using the constants below to specify the patch date.
  * make sure these go in chronological order
  

*/


#define PACKET_BUFFER_SIZE 1024*1024

//second constants to make patch dates easier
#define YEAR_2004 1072935480
#define YEAR_2005 1104492410
#define MONTH 2629743
#define DAY 86400

int main(int argc, char *argv[]) {
	if(argc != 3) {
		printf("Usage: %s (input packet file) (output build file)\n", argv[0]);
		return(1);
	}
	
	
	PacketFileReader *from;
	BuildFileWriter *to;
	
	from = PacketFileReader::OpenPacketFile(argv[1]);
	if(from == NULL) {
		printf("Error: Unable to open input packet file '%s'\n", argv[1]);
		return(1);
	}
	
	time_t stamp = from.GetStamp();
	
	//figure out what packet file reader we want based on the
	//time stamp in the file.
	if(stamp < (YEAR_2004 + 11*MONTH + 15*DAY)) {	// 12-15-04
		//this is an example of how this would work...
		//this date is actually worthless since nobody could have collected before this
		printf("This packet file was generated before the 12-15-04 patch.\n");
		to = new BuildFileWriter_121504();
	} else if(stamp < (YEAR_2005 + 1*MONTH + 25*DAY)) {	// 02-25-05
		//this is an example of how this would work...
		//this date is actually worthless since nobody could have collected before this
		printf("This packet file was generated before the 12-15-04 patch.\n");
		to = new BuildFileWriter_121504();
	
	//ADD NEW PATCH DATES HERE!
	} else {
		//ELSE: assume it uses the most recent collector
		printf("This packet file was generated with the most current patch.\n");
		to = new BuildFileWriter_Current();
	}
	
	
	//open up the output
	if(!to->OpenFile(argv[2])) {
		printf("Error: Unable to open output build file '%s'\n", argv[2]);
		return(1);
	}
	
	uint16 eq_op;
	uint32 packlen;
	unsigned char packet_buf[PACKET_BUFFER_SIZE];
	
	//read in each EQ packet, ship it off to the build manager.
	while((packlen = PACKET_BUFFER_SIZE)
	  && from->ReadPacket(eq_op, packlen, packet_buf)) {
		to->WritePacket(eq_op, packlen, packet_buf);
	}
	
	from->CloseFile();
	to->CloseFile();
	
	delete to;
	delete from;
	
	return(0);
}



















