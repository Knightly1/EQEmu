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
/* 
 * EQNetwork classes, by Quagmire
*/

#define RECV_GRANULATIRY 5	//in ms (sleep time between checks)
#define SEND_WAIT 10		//preform send operations every ## recv loops
#define NET_COMPRESS_SIZE 256	//packets longer than this get compressed

#define DATA_RESEND_DELAY_IDLE 300	//ms to wait before resend when the connection is somewhat idle
#define DATA_RESEND_DELAY 500		//ms to wait before resend under any load

#include "../common/debug.h"

#include <iostream>
#include <iomanip>
#ifdef WIN32
	#include <process.h>
#else
	#include <arpa/inet.h>
	#include <unistd.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <fcntl.h>
	#include <stdlib.h>
	#include <string.h>
	#include <pthread.h>
	#include "../common/unix.h"
	#define SOCKET_ERROR -1
#endif
#include "EQNetwork.h"
#include "../common/packet_dump.h"
#include "../common/packet_dump_file.h"
#include "../common/packet_functions.h"
#include "../common/MiscFunctions.h"
#include "../common/crc32.h"
#include "../common/eq_packet_structs.h"
#include "opcodemgr.h"

//an opcode manager must be assigned to this variable for any network stuff to work
OpcodeManager *EQNetworkOpcodeManager = NULL;

using namespace std;

#define EQN_DEBUG			0
#define EQN_DEBUG_Error		0
#define EQN_DEBUG_Packet	0
#define EQN_DEBUG_Fragment	0
#define EQN_DEBUG_ACK		0
#define EQN_DEBUG_Unknown	0
#define EQN_DEBUG_NewStream	0
#define LOG_PACKETS			0
#define LOG_RAW_PACKETS_OUT	0
#define LOG_RAW_PACKETS_IN	0
//#define PRIORITYTEST

template <typename type>                    // LO_BYTE
type  LO_BYTE (type a) {return (a&=0xff);}  
template <typename type>                    // HI_BYTE 
type  HI_BYTE (type a) {return (a&=0xff00);} 
template <typename type>                    // LO_WORD
type  LO_WORD (type a) {return (a&=0xffff);}  
template <typename type>                    // HI_WORD 
type  HI_WORD (type a) {return (a&=0xffff0000);} 
template <typename type>                    // HI_LOSWAPshort
type  HI_LOSWAPshort (type a) {return (LO_BYTE(a)<<8) | (HI_BYTE(a)>>8);}  
template <typename type>                    // HI_LOSWAPlong
type  HI_LOSWAPlong (type x) {return (LO_WORD(a)<<16) | (HIWORD(a)>>16);}  

EQNetworkServer::EQNetworkServer(int16 iPort)
{
	RunLoop = false;
	pPort = iPort;
	pOpen = false;
#ifdef WIN32
	WORD version = MAKEWORD (1,1);
	WSADATA wsadata;
	WSAStartup (version, &wsadata);
#endif
	sock = 0;
	send_ticks = 0;
}

EQNetworkServer::~EQNetworkServer() {
	Close();
	RunLoop = false;
	MLoopRunning.lock();
	MLoopRunning.unlock();
#ifdef WIN32
	WSACleanup();
#endif
	connection_list.clear();
	while (!NewQueue.empty())
		NewQueue.pop(); // they're deleted with the list, clear this queue so it doesnt try to delete them again
}

bool EQNetworkServer::Open(int16 iPort) {
	LockMutex lock(&MOpen);
	if (iPort && pPort != iPort) {
		if (pOpen)
			return false;
		pPort = iPort;
	}
	if (!RunLoop) {
		RunLoop = true;
#ifdef WIN32
		_beginthread(EQNetworkServerLoop, 0, this);
#else
		pthread_t thread;
		pthread_create(&thread, NULL, &EQNetworkServerLoop, this);
#endif
	}
	if (pOpen) {
		return true;
	}
	else {
		struct sockaddr_in address;
//		int reuse_addr = 1;
		int bufsize = 64 * 1024; // 64kbyte send/recieve buffers, up from default of 8k
#ifdef WIN32
		unsigned long nonblocking = 1;
#endif

	  /* Setup internet address information.  
		 This is used with the bind() call */
		memset((char *) &address, 0, sizeof(address));
		address.sin_family = AF_INET;
		address.sin_port = htons(pPort);
		address.sin_addr.s_addr = htonl(INADDR_ANY);

		/* Setting up UDP port for new clients */
		sock = socket(AF_INET, SOCK_DGRAM, 0);
		if (sock < 0) {
			return false;
		}

//#ifdef WIN32
//		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (char *) &reuse_addr, sizeof(reuse_addr));
		setsockopt(sock, SOL_SOCKET, SO_RCVBUF, (char*) &bufsize, sizeof(bufsize));
		setsockopt(sock, SOL_SOCKET, SO_SNDBUF, (char*) &bufsize, sizeof(bufsize));
//#else
//		setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &reuse_addr, sizeof(reuse_addr));
//		setsockopt(sock, SOL_SOCKET, SO_RCVBUF, &bufsize, sizeof(bufsize));
//		setsockopt(sock, SOL_SOCKET, SO_SNDBUF, &bufsize, sizeof(bufsize));
//#endif

		if (bind(sock, (struct sockaddr *) &address, sizeof(address)) < 0) {
#ifdef WIN32
			closesocket(sock);
#else
			close(sock);
#endif
			return false;
		}

#ifdef WIN32
		ioctlsocket (sock, FIONBIO, &nonblocking);
#else
		fcntl(sock, F_SETFL, O_NONBLOCK);
#endif
		pOpen = true;
		return true;
	}
}

void EQNetworkServer::Close() {
	SetOpen(false);
	if (sock) {
#ifdef WIN32
		closesocket(sock);
#else
		close(sock);
#endif
	}
	sock = 0;
}

bool EQNetworkServer::IsOpen() {
	MOpen.lock();
	bool ret = pOpen;
	MOpen.unlock();
	return ret;
}

void EQNetworkServer::SetOpen(bool iOpen) {
	MOpen.lock();
	pOpen = iOpen;
	MOpen.unlock();
}

void EQNetworkServer::Process() {
	_CP(EQNetworkServer_Process);
	if (!IsOpen()) {
		if (sock) {
#ifdef WIN32
			closesocket(sock);
#else
			close(sock);
#endif
			sock = 0;
		}
		return;
	}

    uchar		buffer[1518];
	
    int			status;
    struct sockaddr_in	from;
    unsigned int	fromlen;

    from.sin_family = AF_INET;
    fromlen = sizeof(from);
	
	//receive any packets waiting in our in queue
	while (1) {
#ifdef WIN32
		status = recvfrom(sock, (char *) buffer, sizeof(buffer), 0,(struct sockaddr*) &from, (int *) &fromlen);
#else
		status = recvfrom(sock, buffer, sizeof(buffer), 0,(struct sockaddr*) &from, &fromlen);
#endif
		if (status >= 1) {
			RecvData(buffer, status, from.sin_addr.s_addr, from.sin_port);
		}
		else {
			break;
		}
	}
	
	//watch our tic timer to see if we wanna try to send stuff yet
	if(send_ticks < SEND_WAIT) {
		send_ticks++;
		return;
	}
	send_ticks = 0;
	
	//give connections a chance to do something.
	map <string, EQNetworkConnection*>::iterator connection;
	for (connection = connection_list.begin( ); connection != connection_list.end( );)
	{
		//clean up NULL connections
		if(!connection->second)
		{
			map <string, EQNetworkConnection*>::iterator tmp=connection;
			connection++;
			connection_list.erase(tmp);
			continue;
		}
		EQNetworkConnection* eqnc_data = connection->second; 
		//watch for free or inactive connections
		if (eqnc_data->IsFree() && (!eqnc_data->CheckNetActive())) { 
			map <string, EQNetworkConnection*>::iterator tmp=connection;
			connection++;
			safe_delete(eqnc_data);
			connection_list.erase(tmp);
		}
		//else, if the connection is not running its own loop thread
		//allow it to process here...
		else if(!eqnc_data->RunLoop) {
			eqnc_data->Process(sock);
			connection++;
		}
	}
}

void EQNetworkServer::RecvData(uchar* data, int32 size, int32 irIP, int16 irPort) {
/*
	CHANGE HISTORY

	Version		Author		Date		Comment
	1			Unknown		Unknown		Initial Revision
	2			Joolz		05-Jan-2003	Optimised
	3			Quagmire	05-Feb-2003	Changed so 2 connection objects wouldnt be created for the same ip/port pair, often happened
*/

	// Check for invalid data
	if (!data || size <= 4) return;
	if (CRC32::Generate(data, size-4) != ntohl(*((int32*) &data[size-4]))) {
#if EQN_DEBUG_Error >= 1
		cout << "Incomming Packet failed checksum" << endl;
#endif
		return;
	}

	char temp[25];
	sprintf(temp,"%lu:%d",irIP,irPort);
	EQNetworkConnection* tmp = NULL;
	map <string, EQNetworkConnection*>::iterator connection;
	if ((connection=connection_list.find(temp))!=connection_list.end())
		tmp=connection->second;
	if(tmp != NULL && tmp->GetrPort() == irPort)
	{
		tmp->RecvData(data, size);
		return;
	}
	else if(tmp != NULL  && tmp->GetrPort() != irPort)
	{
		printf("Conflicting IPs & Ports: IP %i and Port %i is conflicting with IP %i and Port %i\n",irIP,irPort,tmp->GetrIP(),tmp->GetrPort());
		return;
	}

	if (data[0] & 0x20) { // check for SEQStart
#if EQN_DEBUG >= 4
		cout << "New EQNetwork Connection." << endl;
#endif
		EQNetworkConnection* tmp = new EQNetworkConnection(irIP, irPort);
		tmp->RecvData(data, size);
		connection_list[temp]=tmp;
		if (connection_list.find(temp)==connection_list.end()) {
			cerr <<"Could not find new connection we just added!" << endl;
		}
		MNewQueue.lock();
		NewQueue.push(tmp);
		MNewQueue.unlock();
		return;
	}
#if EQN_DEBUG >= 4
	struct in_addr	in;
	in.s_addr = irIP;
	cout << "WARNING: Stray packet? " << inet_ntoa(in) << ":" << irPort << endl;
#endif
}

EQNetworkConnection* EQNetworkServer::NewQueuePop() {
	EQNetworkConnection* ret = 0;
	MNewQueue.lock();
	if (!NewQueue.empty()) {
		ret = NewQueue.front();
		NewQueue.pop();
	}
	MNewQueue.unlock();
	return ret;
}

#ifdef WIN32
	void EQNetworkServerLoop(void* tmp)
#else
	void* EQNetworkServerLoop(void* tmp)
#endif
{
#ifdef WIN32
	SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#endif
	EQNetworkServer* eqns = (EQNetworkServer*) tmp;
	eqns->MLoopRunning.lock();
	while (eqns->RunLoop) {
		{
			_CP(EQNetworkServerLoop);
			eqns->Process();
		}
		Sleep(RECV_GRANULATIRY);
	}
	eqns->MLoopRunning.unlock();
#ifdef WIN32
	_endthread();
#else
	return 0;
#endif
}

#ifdef WIN32
	void EQNetworkConnectionInLoop(void* tmp)
#else
	void* EQNetworkConnectionInLoop(void* tmp)
#endif
{
	EQNetworkConnection* eqnc = (EQNetworkConnection*) tmp;
#ifdef _DEBUG
	if (eqnc->ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnectionInLoop: eqnc->ConnectionType != Outgoing");
	}
#endif
	eqnc->MLoopRunning.lock();
//	Timer* tmp_timer = new Timer(100);
//	tmp_timer->Start();
	while (eqnc->RunLoop) {
		{
			_CP(EQNetworkConnectionInLoop);
//			if(tmp_timer->Check())
				eqnc->DoRecvData();
		}
		Sleep(100);
	}
//	safe_delete(tmp_timer);
	eqnc->MLoopRunning.unlock();
#ifdef WIN32
	_endthread();
#else
	return 0;
#endif
}

