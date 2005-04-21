#include "../common/eq_packet_structs.h"
#include "../common/EQPacket.h"
#include "PacketHandler.h"
#include "EQStreamPair.h"
#include <iostream>

using namespace std;

class PacketHeaderHandler : public PacketHandler {
public:
	PacketHeaderHandler(bool iclean)
	: PacketHandler(DelHandler) {
		clean = iclean;
	}

	virtual void ToClientPacket(EQStreamType Type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *p) {
		if(clean) {
			//drop some junk
			if(emu_opcode == OP_ClientUpdate || emu_opcode == OP_MobUpdate)
				return;
		}
		cout << "Server->Client: ";
//		p->DumpRawHeaderNoTime();
		p->DumpRawHeader();
	}
	virtual void ToServerPacket(EQStreamType Type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *p) {
		if(clean) {
			//drop some junk
			if(emu_opcode == OP_ClientUpdate || emu_opcode == OP_MobUpdate)
				return;
		}
		cout << "Client->Server: ";
//		p->DumpRawHeaderNoTime();
		p->DumpRawHeader();
	}
	
protected:
	bool clean;
private:
	//this seems dumb, but is essential for classes in DLLs
	static void DelHandler(PacketHandler *to_delete) {
		delete((PacketHeaderHandler *) to_delete);
	}
};


#ifdef PCGUI

void SetupPacketHeaderHandler(HandlerCallbacks *calls, bool clean) {
	calls->AddPacketHandler(new PacketHeaderHandler(clean));
}

#else

DLLFUNC int on_load(const HandlerCallbacks *calls, const char *arg)
{
	string cln = "clean";
	bool c = false;
	if(cln == arg)
		c = true;
	
	calls->AddPacketHandler(new PacketHeaderHandler(c));
	return 1;
}

DLLFUNC int on_unload(const HandlerCallbacks *calls)
{
	return 1;
}

#endif
