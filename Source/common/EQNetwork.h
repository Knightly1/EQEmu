/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2002  EQEMu Development Team (http://eqemu.org)

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#ifndef EQNETWORK_H
#define EQNETWORK_H

#include "../common/debug.h"

//uncomment this to enable the packet profiler. Counts the number
//of each type of packet sent or received on a connection.
#ifdef ZONE
//#define PACKET_PROFILER 1
#endif

#include <string.h>
#include <map>
#include <list>
#include <queue>
#include <string>
using namespace std;

#include "../common/types.h"
#include "../common/timer.h"
#include "../common/linked_list.h"
#include "../common/queue.h"
#include "../common/Mutex.h"
#include "../common/packet_functions.h"
#include "../common/opcodemgr.h"
#ifdef PACKET_PROFILER
#include "../common/rdtsc.h"
#endif

#define EQNC_TIMEOUT	60000
#define NAS_TIMER	100
#define KA_TIMER	400 /* keeps the lag bar constant */
#define MAX_HEADER_SIZE	39 // Quag: 39 is the max header + opcode + crc32 + unknowns size

#define COMBINE_MAX_PIECE 4096 //dont bother to combine a packet if its bigger than this, limit is 0xFFFF
#define COMBINE_PERIOD 200	//ms between each forced combined send
#define COMBINE_DEFLATE_SIZE 128

class EQNetworkServer;
class EQNetworkConnection;
class EQNetworkPacket;
class EQNetworkFragmentGroupList;
class EQNetworkFragmentGroup;
typedef EQNetworkServer				EQNServer;
typedef EQNetworkConnection			EQNConnection;
typedef EQNetworkPacket				EQNPacket;
typedef EQNetworkFragmentGroupList	EQNFragmentGroupList;
typedef EQNetworkFragmentGroup		EQNFragmentGroup;

class OpcodeManager;
extern OpcodeManager *EQNetworkOpcodeManager;

#define	FLAG_COMPRESSED 0x1000
#define	FLAG_COMBINED	0x2000
#define	FLAG_ENCRYPTED	0x4000
#define	FLAG_IMPLICIT	0x8000
#define FLAG_ALL		0xF000
#define StripFlags(x)	(x & ~FLAG_ALL)

// Optimistic compression, used for guessing pre-alloc size on debug output
#define BEST_COMPR_RATIO 300

enum eappCompressed { appNormal, appInflated, appDeflated };
class APPLAYER {
//I use friend to ensure these are the only people who know anything
//about real opcodes, without any doubt.
friend class EQNetworkConnection;
friend class EQDataPacket;
public:
	APPLAYER(EmuOpcode in_opcode = OP_Unknown, int32 in_size = 0) {
		size = in_size;
		if(in_opcode != 0)
			SetOpcode(in_opcode);
		else
			opcode = OP_Unknown;
		priority = 0;
		compressed = appNormal;
		if (size == 0) {
			pBuffer = NULL;

		}
		else {
			pBuffer = new uchar[size];
			memset(pBuffer, 0, size);
		}
		refCount = 1;
	}
	virtual ~APPLAYER() {
#if EQDEBUG >= 4
		if(refCount > 1) {
			LogFile->write(EQEMuLog::Debug, "Error: Packet with opcode 0x%.4x deleted with a refcount of %d", opcode, refCount);
		}
#endif
		safe_delete_array(pBuffer);
	}
	virtual APPLAYER* Copy() const {
		if (this == NULL) {
			return 0;
		}
		APPLAYER* ret;
		ret = new APPLAYER(OP_Unknown, this->size);
		ret->opcode = opcode;	//dont use set, or it will translate
		if (this->size)
			memcpy(ret->pBuffer, this->pBuffer, this->size);
		ret->priority = this->priority;
		ret->compressed = this->compressed;
		return ret;
	}
	bool Deflate() {
		if ((!this) || (!this->pBuffer) || (!this->size))
			return false;
		if (compressed != appNormal)
			return false;
		uchar* tmp = new uchar[this->size + 128];
		int32 tmpsize = DeflatePacket(this->pBuffer, this->size, tmp, this->size + 128);
		if (!tmpsize) {
			safe_delete_array(tmp);
			return false;
		}
		compressed = appDeflated;
		this->size = tmpsize;
		uchar* tmpdel = this->pBuffer;
		this->pBuffer = tmp;
		this->opcode |= FLAG_COMPRESSED;
		safe_delete(tmpdel);
		return true;
	}
	bool Inflate(int32 bufsize = 0) {
		if (!(this->opcode & FLAG_COMPRESSED))
			return true;
		if ((!this) || (!this->pBuffer) || (!this->size))
			return false;
		if (compressed != appNormal)
			return false;
		if (!bufsize){
			bufsize = this->size * 40;
		}
		uchar* tmp = new uchar[bufsize];
		int32 tmpsize = InflatePacket(this->pBuffer, this->size, tmp, bufsize);
		if (!tmpsize) {
			safe_delete(tmp);
			return false;
		}
		compressed = appInflated;
		this->size = tmpsize;
		uchar* tmpdel = this->pBuffer;
		this->pBuffer = tmp;
		safe_delete(tmpdel);
		this->opcode &= ~FLAG_COMPRESSED;
		return true;
	}
	
protected:
	inline void SetRealOpcode(uint16 eq_op) { opcode = eq_op; }
	uint16  opcode;
public:
	int32  size;
	uchar* pBuffer;
	int8 priority;
	eappCompressed compressed;
	
	#ifdef PACKETCOLLECTOR
		uint32 from_ip,to_ip;
		bool to_server;
		sint64* encrypt_key;
	#endif
	
	
	//these routines take and return emu opcodes, they should never
	//be used by somebody who knows or wants real EQ opcodes
	inline void SetOpcode(EmuOpcode emu_op) {
		
		//preserve flags on set... kill this when flags disappear
		
		uint16 flags = emu_op & FLAG_ALL;
		opcode = EQNetworkOpcodeManager->EmuToEQ((EmuOpcode)(StripFlags(emu_op)));
		
#if EQDEBUG >= 4
		if(opcode == OP_Unknown) {
			LogFile->write(EQEMuLog::Debug, "Unable to convert Emu opcode %s (%d) into an EQ opcode.", OpcodeNames[emu_op], emu_op);
		}
#endif
		
		opcode |= flags;
	}
	
	inline const EmuOpcode GetOpcode() const {
		EmuOpcode emu_op;
		emu_op = EQNetworkOpcodeManager->EQToEmu(StripFlags(opcode));
#if EQDEBUG >= 4
		if(emu_op == OP_Unknown) {
			LogFile->write(EQEMuLog::Debug, "Unable to convert EQ opcode 0x%.4x to an emu opcode.", opcode);
		}
#endif
		return(emu_op);
	}
	
	void PacketReferenced() {
		refCount++;
	}
	
	//decrement the reference count, delete if it hits 0
	static void PacketUsed(APPLAYER **it_p) {
		APPLAYER* it = *it_p;
		if(it == NULL)
			return;
		it->refCount--;

#if EQDEBUG >= 4
		if(it->refCount < 0)
			LogFile->write(EQEMuLog::Debug, "Error: Packet with opcode 0x%.4x got to a negative refcount of %d", it->opcode, it->refCount);
		else
#endif
		if(it->refCount == 0)
			safe_delete(*it_p);
	}
protected:
	sint32 refCount;	//number of references to this packet.
};

#ifdef COMBINED
class CombinedAPPLAYER : public APPLAYER {
public:
	
	CombinedAPPLAYER(EmuOpcode in_opcode = OP_Unknown, int32 in_size = 0) : APPLAYER(in_opcode, in_size) {
		combined_size = 0;
	}
	
	virtual APPLAYER* Copy() const {
		if (this == NULL) {
			return 0;
		}
		CombinedAPPLAYER* ret;
		if(combined_size > 0) {	//we are a combined packet
			ret = new CombinedAPPLAYER(OP_Unknown, 0);
			ret->opcode = opcode;	//dont use set, or it will translate
			ret->pBuffer = new uchar[this->combined_size];
			memcpy(ret->pBuffer, this->pBuffer, this->combined_size);
			ret->combined_size = this->combined_size;
		} else {	//not combined... yet
			ret = new CombinedAPPLAYER(OP_Unknown, this->size);
			ret->opcode = opcode;	//dont use set, or it will translate
			if (this->size)
				memcpy(ret->pBuffer, this->pBuffer, this->size);
		}
		ret->priority = this->priority;
		ret->compressed = this->compressed;
		return ret;
	}
	
	//quick wrapper cause c++ dosent like to cast this
	static void PacketUsed(CombinedAPPLAYER **it_p) {
		APPLAYER *app = *it_p;
		APPLAYER::PacketUsed(&app);
		if(app == NULL)
			*it_p = NULL;
	}
	
	
	
	//all the combined code should leave the size of the FIRST packet
	//in the combined packet in the size field, and store the total
	//length of the packet in combined_size
	int32  combined_size;
	//this is the offset into the buffer where the last opcode
	//was put, so we can flag it combined if another comes in.
	int32  opcode_offset;
	
	//this is used for implicit length stuff, simple resize
	void combine_append(int32 add_size, uchar *data, bool implicit);
	
	//this handles initial packet conditions as well as
	//manages the lengths and flags embedded in the combined packet.
	void combine_add(int16 in_opcode, int32 in_size, uchar *data);
};
#endif

    /************ Contain ack stuff ************/
struct ACK_INFO {
	ACK_INFO() { dwARQ = 0; dbASQ_high = dbASQ_low = 0; dwGSQ = 0; }

	int16   dwARQ; //Current request ack
	int16   dbASQ_high : 8,
			dbASQ_low  : 8;
	int16   dwGSQ; //Main sequence number SHORT#2
};

#define ACK_ONLY_SIZE   6
#define ACK_ONLY    0x400

/************ PACKETS ************/
struct EQPACKET_HDR_INFO {
    int8    
        a0_Unknown  :   1,
        a1_ARQ      :   1,
        a2_Closing  :   1,
        a3_Fragment :   1,

        a4_ASQ      :   1,
        a5_SEQStart :   1,
        a6_Closing  :   1,
        a7_SEQEnd   :   1;
    
    int8
        b0_SpecARQ  :   1,
        b1_Unknown  :   1,
        b2_ARSP     :   1,
        b3_Unknown  :   1,

        b4_Unknown  :   1,
        b5_Unknown  :   1,
        b6_Unknown  :   1,
        b7_Unknown  :   1;
};

struct FRAGMENT_INFO {
    int16 dwSeq;
    int16 dwCurr;
    int16 dwTotal;
};

struct InQueue_Struct {
	APPLAYER* app;
	bool ackreq;
};

#ifndef DEF_eConnectionType
#define DEF_eConnectionType
enum eConnectionType {Incomming, Outgoing};
#endif 

#ifdef WIN32
	void EQNetworkServerLoop(void* tmp);
	void EQNetworkConnectionInLoop(void* tmp);
	void EQNetworkConnectionOutLoop(void* tmp);
#else
	void* EQNetworkServerLoop(void* tmp);
	void* EQNetworkConnectionInLoop(void* tmp);
	void* EQNetworkConnectionOutLoop(void* tmp);
#endif


class EQNetworkServer {
public:
	EQNetworkServer(int16 iPort = 0);
	virtual ~EQNetworkServer();

	bool	Open(int16 iPort = 0);			// opens the port
	void	Close();	// closes the port
	void	KillAll();						// kills all clients
	inline int16	GetPort()		{ return pPort; }

	EQNetworkConnection* NewQueuePop();
protected:
#ifdef WIN32
	friend void EQNetworkServerLoop(void* tmp);
#else
	friend void* EQNetworkServerLoop(void* tmp);
#endif
	void		Process();
	bool		IsOpen();
	void		SetOpen(bool iOpen);
	bool		RunLoop;
	Mutex		MLoopRunning;
private:
	void		RecvData(uchar* data, int32 size, int32 irIP, int16 irPort);
#ifdef WIN32
	SOCKET	sock;
#else
	int		sock;
#endif
	int16	pPort;
	bool	pOpen;
	Mutex	MNewQueue;
	Mutex	MOpen;
	int		send_ticks;

	map<string, EQNetworkConnection *> connection_list;
	queue<EQNetworkConnection *>		NewQueue;
};

class EQNetworkFragmentGroupList {
public:
	EQNetworkFragmentGroupList();
	virtual ~EQNetworkFragmentGroupList();

	EQNetworkFragmentGroup*	GotPacket(EQNetworkPacket* pack);
	EQNetworkFragmentGroup*	FindGroup(int16 fragseq);
	void					DeleteGroup(int16 fragseq);
	void					CheckTimers();
private:
	LinkedList<EQNetworkFragmentGroup*>	list;
	Timer* timeout_check_timer;
};

class EQConnectionState {
public:
	EQConnectionState() {
		decode_key = 0x0;
	}
	virtual inline int32 GetrIP()	{ return 0; }
	virtual inline int16 GetrPort()	{ return 0; }
	virtual inline int32 GetlIP()	{ return 0; }
	virtual inline int16 GetlPort()	{ return 0; }
protected:
	friend class EQDataPacket;
	sint64	decode_key;
private:
};

/*  Changes
 *  #		Name		Date	Changes
 *  1		?		1-18-04 None, first
 *  2		David Alighieri	1-18-04	Added private members void AddToCombined(APPLAYER) and APPLAYER* CombinedPacket;	
 *
 */
#define EQNC_Init		0		// Just made, waiting for orders
#define EQNC_Active		100		// Up and running
#define	EQNC_Closing	101		// Received closing bits, about to die
#define	EQNC_Closed		102		// Closing bits sent, no more packets may be added to the queue
#define	EQNC_Finished	150		// Shutdown was graceful, last packets sent, ready to be deleted
#define EQNC_Error		250		// Error has occured, no longer functional (delete when free)
class EQNetworkConnection : public EQConnectionState {
public:
	EQNetworkConnection(int32 irIP, int16 irPort);
	EQNetworkConnection();				// for outgoing connections
	virtual ~EQNetworkConnection();

	// Functions for outgoing connections
	bool			OpenSock(char* irAddress, int16 irPort, char* errbuf = 0);
	bool			OpenSock(int32 irIP, int16 irPort);
	void			CloseSock();

	void			QueuePacket(const APPLAYER* app, bool ackreq = true);	// InQueuePush()
	void			PacketQueueFlush(APPLAYER* app);						// QueueFlush() on zone
	
	void			FastQueuePacket(APPLAYER** app, bool ackreq = true);	// Faster than QueuePacket, but you cant call twice on the same app
	APPLAYER*		PopPacket(); // OutQueuePop()
	inline int32	GetrIP()		{ return rIP; }
	inline int16	GetrPort()		{ return rPort; }
	int8			GetState();
	bool			CheckActive();
	void			Close();
	void			Free();		// Inform EQNetworkServer that this connection object is no longer referanced
	void			RemoveData();

	void			SetDataRate(float in_datarate)	{ datarate_sec = (sint32) (in_datarate * 1024); datarate_tic = datarate_sec / 10; dataflow = 0; datahigh = 0; } // conversion from kb/sec to byte/100ms, byte/1000ms
	float			GetDataRate()					{ return (float)datarate_sec / 1024; } // conversion back to kb/second
	inline bool		DataQueueFull()					{ return (dataflow > datarate_sec); }
	inline sint32	GetDataFlow()					{ return dataflow; }
	inline sint32	GetDataHigh()					{ return datahigh; }
	void RemovePacket(EQNetworkPacket* pack);
	void			RecvData(uchar* data, int32 size);
	void			OutQueuePush(APPLAYER* app);
#ifdef COMBINED
	void			EnableCombining() { combined_timer.Enable(); }
#endif

#ifdef PACKET_PROFILER
	void DumpPacketProfile();
#endif

protected:
	friend class EQNetworkServer;
#ifdef WIN32
	void			Process(SOCKET sock);
#else
	void			Process(int sock);
#endif
	void			SetState(int8 iState);
	inline bool		IsMine(int32 irIP, int16 irPort) { return (rIP == irIP && rPort == irPort); }
	inline bool		IsFree() { return pFree; }
	bool			CheckNetActive();

	friend class EQNetworkPacket;
	ACK_INFO		CACK;
	ACK_INFO		SACK;
	Timer*			no_ack_sent_timer;

#ifdef WIN32
	friend void EQNetworkConnectionInLoop(void* tmp);
	friend void EQNetworkConnectionOutLoop(void* tmp);
	SOCKET			outsock;
#else
	friend void* EQNetworkConnectionInLoop(void* tmp);
	friend void* EQNetworkConnectionOutLoop(void* tmp);
	int				outsock;
#endif
	bool			RunLoop;
	Mutex			MLoopRunning;
	Mutex			MSocketLock;
	void			DoRecvData();

private:
	eConnectionType	ConnectionType;
	bool			ProcessPacket(EQNetworkPacket* pack, bool from_buffer);
	void			IncomingARSP(int16 arsp);
	void			IncomingARQ(int16 arq);
	void			CheckBufferedPackets();

	void			MakeEQPacket(APPLAYER* app, bool ackreq = true);
	InQueue_Struct*	InQueuePop();
	
#ifdef COMBINED
	bool AddToCombined(APPLAYER* app);
	void SendCombinedPackets();
	void SendACombinedPacket(CombinedAPPLAYER* app);
	CombinedAPPLAYER *CombinedPacket;
	CombinedAPPLAYER *ImplicitCombinedPacket;
	uint32 implicit_counter_offset;
	uint16 last_opcode;
	Timer combined_timer;
	Mutex MCombined;
#endif
	
	int32	rIP;
	int16	rPort; // network byte order

	sint32	dataflow;		// incremented by bytes sent with every outgoing packet, decremented by datarate_tic every 100ms
	sint32	datahigh;		// used to delay resends if dataflow has been near max
	int8	pState;
	bool	pFree;
	EQNetworkFragmentGroupList		fraglist;
	MyQueue<InQueue_Struct>			InQueue;
	MyQueue<APPLAYER>				OutQueue;
	LinkedList<EQNetworkPacket*>	SendQueue;
	LinkedList<EQNetworkPacket*>	BufferedPackets;
	Mutex	MInQueueLock;
	Mutex	MOutQueueLock;
	Mutex	MStateLock;

    	int16	dwFragSeq;   //current fragseq
	int16	dwLastCACK;

	Timer*	keep_alive_timer;
	Timer*	timeout_timer;
	Timer*	datarate_timer;
	Timer*  datakeepalive_timer;
	Timer*  queue_check_timer;

	sint32	datarate_sec;	// bytes/1000ms
	sint32	datarate_tic;	// bytes/100ms
	
#ifdef PACKET_PROFILER
	typedef struct {
		uint32 count;
		uint32 lengthSum;
	} _ppData;
	map<uint16, _ppData> _packetProfileIn;	//opcode -> data
	map<uint16, _ppData> _packetProfileOut;	//opcode -> data
	RDTSC_Timer _packetProfileTimer;
#endif
};

class EQNetworkPacket {
public:
	EQNetworkPacket(EQNetworkConnection* inetcon);
	virtual ~EQNetworkPacket();
	
    bool			DecodePacket(uchar* data, int32 size);
	void			AddData(uchar* data, int32 size);
    int32			ReturnPacket(uchar** data);
	void			Clear();
	
    EQPACKET_HDR_INFO	HDR;
	
	int16				dwSEQ;			// Only used on incomming packets
    int16				dwARSP;			// Only used on incomming packets
    int16				dwARQ;
    int16				dbASQ_high	: 8,
						dbASQ_low	: 8;
    int16				dwOpCode;		//Not all packet have opcodes. 
	
    FRAGMENT_INFO		fraginfo;       //Fragment info
    int32				dwExtraSize;	//Size of additional info.
    uchar*				pExtra;			//Additional information
	int32				LastSent;
	int8				SentCount;
	int8				priority;
private:
	EQNetworkConnection* netcon;
};

class EQNetworkFragmentGroup {
public:
	EQNetworkFragmentGroup(int16 ifragseq, int16 inum_fragments);
	virtual ~EQNetworkFragmentGroup();

	void			Add(EQNetworkPacket* pack);
	bool			Ready();
	int32			Assemble(uchar** data);

	inline int16	GetFragSeq()		{ return fragseq; }
protected:
	friend class EQNetworkFragmentGroupList;
	Timer*		timeout_timer;
private:
	int16		fragseq;		//Sequence number
	int16		num_fragments;
	uchar**		fragments;
	int32*		fragment_sizes;
};

class EQDataPacket {
public:
	EQDataPacket(EQConnectionState* iCS);
	virtual ~EQDataPacket();
	
	// Unwravel function for FLAG_IMPLICIT
	static	uint32 implicitlen(uint16 opcode);
	
	bool	Decode(sint64* key, int8* buf, sint32 buflen);
	bool	Decode(sint64* key, EQNetworkFragmentGroup* fg);
	APPLAYER* GetApp();
private:
	bool	Decode(sint64* key, int16 opCode, int8* buf, sint32 buflen);
	bool	DecryptPacket(int8* buf, sint32 buflen);
	
	MyQueue<APPLAYER> OutQueue;
	EQConnectionState* pCS;
};

#endif