#ifdef WIN32
	void EQNetworkConnectionOutLoop(void* tmp)
#else
	void* EQNetworkConnectionOutLoop(void* tmp)
#endif
{
	EQNetworkConnection* eqnc = (EQNetworkConnection*) tmp;
#ifdef _DEBUG
	if (eqnc->ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnectionOutLoop: eqnc->ConnectionType != Outgoing");
	}
#endif
	eqnc->MLoopRunning.lock();
//	Timer* tmp_timer = new Timer(100);
//	tmp_timer->Start();
	while (eqnc->RunLoop) {
		{
			_CP(EQNetworkConnectionOutLoop);
//			if(tmp_timer->Check())
				eqnc->Process(eqnc->outsock);
		}
		Sleep(100);
	}
//	safe_delete(tmp_timer);
	eqnc->MLoopRunning.unlock();
#ifdef WIN32
	_endthread();
#else
	return 0;
#endif
}
	
EQNetworkConnection::EQNetworkConnection(int32 irIP, int16 irPort)
#ifdef COMBINED
: combined_timer(COMBINE_PERIOD)
#endif
 {
	ConnectionType = Incomming;
	rIP = irIP;
	rPort = irPort;
	dataflow = 0;
	pState = EQNC_Init;
	pFree = false;
	SACK.dwGSQ = 0;
    	dwFragSeq = 0;
	outsock = 0;
	RunLoop = false;

	timeout_timer = new Timer(EQNC_TIMEOUT);
	no_ack_sent_timer = new Timer(NAS_TIMER);
    	no_ack_sent_timer->Disable();
	keep_alive_timer = new Timer(KA_TIMER);
	datarate_timer = new Timer(100, true);
//	datakeepalive_timer = new Timer(1000);
#ifdef COMBINED
	ImplicitCombinedPacket = NULL;
	CombinedPacket = NULL;
	last_opcode = 0;
	implicit_counter_offset = 0;
	combined_timer.Disable();
#endif
	SetDataRate(500);
	if (rIP && rPort)
		pState = EQNC_Active;
#ifdef PACKET_PROFILER
	_packetProfileTimer.start();
#endif
}

EQNetworkConnection::EQNetworkConnection() 
#ifdef COMBINED
: combined_timer(COMBINE_PERIOD)
#endif
{
#ifdef WIN32
	WORD version = MAKEWORD (1,1);
	WSADATA wsadata;
	WSAStartup (version, &wsadata);
#endif
	ConnectionType = Outgoing;
	rIP = 0;
	rPort = 0;
	dataflow = 0;
	pState = EQNC_Init;
	pFree = false;
	SACK.dwGSQ = 0;
    	dwFragSeq = 0;
	outsock = 0;
	RunLoop = false;

	timeout_timer = new Timer(EQNC_TIMEOUT);
	no_ack_sent_timer = new Timer(NAS_TIMER);
    	no_ack_sent_timer->Disable();
	keep_alive_timer = new Timer(KA_TIMER);
	queue_check_timer = new Timer(2000);
	datarate_timer = new Timer(100, true);
#ifdef COMBINED
	ImplicitCombinedPacket = NULL;
	CombinedPacket = NULL;
	last_opcode = 0;
	implicit_counter_offset = 0;
	combined_timer.Disable();
#endif
	SetDataRate(500);
#ifdef PACKET_PROFILER
	_packetProfileTimer.start();
#endif
}

EQNetworkConnection::~EQNetworkConnection() {
#ifdef PACKET_PROFILER
	DumpPacketProfile();
#endif
	if (ConnectionType == Outgoing) {
		CloseSock();
		RunLoop = false;
		MLoopRunning.lock();
		MLoopRunning.unlock();
#ifdef WIN32
		WSACleanup();
#endif
	}
	InQueue_Struct* iqs = 0;
	while ((iqs = InQueue.pop())) {
		APPLAYER::PacketUsed(&iqs->app);
//		safe_delete(iqs->app);
		safe_delete(iqs);
	}
	APPLAYER* app = 0;
	while ((app = OutQueue.pop())) {
//		safe_delete(app);
		APPLAYER::PacketUsed(&app);
	}
#ifdef COMBINED
	safe_delete(CombinedPacket);
#endif
	
	safe_delete(timeout_timer);
	safe_delete(no_ack_sent_timer);
	safe_delete(keep_alive_timer);
	safe_delete(datarate_timer);
//	safe_delete(datakeepalive_timer);
}

#ifdef PACKET_PROFILER
void EQNetworkConnection::DumpPacketProfile() {
	
	_packetProfileTimer.stop();	//yes, this can be stopped multiple times
	float ttime = _packetProfileTimer.getDuration();
	
	struct in_addr ia;
	ia.s_addr = rIP;
	LogFile->write(EQEMuLog::Debug, "Packet profile for connection %s:%i", inet_ntoa(ia), ntohs(rPort));
	LogFile->write(EQEMuLog::Debug, "   Total connection lifetime: %.4f ms", ttime);
	uint32 ratio;
	
	ttime /= 1000.0f;	//convert to seconds
	
	float rate;
	map<uint16, _ppData>::iterator cur, stop;
	cur = _packetProfileIn.begin();
	stop = _packetProfileIn.end();
	LogFile->write(EQEMuLog::Debug, " Incoming Packets:");
	while(cur != stop) {
		const _ppData &pp = cur->second;
		ratio = pp.lengthSum / pp.count;
		rate = pp.count / ttime;
		LogFile->write(EQEMuLog::Debug, "   0x%.4x: count=%lu, length=%lu, avg. length=%lu, rate=%.2f/s", cur->first, pp.count, pp.lengthSum, ratio, rate);
		cur++;
	}
	
	cur = _packetProfileOut.begin();
	stop = _packetProfileOut.end();
	LogFile->write(EQEMuLog::Debug, " Outgoing Packets:");
	while(cur != stop) {
		const _ppData &pp = cur->second;
		ratio = pp.lengthSum / pp.count;
		rate = pp.count / ttime;
		LogFile->write(EQEMuLog::Debug, "   0x%.4x: count=%lu, length=%lu, avg. length=%lu, rate=%.2f/s", cur->first, pp.count, pp.lengthSum, ratio, rate);
		cur++;
	}
	
	//count lengths of all our queues, maybe it will help some day
	int inq_count = 0, outq_count = 0, sendq_count = 0, bp_count = 0;
	MInQueueLock.lock();
	inq_count = InQueue.count();
	MInQueueLock.unlock();
	MOutQueueLock.lock();
	inq_count = OutQueue.count();
	MOutQueueLock.unlock();
	
	LinkedListIterator<EQNetworkPacket*> iterator(SendQueue);
	iterator.Reset();
	if (iterator.MoreElements()) {
		sendq_count++;
		iterator.Advance();
	}
	
	
	LinkedListIterator<EQNetworkPacket*> iterator2(BufferedPackets);
	iterator2.Reset();
	while(iterator2.MoreElements()) {
		bp_count++;
		iterator2.Advance();
	}

	LogFile->write(EQEMuLog::Debug, " Packet Queues:");
	LogFile->write(EQEMuLog::Debug, "   In: %d, Out: %d, Send: %d, Buffered: %d", inq_count, outq_count, sendq_count, bp_count);

}
#endif

int8 EQNetworkConnection::GetState() {
	MStateLock.lock();
	int8 ret = pState;
	MStateLock.unlock();
	return ret;
}

bool EQNetworkConnection::CheckActive() {
	int8 tmp = GetState();
	return (tmp == EQNC_Active);
}

bool EQNetworkConnection::CheckNetActive() {
	int8 tmp = GetState();
	return (tmp == EQNC_Active || tmp == EQNC_Closing || tmp == EQNC_Closed);
}

void EQNetworkConnection::SetState(int8 iState) {
	MStateLock.lock();
	pState = iState;
	MStateLock.unlock();
}

void EQNetworkConnection::Free() {
#ifdef _DEBUG
	if (ConnectionType == Outgoing) {
		ThrowError("EQNetworkConnection::Free() called on an outgoing connection!");
	}
#endif
	if (CheckActive())
		Close();
	pFree = true;
}

APPLAYER* EQNetworkConnection::PopPacket() {
	APPLAYER* ret = 0;
	MOutQueueLock.lock();
	ret = OutQueue.pop();
	MOutQueueLock.unlock();
//if(ret) {
//printf("Queueing packet %s (0x%.4x) to server:\n", EQNetworkOpcodeManager->EQToName(ret->opcode), ret->opcode);
//DumpPacket(ret);
//}
	return ret;
}

void EQNetworkConnection::OutQueuePush(APPLAYER* app) {
	MOutQueueLock.lock();
	OutQueue.push(app);
	MOutQueueLock.unlock();
}

void EQNetworkConnection::QueuePacket(const APPLAYER* app, bool ackreq) {
	if (!app) {
		ThrowError("EQNetworkConnection::QueuePacket(): app = 0!");
	}
	APPLAYER* c = app->Copy();
	FastQueuePacket(&c, ackreq);
}

void EQNetworkConnection::FastQueuePacket(APPLAYER** app, bool ackreq) {
	if (!(*app)) {
		ThrowError("EQNetworkConnection::FastQueuePacket(): *app = 0!");
	}
//if(print_eqn_shit) {
//printf("Queueing %s (0x%.4x) to client:\n", EQNetworkOpcodeManager->EQToName((*app)->opcode), (*app)->opcode);
//DumpPacket(*app);
//}
	if (!CheckActive() || EQNetworkOpcodeManager == NULL) {
//		safe_delete(*app);
		APPLAYER::PacketUsed(app);
		return;
	}
	
	APPLAYER* thisapp = *app;
	
#if EQDEBUG >= 5
	if(StripFlags(thisapp->opcode) == 0) {
		printf("Packet with unresolvable opcode queued. size=%d\n", thisapp->size);
		APPLAYER::PacketUsed(app);
		return;
	}
#endif
//printf("Queueing packet %s (0x%.4x) to client:\n", EQNetworkOpcodeManager->EQToName(thisapp->opcode), thisapp->opcode);
//DumpPacket(thisapp);
	
	if((thisapp->opcode & FLAG_COMPRESSED) == 0 && (*app)->size > NET_COMPRESS_SIZE) {
		thisapp->Deflate();
	}
	
	InQueue_Struct* iqs = new InQueue_Struct;
	iqs->app = thisapp;
	*app = 0;
	iqs->ackreq = ackreq;
	MInQueueLock.lock();
	InQueue.push(iqs);
	MInQueueLock.unlock();
}

void EQNetworkConnection::RemoveData() {
	MOutQueueLock.lock();
	MInQueueLock.lock();
	OutQueue.clear();
	InQueue.clear();
	MOutQueueLock.unlock();
	MInQueueLock.unlock();
}

void EQNetworkConnection::Close() {
	if (GetState() != EQNC_Active)
		return;
	SetState(EQNC_Closing);
	InQueue_Struct* iqs = new InQueue_Struct;
	iqs->app = 0;
	iqs->ackreq = true;
	MInQueueLock.lock();
	InQueue.push(iqs);
	MInQueueLock.unlock();
}

bool EQNetworkConnection::OpenSock(char* irAddress, int16 irPort, char* errbuf) {
	if (ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnection::Connect() called on an incoming connection!");
		return false;
	}
	int32 tmp = ResolveIP(irAddress, errbuf);
	if (tmp == 0) {
		return false;
	}
	return OpenSock(tmp, irPort);
}

