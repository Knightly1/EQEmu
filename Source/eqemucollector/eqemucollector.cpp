/* 
	Copyright (C) 2005 Michael S. Finger
	& 2005 Everquest Emulator Team

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
#include "StdAfx.h"
#include "../common/debug.h"
#include <string>
#include <iostream>
#include <map>
#include <pcap.h>
#include <signal.h>
#include "pcvars.h"
#ifdef WIN32
	#include <conio.h>
	#include <pcap.h>
	#include <windows.h>
	#include <process.h>
#else
	#include <sys/socket.h>
	#include <sys/resource.h>
	#include <arpa/inet.h>
#endif
#include "EQStreamPair.h"
#include "../common/misc.h"
#include "../common/files.h"
#include "protocol.h"
#include "../common/opcodemgr.h"
#include "StreamPairManager.h"
#include "PluginManager.h"
#include "PacketHandler.h"





using namespace std;


#ifdef FREEBSD
//FatherNitwit: freebsd's built in pcap dosent have pcap_next_ex
//so heres my non-thread safe hack version of it


struct __bsd_pcap_crap_struct {
	struct pcap_pkthdr *header;
	const u_char **data;
};

static void __bsd_pcap_crap_handler(unsigned char *u, const pcap_pkthdr *h, const unsigned char *pkt) {
	struct __bsd_pcap_crap_struct *s = (struct __bsd_pcap_crap_struct *) u;
	//copy in the header info
	*s->header = *h;
	//get a pointer to the body
	*s->data = pkt;
}

int pcap_next_ex(pcap_t *p, struct pcap_pkthdr **pkt_header, const u_char **pkt_data);
int pcap_next_ex(pcap_t *p, struct pcap_pkthdr **pkt_header, const u_char **pkt_data) {
	struct __bsd_pcap_crap_struct s;
	
	//this is what makes this not thread safe.
	static pcap_pkthdr header;
	s.header = &header;
	*pkt_header= &header;
	
	s.data = pkt_data;
	
	return(
		pcap_dispatch(p, 1, __bsd_pcap_crap_handler, (unsigned char *) &s)
	);
}

#endif



#ifndef WIN32
	//unix specific stuff
	Mutex MSignal;
#else
	//windows specific crap
	WCHAR*	wpcapdevice			= NULL;
	bool CheckIsWin95();
	bool IsWin95 = CheckIsWin95();
	void* GetAdapterFromList(void* device, int index);
#endif

//char errbuf[PCAP_ERRBUF_SIZE];
PCMode mode = noMode;
pcap_t *handle = NULL;
char*	pcapdevice			= NULL;
bool OpcodeLoadFailed = false;
bool DoLoop = false;
bool ProcessThreadRunning = false;
StreamPairManager stream_mgr(false);
PluginManager *plugins = NULL;
int debug=0;
char sniffhost[255]			= "";

//privacy stuff:
bool privacy_mode = false;
char privacy_name[64] = "";
char _zero_buffer[64] = "";


void handle_packet(unsigned char* x, const struct pcap_pkthdr* header, const unsigned char* data)
{
/*uint32_t ipHeaderLength, length, m_dataLength;
struct ip* m_ip;
struct udphdr *m_udp;
string stype="unknown";
char temp1[30],temp2[30];
*/
map<string,EQStreamPair *>::iterator stream_itr;
/*

	data+=sizeof (struct ether_header);

	// we start at the IP header
	m_ip = (struct ip*)data;

	if (m_ip->ip_p != 17 )
		return;

	// retrieve the total length from the header
	m_dataLength = ntohs (m_ip->ip_len);

	// use this length to caclulate the rest
	length = m_dataLength;

	// skip past the IP header
	ipHeaderLength	= m_ip->ip_hl * 4;
	length	-= ipHeaderLength;
	data += ipHeaderLength;

	// get the UDP header
	m_udp	 = (struct udphdr *) data;

	// skip over UDP header
	length	-= sizeof	(struct udphdr);
	data += (sizeof (struct udphdr));

	uint16_t sport=ntohs(m_udp->source);
	uint16_t dport=ntohs(m_udp->dest);
*/
	ETHERNET_FRAME	*eh;
	IP_HEADER		*iph;
	UDP_HEADER		*uh;
	uchar			*udp_data;
	int32			datalen = 0;
	int32			index = 0;
	
	eh = (ETHERNET_FRAME*) &data[index];
	index += sizeof(ETHERNET_FRAME);

	if (ntohs(eh->FrameType) != ETHERNET_FRAME_TYPE_IP) {
		#if DEBUGLEVEL >= 2
		cout << "Got a non-IP packet" << endl;
		#endif
		return;
	}
	
	iph = (IP_HEADER*) &data[index];
	index += sizeof(IP_HEADER);
	uh = (UDP_HEADER*) &data[index];
	index += sizeof(UDP_HEADER);
	udp_data = const_cast<unsigned char *>(&data[index]);
	datalen = ntohs(uh->length) - sizeof(UDP_HEADER);
	
	#if DEBUG_EtherPACKET >= 5
		struct in_addr	src, dst;
		src.s_addr = iph->src;
		dst.s_addr = iph->dest;
		cout << "Ether: " << inet_ntoa(src) << ":" << ntohs(uh->src_port) << " -> ";
		cout << inet_ntoa(dst) << ":" << ntohs(uh->dest_port) << " size: " << buflen << endl;
		#if DEBUG_EtherPACKET >= 9
			DumpPacket(buf, buflen);
		#endif
	#endif
	
	if (datalen == 0) {
		#if DEBUG_EtherPACKET >= 3
		cout << "Dropping Ether packet: datalen == 0" << endl;
		#endif
		return;
	}

	uint16 sport = ntohs(uh->src_port);
	uint16 dport = ntohs(uh->dest_port);
	if (sport <= 1024 || dport <= 1024) {
		#if DEBUG_EtherPACKET >= 3
		cout << "Dropping Ether packet: port <= 1024" << endl;
		#endif
		return;
	}
	/*if (!(iph->src == sniffIP || iph->dest == sniffIP)) {
		#if DEBUG_EtherPACKET >= 3
		cout << "Dropping Ether packet: ip != sniffIP" << endl;
		#endif
		return;
	}*/
	
	unsigned long sip = (unsigned long) iph->src;
	unsigned long dip = (unsigned long) iph->dest;


	stream_mgr.Process(sip, sport, dip, dport, header->ts, udp_data, datalen);
	
