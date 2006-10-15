#define KEEP_DEFINES
#include "../common/debug.h"
#include "patch_local.h"
#include "../common/opcodemgr.h"
#include "../common/files.h"

namespace EQE_Patch_Local {

ExtractorConcreteFactory::ExtractorConcreteFactory(const char *filename)
: ExtractorAbstractFactory(OPCODES_FILE),
zone_info(filename)
{
}

//Build all the extractors using our current structs
#include "Extractors.cpp"

//Build the build file writer using our current structs
//offsets into the serialized items:
#define ITEM_ID_POS 14
#define ITEM_NAME_POS 11
#define ITEM_MAX_FIELDS 16	//just has to be more than we need.
#define ITEM_SLOT_POS 2
#define ITEM_MERCHANT_SLOT_POS 2
#include "BuildWriter.cpp"


};	//end namespace



