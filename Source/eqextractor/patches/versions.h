#ifndef EQE_VERSIONS_H
#define EQE_VERSIONS_H

//here for convenience as its used by all patches
#include "../common/MiscFunctions.h"
#include "../common/misc.h"
#include "../common/packet_dump.h"
#include "ExtractDB.h"

#ifndef WIN32
#include <netinet/in.h>
#endif

//#include <mysql.h>
#include <algorithm>

class OpcodeManager;
class PatchBuildFileWriterInterface;
namespace EQExtractor {
	class ExtractCollector;
};

class ExtractorAbstractFactory {
	//acts as an interface for concrete factories for each patch
	//acts as a factory itself to instantiate the concrete factories
public:
	ExtractorAbstractFactory(const char *opcodes_file);
	virtual ~ExtractorAbstractFactory();
	
	//Factory methods:
	static ExtractorAbstractFactory *GetExtractorFactoryByName(const char *patch_name, const char *filename);
	static ExtractorAbstractFactory *GetExtractorFactoryByDate(uint32 unix_timestamp, const char *filename);
	
	static void ListExtractorFactories();
	
	//Abstract Factory methods:
	virtual EQExtractor::ExtractCollector *getZoneInfoExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newDoorExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newFuzzyDoorExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newAAExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newZoneHeaderExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newZonePointExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newObjectExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newFuzzyObjectExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newTributeExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newTributeTextExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newBookTextExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newTitleExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newRecipeExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newTaskExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newTaskHistoryExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newSpawnExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newSpawnListExtractor() = 0;
	virtual EQExtractor::ExtractCollector *newCharacterExtractor(uint32 char_id) = 0;
	virtual PatchBuildFileWriterInterface *newBuildFileWriter() = 0;
	
	OpcodeManager *opcodeManager;
};



#endif