/*	
//	sprintf(temp1,"%lu:%u-%lu:%u",(unsigned long)m_ip->ip_src.s_addr,sport,(unsigned long)m_ip->ip_dst.s_addr,dport);
//	sprintf(temp2,"%lu:%u-%lu:%u",(unsigned long)m_ip->ip_dst.s_addr,dport,(unsigned long)m_ip->ip_src.s_addr,sport);

	if ((stream_itr=streams.find(temp1))!=streams.end() || (stream_itr=streams.find(temp2))!=streams.end()) {
		stream_itr->second->Process(udp_data, datalen, sip, sport, dip, dport, header->ts.tv_sec, header->ts.tv_usec);
	} else {
		//if (data[1]==0x01 || data[1]==0x02) {
			EQStreamPair *sp=new EQStreamPair;
			sp->Process(udp_data, datalen, sip, sport, dip, dport, header->ts.tv_sec, header->ts.tv_usec);
	
			streams[temp1]=sp;
		//}
	}*/
	
/*	//create our stream descriptor
	EQStreamInfo sinfo(sip, dip, sport, dport);
	
	EQStreamPair *spair;
	spair = streams.GetStream(sinfo);
	if(spair == NULL) {
		//new stream pair
		spair = new EQStreamPair();
		streams.AddStream(sinfo, spair);
	}
	
	spair->Process(udp_data, datalen, sip, sport, dip, dport, header->ts.tv_sec, header->ts.tv_usec);
*/
}


void CatchSignal(int);
void Exit();

void SetupGlobals() {
	if (signal(SIGINT, CatchSignal) == SIG_ERR) {
		cerr << "Could not set signal handler" << endl;
		Exit();
	}
	
	memset(_zero_buffer, 0, 64);
	
//	load_opcode_names();
	
	OpcodeLoadFailed = false;
	EQOpcodeManager = new RegularOpcodeManager();
	if(!EQOpcodeManager->LoadOpcodes(OPCODES_FILE)) {
		cerr << "Unable to load opcodes map. Names will not be resolved." << endl;
		delete EQOpcodeManager;
		EQOpcodeManager = new NullOpcodeManager();
		OpcodeLoadFailed = true;
	}
	printf("Opcodes file %s successfully loaded.\n", OPCODES_FILE);
}

void SetupCollectEnvironment() {
}


