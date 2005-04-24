#ifndef EXPLORERS_H
#define EXPLORERS_H

#include "StructExplorer.h"


class ZoneHeaderExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len);
};

class ObjectExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len);
};

class SpawnExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len);
};


#endif



