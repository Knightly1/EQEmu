//no macro guard on purpose!

//this is a single implementation with a single name, but is included
//into many different namespaces which yeilds many different concrete factories
class ExtractorConcreteFactory : public ExtractorAbstractFactory {
public:
	ExtractorConcreteFactory(const char *filename);
	
	//this patch's zone info extractor, for use by anybody who needs it.
	ZoneInfoExtractor zone_info;
	
	//Abstract Factory methods:
	virtual EQExtractor::ExtractCollector *getZoneInfoExtractor() { return(&zone_info); }
	virtual EQExtractor::ExtractCollector *newDoorExtractor() { return(new DoorExtractor(&zone_info)); }
	virtual EQExtractor::ExtractCollector *newFuzzyDoorExtractor() { return(new FuzzyDoorExtractor(&zone_info)); }
	virtual EQExtractor::ExtractCollector *newAAExtractor() { return(new AAExtractor()); }
	virtual EQExtractor::ExtractCollector *newZoneHeaderExtractor() { return(new ZoneHeaderExtractor(&zone_info)); }
	virtual EQExtractor::ExtractCollector *newZonePointExtractor() { return(new ZonePointExtractor(&zone_info)); }
	virtual EQExtractor::ExtractCollector *newObjectExtractor() { return(new ObjectExtractor()); }
	virtual EQExtractor::ExtractCollector *newFuzzyObjectExtractor() { return(new FuzzyObjectExtractor()); }
	virtual EQExtractor::ExtractCollector *newTributeExtractor() { return(new TributeExtractor()); }
	virtual EQExtractor::ExtractCollector *newTributeTextExtractor() { return(new TributeTextExtractor()); }
	virtual EQExtractor::ExtractCollector *newBookTextExtractor() { return(new BookTextExtractor()); }
	virtual EQExtractor::ExtractCollector *newTitleExtractor() { return(new TitleExtractor()); }
	virtual EQExtractor::ExtractCollector *newRecipeExtractor() { return(new RecipeExtractor()); }
	virtual EQExtractor::ExtractCollector *newTaskExtractor() { return(NULL/*new TaskExtractor()*/); }
	virtual EQExtractor::ExtractCollector *newTaskHistoryExtractor() { return(new TaskHistoryExtractor()); }
	virtual EQExtractor::ExtractCollector *newSpawnExtractor() { return(new SpawnExtractor(&zone_info)); }
	virtual EQExtractor::ExtractCollector *newSpawnListExtractor() { return(new SpawnListExtractor()); }
	virtual EQExtractor::ExtractCollector *newCharacterExtractor(uint32 charid) { return(new CharacterExtractor(charid, &zone_info)); }
	virtual PatchBuildFileWriterInterface *newBuildFileWriter() { return(new PatchBuildFileWriter()); }
};



