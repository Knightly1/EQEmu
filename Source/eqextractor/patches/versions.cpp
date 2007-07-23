
#include "../common/debug.h"
#include <string.h>
#include "patches/versions.h"
#include "../common/opcodemgr.h"
#include "../common/buildfile.h"
//Must include all valid version headers
#include "patches/patch_current.h"
#include "patches/patch_local.h"
#include "patches/patch_021505.h"
#include "patches/patch_121504.h"
#include "patches/patch_051105.h"
#include "patches/patch_Titanium.h"
#include "patches/patch_Anniversary.h"
#include "patches/patch_62.h"

ExtractorAbstractFactory::ExtractorAbstractFactory(const char *opcodes_file) {
	opcodeManager = new RegularOpcodeManager();
	
	//we dont care how this turns out, worst case it falls
	//back to a NULL manager and dosent do anything.
	opcodeManager->LoadOpcodes(opcodes_file, false);
}

ExtractorAbstractFactory::~ExtractorAbstractFactory() {
	safe_delete(opcodeManager);
}

void ExtractorAbstractFactory::ListExtractorFactories() {
	printf("Avaliable Patches:\n"
		"\tcurrent\t- The current eqemu code (Live compat, however good it is)\n"
		"\tlocal\t- The current structures in patches/patch_local.h\n"
		"\tDated:\n"
		"\tTitanium - The Titanium boxed edition (10/27/2005->12/06/2005)\n"
		"\tAnniversary - The Anniversary boxed edition (02/13/07->03/14/07)\n"
		"\tClient62 - The eqemu 6.2 compatible client (July 11th 2005 -> 09/13/2005)\n"
		"\t051105 - The May 11th 2005 patch\n"
		"\t021505 - The Febuary 15 2005 patch, DoN release, new net code\n"
		"\t121504 - The December 15th 2004 patch.\n"
		"\n"
	);
}

ExtractorAbstractFactory *ExtractorAbstractFactory::GetExtractorFactoryByName(const char *patch_name, const char *filename) {
	if(!strcasecmp(patch_name, "current")) {
		return(new EQE_Patch_Current::ExtractorConcreteFactory(filename));
	}
	if(!strcasecmp(patch_name, "local")) {
		return(new EQE_Patch_Local::ExtractorConcreteFactory(filename));
	}
	if(!strcasecmp(patch_name, "021505")) {
		return(new EQE_Patch_021505::ExtractorConcreteFactory(filename));
	}
	if(!strcasecmp(patch_name, "121504")) {
		return(new EQE_Patch_121504::ExtractorConcreteFactory(filename));
	}
	if(!strcasecmp(patch_name, "051105")) {
		return(new EQE_Patch_051105::ExtractorConcreteFactory(filename));
	}
	if(!strcasecmp(patch_name, "Client62")) {
		return(new EQE_Patch_Client62::ExtractorConcreteFactory(filename));
	}
	if(!strcasecmp(patch_name, "Titanium")) {
		return(new EQE_Patch_Titanium::ExtractorConcreteFactory(filename));
	}
	if(!strcasecmp(patch_name, "Anniversary")) {
		return(new EQE_Patch_Anniversary::ExtractorConcreteFactory(filename));
	}
	return(NULL);
}

//second constants to make patch dates easier
#define YEAR_2004 1072935480
#define YEAR_2005 1104492410
#define YEAR_2007 1167606262
#define MONTH 2629743
#define DAY 86400

ExtractorAbstractFactory *ExtractorAbstractFactory::GetExtractorFactoryByDate(uint32 stamp, const char *filename) {
	//dated patches
//printf("date %d, 5/11=%d, 2/15=%d\n", stamp, YEAR_2005 + 6*MONTH + 10*DAY, YEAR_2005 + 4*MONTH + 10*DAY);
	if(stamp < (YEAR_2004 + 11*MONTH + 14*DAY)) {
		fprintf(stderr, "# Detected Patch before our known history. Unable to process.\n");
		return(NULL);
	} else if(stamp < (YEAR_2005 + 1*MONTH + 14*DAY)) {
		// patch 12-15-04 to 02-15-05
		fprintf(stderr, "# Auto-detected patch date of 12-15-04\n");
		return(new EQE_Patch_121504::ExtractorConcreteFactory(filename));
	} else if(stamp < (YEAR_2005 + 4*MONTH + 10*DAY)) {
		// patch after 02-15-05 to 05-11-05, 1115875382
		fprintf(stderr, "# Auto-detected patch date of 02-15-05\n");
		return(new EQE_Patch_021505::ExtractorConcreteFactory(filename));
	} else if(stamp < (YEAR_2005 + 6*MONTH + 10*DAY)) {
		// patch after 05-11-05 to 07-11-05, 1121134868
		fprintf(stderr, "# Auto-detected patch date of 05-15-05\n");
		return(new EQE_Patch_051105::ExtractorConcreteFactory(filename));
	} else if(stamp < (YEAR_2005 + 8*MONTH + 12*DAY)) {
		// patch after 07-11-05 to 09-12-05
		fprintf(stderr, "# Auto-detected patch date of 07-11-05 (Client62)\n");
		return(new EQE_Patch_Client62::ExtractorConcreteFactory(filename));
	} else if(stamp < (YEAR_2005 + 9*MONTH + 26*DAY)) {
		// patch after 09-13-05 to 10-26-05
		fprintf(stderr, "# Auto-detected unsupported patch date of 09-13-05\n");
		return(NULL);
	} else if(stamp < (YEAR_2005 + 11*MONTH + 6*DAY)) {
		// patch after 10-27-05 to 12-06-05
		fprintf(stderr, "# Auto-detected patch date of 10-27-05 (Titanium)\n");
		return(new EQE_Patch_Titanium::ExtractorConcreteFactory(filename));

	} else if(stamp < (YEAR_2007 + 1*MONTH + 12*DAY)) {
		// No work done from 12/6/05 to 02/13/07
		fprintf(stderr, "# Invalid Patch date 12/6/05 to 02/13/07\n");
		return(NULL);
	} else if(stamp < (YEAR_2007 + 2*MONTH + 14*DAY)) {
		// patch after 02/13/07 to 03/14/07
		fprintf(stderr, "# Auto-detected patch date of 02-13-07 (Anniversary)\n");
		return(new EQE_Patch_Anniversary::ExtractorConcreteFactory(filename));
		
	} //current patch on: 
	
	fprintf(stderr, "# Time Stamp is after any known patch, using current (live compat) structs.\n");
	//if its not one of the past patches, assume its the most recent.
	return(new EQE_Patch_Current::ExtractorConcreteFactory(filename));
}