bool EQNetworkConnection::OpenSock(int32 irIP, int16 irPort) {
	if (ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnection::Connect() called on an incoming connection!");
		return false;
	}
	rIP = irIP;
	rPort = htons(irPort);
	if (!(rIP && rPort))
		return false;
	LockMutex lock(&MStateLock);
	if (pState != EQNC_Init)
		return false;

	LockMutex lock2(&MSocketLock);
	outsock = socket(AF_INET, SOCK_DGRAM, 0);
	if (outsock <= 0) {
		outsock = 0;
		return false;
	}

    struct sockaddr_in	server_sin;
	server_sin.sin_family = AF_INET;
	server_sin.sin_addr.s_addr = rIP;
	server_sin.sin_port = rPort;
#ifdef WIN32
	if (connect(outsock, (SOCKADDR*) &server_sin, sizeof (server_sin)) == SOCKET_ERROR) {
		closesocket(outsock);
		outsock = 0;
		return false;
	}
#else
	if (connect(outsock, (struct sockaddr *) &server_sin, sizeof (server_sin)) == SOCKET_ERROR) {
		close(outsock);
		outsock = 0;
		return false;
	}
#endif
#ifdef WIN32
	unsigned long nonblocking = 1;
	ioctlsocket(outsock, FIONBIO, &nonblocking);
#else
	fcntl(outsock, F_SETFL, O_NONBLOCK);
#endif

	if (!RunLoop) {
		RunLoop = true;
#ifdef WIN32
		_beginthread(EQNetworkConnectionInLoop, 0, this);
		_beginthread(EQNetworkConnectionOutLoop, 0, this);
#else
		pthread_t inthread;
		pthread_create(&inthread, NULL, &EQNetworkConnectionInLoop, this);

		pthread_t outthread;
		pthread_create(&outthread, NULL, &EQNetworkConnectionOutLoop, this);
#endif
	}
	pState = EQNC_Active;
	return true;
}

void EQNetworkConnection::CloseSock() {
	if (ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnection::Connect() called on an incoming connection!");
		return;
	}
	
	LockMutex lock(&MStateLock);
	if (pState == EQNC_Init)
		return;

	InQueue_Struct* iqs = 0;
	while ((iqs = InQueue.pop())) {
//		safe_delete(iqs->app);
		APPLAYER::PacketUsed(&iqs->app);
		delete iqs;
	}
	APPLAYER* app = 0;
	while ((app = OutQueue.pop())) {
//		safe_delete(app);
		APPLAYER::PacketUsed(&app);
	}
	LockMutex lock2(&MSocketLock);
	if (outsock) {
#ifdef WIN32
		closesocket(outsock);
#else
		close(outsock);
#endif
	}
	outsock = 0;
	pState = EQNC_Init;
}

void EQNetworkConnection::DoRecvData() {
	if (ConnectionType != Outgoing) {
		ThrowError("EQNetworkConnection::Connect() called on an incoming connection!");
		return;
	}
	LockMutex lock(&MSocketLock);
	if (!outsock)
		return;

	uchar		buffer[1518];
	int			status;

	while (1) {
#ifdef WIN32
		status = recv(outsock, (char*) buffer, sizeof(buffer), 0);
#else
		status = recv(outsock, buffer, sizeof(buffer), 0);
#endif
		if (status >= 1) {
			RecvData(buffer, status);
		}
		else {
			break;
		}
	}
}

InQueue_Struct* EQNetworkConnection::InQueuePop() {
	if (MInQueueLock.trylock()) {
		InQueue_Struct* ret = InQueue.pop();
		MInQueueLock.unlock();
		return ret;
	}
	else {
		return 0;
	}
}

#ifdef WIN32
void EQNetworkConnection::Process(SOCKET sock)
#else
void EQNetworkConnection::Process(int sock)
#endif
{
	_CP(EQNetworkConnection_Process);
	if (!CheckNetActive())
		return;
	InQueue_Struct* iqs = NULL;
	while ((iqs = InQueuePop())) {
		MakeEQPacket(iqs->app, iqs->ackreq);
//		safe_delete(iqs->app);
		APPLAYER::PacketUsed(&iqs->app);
		safe_delete(iqs);
	}
	
#ifdef COMBINED
	if(combined_timer.Check()) {
		SendCombinedPackets();
	}
#endif 
	
	if (timeout_timer->Check()) {
		#if EQN_DEBUG_Error >= 1
			cout << "Connection timeout." << endl;
		#endif
		Close();
//		SetState(EQNC_Error);
	}
	
	fraglist.CheckTimers();
	
	if (datarate_timer->Check(0))
	{
		datarate_timer->Start();
		if (DataQueueFull()) {
			datahigh++;
			if (datahigh > 15)
				datahigh = 15;
		}
		else if (datahigh) {
			datahigh--;
		}
		dataflow -= datarate_tic;
		if (dataflow < 0)
			dataflow = 0;
	}
	
	EQNetworkPacket* pack;
	int32 size;
	uchar* data;
	sockaddr_in to;	
	memset(&to, 0, sizeof(to));
	to.sin_family = AF_INET;
	to.sin_port = rPort;
	to.sin_addr.s_addr = rIP;
	
	LinkedListIterator<EQNetworkPacket*> iterator(SendQueue);
	iterator.Reset();
	if (iterator.MoreElements()) {
		keep_alive_timer->Start();
	}
	else if (IsFree()) {
		SetState(EQNC_Finished);
	}
	
	int nas=0, ka=0;
	if ( (GetState() == EQNC_Active) && ((nas=no_ack_sent_timer->Check(0)) || (ka=keep_alive_timer->Check(0))) )
	{
#if EQN_DEBUG_ACK >= 5
		printf("KEEP ALIVE: %s   NAS: %s\n", ka?"Yes":"No", nas?"Yes":"No");
#endif
		APPLAYER outapp;
		outapp.opcode = 0xFFFF;
		outapp.priority = 5;
		// if we're making a keepalive packet set the ackreq bit
		MakeEQPacket(&outapp, ka);
	}
	
	iterator.Reset();
	while (iterator.MoreElements() && (!DataQueueFull())) {
		pack = iterator.GetData();
//#ifdef WORLD
		// We are world, so we need to have the higher times or modem users can't get in
		// Fixed this, there's no datarate setting in world, so it was using the default in the constructor (500), now set to 5
//		if (((pack->LastSent + 750) <= Timer::GetCurrentTime()) || (datahigh < 5 && (pack->LastSent + 500) <= Timer::GetCurrentTime())) {
//#else
		if (((pack->LastSent + DATA_RESEND_DELAY) <= Timer::GetCurrentTime()) || (datahigh < 5 && (pack->LastSent + DATA_RESEND_DELAY_IDLE) <= Timer::GetCurrentTime())) {
//#endif
#if EQN_DEBUG >= 8
			cout << "ARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec;
			cout << " pack->LastSent: " << setw(8) << setfill(' ') << pack->LastSent;
			cout << " Timer: " << setw(8) << setfill(' ') << Timer::GetCurrentTime();
			cout << " sentcount: " << setw(2) << setfill(' ') << (int) pack->SentCount;
			cout << " datahigh: "  << setw(3) << setfill(' ') << (int) datahigh << endl;
#endif
			size = pack->ReturnPacket(&data);
			
			#ifdef PACKET_PROFILER
				if(_packetProfileOut.count(pack->dwOpCode) == 1) {
					_ppData &tmp_pp = _packetProfileOut[pack->dwOpCode];
					tmp_pp.count++;
					tmp_pp.lengthSum += size;
				} else {
					_ppData tmp_pp;
					tmp_pp.count = 1;
					tmp_pp.lengthSum = size;
					_packetProfileOut[pack->dwOpCode] = tmp_pp;
				}
			#endif
			
			#if LOG_RAW_PACKETS_OUT >= 1
				cout << setw(8) << setfill(' ') << Timer::GetCurrentTime() << " Outgoing RAW packet: opcode=0x" << hex << setw(4) << setfill('0') << pack->dwOpCode << dec << " size=" << setw(5) << setfill(' ') << size << " headers: ";
				cout << (int) pack->HDR.a0_Unknown;
				cout << (int) pack->HDR.a1_ARQ;
				cout << (int) pack->HDR.a2_Closing;
				cout << (int) pack->HDR.a3_Fragment;
				cout << " ";
				cout << (int) pack->HDR.a4_ASQ;
				cout << (int) pack->HDR.a5_SEQStart;
				cout << (int) pack->HDR.a6_Closing;
				cout << (int) pack->HDR.a7_SEQEnd;
				cout << "  ";
				cout << (int) pack->HDR.b0_SpecARQ;
				cout << (int) pack->HDR.b1_Unknown;
				cout << (int) pack->HDR.b2_ARSP;
				cout << (int) pack->HDR.b3_Unknown;
				cout << " ";
				cout << (int) pack->HDR.b4_Unknown;
				cout << (int) pack->HDR.b5_Unknown;
				cout << (int) pack->HDR.b6_Unknown;
				cout << (int) pack->HDR.b7_Unknown;
				cout << endl;
				#if LOG_RAW_PACKETS_OUT >= 2
					cout << "SEQ";
					if (pack->HDR.a5_SEQStart)
						cout << " Start";
					cout << ": 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << " ";
					if (pack->HDR.a1_ARQ)
						cout << "ARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
					if (pack->HDR.b2_ARSP)
						cout << "ARSP: 0x" << hex << setw(4) << setfill('0') << pack->dwARSP << dec << " ";
					if (pack->HDR.a4_ASQ) {
						cout << "ASQ_high: 0x" << hex << setw(2) << setfill('0') << pack->dbASQ_high << dec << " ";
						if (pack->HDR.a1_ARQ)
							cout << "ASQ_low: 0x" << hex << setw(2) << setfill('0') << pack->dbASQ_low << dec << " ";
					}
					cout << endl;
					if (pack->HDR.a3_Fragment)
						cout << "Frag: SEQ: " << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << " Cur: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwCurr << dec << " Total: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwTotal << dec << " " << endl;
					#if LOG_RAW_PACKETS_OUT >= 9
						DumpPacket(data, size);
					#elif LOG_RAW_PACKETS_OUT >= 3
						if (size >= 32)
							DumpPacket(data, 32);
						else
							DumpPacket(data, size);
					#endif
				#endif
			#endif
#if EQN_DEBUG_Unknown >= 10
if (rand() % 5 != 0)
#endif
			sendto(sock, (char*) data, size, 0, (sockaddr*) &to, sizeof(to));
			delete[] data;
			dataflow += size;
			if (!pack->HDR.a1_ARQ) {
				iterator.RemoveCurrent();
				continue;
			}
			
			pack->LastSent = Timer::GetCurrentTime();
			if (pack->SentCount++ > 15) {
				#if EQN_DEBUG_Error >= 1
					cout << "Dropping connection, SentCount > 15 on ARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << ", SEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << endl;
				#endif
				SetState(EQNC_Error);
//				MakeEQPacket(0);
			}
		}
		iterator.Advance();
	}
}

