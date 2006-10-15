#include "../common/eq_packet_structs.h"
#include "PacketHandler.h"
#include "EQPacket.h"
#include "EQStreamPair.h"
#include <iostream>

using namespace std;


class UnknownPacketHandler : public PacketHandler {
public:
	UnknownPacketHandler()
	: PacketHandler(DelHandler) {
	}

	virtual void ToClientPacket(EQStreamType t, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p) {
		if(emu_opcode != OP_Unknown)
			return;
		cout << "Server->Client: ";
		p->DumpRaw();
	}
	virtual void ToServerPacket(EQStreamType t, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p) {
		if(emu_opcode != OP_Unknown)
			return;
		cout << "Client->Server: ";
		p->DumpRaw();
	}
	
private:
	//this seems dumb, but is essential for classes in DLLs
	static void DelHandler(PacketHandler *to_delete) {
		delete((UnknownPacketHandler *) to_delete);
	}
};

extern "C" int on_load(const HandlerCallbacks *calls, const char *arg)
{
	calls->AddPacketHandler(new UnknownPacketHandler());
	return 1;
}

extern "C" int on_unload(const HandlerCallbacks *calls)
{
	return 1;
}

