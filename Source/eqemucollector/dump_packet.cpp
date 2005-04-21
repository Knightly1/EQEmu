#include "../common/eq_packet_structs.h"
#include "PacketHandler.h"
#include "EQPacket.h"
#include "EQStreamPair.h"
#include <iostream>

using namespace std;


class PacketDumpHandler : public PacketHandler {
public:
	PacketDumpHandler()
	: PacketHandler(DelHandler) {
	}

	virtual void ToClientPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *p) {
		cout << EQStream::StreamTypeString(type) << "->Client: ";
		p->DumpRaw();
	}
	virtual void ToServerPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *p) {
		cout << "Client->" << EQStream::StreamTypeString(type) << ": ";
		p->DumpRaw();
	}
	
private:
	//this seems dumb, but is essential for classes in DLLs
	static void DelHandler(PacketHandler *to_delete) {
		delete((PacketDumpHandler *) to_delete);
	}
};

extern "C" int on_load(const HandlerCallbacks *calls, const char *arg)
{
	calls->AddPacketHandler(new PacketDumpHandler());
	return 1;
}

extern "C" int on_unload(const HandlerCallbacks *calls)
{
	return 1;
}

