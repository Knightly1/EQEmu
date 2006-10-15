/*

EQ Extractor, by Father Nitwit 2005


*/

#ifndef BuildWriterInterface_H
#define BuildWriterInterface_H

#include "ExtractCollector.h"
#include "../common/buildfile.h"


class PatchBuildFileWriterInterface 
  : public ::BuildFileWriter, public EQExtractor::ExtractBase {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server) = 0;
	virtual void WritePacket(EmuOpcode emu_op, uint32 packlen, unsigned char *packet) = 0;
};


#endif


