#include "../common/eq_packet_structs.h"
#include "PacketHandler.h"
#include "EQPacket.h"
#include "EQStreamPair.h"
#include <iostream>

using namespace std;


class MessagePacketHandler : public PacketHandler {
public:
	MessagePacketHandler()
	: PacketHandler(DelHandler) {
	}

	virtual void ToClientPacket(EQStreamType t, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p) {
		switch(emu_opcode) {
		case OP_FormattedMessage: {
			FormattedMessage_Struct *s = (FormattedMessage_Struct *) p->pBuffer;
			printf("Format %d: ", s->string_id);
			int len;
			const char *msg = s->message;
			len = strlen(msg);
			while(len > 0) {
				printf("'%s' ", msg);
				msg += len + 1;
				len = strlen(msg);
			}
			printf("\n");
			break;
		}
		case OP_SimpleMessage: {
			SimpleMessage_Struct *s = (SimpleMessage_Struct *) p->pBuffer;
			printf("Simple: %d\n", s->string_id);
			break;
		}
		case OP_ChannelMessage: {
			ChannelMessage_Struct *s = (ChannelMessage_Struct *) p->pBuffer;
			printf("Channel: %s->%s: \"%s\"\n", s->sender, s->targetname, s->message);
			break;
		}
		default:
			break;
		}
	}
	virtual void ToServerPacket(EQStreamType t, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p) {
		ToClientPacket(t, eq_opcode, emu_opcode, p);
	}
	
private:
	//this seems dumb, but is essential for classes in DLLs
	static void DelHandler(PacketHandler *to_delete) {
		delete((MessagePacketHandler *) to_delete);
	}
};

extern "C" int on_load(const HandlerCallbacks *calls, const char *arg)
{
	calls->AddPacketHandler(new MessagePacketHandler());
	return 1;
}

extern "C" int on_unload(const HandlerCallbacks *calls)
{
	return 1;
}