void EQNetworkConnection::RecvData(uchar* data, int32 size) {
	timeout_timer->Start();
	EQNetworkPacket* pack = new EQNetworkPacket(this);
	//dataflow += size; // Sony does not add to the bandwidth limit coming in, this fixes the /anon lag out from spamming
	if (!pack->DecodePacket(data, size)) {
		safe_delete(pack);
		return;
	}
	#if LOG_RAW_PACKETS_IN >= 1
		cout << setw(8) << setfill(' ') << Timer::GetCurrentTime() << " Incomming RAW packet: size=" << setw(5) << setfill(' ') << size << " headers: ";
		cout << (int) pack->HDR.a0_Unknown;
		cout << (int) pack->HDR.a1_ARQ;
		cout << (int) pack->HDR.a2_Closing;
		cout << (int) pack->HDR.a3_Fragment;
		cout << " ";
		cout << (int) pack->HDR.a4_ASQ;
		cout << (int) pack->HDR.a5_SEQStart;
		cout << (int) pack->HDR.a6_Closing;
		cout << (int) pack->HDR.a7_SEQEnd;
		cout << "  ";
		cout << (int) pack->HDR.b0_SpecARQ;
		cout << (int) pack->HDR.b1_Unknown;
		cout << (int) pack->HDR.b2_ARSP;
		cout << (int) pack->HDR.b3_Unknown;
		cout << " ";
		cout << (int) pack->HDR.b4_Unknown;
		cout << (int) pack->HDR.b5_Unknown;
		cout << (int) pack->HDR.b6_Unknown;
		cout << (int) pack->HDR.b7_Unknown;
		cout << endl;
		#if LOG_RAW_PACKETS_IN >= 2
			cout << "SEQ";
			if (pack->HDR.a5_SEQStart)
				cout << " Start";
			cout << ": 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << " ";
			if (pack->HDR.a1_ARQ)
				cout << "ARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
			if (pack->HDR.b2_ARSP)
				cout << "ARSP: 0x" << hex << setw(4) << setfill('0') << pack->dwARSP << dec << " ";
			if (pack->HDR.a4_ASQ) {
				cout << "ASQ_high: 0x" << hex << setw(2) << setfill('0') << pack->dbASQ_high << dec << " ";
				if (pack->HDR.a1_ARQ)
					cout << "ASQ_low: 0x" << hex << setw(2) << setfill('0') << pack->dbASQ_low << dec << " ";
			}
			cout << endl;
			if (pack->HDR.a3_Fragment)
				cout << "Frag: SEQ: " << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << " Cur: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwCurr << dec << " Total: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwTotal << dec << " " << endl;
			#if LOG_RAW_PACKETS_IN >= 9
				DumpPacket(data, size);
			#elif LOG_RAW_PACKETS_IN >= 3
				if (size >= 32)
					DumpPacket(data, 32);
				else
					DumpPacket(data, size);
			#endif
		#endif
	#endif
#if EQN_DEBUG_Unknown >= 7
	if (pack->dwARQ - dwLastCACK > 10) {
		cout << "Debug: pack->dwARQ - dwLastCACK > 10" << endl;
		if (size >= 32)
			DumpPacket(data, 32);
		else
			DumpPacket(data, size);
		cout << endl;
	}
#endif
	if (ProcessPacket(pack, false))
		safe_delete(pack);
	CheckBufferedPackets();
}

void EQNetworkConnection::RemovePacket(EQNetworkPacket* pack) {
	LinkedListIterator<EQNetworkPacket*> iterator(SendQueue);

	iterator.Reset();
	while(iterator.MoreElements())
	{
		if (iterator.GetData() == pack) {
			iterator.RemoveCurrent();
			break;
		}
		iterator.Advance();
	}
}

bool EQNetworkConnection::ProcessPacket(EQNetworkPacket* pack, bool from_buffer) {
    /************ CHECK FOR ACK/SEQ RESET ************/ 
    if(pack->HDR.a5_SEQStart) {
		if (dwLastCACK == pack->dwARQ - 1)
			return true;
//		cout << "resetting SACK.dwGSQ1" << endl;
//		SACK.dwGSQ      = 0;            //Main sequence number SHORT#2
		dwLastCACK      = pack->dwARQ-1;//0;
//		CACK.dwGSQ = 0xFFFF; changed next if to else instead
	}
    // Agz: Moved this, was under packet resend before..., later changed to else statement...
	else if( (pack->dwSEQ - CACK.dwGSQ) <= 0 && !from_buffer) {  // Agz: if from the buffer i ignore sequence number..
#if EQN_DEBUG_Error >= 6
		cout << Timer::GetCurrentTime() << " ";
		cout << "Invalid packet ";
		cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << " ";
		cout << "CACK.dwGSQ: 0x" << hex << setw(4) << setfill('0') << CACK.dwGSQ << dec << " ";
		cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
		cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << endl;
#endif
		return true; //Invalid packet
	}
#if EQN_DEBUG_Packet >= 8
	cout << Timer::GetCurrentTime() << " ";
	cout << "Valid packet ";
	cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << " ";
	cout << "CACK.dwGSQ: 0x" << hex << setw(4) << setfill('0') << CACK.dwGSQ << dec << " ";
	cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
	cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << endl;
#endif

    CACK.dwGSQ = pack->dwSEQ; //Get current sequence #.

    /************ Process ack responds ************/
	// Quagmire: Moved this to above "ack request" checking in case the packet is dropped in there
	if(pack->HDR.b2_ARSP)
		IncomingARSP(pack->dwARSP);
    /************ End process ack rsp ************/

    // Does this packet contain an ack request?
	if(pack->HDR.a1_ARQ) {
		// Is this packet a packet we dont want now, but will need later?
		if(pack->dwARQ - dwLastCACK > 1 && pack->dwARQ - dwLastCACK < 16) { // Agz: Added 16 limit
			#ifdef _DEBUG
				// Debug check, if we want to buffer a packet we got from the buffer something is wrong...
				if (from_buffer) {
					cerr << "ERROR: Rebuffering a packet in EQNetworkConnection::ProcessPacket" << endl;
					return true;
				}
			#endif
			LinkedListIterator<EQNetworkPacket*> iterator(BufferedPackets);
			iterator.Reset();
			while(iterator.MoreElements()){
				if (iterator.GetData()->dwARQ == pack->dwARQ) {
					#if EQN_DEBUG_Packet >= 5
						cout << Timer::GetCurrentTime() << " ";
						cout << "Tried to buffer this packet, but it was already buffered ";
						cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
						cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
						cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << endl;
					#endif
					return true; // This packet was already buffered
				}
				iterator.Advance();
			}

			#if EQN_DEBUG_Packet >= 6
				cout << Timer::GetCurrentTime() << " ";
				cout << "Buffering this packet ";
				cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
				cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
				cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << endl;
			#endif

			BufferedPackets.Append(pack);
			return false;
		}
        // Is this packet a resend we have already processed?
		if(pack->dwARQ - dwLastCACK <= 0) {
			#if EQN_DEBUG_Packet >= 5
				cout << Timer::GetCurrentTime() << " ";
				cout << "Duplicate packet received ";
				cout << "pack->dwARQ: 0x" << hex << setw(4) << setfill('0') << pack->dwARQ << dec << " ";
				cout << "dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
				cout << "pack->dwSEQ: 0x" << hex << setw(4) << setfill('0') << pack->dwSEQ << dec << endl;
			#endif
// Quag: No need for this really
//			no_ack_sent_timer->Trigger(); // Added to make sure we send a new ack respond
			return true;
		}
	}
    
	/************ START ACK REQ CHECK ************/
	if (pack->HDR.a1_ARQ || pack->HDR.b0_SpecARQ) {
		IncomingARQ(pack->dwARQ);
	}
	if (pack->HDR.b0_SpecARQ) { // Send the ack reponse right away
		no_ack_sent_timer->Trigger();
		#if EQN_DEBUG_ACK >= 3
			cout << "Special ARQ received." << endl;
		#endif
	}
	/************ END ACK REQ CHECK ************/

	/************ CHECK FOR THREAD TERMINATION ************/
	if(pack->HDR.a2_Closing && pack->HDR.a6_Closing) {
		MStateLock.lock();
		if(pState == EQNC_Active) {
			#if EQN_DEBUG >= 1
				cout << "Closing bits received from client" << endl;
			#endif
			pState = EQNC_Closing;
			MStateLock.unlock();
			MakeEQPacket(0); // Agz: Added a close packet
		}
		else {
			MStateLock.unlock();
		}
	}
	/************ END CHECK THREAD TERMINATION ************/

	/************ Get ack sequence number ************/
	if(pack->HDR.a4_ASQ) {
		CACK.dbASQ_high = pack->dbASQ_high;
		if(pack->HDR.a1_ARQ)
			CACK.dbASQ_low = pack->dbASQ_low;
	}
    
	/************ End get ack seq num ************/

	/************ START FRAGMENT CHECK ************/
	/************ IF - FRAGMENT ************/
	if(pack->HDR.a3_Fragment) {
		#if EQN_DEBUG_Fragment >= 5
			cout << endl << Timer::GetCurrentTime() << " Fragment: ";
			cout << "pack->fraginfo.dwSeq: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << " ";
			cout << "pack->fraginfo.dwCurr: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwCurr << dec << " ";
			cout << "pack->fraginfo.dwTotal: 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwTotal << dec << endl;
		#endif
		EQNetworkFragmentGroup* fragment_group = 0;
		fragment_group = fraglist.GotPacket(pack);

		// If we have all the fragments to complete this group
		if(fragment_group->Ready()) {
//			#if EQN_DEBUG_Fragment >= 3
//				cout << Timer::GetCurrentTime() << " Getting fragment opcode: " << hex << setw(4) << setfill('0') << fragment_group->GetOpcode() << dec << endl;
//			#endif
			#if EQN_DEBUG_Fragment >= 2
				cout << "Fragment_group 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << " finished" << endl;
			#endif

			//Collect fragments and put them as one packet on the OutQueue
			EQDataPacket eqdp(this);
			
			if (!eqdp.Decode(&decode_key, fragment_group)) {
				cout << "EQNetworkConnection::ProcessPacket() !eqdp.Decode" << endl;
				SetState(EQNC_Error);
			}
			APPLAYER *app = 0;
			while ((app = eqdp.GetApp())) {
#ifdef PACKET_PROFILER
				if (app && app->opcode != 0 && app->opcode != 0xFFFF) {
					if(_packetProfileIn.count(app->opcode) == 1) {
						_ppData &tmp_pp = _packetProfileIn[app->opcode];
						tmp_pp.count++;
						tmp_pp.lengthSum += app->size;
					} else {
						_ppData tmp_pp;
						tmp_pp.count = 1;
						tmp_pp.lengthSum = app->size;
						_packetProfileIn[app->opcode] = tmp_pp;
					}
				}
#endif
#if LOG_PACKETS >= 1
	if (app && app->opcode != 0 && app->opcode != 0xFFFF) {
		cout << "Logging incoming packet. OPCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << setw(5) << setfill(' ') << app->size << endl;
		#if LOG_PACKETS == 2
			if (app->size >= 32)
				DumpPacket(app->pBuffer, 32);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS == 3
			if (app->size >= 512)
				DumpPacket(app->pBuffer, 512);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS >= 4
			DumpPacket(app);
		#endif
	}
#endif
				OutQueuePush(app);
			}
			fraglist.DeleteGroup(pack->fraginfo.dwSeq);
            return true;
        }
        else {
			#if EQN_DEBUG_Fragment >= 4
                cout << "FRAGMENT_GROUP not finished wait for more... " << endl;
			#endif
			return true;
		}
	}
	/************ ELSE - NO FRAGMENT ************/
    else {
		if (!pack->dwExtraSize) {
			// pure ack or pure ackreq, doesnt need to leave this class
			return true;
		}
		EQDataPacket eqdp(this);
		if (!eqdp.Decode(&decode_key, pack->pExtra, pack->dwExtraSize)) {
			cout << "EQNetworkConnection::ProcessPacket() !eqdp.Decode" << endl;
			SetState(EQNC_Error);
		}
		APPLAYER* app = 0;
		while ((app = eqdp.GetApp())) {
#ifdef PACKET_PROFILER
			if (app && app->opcode != 0 && app->opcode != 0xFFFF) {
				if(_packetProfileIn.count(app->opcode) == 1) {
					_ppData &tmp_pp = _packetProfileIn[app->opcode];
					tmp_pp.count++;
					tmp_pp.lengthSum += app->size;
				} else {
					_ppData tmp_pp;
					tmp_pp.count = 1;
					tmp_pp.lengthSum = app->size;
					_packetProfileIn[app->opcode] = tmp_pp;
				}
			}
#endif
#if LOG_PACKETS >= 1
	if (app && app->opcode != 0 && app->opcode != 0xFFFF) {
		cout << "Logging incoming packet. OPCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << setw(5) << setfill(' ') << app->size << endl;
		#if LOG_PACKETS == 2
			if (app->size >= 32)
				DumpPacket(app->pBuffer, 32);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS == 3
			if (app->size >= 512)
				DumpPacket(app->pBuffer, 512);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS >= 4
			DumpPacket(app);
		#endif
	}
#endif
			OutQueuePush(app);
		}
		return true;
	}
	/************ END FRAGMENT CHECK ************/

	cout << endl << "reached end of ProcessPacket?!" << endl << endl;
	return true;
}

