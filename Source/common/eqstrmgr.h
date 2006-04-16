#ifndef _EQSTRMGR_H
#define _EQSTRMGR_H

#include <map>
#include <string>
#include "types.h"

using namespace std;

class EQStringManager {
	private:
		map <uint16,string> Strings;
	public:
		bool LoadStringFile(const char *filename);
		bool Lookup(uint16 num, string &str);
};

#endif
