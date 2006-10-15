#ifndef EQE_PATCH_EXAMPLE_H
#define EQE_PATCH_EXAMPLE_H

#include "../common/types.h"

//this needs to be outside of the namespaces.
#include "ExtractCollector.h"
#include "patches/versions.h"
#include "../common/buildfile.h"
#include "BuildWriterInterface.h"

namespace EQE_Patch_Example {
	using namespace std;
	using namespace EQExtractor;
	
/**********************************************************************************************************
	eq_packet_structs.h
**********************************************************************************************************/	

#pragma pack(1)
	//Paste in the contents of eq_packet_structs.h
	//Paste in SpawnAppearance part of eq_constants.h
#pragma pack()

#include "cleardefs.h"
	
/**********************************************************************************************************
     Methods:
**********************************************************************************************************/	

	//declare all the extractors
	#include "Extractors.h"
	
	//our packet file writer
	#include "BuildWriter.h"

	//Declare the factory
	#include "factory.h"

};	//end namespace

#endif