void EQNetworkConnection::IncomingARSP(int16 arsp) { 
	#if EQN_DEBUG_ACK >= 3
        cout << "Incoming ack response: " << arsp << " SACK.dwARQ: " << hex << setw(4) << setfill('0') << SACK.dwARQ << dec << endl;
	#endif
	#if EQN_DEBUG_ACK >= 4
		bool MoreARQPackets = false;
	#endif
	LinkedListIterator<EQNetworkPacket*> iterator(SendQueue);
	iterator.Reset();
	while (iterator.MoreElements()) {
		if (arsp >= iterator.GetData()->dwARQ) {
			#if EQN_DEBUG_ACK >= 5
				cout << "Removing " << iterator.GetData()->dwARQ << " from resendqueue" << endl;
			#endif
			iterator.RemoveCurrent();
		}
		else {
			#if EQN_DEBUG_ACK >= 4
				if (iterator.GetData()->HDR.a1_ARQ) {
					MoreARQPackets = true;
				}
			#endif
			iterator.Advance();
		}
	}
	#if EQN_DEBUG_ACK >= 4
		if (!MoreARQPackets) {
			cout << Timer::GetCurrentTime() << " no more ARQ on SendQueue";
			cout << " dwARSP: 0x" << hex << setw(4) << setfill('0') << arsp << dec;
			cout << " SACK.dwARQ: 0x" << hex << setw(4) << setfill('0') << SACK.dwARQ << dec << endl;
		}
	#endif
}

void EQNetworkConnection::IncomingARQ(int16 arq) {
	CACK.dwARQ = arq;
	dwLastCACK = arq;
    
	if (!no_ack_sent_timer->Enabled()) {
		no_ack_sent_timer->Start(); // Agz: If we dont get any outgoing packet we can put an 
		// ack response in before NAS_TIMER has passed we send a pure ack response
		#if EQN_DEBUG_ACK >= 4
			cout << Timer::GetCurrentTime() << " no_ack_sent_timer->Start() (" << NAS_TIMER << "ms)" << endl;
		#endif
    }
}

void EQNetworkConnection::CheckBufferedPackets() {
	#if EQN_DEBUG_Packet >= 6
		int num=0; // Counting buffered packets for debug output
	#endif
	LinkedListIterator<EQNetworkPacket*> iterator(BufferedPackets);
	iterator.Reset();
	while(iterator.MoreElements()) {
		// Check if we have a packet we want already buffered
		if (iterator.GetData()->dwARQ - dwLastCACK == 1) {
			#if EQN_DEBUG_Packet >= 7
				cout << Timer::GetCurrentTime() << " ";
				cout << "Found a packet we want in the incoming packet buffer";
				cout << "pack->dwARQ:" << hex << setw(4) << setfill('0') << iterator.GetData()->dwARQ << dec << " ";
				cout << "dwLastCACK:" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
				cout << "pack->dwSEQ:" << hex << setw(4) << setfill('0') << iterator.GetData()->dwSEQ << dec << endl;
			#endif
			ProcessPacket(iterator.GetData(), true);
			iterator.RemoveCurrent();	// This will call delete on the packet
			iterator.Reset();			// Start from the beginning of the list again
			#if EQN_DEBUG_Packet >= 6
				num=0;					// Reset the counter
			#endif
			continue;
		}
		if (iterator.GetData()->dwARQ <= dwLastCACK) {
			#if EQN_DEBUG_Packet >= 1 || EQN_DEBUG_Error >= 1
				cout << Timer::GetCurrentTime() << " ";
				cout << "Found an old packet in the buffer, discarding. ";
				cout << "pack->dwARQ:" << hex << setw(4) << setfill('0') << iterator.GetData()->dwARQ << dec << " ";
				cout << "dwLastCACK:" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
				cout << "pack->dwSEQ:" << hex << setw(4) << setfill('0') << iterator.GetData()->dwSEQ << dec << endl;
			#endif
			iterator.RemoveCurrent();
			continue;
		}
		#if EQN_DEBUG_Packet >= 6
			num++;
		#endif
		iterator.Advance();
	}

	#if EQN_DEBUG_Packet >= 6
		if (num)
			cout << "Number of packets still in buffer: " << num << endl;
	#endif
}

void EQNetworkConnection::MakeEQPacket(APPLAYER* app, bool ackreq) {
#if LOG_PACKETS >= 1
	if (app && app->opcode != 0 && app->opcode != 0xFFFF) {
		cout << "Logging outgoing packet. OPCode: 0x" << hex << setw(4) << setfill('0') << app->opcode << dec << ", size: " << setw(5) << setfill(' ') << app->size << endl;
		#if LOG_PACKETS == 2
			if (app->size >= 32)
				DumpPacket(app->pBuffer, 32);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS == 3
			if (app->size >= 512)
				DumpPacket(app->pBuffer, 512);
			else
				DumpPacket(app);
		#endif
		#if LOG_PACKETS >= 4
			DumpPacket(app);
		#endif
	}
#endif

#ifdef COMBINED
	//try to add this packet to the combined queues.
	if(AddToCombined(app)) //Always want an ack on it?  I guess you could keep track of it
		return;
#endif

	MStateLock.lock();
//	if(pState == EQNC_Closing) {
	if (!app) { // Quag: should fix packets not getting sent before the closing
		SetState(EQNC_Closing);
/*		if (app) {
			MStateLock.unlock();
			return;
		}*/
		pState = EQNC_Closed;
		MStateLock.unlock();
		EQNetworkPacket* pack = new EQNetworkPacket(this);
		pack->HDR.a6_Closing    = 1;// Agz: Lets try to uncomment this line again
		pack->HDR.a2_Closing    = 1;// and this
		pack->HDR.a1_ARQ        = 1;// and this
//      pack->dwARQ             = 1;// and this, no that was not too good
		pack->dwARQ             = SACK.dwARQ++;// try this instead
		pack->priority = 6;
		
		SendQueue.Append(pack);
		SetState(EQNC_Closed);
        return;
    }
	else {
		MStateLock.unlock();
	}
	
	if (!app) {
		#if EQN_DEBUG >= 2
			cout << "EQNetworkConnection::MakeEQPacket app == 0" << endl;
		#endif
		return;
	}
	//bool bFragment= false; //This is set later on if fragseq should be increased at the end.
	
	/************ IF opcode is == 0xFFFF it is a request for pure ack creation ************/
	if(app->opcode == 0xFFFF) {
		EQNetworkPacket* pack = new EQNetworkPacket(this);
		
		if(!SACK.dwGSQ) {
			SACK.dwGSQ++;
//			pack->HDR.a5_SEQStart	= 1;			// Agz: hmmm, yes commenting this makes the client connect to zone
													//      server work and the world server doent seem to care either way
			SACK.dwARQ				= rand()%0x3FFF;//Current request ack
			SACK.dbASQ_high			= 1;			//Current sequence number
			SACK.dbASQ_low			= 0;			//Current sequence number
			
			#if EQN_DEBUG_ACK >= 1
				cout << "New random SACK.dwARQ" << endl;
			#endif
		}
		
//		pack->dwOpCode = 0xFFFF;
		#if EQN_DEBUG_ACK >= 2
			cout << Timer::GetCurrentTime() << " Pure ack sent ";
			cout << "pack->dwARSP=dwLastCACK: 0x" << hex << setw(4) << setfill('0') << dwLastCACK << dec << " ";
			cout << "pack->dwSEQ=SACK.dwGSQ: 0x" << hex << setw(4) << setfill('0') << SACK.dwGSQ << dec << endl;
		#endif
		pack->priority = app->priority;
		no_ack_sent_timer->Disable(); // we just queued the ack
		keep_alive_timer->Start();
		SendQueue.Append(pack);
		
		return;
	}
	
	int frags = (app->size >> 9);
	uchar* appBuffer = app->pBuffer;
	
	if (frags)
		ackreq = true; // Fragmented packets must have ackreq set

	#if EQN_DEBUG_Fragment >= 4
		cout << Timer::GetCurrentTime() << " frags: " << frags << "; size=" << app->size << endl;	
	#endif
	
	if (GetState() == EQNC_Active) {
	/************ PM STATE = ACTIVE ************/
		for (int i=0; i<=frags; i++) {
			EQNetworkPacket* pack = new EQNetworkPacket(this);
			
			if(!SACK.dwGSQ) {
				SACK.dwGSQ++;
				pack->HDR.a5_SEQStart   = 1;
				SACK.dwARQ              = rand()%0x3FFF;//Current request ack
				SACK.dbASQ_high         = 1;            //Current sequence number
				SACK.dbASQ_low          = 0;            //Current sequence number   
				#if EQN_DEBUG_ACK >= 1
					cout << "New random SACK.dwARQ: 0x" << hex << setw(4) << setfill('0') << SACK.dwARQ << dec << endl;
				#endif
			}
			
			//IF NON PURE ACK THEN ALWAYS INCLUDE A ACKSEQ              // Agz: Not anymore... Always include ackseq if not a fragmented packet
			if (i==0 && ackreq) // If this will be a fragmented packet, only include ackseq in first fragment
				pack->HDR.a4_ASQ = 1;                                   // This is what the eq servers does
			
			/************ Caculate the next ACKSEQ/acknumber ************/
			/************ Check if its a static ackseq ************/
			if( HI_BYTE(app->opcode) == 0x2000) {
				if(app->size == 15)
					pack->dbASQ_low = 0xb2;
				else
					pack->dbASQ_low = 0xa1;
			}
			/************ Normal ackseq ************/
			else {
				//If not pure ack and opcode != 0x20XX then
				if (ackreq) { // If the caller of this function wants us to put an ack request on this packet
					pack->HDR.a1_ARQ = 1;
					pack->dwARQ      = SACK.dwARQ++;
				}
				
				if(pack->HDR.a1_ARQ && pack->HDR.a4_ASQ) {
					pack->dbASQ_low  = SACK.dbASQ_low;
					pack->dbASQ_high = SACK.dbASQ_high;
				}
				else if(pack->HDR.a4_ASQ) {
					pack->dbASQ_high = SACK.dbASQ_high;
				}
				if(pack->HDR.a4_ASQ)
					SACK.dbASQ_low++;
			}
			
			/************ Check if this packet should contain op ************/
			if (app->opcode && i == 0) {
				pack->dwOpCode = app->opcode;
			}
			/************ End opcode check ************/
			
			/************ SHOULD THIS PACKET BE SENT AS A FRAGMENT ************/
			if(frags) {
				pack->HDR.a3_Fragment = 1;
			}
			/************ END FRAGMENT CHECK ************/
			
			if(app->size && app->pBuffer) {
				if(pack->HDR.a3_Fragment) {
					// If this is the last packet in the fragment group
					if(i == frags) {
						// Calculate remaining bytes for this fragment
						pack->dwExtraSize = app->size-510-512*((app->size/512)-1);
					}
					else if(i == 0) {
						pack->dwExtraSize = 510; // The first packet in a fragment group has 510 bytes for data
					}
					else {
						pack->dwExtraSize = 512; // Other packets in a fragment group has 512 bytes for data
					}
					pack->fraginfo.dwCurr	= i;
					pack->fraginfo.dwSeq	= dwFragSeq;
					pack->fraginfo.dwTotal	= frags + 1;
				}
				else {
					pack->dwExtraSize = (int16) app->size;
				}
				pack->pExtra = new uchar[pack->dwExtraSize];
				memcpy(pack->pExtra, appBuffer, pack->dwExtraSize);
				appBuffer += pack->dwExtraSize; //Increase counter
			}       
			
			/******************************************/
			/*********** !PACKET GENERATED! ***********/
			/******************************************/
			SendQueue.Append(pack);
		}//end for
		
		if(frags)
			dwFragSeq++;        
	} //end if
}


