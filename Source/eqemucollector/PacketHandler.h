#ifndef _PACKETHANDLER_H
#define _PACKETHANDLER_H

#include "../common/EQStream.h"
#include "../common/types.h"
#include "../common/emu_opcodes.h"

class EQStreamPair;
class EQRawApplicationPacket;

class PacketHandler;

typedef void (*PacketHandlerDeleteMethod)(PacketHandler *to_delete);

class PacketHandler {
public:
	PacketHandler(PacketHandlerDeleteMethod del_proc);

/*
	Because windows DLLs cannot make callbacks into the main program,
	we pass down both opcodes everywhere, since it will be something that
	almost every plugin will need. If they need more functionality on
	the packet object, the windows DLL will need to statically link the
	code for that additional functionality.
*/
	virtual void ToClientPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *app) = 0;
	virtual void ToServerPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *app) = 0;
	
	PacketHandlerDeleteMethod GetDeleteMethod() const { return(_delete_proc); }
	
protected:
	PacketHandlerDeleteMethod _delete_proc;
};

class StreamPacketHandler : public PacketHandler {
public:
	StreamPacketHandler(PacketHandlerDeleteMethod del_proc, const EQStreamPair *s);
	
protected:
	const EQStreamPair *stream;
};

//returns a PacketAcceptor object to attach to this stream, or NULL for none 
typedef StreamPacketHandler *(*StreamCreateHandler)(EQStreamType, const EQStreamPair *sp);

//typedef void (*PacketHandler)(const EQStreamPair *sp, const EQRawApplicationPacket *app);
typedef void (*StreamDestroyHandler)(const EQStreamPair *sp);


typedef struct {
	void (*AddStreamCreateHandler)(StreamCreateHandler f);
	void (*AddStreamDestroyHandler)(StreamDestroyHandler f);
	void (*AddPacketHandler)(PacketHandler *a);
} HandlerCallbacks;

//not to be called from a DLL
extern void ClearPacketHandlers();
extern void GetHandlerCallbacks(HandlerCallbacks *it);

//prototype for DLL functions:
extern "C" int on_load(const HandlerCallbacks *calls, const char *arg);
extern "C" int on_unload(const HandlerCallbacks *calls);


#ifdef WIN32
	#define DLLFUNC extern "C" __declspec(dllexport)
#else
	#define DLLFUNC extern "C"
#endif


#endif
