#ifndef EQE_PATCH_CURRENT_H
#define EQE_PATCH_CURRENT_H

//this needs to be outside of the namespaces.
#include "ExtractCollector.h"
#include "patches/versions.h"
#include "../common/buildfile.h"
#include "BuildWriterInterface.h"

namespace EQE_Patch_Current {
	using namespace std;
	using namespace EQExtractor;
	
	//pull in the structures we need
	#include "../common/eq_constants.h"
//	#include "../common/eq_packet_structs.h"
	#include "../common/patches/Live_structs.h"
	using namespace Live::structs;
	
	//backwards compatibility, will not get used:
	#pragma pack(1)
	struct SpawnPositionUpdate_Struct
	{
	/*0000*/ uint16		spawn_id;
	/*0002*/ sint64		y:19, z:19, x:19, u3:7;
	/*0010*/ unsigned short	heading:12,unused2:4;
	/*0012*/
	};
	#pragma pack()
	
	#include "cleardefs.h"

	//declare all the extractors
	#include "Extractors.h"
	
	//our packet file writer
	#include "BuildWriter.h"

	//Declare the factory
	#include "factory.h"

};	//end namespace

#endif