#ifdef COMBINED
//Father Nitwit's combine code.


void CombinedAPPLAYER::combine_add(int16 in_opcode, int32 in_size, uchar *data) {
	if(combined_size == 0) {
		//first packet, the regular EQ protocol stuff will add the 
		//opcode to this part.
		opcode = in_opcode & ~FLAG_IMPLICIT;	//implicit is applied later
		size = in_size;
		opcode_offset = 0;
		
		//reallocate and fill our buffer
		safe_delete(pBuffer);	//do we need this?
		
		//figure out our header length, without an opcode
		int32 add_size;
		if(in_opcode & FLAG_IMPLICIT)
			add_size = 0;
		else if(in_size > 0xFF)
			add_size = 3;	//1+2 bytes of length field
		else
			add_size = 1;	//1 bytes of length field
		
		combined_size = size+add_size;
		pBuffer = new uchar[combined_size];
		
		//now the length, if we have one
		uchar *tmp = pBuffer;
		if((in_opcode & FLAG_IMPLICIT) == 0) {
			if(in_size > 0xFF) {
				//add an extended langth code
				*tmp = 0xFF;
				tmp++;
				uint16 *len = (uint16 *) tmp;
				*len = htons(in_size);
				tmp += sizeof(uint16);
			} else {
				*tmp = in_size;
				tmp++;
			}
		}
		
		//copy in the new data now
		memcpy(pBuffer+add_size, data, size);
		
	} else {	//not our first packet.
		opcode |= FLAG_COMBINED;	//once we have 2+, flag entire thing combined
		in_opcode |= FLAG_COMBINED;	//everything after first is combined too
		
		//figure out our header length
		int32 add_size;
		if(in_opcode & FLAG_IMPLICIT)
			add_size = 2;	//just an opcode
		else if(in_size > 0xFF)
			add_size = 5;	//opcode + 1+2 bytes of length field
		else
			add_size = 3;	//opcode + 1 bytes of length field
		
		//reallocate our buffer.
		uchar *tmp = pBuffer;
		uint32 new_size = combined_size + in_size + add_size;
		pBuffer = new uchar[new_size];
		//copy in the old packets
		memcpy(pBuffer, tmp, combined_size);
		safe_delete(tmp);	//free old buffer
		
		//set our header..
		tmp = pBuffer + combined_size;
		//first an opcode, everybody has one of those.
		opcode_offset = combined_size;	//remember where the opcode is.
		uint16 *op = (uint16 *) tmp;
		*op = in_opcode & ~FLAG_IMPLICIT;	//implicit is set elsewhere if at all
		tmp += sizeof(uint16);
		
		//now the length, if we have one
		if((in_opcode & FLAG_IMPLICIT) == 0) {
			if(in_size > 0xFF) {
				//add an extended langth code
				*tmp = 0xFF;
				tmp++;
				uint16 *len = (uint16 *) tmp;
				*len = htons(in_size);
				tmp += sizeof(uint16);
			} else {
				*tmp = in_size;
				tmp++;
			}
		}
		
		//now throw in the new body.
		memcpy(tmp, data, in_size);
		
		//our combined packet is now this long.
		combined_size = new_size;
	}
}


//used by implicit length and crc stuff.
void CombinedAPPLAYER::combine_append(int32 add_size, uchar *data, bool implicit) {
	if(combined_size == 0)
		return;	//invalid
	
	uint32 imp_add = 0;
	//if this is adding an implicit packet...
	if(implicit) {
		//if we are the first implicit add, we need a flag and counter
		if(opcode_offset == 0) {	//0 is a special value
			if((opcode & FLAG_IMPLICIT) == 0) {
				//not allready flagged, flag it and add counter
				opcode |= FLAG_IMPLICIT | FLAG_COMBINED;
				imp_add = 1;
			}
		} else {
			uint16 *last_op = (uint16 *) (pBuffer + opcode_offset);
			if((*last_op & FLAG_IMPLICIT) == 0) {
				//not allready flagged, flag it and add counter
				*last_op = *last_op | FLAG_IMPLICIT | FLAG_COMBINED;
				imp_add = 1;
			}
		}
	}
	
	//reallocate our buffer
	uchar* old = pBuffer;
	int32 new_size = combined_size + add_size + imp_add;
	pBuffer = new uchar[new_size];
	//copy in the old data
	memcpy(pBuffer, old, combined_size);
	if(imp_add == 1) {
		//set the counter to 1
		*(pBuffer+combined_size) = 1;
	}
	//copy in the new data
	memcpy(pBuffer + combined_size + imp_add, data, add_size);
	//set our new total size
	combined_size = new_size;
	safe_delete(old);
}

/*

	I seemed to have a major stability problem if I let the implcitly packed
	packets get large enough to get fragmented below. Because of this, they
	are prevented from growing too large.

*/
bool EQNetworkConnection::AddToCombined(APPLAYER* app) {
	if(!app)
		return(false);
	//make sure combining is enabled.
	if(!combined_timer.Enabled())
		return(false);
	//priority (this also allows us to ignore ourself)
	if(app->priority == 6)
		return(false);
	//size limitations
	//this assumes that all implicit packets are smaller than a fragment
	if(app->size < 6 || app->size > COMBINE_MAX_PIECE)
//	if(app->size < 6 || app->size > 490)
		return(false);
	//cant combine an encrypted packet
	if(app->opcode & FLAG_ENCRYPTED)
		return(false);
	//cant combine a compressed packet either. But we could re-compress it...
	if(app->opcode & FLAG_COMPRESSED)
		return(false);
	
	
	//not compressed or encrypted. Not combined. We manage implicit ourself.
	app->opcode = StripFlags(app->opcode);
	
	//figure out if it is an implicit length op.
	int32 len = EQDataPacket::implicitlen(app->opcode);
	
	
	MCombined.lock();
	if(len != 0) {
		//we are implicit length

		//make sure we have a combined packet
		if(ImplicitCombinedPacket == NULL) {
			ImplicitCombinedPacket = new CombinedAPPLAYER();
			ImplicitCombinedPacket->priority = 6;
			implicit_counter_offset = 0;
			last_opcode = 0;
		} else if((ImplicitCombinedPacket->combined_size+app->size) > 506) {	//size is rough
			//make sure our packet dosent get larger than a fragment. It seems
			//to piss the EQ client off if an implicit/combined packet gets
			//fragmented.
			SendACombinedPacket(ImplicitCombinedPacket);
			ImplicitCombinedPacket = new CombinedAPPLAYER();
			ImplicitCombinedPacket->priority = 6;
			implicit_counter_offset = 0;
			last_opcode = 0;
		}
		
		//look for repeated opcode
		if(last_opcode == app->opcode) {
			//repeated implicit opcodes, special handling
			//get the pointer to our combine counter
			
			if(implicit_counter_offset == 0) {
				//we dont have a counter yet, but it will be at the end of the
				//current packet when we grow it. It will start at 1 allready
				implicit_counter_offset = ImplicitCombinedPacket->combined_size;
				//grow the packet as needed
				ImplicitCombinedPacket->combine_append(len, app->pBuffer, true);
				MCombined.unlock();
				return(true);
			} else {
				//counter is allready there, just increment it
				uchar *counter = ImplicitCombinedPacket->pBuffer + implicit_counter_offset;
				if(*counter < 0xFF) {
					//we can tack on to the end of the contiguous block.
					*counter = *counter + 1;	//add one to our counter.
					//grow the packet as needed
					ImplicitCombinedPacket->combine_append(len, app->pBuffer, true);
					MCombined.unlock();
					return(true);
				} //else, fall through and treat it like a new implicit.
			}
		}
		//else, new implicit opcode.
		
		//add a new block to the combined packet.
		ImplicitCombinedPacket->combine_add(app->opcode|FLAG_IMPLICIT, len, app->pBuffer);
		
		//set our counter offset to 0 to indicate we dont have one yet
		implicit_counter_offset = 0;
		
		//set last opcode
		last_opcode = app->opcode;
	} else {
		
		//make sure we have a combined packet
		if(CombinedPacket == NULL) {
			CombinedPacket = new CombinedAPPLAYER();
			CombinedPacket->priority = 6;
		}
		//Unlike implicit packets, this seems to work without this:
		/*else if((CombinedPacket->combined_size+app->size) > 506) {	//size is rough
			//this will make the combined packet larger than a fragment, why bother
			//just send what we have in our combined, and start a new one.
			SendACombinedPacket(CombinedPacket);
			CombinedPacket = new APPLAYER(0, 0);
			CombinedPacket->priority = 6;
		}*/
		
		//grow the combined packet as needed.
		CombinedPacket->combine_add(app->opcode, app->size, app->pBuffer);
		
		//set last opcode, used by implicit stuff. needed if they dont have diff packets
		//last_opcode = app->opcode;
	}
	
	MCombined.unlock();
	return(true);
}

void EQNetworkConnection::SendCombinedPackets() {
	MCombined.lock();
	
	if(CombinedPacket == NULL && ImplicitCombinedPacket == NULL)
		return;
	
	CombinedAPPLAYER *app = CombinedPacket;
	CombinedAPPLAYER *appi = ImplicitCombinedPacket;
	//clear all our packet state
	CombinedPacket = NULL;
	ImplicitCombinedPacket = NULL;
	implicit_counter_offset = 0;
	last_opcode = 0;
	
	MCombined.unlock();
	
	if(app)
		SendACombinedPacket(app);
	if(appi)
		SendACombinedPacket(appi);
}

void EQNetworkConnection::SendACombinedPacket(CombinedAPPLAYER* app) {
	//one last step, we gotta append part of the CRC... I dunno why...
	//some code from image's old combine stuff
	
	if(app->opcode & FLAG_COMBINED) {
//printf("Sending combined packet of length %d\n", app->combined_size);
		
		//We have to grab the last two bytes of the CRC to add to the packet (Why not just use the entire crc?)
		union imageData { 
			int32 bignum;
			int16 littlenum;
		} crcu;

		int32 crc = CRC32::Generate(app->pBuffer, app->combined_size);
		crcu.bignum = crc;
		
		app->combine_append(sizeof(int16), (uchar *)&crcu.littlenum, false);
		
		app->size = app->combined_size;	//MakeEQPacket looks at size only, but dosent put it in the packet

		//DumpPacket(app);
		
		//compress bigger packets for a little savings
		if(app->size > COMBINE_DEFLATE_SIZE)
			app->Deflate();
		
		MakeEQPacket(app);
		CombinedAPPLAYER::PacketUsed(&app);
	} else {
//printf("Sending combined but alone packet of length %d\n", app->size);
		//we only got one packet, send it as non-combined.
		
		int32 imp = EQDataPacket::implicitlen(app->opcode);
		if(imp > 0) {
			MakeEQPacket(app); // No changes to make
			CombinedAPPLAYER::PacketUsed(&app);
		} else {
			//we want to skip the length pieces we added
			uchar *orig = app->pBuffer;
			if(app->size > 0xFF) {
				app->pBuffer += 3;
			} else {
				app->pBuffer += 1;
			}
			MakeEQPacket(app); // No changes to make
			app->pBuffer = NULL;
			safe_delete(orig);
			CombinedAPPLAYER::PacketUsed(&app);
		}
	}
}