bool SetupLiveCollect() {
	struct bpf_program filter;
	char errbuf[PCAP_ERRBUF_SIZE];
	memset(errbuf, 0, sizeof(errbuf));
	char filter_app[200];
	bpf_u_int32 mask;
	bpf_u_int32 net;
    struct in_addr	in;
	
#if (defined(WIN32) && (!defined(_DEBUG)))
	SetPriorityClass(GetCurrentProcess(), HIGH_PRIORITY_CLASS);
#endif
	memset(filter_app, 0, sizeof(filter_app));
//	sprintf(filter_app, "udp and host %s", sniffhost);
	sprintf(filter_app, "udp");
	
#ifdef WIN32
	if (IsWin95) {
#endif
		if (pcapdevice == NULL) {
			pcapdevice = pcap_lookupdev(errbuf);
			if (pcapdevice == NULL) {
				cout << "Error: pcap_lookupdev: '" << errbuf << "'" << endl;
				Exit();
			}
		}
		pcap_lookupnet(pcapdevice, &net, &mask, errbuf);
#ifdef WIN32
	}
	else {
		if (wpcapdevice == NULL) {
			wpcapdevice = (WCHAR*) pcap_lookupdev(errbuf);
			if (wpcapdevice == NULL) {
				cout << "Error: pcap_lookupdev: '" << errbuf << "'" << endl;
				Exit();
			}
		}
		pcap_lookupnet((char*) wpcapdevice, &net, &mask, errbuf);
	}
#endif
net = 0;
#ifdef WIN32
	in.s_addr = ntohl(net);
#else
	in.s_addr = net;
#endif
#ifdef WIN32
	cout << "Sniffing on '";
	if (IsWin95)
		cout << pcapdevice;
	else {
		/*WCHAR* tmp = wpcapdevice;
		while (*tmp != 0) {
			cout << (char) *tmp++;
		}*/
		cout << ((char*)wpcapdevice);
	}
	cout << "', ";
#endif
	cout << "net=" << inet_ntoa(in) << ", target=" << sniffhost;
	//in.s_addr = eqpsl.sniffIP;
	//cout << " (" << inet_ntoa(in) << ")";
	cout << endl;
	
	if(privacy_mode) {
		cout << "Privacy mode enabled, privacy name: '" << privacy_name << "'" << endl;
	}
	
#ifdef WIN32
	if (IsWin95) {
#endif
		handle = pcap_open_live(pcapdevice, 8192, 1, 100, errbuf);
#ifdef WIN32
	}
	else
		handle = pcap_open_live((char*) wpcapdevice, 8192, 1, 100, errbuf);
#endif
	if (handle == NULL) {
		cout << "Error: pcap_open_live: '" << errbuf << "'" << endl;
		return(false);
	}
	pcap_compile(handle, &filter, filter_app, 0, net);
	if (pcap_setfilter(handle, &filter)) {
		cerr << "Error: pcap_setfilter("<<filter_app<<"): " << pcap_geterr(handle) << endl;
		return(false);
	}
	return(true);
}

bool SetupFileCollect(const char *filename) {
	char errbuf[PCAP_ERRBUF_SIZE];
	if ((handle=pcap_open_offline(filename,errbuf))==NULL) {
		cerr << "pcap_open_offline() on '" << filename << "' failed: " << errbuf << endl;
		return(false);
	}
	cout << "Successfully opened offline file: " << filename << endl;
	return(true);
}

void ClearCollectEnvironment() {
	
	cout << "Cleaning up collecting environment." << endl;
	
	DoLoop = false;
	while (ProcessThreadRunning) {
		Sleep(10);
	}
	
	stream_mgr.CloseAll();
	
}

void Exit() {
	ClearCollectEnvironment();
	
	safe_delete(plugins);
}

ThreadReturnType ProcessThread(void *tmp);

void CatchSignal(int sig_num) {
#ifndef WIN32
	MSignal.lock();
#endif
	//cerr << "Got signal: " << sig_num << endl;
	printf("Got signal %d\n", sig_num);
	DoLoop = false;
#ifndef WIN32
	MSignal.unlock();
#endif
}


