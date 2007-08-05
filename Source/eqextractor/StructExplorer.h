#ifndef STRUCTEXPLORER_H
#define STRUCTEXPLORER_H

#include "../common/types.h"
#include "../common/emu_opcodes.h"

#include <stdio.h>
#include <map>
#include <string>
#include <vector>

using namespace std;

#define PrintQuadrent(from, field) \
 PrintQuadrent(#field , *((const uint32 *) &from->field))
#define PrintBlock(from, field) \
 PrintBlockReal(#field , (const char *) &from->field, sizeof(from->field))
#define PrintFloats(from, field) \
 PrintFloatsReal(#field , (const char *) &from->field, sizeof(from->field))

#define PrintSimple(from, field) \
 printf(#field ": %lu\n", uint32(from->field))
#define PrintSimpleStr(from, field) \
 printf(#field ": %s\n", from->field)

/*
*/
class StructExplorer {
public:
	virtual ~StructExplorer() {}
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) = 0;

protected:
	void PrintBlockReal(const char *field_name, const char *data, uint32 length);
	void PrintFloatsReal(const char *field_name, const char *data, uint32 length);
	void PrintQuadrentReal(const char *field_name, uint32 data);
};


#endif