/*void EQNetworkConnection::CreateCombinedPacket()
{
	if(!CombinedPacket || (!combined_timer->Check()) || (CombinedPacket->priority == 6))
		return;

	CombinedPacket->priority = 6;

	if((CombinedPacket->opcode & FLAG_COMBINED))
	{
		uchar* finalbuffer = new uchar[CombinedPacket->size+2];
		memcpy(finalbuffer,CombinedPacket->pBuffer,CombinedPacket->size);
		CombinedPacket->size = CombinedPacket->size+2;
		//We have to grab the last two bytes of the CRC to add to the packet (Why not just use the entire crc?)
		union imageData { 
			int bignum;
			int16 littlenum;
		}imagetest;

		int32 crc = CRC32::Generate(finalbuffer,CombinedPacket->size-2);
		imagetest.bignum=crc;

		memcpy(&finalbuffer[CombinedPacket->size-2],&imagetest.littlenum,sizeof(int16));
		safe_delete_array(CombinedPacket->pBuffer);
		CombinedPacket->pBuffer = finalbuffer;
		//DumpPacket(CombinedPacket);
		if(CombinedPacket->size > 125)
			CombinedPacket->Deflate();

		MakeEQPacket(CombinedPacket);
		safe_delete(CombinedPacket);
	}
	else // If its been over a second and the combinedpacket exists but no additional packets are included
	{
		int32 imp = EQDataPacket::implicitlen(CombinedPacket->opcode);
		if(imp)
			MakeEQPacket(CombinedPacket); // No changes must be made
		else
		{
			//Remove the size from the packet
			int size = CombinedPacket->size-1;
			uchar* newbuffer = new uchar[size];
			memcpy(newbuffer,&CombinedPacket->pBuffer[1],CombinedPacket->size-1);
			safe_delete_array(CombinedPacket->pBuffer); // Delete old outdated buffer
			CombinedPacket->pBuffer = newbuffer; // Point to the new buffer
			CombinedPacket->size = size; // Update the size	

			if(CombinedPacket->size > 125)
				CombinedPacket->Deflate();

			MakeEQPacket(CombinedPacket);
		}
		safe_delete(CombinedPacket);
	}
}
*/
#endif 
	
EQNetworkPacket::EQNetworkPacket(EQNetworkConnection* inetcon) {
	netcon = inetcon;
	pExtra = 0;
	Clear();
}

EQNetworkPacket::~EQNetworkPacket() {
	safe_delete_array(pExtra);
}

void EQNetworkPacket::Clear() {
	*((int16*)&HDR) = 0;

	dwSEQ			= 0;
	dwARSP          = 0;
	dwARQ           = 0;
	dbASQ_low       = 0;        
	dbASQ_high      = 0;
	dwOpCode        = 0;    
	fraginfo.dwCurr = 0;
	fraginfo.dwSeq  = 0;
	fraginfo.dwTotal= 0;
	dwExtraSize     = 0;
	safe_delete_array(pExtra);

	LastSent		= 0;
	SentCount		= 0;
}

void EQNetworkPacket::AddData(uchar* data, int32 size) {
	safe_delete_array(pExtra);
	pExtra = new uchar[size];
	memcpy(pExtra, data, size);
}

bool EQNetworkPacket::DecodePacket(uchar* data, int32 size) {
    
	
	/************ LOCAL VARS ************/
    int32	o	= 0;
    
/************ START PROCESSING ************/

/*	if (size < 10) {	// Agz: Adding checks for illegal packets, so we dont read beyond the buffer
						//      Minimum normal packet length is 10
		cerr << "EQPacket.cpp: Illegal packet length" << endl;
		return false;      // TODO: Do we want to check crc checksum on the incoming packets?
    }*/

    memcpy(&HDR, data, 2);
    o += 2;

	dwSEQ = ntohs(*((int16*)&data[o]));
	o += 2;
    

    /************ CHECK ACK FIELDS ************/
    /**********/
    //Common ACK Response
    if(HDR.b2_ARSP) {
        dwARSP = ntohs(*((int16*)&data[o]));
        o += 2;
    }
    if (HDR.b3_Unknown) { // this is definately a SACK or NACK
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b3: 0x" << hex << setw(4) << setfill('0') << *((int16*)&data[o]) << dec << endl;
		#endif
		o += 2;
	}
    // Agz: Dont know what this HDR.b4_Unknown data is, gets sent when there is packetloss
    if (HDR.b4_Unknown) {
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b4: 0x" << hex << setw(2) << setfill('0') << (int) data[o] << dec << endl;
		#endif
		o++; // One unknown byte
    }
	/*
	  See Agz's comment above about HDR.b4.
	  Same story with b5, b6, b7, but they're 2, 4 and 8 bytes respectively.
	  -Quagmire
	*/
    if (HDR.b5_Unknown) {
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b5: 0x" << hex << setw(4) << setfill('0') << *((int16*)&data[o]) << dec << endl;
		#endif
        o += 2; // 2 unknown bytes
    }
	if (HDR.b6_Unknown) {
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b6: 0x" << hex << setw(8) << setfill('0') << *((int32*)&data[o]) << dec << endl;
		#endif
		o += 4; // 4 unknown bytes
	}
	if (HDR.b7_Unknown) {
		#if EQN_DEBUG_Unknown >= 5
			cout << "HDR.b7: 0x" << hex << setw(8) << setfill('0') << *((int32*)&data[o]) << dec << " 0x" << hex << setw(8) << setfill('0') << *((int32*)&data[o+4]) << dec << endl;
		#endif
		o += 8; // 8 unknown bytes
	}
#if EQN_DEBUG_Unknown >= 8
	if (HDR.b4_Unknown || HDR.b5_Unknown || HDR.b6_Unknown || HDR.b7_Unknown) {
		if (size >= 32)
			DumpPacket(data, 32);
		else
			DumpPacket(data, size);
	}
#endif
    
	/************/ 
    //Common  ACK Request
	if(HDR.a1_ARQ) {
		dwARQ = ntohs(*((int16*)&data[o]));
		o += 2;
	}
	/************ END CHECK ACK FIELDS ************/



	/************ CHECK FRAGMENTS ************/
	if(HDR.a3_Fragment) { 
		if (size < 16) { // Agz: Adding checks for illegal packets, so we dont read beyond the buffer
			cerr << "Illegal packet length" << endl;
			return false;
		}

		//Extract frag info.
		fraginfo.dwSeq   = ntohs(*((int16*)&data[o]));
		o += 2;
		fraginfo.dwCurr  = ntohs(*((int16*)&data[o]));
		o += 2;
		fraginfo.dwTotal = ntohs(*((int16*)&data[o]));
		o += 2;
	}
	/************ END CHECK FRAGMENTS ************/
    


	/************ CHECK ACK SEQUENCE ************/
	if(HDR.a4_ASQ) {
		dbASQ_high = data[o];
		o++;
		if(HDR.a1_ARQ) {
			dbASQ_low  = data[o];
			o++;
		}
	}
	/************ END CHECK ACK SEQUENCE ************/
    
	/************ GET OPCODE/EXTRACT DATA ************/
//  if( (size-4) != ACK_ONLY_SIZE && !(HDR.a2_Closing && HDR.a6_Closing))
//-4 (crc dont count) // Agz: Added closing checks...
	if(size-o > 4 && !(HDR.a2_Closing && HDR.a6_Closing)) { // Agz: Another bug fix. I got a arq&arsp packet with no data from the client.
		/************ Extract applayer ************/
//		if(!fraginfo.dwCurr) { //OPCODE only included in first fragment!
//			dwOpCode = ntohs(*((int16*)&data[o]));
//			o += 2;
//		}

        dwExtraSize = (size - o) - 4;
		if (size < (o + 4)) {
			#if EQN_DEBUG_Error >= 3
				cerr << "ERROR: Packet we couldnt handle, ExtraSize < 0" << endl;
				#if EQN_DEBUG_Error >= 9
					DumpPacket(data, size);
				#endif
			#endif
            return false;
		}
		if (dwExtraSize) {
			pExtra  = new uchar[dwExtraSize];
			memcpy(pExtra, &data[o], dwExtraSize);
		}
	}
	else {   /************ PURE ACK ************/
		dwOpCode    = 0;
		pExtra      = 0;
		dwExtraSize = 0;
	}
	/************ END GET OPCODE/EXTRA DATA ************/


/************ END PROCESSING ************/
	return true;
}

int32 EQNetworkPacket::ReturnPacket(uchar** data) {
	*data = new uchar[dwExtraSize + MAX_HEADER_SIZE];
	uchar* buf = *data;
	int32 o = 4;
	this->dwSEQ = netcon->SACK.dwGSQ;
	*((int16*)&buf[2]) = ntohs(netcon->SACK.dwGSQ++);

	if (netcon->CACK.dwARQ) {
		dwARSP = netcon->CACK.dwARQ;
		HDR.b2_ARSP = 1;				//Set ack response field
		*((int16*)&buf[o]) = ntohs(netcon->CACK.dwARQ);
		o += 2;
		netcon->CACK.dwARQ = 0;
		netcon->no_ack_sent_timer->Disable();
	}
	else if (dwOpCode == 0xFFFF) {
		dwARSP = netcon->dwLastCACK;
		HDR.b2_ARSP = 1;				//Set ack response field
		*((int16*)&buf[o]) = ntohs(netcon->dwLastCACK);
		o += 2;
		netcon->CACK.dwARQ = 0;
		netcon->no_ack_sent_timer->Disable();
	}
	else {
		HDR.b2_ARSP = 0;
	}

	if (HDR.a1_ARQ) {
		*((int16*)&buf[o]) = ntohs(dwARQ);
		o += 2;
	}
	if(HDR.a3_Fragment) {
        *((int16*)&buf[o]) = ntohs(fraginfo.dwSeq);
		o += 2;
        *((int16*)&buf[o]) = ntohs(fraginfo.dwCurr);
		o += 2;
        *((int16*)&buf[o]) = ntohs(fraginfo.dwTotal);
		o += 2;
	}
    if(HDR.a4_ASQ && HDR.a1_ARQ) {
		*&buf[o++] = dbASQ_high;
		*&buf[o++] = dbASQ_low;
	}
	else if(HDR.a4_ASQ) {
		*&buf[o++] = dbASQ_high;
	}
	if (dwOpCode) {
        *((int16*)&buf[o]) = dwOpCode;
		o += 2;
	}
	if(pExtra) {
		memcpy(&buf[o], pExtra, dwExtraSize);
		o += dwExtraSize;
	}
	memcpy(buf, &HDR, 2);
	*((int32*)&buf[o]) = htonl(CRC32::Generate(buf, o));
	o += 4;
	return o;
}

EQNetworkFragmentGroup::EQNetworkFragmentGroup(int16 ifragseq, int16 inum_fragments) {
	timeout_timer = new Timer(15000);
	fragseq = ifragseq;
	num_fragments = inum_fragments;
	fragments = new uchar*[num_fragments];
	fragment_sizes = new int32[num_fragments];
	for (int i=0; i<num_fragments; i++) {
		fragments[i] = 0;
		fragment_sizes[i] = 0;
	}
}

EQNetworkFragmentGroup::~EQNetworkFragmentGroup() {
	safe_delete(timeout_timer);
	for (int i=0; i<num_fragments; i++) {
		if (fragments[i])
			safe_delete_array(fragments[i]);
	}
	safe_delete_array(fragments);
	safe_delete_array(fragment_sizes);
}

void EQNetworkFragmentGroup::Add(EQNetworkPacket* pack) {
#ifdef _DEBUG
	if (pack->fraginfo.dwSeq != fragseq) {
		ThrowError("EQNetworkFragmentGroup::Add(): pack->fraginfo.dwSeq != fragseq");
	}
	if (pack->fraginfo.dwCurr >= num_fragments) {
		ThrowError("EQNetworkFragmentGroup::Add(): pack->fraginfo.dwCurr >= num_fragments");
	}
#endif
	timeout_timer->Start();
	if (fragments[pack->fraginfo.dwCurr])
		return; // already got it, this one musta been a resend
	fragments[pack->fraginfo.dwCurr] = pack->pExtra;
	pack->pExtra = 0;
	fragment_sizes[pack->fraginfo.dwCurr] = pack->dwExtraSize;
}

