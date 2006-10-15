#ifndef EXPLORERS_H
#define EXPLORERS_H

#include "StructExplorer.h"
#include <map>
using namespace std;


class PlayerProfileExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class ZoneHeaderExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class ObjectExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class SpawnExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class SpellExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
protected:
	map<uint16, uint8> m_classMap;
	map<uint16, string> m_nameMap;
};

class ClientUpdateExplorer : public StructExplorer {
public:
	ClientUpdateExplorer(int32 forid_) : StructExplorer() {
		forid = forid_;
	}
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
protected:
	int32 forid;
};

class ArrowExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class UnknownExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class Unknown2Explorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class MessageExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class SpawnListExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};

class DumpUnknownExplorer : public StructExplorer {
public:
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len, bool to_server);
};


#endif



