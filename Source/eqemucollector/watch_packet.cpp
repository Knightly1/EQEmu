#include "../common/eq_packet_structs.h"
#include "PacketHandler.h"
#include "EQPacket.h"
#include "EQStreamPair.h"
#include <iostream>

using namespace std;


class PacketWatchHandler : public PacketHandler {
public:
	PacketWatchHandler(uint16 op)
	: PacketHandler(DelHandler) {
		watch_op = op;
	}

	virtual void ToClientPacket(EQStreamType t, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p) {
		if(eq_opcode != watch_op)
			return;
		cout << "Server->Client: ";
		p->DumpRaw();
	}
	virtual void ToServerPacket(EQStreamType t, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p) {
		if(eq_opcode != watch_op)
			return;
		cout << "Client->Server: ";
		p->DumpRaw();
	}
	
protected:
	uint16 watch_op;
	
private:
	//this seems dumb, but is essential for classes in DLLs
	static void DelHandler(PacketHandler *to_delete) {
		delete((PacketWatchHandler *) to_delete);
	}
};

extern "C" int on_load(const HandlerCallbacks *calls, const char *arg)
{
	uint16 op;
	if(sscanf(arg, "0x%x", &op) != 1) {
		printf("Unable to read opcode argument, not watching '%s'\n", arg);
		return(0);
	}
	calls->AddPacketHandler(new PacketWatchHandler(op));
	return 1;
}

extern "C" int on_unload(const HandlerCallbacks *calls)
{
	return 1;
}