int32 EQNetworkFragmentGroup::Assemble(uchar** data) {
	int32 size = 0;
	int i;
#ifdef _DEBUG
	if (!Ready()) {
		ThrowError("EQNetworkFragmentGroup::Assemble() when not ready");
	}
#endif
	for (i=0; i<num_fragments; i++) {
		size += fragment_sizes[i];
	}
	*data = new uchar[size];
	uchar* tmp = *data;
	for (i=0; i<num_fragments; i++) {
		memcpy(tmp, fragments[i], fragment_sizes[i]);
		tmp += fragment_sizes[i];
	}
	return size;
}

bool EQNetworkFragmentGroup::Ready() {
	for (int i=0; i<num_fragments; i++) {
		if (!fragments[i])
			return false;
#ifdef _DEBUG
		if (!fragment_sizes[i]) {
			ThrowError("EQNetworkFragmentGroup::Ready(): fragment_sizes[i] == 0");
		}
#endif
	}
	return true;
}

EQNetworkFragmentGroupList::EQNetworkFragmentGroupList() {
	timeout_check_timer = new Timer(5000);
}

EQNetworkFragmentGroupList::~EQNetworkFragmentGroupList() {
	safe_delete(timeout_check_timer);
}

void EQNetworkFragmentGroupList::CheckTimers() {
	if (!timeout_check_timer->Check())
		return;
	LinkedListIterator<EQNetworkFragmentGroup*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->timeout_timer->Check()) {
			#if EQN_DEBUG_Fragment >= 2
				cout << "Fragment_group 0x" << hex << setw(4) << setfill('0') << iterator.GetData()->GetFragSeq() << dec << " timed out." << endl;
			#endif
			iterator.RemoveCurrent();
		}
		else
			iterator.Advance();
	}
}

EQNetworkFragmentGroup* EQNetworkFragmentGroupList::GotPacket(EQNetworkPacket* pack) {
	LinkedListIterator<EQNetworkFragmentGroup*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->GetFragSeq() == pack->fraginfo.dwSeq) {
			iterator.GetData()->Add(pack);
			return iterator.GetData();
		}
		iterator.Advance();
	}
	#if EQN_DEBUG_Fragment >= 2
		cout << "New Fragment_group 0x" << hex << setw(4) << setfill('0') << pack->fraginfo.dwSeq << dec << endl;
	#endif
	EQNetworkFragmentGroup* fg = new EQNetworkFragmentGroup(pack->fraginfo.dwSeq, pack->fraginfo.dwTotal);
	fg->Add(pack);
	list.Insert(fg);
	return fg;
}

EQNetworkFragmentGroup* EQNetworkFragmentGroupList::FindGroup(int16 fragseq) {
	LinkedListIterator<EQNetworkFragmentGroup*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->GetFragSeq() == fragseq) {
			return iterator.GetData();
		}
		iterator.Advance();
	}
	return 0;
}

void EQNetworkFragmentGroupList::DeleteGroup(int16 fragseq) {
	LinkedListIterator<EQNetworkFragmentGroup*> iterator(list);

	iterator.Reset();
	while (iterator.MoreElements()) {
		if (iterator.GetData()->GetFragSeq() == fragseq) {
			iterator.RemoveCurrent();
			return;
		}
		else {
			iterator.Advance();
		}
	}
}

EQDataPacket::EQDataPacket(EQConnectionState* iCS) {
	pCS = iCS;
}

EQDataPacket::~EQDataPacket() {}

APPLAYER* EQDataPacket::GetApp() {
	return OutQueue.pop();
}

// Assemble a fragment group into one packet and decode
bool EQDataPacket::Decode(sint64* key, EQNetworkFragmentGroup* fg) {
	int8* data = 0;
	int32 tmpSize = fg->Assemble(&data);
	bool ret = Decode(key, data, tmpSize);
	safe_delete_array(data);
	return ret;
}

// Decode a raw UDP payload (which includes opcode)
bool EQDataPacket::Decode(sint64* key, int8* buf, sint32 buflen) {
	if (buflen < 2)
		return false;
	
	if (buflen == 2) {
		APPLAYER* app = new APPLAYER();
		app->SetRealOpcode((*((int16*) buf)) &~0xF000); //avoid translation
		OutQueue.push(app);
		return true;
	}
	
	#if EQN_DEBUG_NewStream >= 9
		struct in_addr	in = {0};
		if (pCS)
			in.s_addr = pCS->GetrIP();
		cout << "EQDataPacket::Decode(): " << inet_ntoa(in) << ":" << pCS->GetrPort();
		if (pCS && pCS->GetlIP()) {
			in.s_addr = pCS->GetlIP();
			cout << " -> " << inet_ntoa(in) << ":" << pCS->GetlPort();
		}
		cout << " buflen=" << buflen << endl;
		DumpPacket(buf, buflen);
	#endif
	
	// First two bytes are opcode, the rest is the message data
	return Decode(key, *((int16*) buf), &buf[2], buflen - 2);
}

// Following function is mostly from ShowEQ. http://seq.sf.net/
// Packet data is decrypted, decompressed, and separated (as necessary)
bool EQDataPacket::Decode(sint64* key, int16 opCode, int8* buf, sint32 buflen) {
	
	// ############################
	// Packet Encryption
	// 8 bytes at a variable offset within the first few bytes of each packet can
	// be encrypted.  The decryption key for these 8 bytes is calculated based
	// on the key for previous encrypted packet
	// ############################
	if (opCode & FLAG_ENCRYPTED) {
		#if EQN_DEBUG_NewStream >= 6
			LogFile->write(EQEMuLog::Debug, "Encrypted packet: 0x%04.4x, size=%i ", opCode, buflen);
		#endif
		#if EQN_DEBUG_NewStream >= 8
			if (buflen >= 32)
				DumpPacket(buf, 32);
			else
				DumpPacket(buf, buflen);
		#endif
		
		sint64 offset = (*key % 5) + 5;
		sint64* pos = (sint64*)(buf + offset);
		
		// Apply decryption
		*pos ^= *key;
		
		// Save key to be used on next packet
		*key ^= *pos;
		*key += buflen;
		
		opCode &= ~FLAG_ENCRYPTED;
	}
	
	// ############################
	// Packet Compression
	// Standard zlib compression is used.  Typically, if decompression fails, it means
	// encryption failed for the packet and the compressed header cannot be read.
	// ############################
	if (opCode & FLAG_COMPRESSED) {
		#if EQN_DEBUG_NewStream >= 6
			LogFile->write(EQEMuLog::Debug, "Compressed packet: 0x%04.4x, size=%i", opCode, buflen);
		#endif
		
		uint32 dcomplen = BEST_COMPR_RATIO * buflen;
		uint8* decompressed = new int8[dcomplen];
		
		#if EQN_DEBUG_NewStream >= 7
			LogFile->write(EQEMuLog::Debug, "Attempting decompression: dcomplen=%i", dcomplen);
		#endif
		#if EQN_DEBUG_NewStream >= 8
			if (buflen >= 32)
				DumpPacket(buf, 32);
			else
				DumpPacket(buf, buflen);
		#endif
		
		bool ret = false;
		dcomplen = InflatePacket(buf, buflen, decompressed, dcomplen);
		if (!dcomplen) {
			LogFile->write(EQEMuLog::Error, "Decompression failed on 0x%04.4x", opCode);
		}
		else {
			opCode = opCode & ~FLAG_COMPRESSED;
			ret = Decode(key, opCode, decompressed, dcomplen);
		}
		
		safe_delete_array(decompressed);
		return ret;
	}
	
	// ############################
	// Packet Combination
	// Many EQ messages are combined into a single UDP packet.  Some of these messages
	// have a length defined directly in the packet stream, whereas the lengths of
	// others are assumed to be a particular length (implicitly)
	// ############################
	while (buflen > 0) {
		if (opCode & FLAG_COMBINED) {
			#if EQN_DEBUG_NewStream >= 6
				LogFile->write(EQEMuLog::Debug, "Combined packets: 0x%04.4x, size=%i", opCode, buflen);
			#endif
			
			bool repeatop = false;
			uint8* dptr = buf;
			sint32 left = buflen;
			int32 count = 1;
			
			if (opCode & FLAG_IMPLICIT) {
				#if EQN_DEBUG_NewStream >= 6
					LogFile->write(EQEMuLog::Debug, "Implied opcode packet: 0x%04.4x, size=%i", opCode, buflen);
				#endif
				
				opCode = opCode & ~FLAG_IMPLICIT;
				repeatop = true;
			}
			opCode = opCode & ~FLAG_COMBINED;
			
			while (count > 0) {
				sint32 size;
				
				size = EQDataPacket::implicitlen(opCode);
				if (size == 0) {	//not an implicity length packet
					if (dptr[0] == 0xff) {	//larger than 255, take next two bytes
						left--;
						dptr++;
						size = ntohs(*((uint16 *)dptr));
						left -= 2;
						dptr += 2;
					} else {
						size = dptr[0];
						left--;
						dptr++;
					}
				}
				
				if (size > left) {
					LogFile->write(EQEMuLog::Error, "Error: size > left (size=%i, left=%i, opcode=0x%04.4x", size, left, opCode);
					return false;
				}
				
				if ((size <= 0) || (size >= (BEST_COMPR_RATIO * buflen)) || left < 0 || (left >= (BEST_COMPR_RATIO * buflen))) {
					LogFile->write(EQEMuLog::Error, "Error: Combined packets: Size or left is an impossible value (size=%i, left=%i, opcode=0x%04.4x)", size, left, opCode);
					return false;
				}
				
				APPLAYER* app = new APPLAYER(OP_Unknown, size);
				app->SetRealOpcode(opCode);	//avoid translation
				memcpy(app->pBuffer, dptr, size);
				OutQueue.push(app);
				
				dptr += size;
				left -= size;
				
				count--;
				if (repeatop) {
					count = dptr[0];	//this is how many implicitly packed packets of this opcode there are
					dptr++;
					left--;
					repeatop = false;
				}
			}
			
			//if there is enough packet to fit at least an opcode
			//then
			if (left > 2) {
				opCode = *((int16*)dptr);
				buf = dptr + 2;	//gotta skip the opcode.
				buflen = left - 2;
				continue;
			}
			return true;
		}
		else { // not opCode & FLAG_COMBINED
			break;
		}
	}
	APPLAYER* app = new APPLAYER(OP_Unknown, buflen);
	app->SetRealOpcode(opCode);	//avoid translation
	memcpy(app->pBuffer, buf, buflen);
	OutQueue.push(app);
	return true;
}

// Determine the length of a type of eq message
// Taken from ShowEQ (Thanks guys!)
uint32 EQDataPacket::implicitlen(uint16 opcode)
{
//if the size of any implicit packets ever goes over 500ish, the combine code
//is prolly gunna break.
switch (opcode)
  {
  case 0x0021:			// 34
    return 0x08;		// 8
  case 0x0027:			// 40
    return 0x12;		// 18
  case 0x003e:			// 63
    return 0x0c;		// 12
  case 0x00bf:			// 192
    return 0x10;		// 16
  case 0x00e2:			// 228
    return 0x17;		// 23
  case 0x00f3:			// 245
    return 0x04;		// 4
  case 0x0100:			// 258
    return 0x88;		// 136
  case 0x0101:			// 259
    return 0x1f;		// 31
  case 0x012c:			// 302
    return 0x09;		// 9
  case 0x012f:			// 305
    return 0x08;		// 8
  case 0x0140:			// 322
    return 0x04;		// 4
  case 0x022d:			// 542
    return 0x03;		// 3
  case 0x022e:			// 543
    return 0x03;		// 3
  case 0x0243:			// 564
    return 0x06;		// 6
  case 0x0244:			// 565
    return 0x06;		// 6
  default:
    return 0;
  }
}
