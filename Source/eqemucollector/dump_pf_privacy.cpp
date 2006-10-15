#include <time.h>
#include <errno.h>
#include "../common/eq_packet_structs.h"
#include "../common/eq_opcodes.h"
#include "PacketHandler.h"
#include "../common/EQPacket.h"
#include "EQStreamPair.h"
#include "../common/packetfile.h"
#include <string>

using namespace std;

#define LOG_DIRECTORY "logs"
#define PACKET_FILE_NAME "packetlog"
#define ZONE_NAME_LOG "log_zones.txt"


string Privacy_Name;

class PrivacyPacketFileHandler : public StreamPacketHandler {
public:
	PrivacyPacketFileHandler(const EQStreamPair *s);
	virtual ~PrivacyPacketFileHandler();
	
	bool OpenFile();
	virtual void ToClientPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p);
	virtual void ToServerPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p);
	
protected:
	PacketFileWriter *file;
	unsigned long my_number;
	
	static unsigned long file_number;
	
	bool PrivateOpcode(EmuOpcode op);

	void BufferReplace(char *orig, int origlen, 
		const char *search, const char *replace, int searchlen);
	
	char _zero_buffer[64];
private:
	//this seems dumb, but is essential for classes in DLLs
	static void DelHandler(PacketHandler *to_delete) {
		delete((PrivacyPacketFileHandler *) to_delete);
	}
};

#define YEAR_2005 1104492410
unsigned long PrivacyPacketFileHandler::file_number = time(NULL) - YEAR_2005; //smaller number

PrivacyPacketFileHandler::PrivacyPacketFileHandler(const EQStreamPair *s)
: StreamPacketHandler(DelHandler, s)
{
	file = NULL;
	memset(_zero_buffer, 0, sizeof(_zero_buffer));
}

PrivacyPacketFileHandler::~PrivacyPacketFileHandler() {
	safe_delete(file);
}

void PrivacyPacketFileHandler::BufferReplace(char *orig, int origlen, 
	const char *search, const char *replace, int searchlen) {
	char *cur = orig;
	const char *scur = search;
	
//	int foundcount;
	int spos = 0;
	int pos;
	int r;
	
//if(origlen > 50)
//	printf("Searching in buffer of len %d\n", origlen);

	for(pos = 0; pos < origlen; pos++, cur++) {
		if(*cur == *scur) {
			spos++;
			scur++;
			if(spos == searchlen) {
				//found it
				cur -= spos;	//back to the begining
				cur++;
				scur = replace;
				for(r = 0; r < spos; r++, cur++, scur++) {
					*cur = *scur;
				}
				
				//reset search buffer
				spos = 0;
				scur = search;
			}
		} else if(spos != 0) {
			//partial match, but we failed...
			cur -= spos;	//back to the begining of the match
			pos -= spos;
			
			//reset search buffer
			spos = 0;
			scur = search;
		}
		//else, no match, and we havent found anything.
	}
}

bool PrivacyPacketFileHandler::PrivateOpcode(EmuOpcode op) {
	switch(op) {
	//world opcodes:
	case OP_GuildsList:
	case OP_ApproveWorld:
	case OP_SendLoginInfo:
	case OP_SendCharInfo:
	case OP_EnterWorld:
	case OP_SetChatServer:
	case OP_LogServer:
	
	//zone opcodes:
	case OP_ZoneEntry:
	case OP_PlayerProfile:
	case OP_ReqNewZone:
	case OP_RaidUpdate:
//	case OP_CharInfo:
	case OP_ChannelMessage:
	case OP_SimpleMessage:
	case OP_FormattedMessage:
	case OP_GuildMOTD:
	case OP_GuildLeader:
	case OP_GuildPeace:
	case OP_GuildRemove:
	case OP_GuildMemberList:
	case OP_GuildMemberUpdate:
	case OP_GuildInvite:
	case OP_GuildPublicNote:
//	case OP_GetGuildMOTD:
	case OP_GuildDemote:
	case OP_GuildInviteAccept:
	case OP_GuildWar:
//	case OP_GuildUpdate:
	case OP_GuildDelete:
	case OP_GuildManageRemove:
	case OP_GuildManageAdd:
	case OP_GuildManageStatus:
	case OP_WhoAllResponse:
	case OP_Illusion:
		return(true);
	
	default:
		break;
	}
	return(false);
}

bool PrivacyPacketFileHandler::OpenFile() {
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

void PrivacyPacketFileHandler::ToClientPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p) {
	if(PrivateOpcode(emu_opcode))
		return;	//cant log this.
	
	const unsigned char *cbuf;
	unsigned char *buf = NULL;
	
	if(Privacy_Name.length() > 0) {
		buf = new unsigned char[p->size];
		memcpy(buf, p->pBuffer, p->size);
		BufferReplace((char *)buf, p->size, Privacy_Name.c_str(), _zero_buffer, Privacy_Name.length());
		cbuf = buf;
	} else {
		cbuf = p->pBuffer;
	}
	
	if(file != NULL)
		file->WritePacket(p->GetRawOpcode(), p->size, cbuf, false, p->timestamp);
	
	if(buf != NULL)
		safe_delete(buf);
	
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

void PrivacyPacketFileHandler::ToServerPacket(EQStreamType type, uint16 eq_opcode, EmuOpcode emu_opcode, const EQRawApplicationPacket *p) {
	if(PrivateOpcode(emu_opcode))
		return;	//cant log this.
	
	const unsigned char *cbuf;
	unsigned char *buf = NULL;
	
	if(Privacy_Name.length() > 0) {
		buf = new unsigned char[p->size];
		memcpy(buf, p->pBuffer, p->size);
		BufferReplace((char *)buf, p->size, Privacy_Name.c_str(), _zero_buffer, Privacy_Name.length());
		cbuf = buf;
	} else {
		cbuf = p->pBuffer;
	}
	
	if(file != NULL)
		file->WritePacket(p->GetRawOpcode(), p->size, cbuf, true, p->timestamp);
	
	if(buf != NULL)
		safe_delete(buf);
}

StreamPacketHandler *PrivacyPacketFileCreateHandler(EQStreamType type, const EQStreamPair *sp) {
	if(!sp->IsZoneStream())
		return(NULL);	//not interested
	
	PrivacyPacketFileHandler *pfh = new PrivacyPacketFileHandler(sp);
	if(!pfh->OpenFile()) {
		safe_delete(pfh);
		return(NULL);
	}
	
	return(pfh);
}

#ifdef PCGUI

void SetupPrivacyPacketFileCollector(HandlerCallbacks *calls, const char *privacy_nam) {
	Privacy_Name = privacy_nam;
	calls->AddStreamCreateHandler(PrivacyPacketFileCreateHandler);
}

#else

DLLFUNC int on_load(const HandlerCallbacks *calls, const char *arg)
{
	Privacy_Name = arg;
	calls->AddStreamCreateHandler(PrivacyPacketFileCreateHandler);
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

void WriteClientPacket(const EQStreamPair *sp, const EQRawApplicationPacket *p)
{
	if(pfm == NULL)
		return;	//not initilized
	
	PacketFileWriter *out = pfm.FindFile(sp);
	if(out == NULL)
		return;		//unable to find a packet file for this stream
	
	out->WritePacket(p->opcode, p->size, p->pBuffer, false);
}

void WriteServerPacket(const EQStreamPair *sp, const EQRawApplicationPacket *p)
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
