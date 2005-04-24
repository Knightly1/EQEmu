
#include <stdio.h>
#include <stdlib.h>
#include "ExtractDB.h"

ExtractorDB::ExtractorDB() {
	char host[200], user[200], passwd[200], database[200];
	int32 port=0;
	bool compression = false;
	bool items[6] = {false, false, false, false, false, false};
	
	if(!ReadDBINI(host, user, passwd, database, port, compression, items)) {
		exit(1);
	}
	
	if (!items[0] || !items[1] || !items[2] || !items[3])
	{
		printf ("Incomplete DB.INI file.\n");
		printf ("Read README.TXT!\n");
		exit (1);
	}
	
	int32 errnum = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	if (!Open(host, user, passwd, database,port, &errnum, errbuf)) {
		fprintf(stderr, "Failed to connect to database: Error: %s", errbuf);
	}
	else
	{
//		fprintf(stderr, "Using database '%s' at %s\n", database, host);
	}
}



















