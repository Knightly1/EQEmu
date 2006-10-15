#define KEEP_DEFINES
#include "../common/debug.h"
#include "patch_121504.h"
#include "../common/opcodemgr.h"

namespace EQE_Patch_121504 {

ExtractorConcreteFactory::ExtractorConcreteFactory(const char *filename)
: ExtractorAbstractFactory("patch_121504.conf"),
zone_info(filename)
{
}

//Build all the extractors using our current structs
#define NO_ZONE_ID_IN_NEWZONE
#include "Extractors.cpp"

//Build the build file writer using our current structs
#define OLD_DELTAS
//offsets into the serialized items:
//WRONG:
#define ITEM_ID_POS 13
#define ITEM_NAME_POS 10
#define ITEM_MAX_FIELDS 15	//just has to be more than we need.
#define ITEM_SLOT_POS 2
#define ITEM_MERCHANT_SLOT_POS 5
#include "BuildWriter.cpp"


};	//end namespace



