#include <time.h>
#include <errno.h>
#include "../common/eq_packet_structs.h"
#include "../common/eq_opcodes.h"
#include "PacketHandler.h"
#include "../common/EQPacket.h"
#include "EQStreamPair.h"
#include "../common/packetfile.h"

//using namespace std;

#define LOG_DIRECTORY "logs"
#define PACKET_FILE_NAME "packetlog"
#define ZONE_NAME_LOG "log_zones.txt"


class PacketFileHandler : public StreamPacketHandler {
public:
	PacketFileHandler(const EQStreamPair *s);
	virtual ~PacketFileHandler();
	
	bool OpenFile();
	virtual void ToClientPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *p);
	virtual void ToServerPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *p);
	
protected:
	PacketFileWriter *file;
	unsigned long my_number;
	
	static unsigned long file_number;

private:
	//this seems dumb, but is essential for classes in DLLs
	static void DelHandler(PacketHandler *to_delete) {
		delete((PacketFileHandler *) to_delete);
	}
};

#define YEAR_2005 1104492410
unsigned long PacketFileHandler::file_number = time(NULL) - YEAR_2005; //smaller number

PacketFileHandler::PacketFileHandler(const EQStreamPair *s)
: StreamPacketHandler(DelHandler, s)
{
	file = NULL;
}

PacketFileHandler::~PacketFileHandler() {
	safe_delete(file);
}

bool PacketFileHandler::OpenFile() {
	//this is not in the constructor so I can return a value (i dont like exceptions)
	
	//figure out a unique file name... lazy for now
	char buf[128];
	sprintf(buf, "%s/%s-%lu.pf", LOG_DIRECTORY, PACKET_FILE_NAME, file_number);
	my_number = file_number;
	file_number++;
	
	file = new PacketFileWriter(false);
	if(!file->OpenFile(buf)) {
		fprintf(stderr, "Unable to open packet file '%s': %s\n", buf, strerror(errno));
		safe_delete(file);
		return(false);
	}
	return(true);
}

void PacketFileHandler::ToClientPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *p) {
	if(file != NULL)
		file->WritePacket(p->GetRawOpcode(), p->size, p->pBuffer, false, p->timestamp);
	
	if(p->size > 96 && emu_opcode == OP_NewZone) {
		const NewZone_Struct *nz = (const NewZone_Struct *) p->pBuffer;
		
		//we have a zone short name
		FILE *fl = fopen(LOG_DIRECTORY "/" ZONE_NAME_LOG, "a");
		if(fl == NULL) {
			printf("Unable to open zone name log file '%s'\n", LOG_DIRECTORY "/" ZONE_NAME_LOG);
			return;
		}
		
		printf("Log %s/%s-%lu.pf contains zone '%s'\n", LOG_DIRECTORY, PACKET_FILE_NAME, my_number, nz->zone_short_name);
		fprintf(fl, "Log %s/%s-%lu.pf contains zone '%s'\n", LOG_DIRECTORY, PACKET_FILE_NAME, my_number, nz->zone_short_name);
		
		fclose(fl);
	}
}

void PacketFileHandler::ToServerPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQApplicationPacket *p) {
	if(file != NULL)
		file->WritePacket(p->GetRawOpcode(), p->size, p->pBuffer, true, p->timestamp);
}

StreamPacketHandler *PacketFileCreateHandler(EQStreamType type, const EQStreamPair *sp) {
	if(!sp->IsZoneStream())
		return(NULL);	//not interested
	
	PacketFileHandler *pfh = new PacketFileHandler(sp);
	if(!pfh->OpenFile()) {
		safe_delete(pfh);
		return(NULL);
	}
	
	return(pfh);
}

#ifdef PCGUI

void SetupPacketFileCollector(HandlerCallbacks *calls) {
	calls->AddStreamCreateHandler(PacketFileCreateHandler);
}

#else

DLLFUNC int on_load(const HandlerCallbacks *calls, const char *arg)
{
	calls->AddStreamCreateHandler(PacketFileCreateHandler);
	return(1);
}

DLLFUNC int on_unload(const HandlerCallbacks *calls)
{
	return 1;
}

#endif

/*
class PacketFileManager {
public:
	PacketFileManager() {
		file_number = time(NULL) - YEAR_2005;	//smaller number
	}
	
	~PacketFileManager() {
		map<const EQStreamPair *, PacketFileWriter *>::iterator cur, end;
		cur = streams.begin();
		end = streams.end();
		for(; cur != end; cur++) {
			safe_delete(cur->second);
		}
		streams.resize(0);
	}
	
	void NewStream(const EQStreamPair *sp) {
		//figure out a unique file name... lazy for now
		
		char buf[128];
		sprintf(buf, "logs/%s-%lu.pf", PACKET_FILE_NAME, file_number);
		file_number++;
		
		//for now, we dont auto-flush
		PacketFileWriter pfw = new PacketFileWriter(false);
		
		//open the output file.
		if(!pfw->OpenFile(buf)) {
			fprintf(stderr, "Unable to open packet file '%s': %s\n", buf, strerror(errno));
			return;
		}
		
		streams[sp] = pfw;
	}
	
	void CloseStream(const EQStreamPair *sp) {
		map<const EQStreamPair *, PacketFileWriter *>::iterator res;
		res = streams.find(sp);
		if(res == streams.end())
			return;	//not found, this can happen for many legit reasons
		safe_delete(res->second);
		streams.erase(res);
	}
	
	PacketFileWriter *FindFile(const EQStreamPair *sp) {
		map<const EQStreamPair *, PacketFileWriter *>::iterator res;
		res = streams.find(sp);
		if(res == streams.end())
			return(NULL);
		return(res->second);
	}
	
protected:
	map<const EQStreamPair *, PacketFileWriter *> streams;
	unsigned long file_number;
};

PacketFileManager *pfm = NULL;

void WriteClientPacket(const EQStreamPair *sp, const EQApplicationPacket *p)
{
	if(pfm == NULL)
		return;	//not initilized
	
	PacketFileWriter *out = pfm.FindFile(sp);
	if(out == NULL)
		return;		//unable to find a packet file for this stream
	
	out->WritePacket(p->opcode, p->size, p->pBuffer, false);
}

void WriteServerPacket(const EQStreamPair *sp, const EQApplicationPacket *p)
{
	if(pfm == NULL)
		return;	//not initilized
	
	PacketFileWriter *out = pfm.FindFile(sp);
	if(out == NULL)
		return;		//unable to find a packet file for this stream
	
	out->WritePacket(p->opcode, p->size, p->pBuffer, true);
}

void CreateStreamPair(const EQStreamPair *sp) {
	if(pfm == NULL)
		return;	//not initilized
	
	if(!sp->IsZoneStream())
		return;	//not interested
	
	pfm.NewStream(sp);
}

void DestroyStreamPair(const EQStreamPair *sp) {
	if(pfm == NULL)
		return;	//not initilized
	pfm.CloseStream(sp);
}

DLLFUNC int on_load()
{
	pfm = new PacketFileManager();
	AddClientPacketHandler(0, WriteClientPacket);
	AddServerPacketHandler(0, WriteServerPacket);
	AddStreamCreateHandler(CreateStreamPair);
	AddStreamDestroyHandler(DestroyStreamPair);
	return 1;
}

DLLFUNC int on_unload()
{
	safe_delete(pfm);
	return 1;
}
*/
