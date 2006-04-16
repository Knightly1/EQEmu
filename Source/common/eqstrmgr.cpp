#include "eqstrmgr.h"
#include <iostream>
#include <stdio.h>

using namespace std;

bool EQStringManager::LoadStringFile(const char *filename) {
	FILE *opf = fopen(filename, "r");
	if(opf == NULL) {
		fprintf(stderr, "Unable to open string file '%s'.", filename);
		return(false);
	}
	
	//load the opcode file into eq, could swap in a nice XML parser here
	char line[2048];
	int lineno = 0;
	uint16 number,count,length;
	char str[1024];
	while(!feof(opf)) {
		lineno++;
		line[0] = '\0';	//for blank line at end of file
		if(fgets(line, sizeof(line), opf) == NULL)
			break;

		length=strlen(line);
		while(isspace(line[length-1]))
			length--;
		line[length]=0;

		if (lineno==1) {
			//cout << "File version is: " << line << endl;
			continue;
		}

		if (lineno==2) {
			sscanf(line,"%hi %hi",&number,&count);
			//cout << "File contains " << count << " messages." << endl;
			continue;
		}

		sscanf(line,"%hi %[^\n]",&number,str);

		Strings[number]=str;

	}
	fclose(opf);

	return true;
	
}

bool EQStringManager::Lookup(uint16 num, string &str)
{
bool ret=false;
map<uint16,string>::iterator itr;
	if ((itr=Strings.find(num))!=Strings.end()) {
		str=itr->second;
		ret=true;
	}

	return ret;
}
