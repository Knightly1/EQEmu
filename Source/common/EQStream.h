#ifndef _EQPROTOCOL_H

#define _EQPROTOCOL_H

#include <string>
#include <vector>
#include <map>
#include <set>
#ifndef WIN32
#include <netinet/in.h>
#endif
#include "EQPacket.h"
#include "Mutex.h"
#include "../common/opcodemgr.h"
#include "../common/misc.h"
#include "../common/Condition.h"

using namespace std;

#define CLOSED 0
#define ESTABLISHED 1
#define CLOSING 2

#define FLAG_COMPRESSED	0x01
#define FLAG_ENCODED	0x04

#pragma pack(1)
struct SessionRequest {
	uint32 UnknownA;
	uint32 Session;
	uint32 MaxLength;
};

struct SessionResponse {
        uint32 Session;
	uint32 Key;
	uint8 UnknownA;
	uint8 Format;
	uint8 UnknownB;
	uint32 MaxLength;
	uint32 UnknownD;
};

//Deltas are in ms, representing round trip times
struct SessionStats {
/*000*/	uint16 RequestID;
/*002*/	uint32 last_local_delta;
/*006*/	uint32 average_delta;
/*010*/	uint32 low_delta;
/*014*/	uint32 high_delta;
/*018*/	uint32 last_remote_delta;
/*022*/	uint64 packets_sent;
/*030*/	uint64 packets_recieved;
/*038*/
};
	
#pragma pack()

class OpcodeManager;    
extern OpcodeManager *EQNetworkOpcodeManager;

class EQStreamFactory;

typedef enum {
	UnknownStream=0,
	LoginStream,
	WorldStream,
	ZoneStream,
	ChatOrMailStream,
	ChatStream,
	MailStream
} EQStreamType;

class EQStream {
	protected:
		uint32 remote_ip;
		uint16 remote_port;
		uint8 buffer[8192];
		unsigned char *oversize_buffer;
		uint32 oversize_offset,oversize_length;
		uint8 app_opcode_size;
		EQStreamType StreamType;
		bool compressed,encoded;

		//uint32 buffer_len;

		uint32 Session, Key;
		uint16 NextInSeq;
		uint16 NextOutSeq;
		uint32  MaxLen;
		uint16 MaxSends;

		bool in_use;
		Mutex MInUse;

		int State;
		Mutex MState;

		uint32 LastPacket;
		Mutex MVarlock;

		EQApplicationPacket* CombinedAppPacket;
		Mutex MCombinedAppPacket;

		long LastSeqSent;
		Mutex MLastSeqSent;
		void SetLastSeqSent(uint32);

		// Ack sequence tracking.
		long MaxAckReceived,NextAckToSend,LastAckSent;
		long GetMaxAckReceived();
		long GetNextAckToSend();
		long GetLastAckSent();
		void SetMaxAckReceived(uint32 seq);
		void SetNextAckToSend(uint32);
		void SetLastAckSent(uint32);

		Mutex MAcks;

		// Packets waiting to be sent
		vector<EQProtocolPacket *> NonSequencedQueue;
		map<uint16, EQProtocolPacket *> SequencedQueue;
		Mutex MOutboundQueue;

		// Packes waiting to be processed
		vector<EQApplicationPacket *> InboundQueue;
		Mutex MInboundQueue;

#ifdef COLLECTOR
		map<unsigned short,EQProtocolPacket *> PacketQueue;
#endif

		EQStreamFactory *Factory;

	public:
		EQStream() { init(); State=CLOSED; app_opcode_size=2; compressed=true; encoded=false; StreamType=UnknownStream; }
		EQStream(sockaddr_in addr) { init(); remote_ip=addr.sin_addr.s_addr; remote_port=addr.sin_port; State=CLOSED; app_opcode_size=2; compressed=true; encoded=false; StreamType=UnknownStream; }
		virtual ~EQStream() { RemoveData(); }
		inline void SetFactory(EQStreamFactory *f) { Factory=f; }
		inline void init() { Session=0; Key=0; MaxLen=0; NextInSeq=0; NextOutSeq=0; CombinedAppPacket=NULL; MaxAckReceived=-1;NextAckToSend=-1;LastAckSent=-1;LastSeqSent=-1;MaxSends=5;LastPacket=0; oversize_buffer=NULL, oversize_length=0; oversize_offset=0; }
		void SetMaxLen(uint32 length) { MaxLen=length; }

		void QueuePacket(const EQApplicationPacket *p, bool ack_req=true);
		void FastQueuePacket(EQApplicationPacket **p, bool ack_req=true);
		void FlushCombinedPacket();
		void SendPacket(EQApplicationPacket *p);
		void QueuePacket(EQProtocolPacket *p);
		void SendPacket(EQProtocolPacket *p);
		vector<EQProtocolPacket *> convert(EQApplicationPacket *p);
		void NonSequencedPush(EQProtocolPacket *p);
		void SequencedPush(EQProtocolPacket *p);
		void Write(int eq_fd);

		void WritePacket(int fd,EQProtocolPacket *p);

		uint32 GetKey() { return Key; }
		void SetKey(uint32 k) { Key=k; }
		void SetSession(uint32 s) { Session=s; }
		void SetLastPacketTime(uint32 t) {LastPacket=t;}

		void Process(const unsigned char *data, const uint32 length);
		void ProcessPacket(EQProtocolPacket *p);
		virtual void DispatchPacket(EQApplicationPacket *p) { p->DumpRaw(); }

		void SendSessionResponse();
		void SendSessionRequest();
		void SendDisconnect();
		void SendAck(uint16 seq);
		void SendOutOfOrderAck(uint16 seq);
		void SendSessionStatResponse(SessionStats *Stat);

		bool CheckTimeout(uint32 now, uint32 timeout=30) { return  (LastPacket && (now-LastPacket) > timeout); }
		bool Stale(uint32 now, uint32 timeout=30) { return  (LastPacket && (now-LastPacket) > timeout); }

		void InboundQueuePush(EQApplicationPacket *p);
		EQApplicationPacket *PopPacket(); // InboundQueuePop
		EQApplicationPacket *EQStream::PeekPacket();
		void InboundQueueClear();

		void OutboundQueueClear();
		bool HasOutgoingData();

		void RemoveData() { InboundQueueClear(); OutboundQueueClear(); if (CombinedAppPacket) delete CombinedAppPacket; }

		inline bool InUse() { bool flag; MInUse.lock(); flag=in_use; MInUse.unlock(); return flag; }
		inline void SetInUse(const bool flag) { MInUse.lock(); in_use=flag; MInUse.unlock(); }
		inline int GetState() { int s; MState.lock(); s=State; MState.unlock(); return s; }
		inline void SetState(int state) { MState.lock(); State=state; MState.unlock(); }

		inline uint32 GetRemoteIP() { return remote_ip; }
		inline uint32 GetrIP() { return remote_ip; }
		inline uint16 GetRemotePort() { return remote_port; }
		inline uint16 GetrPort() { return remote_port; }


		static EQProtocolPacket *Read(int eq_fd, sockaddr_in *from);

		void Free() { SetInUse(false); }
		void Close() { SendDisconnect(); }
		bool CheckActive() { return GetState()==ESTABLISHED; }
		void SetOpcodeSize(uint8 s) { app_opcode_size = s; }
		void SetStreamType(EQStreamType t);
		inline const EQStreamType GetStreamType() const { return StreamType; }
		static string EQStream::StreamTypeString(EQStreamType t);

#ifdef COLLECTOR
		void ProcessQueue();
#endif
};

#endif
