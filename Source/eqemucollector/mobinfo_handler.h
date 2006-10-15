#ifndef MIHANDLER_H
#define MIHANDLER_H
#include "../common/types.h"
#include "PacketHandler.h"

#include <string>
#include <map>
using namespace std;


class CollectMobHandler : public StreamPacketHandler {
public:
	CollectMobHandler(const EQStreamPair *s);
	virtual ~CollectMobHandler();
	
	bool OpenFile();
	virtual void ToClientPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *app);
	virtual void ToServerPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *app);
	
private:
	//this seems dumb, but is essential for classes in DLLs
	static void DelHandler(PacketHandler *to_delete) {
		delete((CollectMobHandler *) to_delete);
	}
};

extern StreamPacketHandler *CollectMobCreateHandler(EQStreamType type, const EQStreamPair *sp);


class CollectMobStruct {
public:
	CollectMobStruct() { _class = 1; found_merchant = false; move_count = 0; wp_count = 0; }
	string name;
	int _class;
	bool found_merchant;
	int move_count;
	int wp_count;
};
//maps mob in-zone ID -> their info.
extern map<int32, CollectMobStruct> CollectMobList;
//extern bool CollectMobInfo;

#endif