void StartCollecting() {
#ifdef WIN32
	_beginthread(ProcessThread, 0, 0);
	//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
#else
	pthread_t thread;
	pthread_create(&thread, NULL, &ProcessThread, NULL);
	setpriority(PRIO_PROCESS, getpid(), 7);
#endif
	
	DoLoop = true;
//	uploading=false;
	
	
	stream_mgr.CloseAll();	
	
	if(mode == instantFileMode) {
		//just run through the file, not caring about time
		cout << "Starting instant file playback." << endl;
		while (DoLoop) {
			//read a number of packets, then distpatch them. Assuming
			//that we dont care if the other thread picks 
			if(pcap_dispatch(handle, 100, handle_packet, NULL) < 1) {
				break;	//done processing or error... stop either way
			}
			//process them
			stream_mgr.CheckQueues();
		}
	} else if(mode == timedFileMode) {
		cout << "Starting real-time file playback." << endl;
		
		struct pcap_pkthdr *header;
#ifdef WIN32
		u_char *frame;
#else
		const u_char *frame;
#endif
		
		//read the first packet...
		if(pcap_next_ex(handle, &header, &frame) < 1) {
			printf("Unable to read first packet.\n");
		} else {
			uint32 delta;
			struct timeval lasttime, curtime;
			
			//start with a base time of the first packet
			lasttime.tv_sec = header->ts.tv_sec;
			lasttime.tv_usec = header->ts.tv_usec;
			
			do {
				//sleep the delay until this packet is due
				curtime = header->ts;
				if(lasttime.tv_usec > curtime.tv_usec) {
					//if our usec wrapped, undo that effect
					curtime.tv_usec += 1000000;
					curtime.tv_sec -= 1;
				}
				//get our delta in ms
				delta = 1000 * (curtime.tv_sec - lasttime.tv_sec);
				delta += (curtime.tv_usec - lasttime.tv_usec) / 1000;
				//do our sleep
				if(delta > 0) {
					if(delta > 10000) {
						printf("Long sleep for %d ms\n", delta);
					}
					Sleep(delta);
				}
				//set our last time
				lasttime = curtime;
				
				//process the packet
				handle_packet(NULL, header, frame);
			} while(DoLoop && pcap_next_ex(handle, &header, &frame) == 1); 
		}
	} else if(mode == liveCollectMode) {
		cout << "Starting live network collect." << endl;
		while (DoLoop) {
			//changed loop to dispatch some packets and then give up and check DoLoop, then
			//repeat, to allow for interactive things to turn collecting on and off
			//the 100 is arbitrary
			pcap_dispatch(handle, 100, handle_packet, NULL);
		}
	} else {
		cout << "Unknown capture mode. Unable to do anything useful." << endl;
	}
	
	//process anything left in the queues one last time
	stream_mgr.CheckQueues();
	
	
	DoLoop = false;
	
	//wait for the processing thread to finish up
	while (ProcessThreadRunning) {
		Sleep(1);
	}
}

ThreadReturnType ProcessThread(void *tmp) {
	ProcessThreadRunning = true;
	DoLoop = true;
	while (DoLoop) {
#ifdef WIN32
/*		if(kbhit()){
			char input=getch();
			if(input==3)
				CatchSignal(2);
			else if(input==21){
				if(!uploading){
					uploading=true;
					_beginthread(DBUploadThread, 0, 0);
					//SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_ABOVE_NORMAL);
				}
				else
					cout << "You are already uploading items!!\n";

			}
			else if(input==12){
				uploading=false;
				cout << "Cleared upload flag, you may now upload items...\n";
			}

			input=0;
		}*/
#endif
		Timer::SetCurrentTime();
		
		timeout_manager.CheckTimeouts();
		stream_mgr.CheckQueues();
		
		Sleep(10);
	}
	//im not 100% sure why we closed this here:
	//pcap_close(handle);
	ProcessThreadRunning = false;
#ifndef WIN32
	return 0;
#endif
}

#ifdef WIN32

void* GetAdapterFromList(void* device, int index) {
/*	char* Adapter95;
#ifdef WIN32
	WCHAR* Adapter;
#endif
	int i;

#ifdef WIN32
	if (IsWin95) {			// Windows '95
#endif
		Adapter95 = (char*) device;
		for(i=0;i<index-1;i++){
			while(*Adapter95++!=0);
			if(*Adapter95==0)return NULL; 
		}
		return	Adapter95;
#ifdef WIN32
	}
	else {
		Adapter=(WCHAR*)device;
		for(i=0;i<index-1;i++){
			while(*Adapter++!=0);
			if(*Adapter==0)return NULL; 
		}
		return	Adapter;
	}
#endif	*/
	
	char ebuf[PCAP_ERRBUF_SIZE];
	//dunno what the return of this thing is:
	pcap_if_t *list, *olist;
	pcap_findalldevs(&list, ebuf);
	olist = list;
	
	while(list != NULL) {
		if(index == 1) {
			return(list->name);
		}
		index--;
		list = list->next;
	}
	
	//im prolly leaking the contents of the device list
	//but im to lazy to look up how to free it
	
	return(olist == NULL?NULL:olist->name);
}

bool CheckIsWin95() {
	DWORD dwVersion = GetVersion();
	DWORD dwWindowsMajorVersion = (DWORD)(LOBYTE(LOWORD(dwVersion)));

	if (dwVersion >= 0x80000000 && dwWindowsMajorVersion >= 4)			// Windows '95
		return true;
	else
		return false;
}


#endif








