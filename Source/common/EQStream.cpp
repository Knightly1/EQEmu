/* 
	Copyright (C) 2005 Michael S. Finger

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
#ifdef WIN32
	#include <winsock2.h>
#endif
#include "debug.h"
#include <string>
#include <iomanip>
#include <iostream>
#include <vector>
#include <time.h>
#include <sys/types.h>
#ifdef WIN32
	#include <time.h>
#else
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <sys/time.h>
	#include <sys/socket.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include <arpa/inet.h>
#endif
#include "EQPacket.h"
#include "EQStream.h"
#include "EQStreamFactory.h"
#include "misc.h"
#include "Mutex.h"
#include "op_codes.h"
#include "CRC16.h"


void EQStream::init() {
	State=CLOSED;
	StreamType=UnknownStream;
	compressed=true;
	encoded=false;
	app_opcode_size=2;
	active_users = 0;
	Session=0;
	Key=0;
	MaxLen=0;
	NextInSeq=0;
	NextOutSeq=0;
	CombinedAppPacket=NULL;
	MaxAckReceived=-1;
	NextAckToSend=-1;
	LastAckSent=-1;
	LastSeqSent=-1;
	MaxSends=5;
	LastPacket=0;
	oversize_buffer=NULL;
	oversize_length=0;
	oversize_offset=0;
	Factory = NULL;
}

void EQStream::ProcessPacket(EQProtocolPacket *p)
{
uint32 processed=0,subpacket_length=0;
	if (p) {
		// Raw Application packet
		if (p->opcode > 0xff) {
			p->opcode = htons(p->opcode);  //byte order is backwards in the protocol packet
			EQApplicationPacket *ap = p->MakeApplicationPacket(app_opcode_size);
			InboundQueuePush(ap);
			return;
		}

		if (p->opcode!=OP_SessionRequest && p->opcode!=OP_SessionResponse && !Session) {
#ifdef EQN_DEBUG
			cout << "*** Session not initialized, packet ignored " << endl;
			p->DumpRaw();
#endif
			return;
		}

		switch (p->opcode) {
			case OP_Combined: {
				processed=0;
				while(processed<p->size) {
					subpacket_length=*(p->pBuffer+processed);
					EQProtocolPacket *subp=new EQProtocolPacket(p->pBuffer+processed+1,subpacket_length);
					subp->copyInfo(p);
					ProcessPacket(subp);
					delete subp;
					processed+=subpacket_length+1;
				}
			}
			break;
			case OP_AppCombined: {
				processed=0;
				while(processed<p->size) {
					EQApplicationPacket *ap;
					if ((subpacket_length=(unsigned char)*(p->pBuffer+processed))!=0xff) {
						ap=new EQApplicationPacket(p->pBuffer+processed+1,subpacket_length,app_opcode_size);
						processed+=subpacket_length+1;
					} else {
						subpacket_length=ntohs(*(uint16 *)(p->pBuffer+processed+1));
						ap=new EQApplicationPacket(p->pBuffer+processed+3,subpacket_length,app_opcode_size);
						processed+=subpacket_length+3;
					}
					ap->copyInfo(p);
					InboundQueuePush(ap);
				}
			}
			break;
			case OP_Packet: {
				uint16 seq=ntohs(*(uint16 *)(p->pBuffer));
				if (seq>=NextInSeq) {
					if (seq>NextInSeq) {
#ifdef COLLECTOR
						PacketQueue[seq]=p->Copy();
#else
						SendOutOfOrderAck(seq);
#endif
					} else {
						SetNextAckToSend(seq);
						NextInSeq++;
						EQProtocolPacket *subp=new EQProtocolPacket(p->pBuffer+2,p->size-2);
						subp->copyInfo(p);
						ProcessPacket(subp);
						delete subp;
					}
				} else {
#ifdef EQN_DEBUG
					cout << "*** Duplicate packet: Expecting Seq=" << NextInSeq << ", but got Seq=" << seq << endl;
					p->DumpRawHeader(seq);
					cout << endl;
#endif
				}
			}
			break;
			case OP_Fragment: {
				uint16 seq=ntohs(*(uint16 *)(p->pBuffer));
				if (seq>=NextInSeq) {
					if (seq>NextInSeq) {
#ifdef COLLECTOR
						PacketQueue[seq]=p->Copy();
#else
						SendOutOfOrderAck(seq);
#endif
					} else {
						SetNextAckToSend(seq);
						NextInSeq++;
						if (oversize_buffer) {
							memcpy(oversize_buffer+oversize_offset,p->pBuffer+2,p->size-2);
							oversize_offset+=p->size-2;
							//cout << "Oversized is " << oversize_offset << "/" << oversize_length << " (" << (p->size-2) << ") Seq=" << seq << endl;
							if (oversize_offset==oversize_length) {
								EQApplicationPacket *ap=new EQApplicationPacket(oversize_buffer,oversize_offset,app_opcode_size);
								ap->copyInfo(p);
								InboundQueuePush(ap);
								delete[] oversize_buffer;
								oversize_buffer=NULL;
								oversize_offset=0;
							}
						} else if (!oversize_buffer) {
							oversize_length=ntohl(*(uint32 *)(p->pBuffer+2));
							oversize_buffer=new unsigned char[oversize_length];
							memcpy(oversize_buffer,p->pBuffer+6,p->size-6);
							oversize_offset=p->size-6;
							//cout << "Oversized is " << oversize_offset << "/" << oversize_length << " (" << (p->size-6) << ") Seq=" << seq << endl;
						}
					}
				} else {
#ifdef EQN_DEBUG
					cout << "*** Duplicate packet: Expecting Seq=" << NextInSeq << ", but got Seq=" << seq << endl;
					p->DumpRawHeader(seq);
					cout << endl;
#endif
				}
			}
			break;
			case OP_KeepAlive: {
#ifndef COLLECTOR
				NonSequencedPush(new EQProtocolPacket(p->opcode,p->pBuffer,p->size));
#endif
			}
			break;
			case OP_Ack: {
				uint16 seq=ntohs(*(uint16 *)(p->pBuffer));
				SetMaxAckReceived(seq);
			}
			break;
			case OP_SessionRequest: {
				//cout << "Got OP_SessionRequest" << endl;
				init();
				SessionRequest *Request=(SessionRequest *)p->pBuffer;
				Session=ntohl(Request->Session);
				SetMaxLen(ntohl(Request->MaxLength));
#ifndef COLLECTOR
				NextInSeq=0;
				Key=0x11223344;
				SendSessionResponse();
#endif
				SetState(ESTABLISHED);
			}
			break;
			case OP_SessionResponse: {
				init();
				SessionResponse *Response=(SessionResponse *)p->pBuffer;
				SetMaxLen(ntohl(Response->MaxLength));
				Key=ntohl(Response->Key);
				NextInSeq=0;
				SetState(ESTABLISHED);
				if (!Session)
					Session=ntohl(Response->Session);
				compressed=(Response->Format&FLAG_COMPRESSED);
				encoded=(Response->Format&FLAG_ENCODED);

				// Kinda kludgy, but trie for now
				if (compressed) {
					if (remote_port==9000 || (remote_port==0 && p->src_port==9000))
						SetStreamType(WorldStream);
					else
						SetStreamType(ZoneStream);
				} else if (encoded)
					SetStreamType(ChatOrMailStream);
				else
					SetStreamType(LoginStream);
			}
			break;
			case OP_SessionDisconnect: {
				//NextInSeq=0;
				SetState(CLOSED);
			}
			break;
			case OP_OutOfOrderAck: {
#ifndef COLLECTOR
				uint16 seq=ntohs(*(uint16 *)(p->pBuffer));
				if (seq>GetMaxAckReceived()  && seq < NextOutSeq) {
					SetLastSeqSent(GetMaxAckReceived());
				}
#endif
			}
			break;
			case OP_SessionStatRequest: {
#ifndef COLLECTOR
				SessionStats *Stats=(SessionStats *)p->pBuffer;
				uint64 x=Stats->packets_recieved;
				Stats->packets_recieved=Stats->packets_sent;
				Stats->packets_sent=x;
				NonSequencedPush(new EQProtocolPacket(OP_SessionStatResponse,p->pBuffer,p->size));
#endif
			}
			break;
			case OP_SessionStatResponse: {
			}
			break;
			case OP_OutOfSession: {
			}
			break;
			default:
				EQApplicationPacket *ap = p->MakeApplicationPacket(app_opcode_size);
				InboundQueuePush(ap);
				break;
		}
	}
}

void EQStream::QueuePacket(const EQApplicationPacket *p, bool ack_req)
{
	EQApplicationPacket *newp = p->Copy();

	FastQueuePacket(&newp, ack_req);
}

void EQStream::FastQueuePacket(EQApplicationPacket **p, bool ack_req)
{
EQApplicationPacket *pack=*p;
	*p=0;
	if (!ack_req) {
		NonSequencedPush(new EQProtocolPacket(pack->opcode,pack->pBuffer,pack->size));
		delete pack;
	} else {
		SendPacket(pack);
	}
	return;

	MCombinedAppPacket.lock();
	if (!CombinedAppPacket)
		CombinedAppPacket=pack;
	else {
		// Try to combine this one with the one(s) that are there
		if (CombinedAppPacket->combine(pack))
			delete pack;
		else {
			// Couldn't combine them, so queue the currently
			// combined ones and hold this one for combining
			EQApplicationPacket *combpack=CombinedAppPacket;
			CombinedAppPacket=pack;
			MCombinedAppPacket.unlock();

			SendPacket(combpack);

			return;
		}
	}
	MCombinedAppPacket.unlock();
#ifndef COLLECTOR
	if (Factory)
		Factory->SignalWriter();
#endif
}

void EQStream::SendPacket(EQApplicationPacket *p)
{
uint32 chunksize,used;
uint32 length;

	// Convert the EQApplicationPacket to 1 or more EQProtocolPackets
	if (p->size>(MaxLen-8)) { // proto-op(2), seq(2), app-op(2) ... data ... crc(2)
		//cout << "Making oversized packet for: " << endl;
		//cout << p->size << endl;
		//p->DumpRawHeader();
		//dump_message(p->pBuffer,p->size,timestamp());
		//cout << p->size << endl;
		unsigned char *tmpbuff=new unsigned char[p->size+2];
		//cout << hex << (int)tmpbuff << dec << endl;
		length=p->serialize(tmpbuff);

		EQProtocolPacket *out=new EQProtocolPacket(OP_Fragment,NULL,MaxLen-4);
		*(uint32 *)(out->pBuffer+2)=htonl(p->Size());
		memcpy(out->pBuffer+6,tmpbuff,MaxLen-10);
		used=MaxLen-10;
		SequencedPush(out);
		//cout << "Chunk #" << ++i << " size=" << used << ", length-used=" << (length-used) << endl;
		while (used<length) {
			out=new EQProtocolPacket(OP_Fragment,NULL,MaxLen-4);
			chunksize=min(length-used,MaxLen-6);
			memcpy(out->pBuffer+2,tmpbuff+used,chunksize);
			out->size=chunksize+2;
			SequencedPush(out);
			used+=chunksize;
			//cout << "Chunk #"<< ++i << " size=" << chunksize << ", length-used=" << (length-used) << endl;
		}
		//cerr << "1: Deleting 0x" << hex << (uint32)(p) << dec << endl;
		delete p;
		delete[] tmpbuff;
	} else {
		EQProtocolPacket *out=new EQProtocolPacket(OP_Packet,NULL,p->Size()+2);
		p->serialize(out->pBuffer+2);
		SequencedPush(out);
		//cerr << "2: Deleting 0x" << hex << (uint32)(p) << dec << endl;
		delete p;
	}
}

void EQStream::SequencedPush(EQProtocolPacket *p)
{
#ifndef COLLECTOR
	MOutboundQueue.lock();
	*(uint16 *)(p->pBuffer)=htons(NextOutSeq);
	SequencedQueue[NextOutSeq]=p;
	NextOutSeq++;
	MOutboundQueue.unlock();
	if (Factory)
		Factory->SignalWriter();
#endif
}

void EQStream::NonSequencedPush(EQProtocolPacket *p)
{
#ifndef COLLECTOR
	MOutboundQueue.lock();
	NonSequencedQueue.push_back(p);
	MOutboundQueue.unlock();
	if (Factory)
		Factory->SignalWriter();
#endif
}

void EQStream::SendAck(uint16 seq)
{
uint16 Seq=htons(seq);
	SetLastAckSent(seq);
	NonSequencedPush(new EQProtocolPacket(OP_Ack,(unsigned char *)&Seq,sizeof(uint16)));
}

void EQStream::SendOutOfOrderAck(uint16 seq)
{
uint16 Seq=htons(seq);
	NonSequencedPush(new EQProtocolPacket(OP_OutOfOrderAck,(unsigned char *)&Seq,sizeof(uint16)));
}

void EQStream::Write(int eq_fd)
{
vector<EQProtocolPacket *> ReadyToSend;
long maxack;
bool SeqEmpty=false,NonSeqEmpty=false;
map<uint16, EQProtocolPacket *>::iterator sitr;
vector<EQProtocolPacket *>::iterator nsitr;

	MCombinedAppPacket.lock();
	EQApplicationPacket *CombPack=CombinedAppPacket;
	CombinedAppPacket=NULL;
	MCombinedAppPacket.unlock();

	if (CombPack) {
		SendPacket(CombPack);
	}

	// If we got more packets to we need to ack, send an ack on the highest one
	MAcks.lock();
	maxack=MaxAckReceived;
	if (NextAckToSend>LastAckSent)
		SendAck(NextAckToSend);
	MAcks.unlock();

	// Lock the outbound queues while we process
	MOutboundQueue.lock();

	// Adjust where we start sending in case we get a late ack
	if (maxack>LastSeqSent)
		LastSeqSent=maxack;

	// Place to hold the base packet t combine into
	EQProtocolPacket *p=NULL;

	// Find the next sequenced packet to send frm the "queue"
	if ((sitr=SequencedQueue.find(LastSeqSent))!=SequencedQueue.end())
		sitr++;
	else
		sitr=SequencedQueue.begin();

	// Get the first non-sequenced packet in the list
	nsitr=NonSequencedQueue.begin();

	int count=0;
	// Loop until both are empty or MaxSends is reached
	while(!SeqEmpty || !NonSeqEmpty)  {

		// See if there are more non-sequenced packets left
		if (nsitr!=NonSequencedQueue.end()) {

			if (!p) {
				// If we don't have a packet to try to combine into, use this one as the base
				// And remove it form the queue
				p=*nsitr;
				NonSequencedQueue.erase(nsitr);
				nsitr=NonSequencedQueue.begin();

			} else if (!p->combine(*nsitr)) {
				// Tryint to combine this packet with the base didn't work (too big maybe)
				// So just send the base packet (we'll try this packet again later)
				++count;
				ReadyToSend.push_back(p);
				p=NULL;
			} else {
				// Combine worked, so just remove this packet and it's spot in the queue
				delete *nsitr;
				NonSequencedQueue.erase(nsitr);
				nsitr=NonSequencedQueue.begin();
			}
		} else {
			// No more non-sequenced packets
			NonSeqEmpty=true;
		}

		if (ReadyToSend.size()>=MaxSends) {
			// Sent enough this round, lets stop to be fair
			break;
		}

		if (sitr!=SequencedQueue.end()) {
			if (!p) {
				// If we don't have a packet to try to combine into, use this one as the base
				// Copy it first as it will still live until it is acked
				p=sitr->second->Copy();
				LastSeqSent=sitr->first;
				sitr++;
			} else if (!p->combine(sitr->second)) {
				// Trying to combine this packet with the base didn't work (too big maybe)
				// So just send the base packet (we'll try this packet again later)
				++count;
				ReadyToSend.push_back(p);
				p=NULL;
			} else {
				// Combine worked
				LastSeqSent=sitr->first;
				sitr++;
			}
		} else {
			// No more non-sequenced packets
			SeqEmpty=true;
		}

		if (ReadyToSend.size()>=MaxSends) {
			// Sent enough this round, lets stop to be fair
			break;
		}
	}
	// Unlock the queue
	MOutboundQueue.unlock();

	// We have a packet still, must have run out of both seq and non-seq, so send it
	if (p) {
		ReadyToSend.push_back(p);
	}

	// Send all the packets we "made"
	while(ReadyToSend.size()) {
		nsitr=ReadyToSend.begin();
		WritePacket(eq_fd,*nsitr);
		delete *nsitr;
		ReadyToSend.erase(nsitr);
	}
}

void EQStream::WritePacket(int eq_fd, EQProtocolPacket *p)
{
uint32 length;
sockaddr_in address;
unsigned char tmpbuffer[1024];
	address.sin_family = AF_INET;
	address.sin_addr.s_addr=remote_ip;
	address.sin_port=remote_port;
#ifdef NOWAY
	uint32 ip=address.sin_addr.s_addr;
	cout << "Sending to: " 
		<< (int)*(unsigned char *)&ip
		<< "." << (int)*((unsigned char *)&ip+1)
		<< "." << (int)*((unsigned char *)&ip+2)
		<< "." << (int)*((unsigned char *)&ip+3)
		<< "," << (int)ntohs(address.sin_port) << "(" << p->size << ")" << endl;

	p->DumpRaw();
	cout << "-------------" << endl;
#endif
	length=p->serialize(buffer);
	if (p->opcode!=OP_SessionRequest && p->opcode!=OP_SessionResponse) {
		if (compressed) {
			uint32 newlen=EQProtocolPacket::Compress(buffer,length,tmpbuffer,1024);
			memcpy(buffer,tmpbuffer,newlen);
			length=newlen;
		}
		if (encoded) {
			EQProtocolPacket::ChatEncode(buffer,length,Key);
		}

		*(uint16 *)(buffer+length)=htons(CRC16(buffer,length,Key));
		length+=2;
	}
	//dump_message_column(buffer,length,"Writer: ");
	sendto(eq_fd,(char *)buffer,length,0,(sockaddr *)&address,sizeof(address));
}

EQProtocolPacket *EQStream::Read(int eq_fd, sockaddr_in *from)
{
int socklen;
int length=0;
unsigned char buffer[2048];
EQProtocolPacket *p=NULL;
char temp[15];

	socklen=sizeof(sockaddr);
#ifdef WIN32
	length=recvfrom(eq_fd, (char *)buffer, 2048, 0, (struct sockaddr*)from, (int *)&socklen);
#else
	length=recvfrom(eq_fd, buffer, 2048, 0, (struct sockaddr*)from, (socklen_t *)&socklen);
#endif

	if (length>=2) {
		p=new EQProtocolPacket(buffer[1],&buffer[2],length-2);

		uint32 ip=from->sin_addr.s_addr;
		sprintf(temp,"%d.%d.%d.%d:%d",
			*(unsigned char *)&ip,
			*((unsigned char *)&ip+1),
			*((unsigned char *)&ip+2),
			*((unsigned char *)&ip+3),
			ntohs(from->sin_port));
		//cout << timestamp() << "Data from: " << temp << " OpCode 0x" << hex << setw(2) << setfill('0') << (int)p->opcode << dec << endl;
		//dump_message(p->pBuffer,p->size,timestamp());
		
	}
	return p;
}

void EQStream::SendSessionResponse()
{
EQProtocolPacket *out=new EQProtocolPacket(OP_SessionResponse,NULL,sizeof(SessionResponse));
	SessionResponse *Response=(SessionResponse *)out->pBuffer;
	Response->Session=htonl(Session);
	Response->MaxLength=htonl(MaxLen);
	Response->UnknownA=2;
	Response->Format=0;
	if (compressed)
		Response->Format|=FLAG_COMPRESSED;
	if (encoded)
		Response->Format|=FLAG_ENCODED;
	Response->Key=htonl(Key);

	out->size=sizeof(SessionResponse);

	NonSequencedPush(out);
}

void EQStream::SendSessionRequest()
{
EQProtocolPacket *out=new EQProtocolPacket(OP_SessionRequest,NULL,sizeof(SessionRequest));
	SessionRequest *Request=(SessionRequest *)out->pBuffer;
	memset(Request,0,sizeof(SessionRequest));
	Request->Session=htonl(time(NULL));
	Request->MaxLength=htonl(512);

	NonSequencedPush(out);
}

void EQStream::SendDisconnect()
{
	if(GetState() != ESTABLISHED)
		return;
	
	EQProtocolPacket *out=new EQProtocolPacket(OP_SessionDisconnect,NULL,sizeof(uint32));
	*(uint32 *)out->pBuffer=htonl(Session);
	NonSequencedPush(out);
	
	SetState(CLOSING);
}

void EQStream::InboundQueuePush(EQApplicationPacket *p)
{
	MInboundQueue.lock();
	InboundQueue.push_back(p);
	MInboundQueue.unlock();
}

EQApplicationPacket *EQStream::PopPacket()
{
EQApplicationPacket *p=NULL;

	MInboundQueue.lock();
	if (InboundQueue.size()) {
		vector<EQApplicationPacket *>::iterator itr=InboundQueue.begin();
		p=*itr;
		InboundQueue.erase(itr);
	}
	MInboundQueue.unlock();

	return p;
}

EQApplicationPacket *EQStream::PeekPacket()
{
EQApplicationPacket *p=NULL;

	MInboundQueue.lock();
	if (InboundQueue.size()) {
		vector<EQApplicationPacket *>::iterator itr=InboundQueue.begin();
		p=*itr;
	}
	MInboundQueue.unlock();

	return p;
}

void EQStream::InboundQueueClear()
{
EQApplicationPacket *p=NULL;

	MInboundQueue.lock();
	if (InboundQueue.size()) {
		vector<EQApplicationPacket *>::iterator itr;
		for(itr=InboundQueue.begin();itr!=InboundQueue.end();itr++) {
			p=*itr;
			delete p;
		}
		InboundQueue.clear();
	}
	MInboundQueue.unlock();
}

bool EQStream::HasOutgoingData()
{
bool flag;
	
	//once closed, we have nothing more to say
	if(CheckClosed())
		return(false);
	
	MOutboundQueue.lock();
	flag=(!NonSequencedQueue.empty());
	if (!flag) {
		map<uint16, EQProtocolPacket *>::reverse_iterator itr;
		if ((itr=SequencedQueue.rbegin())!=SequencedQueue.rend()) {
			if (itr->first > LastSeqSent) {
				flag=true;
			}
		}
	}
	MOutboundQueue.unlock();

	if (!flag) {
		MAcks.lock();
		flag= (NextAckToSend>LastAckSent);
		MAcks.unlock();
	}

	if (!flag) {
		MCombinedAppPacket.lock();
		flag=(CombinedAppPacket!=NULL);
		MCombinedAppPacket.unlock();
	}

	return flag;
}

void EQStream::OutboundQueueClear()
{
EQProtocolPacket *p=NULL;

	MOutboundQueue.lock();
	if (NonSequencedQueue.size()) {
		vector<EQProtocolPacket *>::iterator itr;
		for(itr=NonSequencedQueue.begin();itr!=NonSequencedQueue.end();itr++) {
			p=*itr;
			delete p;
		}
		NonSequencedQueue.clear();
	}
	if (SequencedQueue.size()) {
		map<uint16, EQProtocolPacket *>::iterator itr;
		for(itr=SequencedQueue.begin();itr!=SequencedQueue.end();itr++) {
			p=itr->second;
			delete p;
		}
		SequencedQueue.clear();
	}
	MOutboundQueue.unlock();
}

void EQStream::Process(const unsigned char *buffer, const uint32 length)
{
static unsigned char newbuffer[2048];
uint32 newlength=0;
	if (EQProtocolPacket::ValidateCRC(buffer,length,Key)) {
		if (encoded) {
			EQProtocolPacket::ChatDecode(newbuffer,newlength-(newlength>8?2:0),Key);
		} else {
			memcpy(newbuffer,buffer,length);
			newlength=length;
		}
		if (compressed) {
			newlength=EQProtocolPacket::Decompress(buffer,length,newbuffer,2048);
		}
		if (buffer[1]!=0x01 && buffer[1]!=0x02 && buffer[1]!=0x1d)
			newlength-=2;
		EQProtocolPacket p(newbuffer,newlength);
		ProcessPacket(&p);
	} else {
#ifdef EQN_DEBUG
		cout << "Incoming packet failed checksum:" <<endl;
		dump_message_column(const_cast<unsigned char *>(buffer),length,"CRC failed: ");
#endif
	}
}

long EQStream::GetMaxAckReceived()
{
	MAcks.lock();
	long l=MaxAckReceived;
	MAcks.unlock();

	return l;
}

long EQStream::GetNextAckToSend()
{
	MAcks.lock();
	long l=NextAckToSend;
	MAcks.unlock();

	return l;
}

long EQStream::GetLastAckSent()
{
	MAcks.lock();
	long l=LastAckSent;
	MAcks.unlock();

	return l;
}

void EQStream::SetMaxAckReceived(uint32 seq)
{
map<unsigned short, EQProtocolPacket *>::iterator itr;
	MAcks.lock();
	MaxAckReceived=seq;
	MAcks.unlock();
	MOutboundQueue.lock();
	if (long(seq) > LastSeqSent)
		LastSeqSent=seq;

	while((itr=SequencedQueue.begin())!=SequencedQueue.end() && itr->first<=seq) {
		delete itr->second;
		SequencedQueue.erase(itr);
	}
	MOutboundQueue.unlock();

}

void EQStream::SetNextAckToSend(uint32 seq)
{
	MAcks.lock();
	NextAckToSend=seq;
	MAcks.unlock();
#ifndef COLLECTOR
	if (Factory)
		Factory->SignalWriter();
#endif
}

void EQStream::SetLastAckSent(uint32 seq)
{
	MAcks.lock();
	LastAckSent=seq;
	MAcks.unlock();
}

void EQStream::SetLastSeqSent(uint32 seq)
{
	MOutboundQueue.lock();
	LastSeqSent=seq;
	MOutboundQueue.unlock();
}

#ifdef COLLECTOR
void EQStream::ProcessQueue()
{
	if (PacketQueue.size()) {
		map<unsigned short,EQProtocolPacket *>::iterator head;
		while((head=PacketQueue.begin()) != PacketQueue.end() && head->first <= NextInSeq) {
			EQProtocolPacket *qp=head->second;
			if ( head->first == NextInSeq) {
				ProcessPacket(qp);
			}
			PacketQueue.erase(head);
			delete qp;
		}
	}
}
#endif

void EQStream::SetStreamType(EQStreamType type)
{
	StreamType=type;
	switch (StreamType) {
		case LoginStream:
			app_opcode_size=1;
			compressed=false;
			encoded=false;
			break;
		case ChatOrMailStream:
		case ChatStream:
		case MailStream:
			app_opcode_size=1;
			compressed=false;
			encoded=true;
			break;
		case ZoneStream:
		case WorldStream:
		default:
			app_opcode_size=2;
			compressed=true;
			encoded=false;
			break;
	}
}

string EQStream::StreamTypeString(EQStreamType t)
{
	switch (t) {
		case LoginStream:
			return "Login";
			break;
		case WorldStream:
			return "World";
			break;
		case ZoneStream:
			return "Zone";
			break;
		case ChatOrMailStream:
			return "Chat/Mail";
			break;
		case ChatStream:
			return "Chat";
			break;
		case MailStream:
			return "Mail";
			break;
		case UnknownStream:
			return "Unknown";
			break;
	}
	return "UnknownType";
}

