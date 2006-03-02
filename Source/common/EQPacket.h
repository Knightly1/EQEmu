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
#ifndef _EQPACKET_H
#define _EQPACKET_H

#include "types.h"
#include <stdio.h>
#include <string.h>
#include <string>

#ifdef WIN32
	#include <time.h>
	#include <windows.h>
	#include <winsock.h>
#else
	#include <sys/time.h>
	#include <netinet/in.h>
#endif

#include "EQStreamType.h"
#include "emu_opcodes.h"
#include "op_codes.h"

using namespace std;

class OpcodeManager;
extern OpcodeManager *RawOpcodeManager;

class EQStream;

class EQPacket {
	friend class EQStream;
public:
	unsigned char *pBuffer;
	uint32 size;
	uint32 src_ip,dst_ip;
	uint16 src_port,dst_port;
	uint32 priority;
	timeval timestamp;

	virtual ~EQPacket();
	void DumpRawHeader(uint16 seq=0xffff, FILE *to = stdout) const;
	void DumpRawHeaderNoTime(uint16 seq=0xffff, FILE *to = stdout) const;
	void DumpRaw(FILE *to = stdout) const;

	void setSrcInfo(uint32 sip, uint16 sport) { src_ip=sip; src_port=sport; }
	void setDstInfo(uint32 dip, uint16 dport) { dst_ip=dip; dst_port=dport; }
	void setTimeInfo(uint32 ts_sec, uint32 ts_usec) { timestamp.tv_sec=ts_sec; timestamp.tv_usec=ts_usec; }
	void copyInfo(const EQPacket *p) { src_ip=p->src_ip; src_port=p->src_port;  dst_ip=p->dst_ip; dst_port=p->dst_port; timestamp.tv_sec=p->timestamp.tv_sec; timestamp.tv_usec=p->timestamp.tv_usec; }
	uint32 Size() const { return size+2; }

//no reason to have this method in zone or world
#if !defined(ZONE) && !defined(WORLD)
	uint16 GetRawOpcode() const { return(opcode); }
#endif	

	void SetOpcode(EmuOpcode op);
	const EmuOpcode GetOpcode() const;

	inline bool operator<(const EQPacket &rhs) {
		return (timestamp.tv_sec < rhs.timestamp.tv_sec || (timestamp.tv_sec==rhs.timestamp.tv_sec && timestamp.tv_usec < rhs.timestamp.tv_usec));
	}
	
protected:
	//this is just a cache so we dont look it up several times on Get()
	//and it is mutable so we can store the cached copy even on a const object
	mutable EmuOpcode emu_opcode;

	//this is a pointer to a pointer to make it less likely that a packet will
	//reference an invalid opcode manager when they are being reloaded.
	OpcodeManager **OpMgr;

	uint16 opcode;

	EQPacket(OpcodeManager **om, const uint16 op, const unsigned char *buf, const uint32 len);
	EQPacket(const EQPacket &p) { OpMgr=p.OpMgr; }
	EQPacket(OpcodeManager **om=NULL) { opcode=0; pBuffer=NULL; size=0; OpMgr=om; }

};

class EQLoginPacket;
class EQChatPacket;
class EQMailPacket;
class EQWorldPacket;
class EQZonePacket;

class EQProtocolPacket : public EQPacket {
public:
	EQProtocolPacket(uint16 op, const unsigned char *buf, uint32 len) : EQPacket(&RawOpcodeManager,op,buf,len) { } 
	EQProtocolPacket(const unsigned char *buf, uint32 len);
	bool combine(const EQProtocolPacket *rhs);
	uint32 serialize (unsigned char *dest) const;
	static bool ValidateCRC(const unsigned char *buffer, int length, uint32 Key);
	static uint32 Decompress(const unsigned char *buffer, const uint32 length, unsigned char *newbuf, uint32 newbufsize);
	static uint32 Compress(const unsigned char *buffer, const uint32 length, unsigned char *newbuf, uint32 newbufsize);
	static void ChatDecode(unsigned char *buffer, int size, int DecodeKey);
	static void ChatEncode(unsigned char *buffer, int size, int EncodeKey);
	EQProtocolPacket *Copy() { return new EQProtocolPacket(opcode,pBuffer,size); }
	EQLoginPacket *MakeLoginPacket() const;
	EQChatPacket *MakeChatPacket() const;
	EQMailPacket *MakeMailPacket() const;
	EQWorldPacket *MakeWorldPacket() const;
	EQZonePacket *MakeZonePacket() const;
	
private:
	EQProtocolPacket(const EQProtocolPacket &p) { OpMgr=p.OpMgr; }
};

class EQApplicationPacket : public EQPacket {
//	friend class EQProtocolPacket;
	friend class EQStream;
public:
	EQApplicationPacket(OpcodeManager **om) : EQPacket(om,0,NULL,0) { }
	EQApplicationPacket(OpcodeManager **om, const EmuOpcode op) : EQPacket(om, op,NULL,0) { SetOpcode(op); }
	EQApplicationPacket(OpcodeManager **om, const EmuOpcode op, const uint32 len) : EQPacket(om,op,NULL,len) { SetOpcode(op); }
	EQApplicationPacket(OpcodeManager **om, const EmuOpcode op, const unsigned char *buf, const uint32 len) : EQPacket(om,op,buf,len) { SetOpcode(op); }
	bool combine(const EQApplicationPacket *rhs);
	uint32 serialize (unsigned char *dest) const;
	uint32 Size() const { return size+app_opcode_size; }
	
	virtual EQApplicationPacket *Copy() const = 0;
	virtual EQStreamType GetPacketType() const = 0;
	
protected:
	int8 app_opcode_size;

private:

	//this constructor should only be used by subclasses, as it
	//assumes the first two bytes of buf are the opcode.
	EQApplicationPacket(const EQApplicationPacket &p) { OpMgr=p.OpMgr; emu_opcode = p.emu_opcode; }

};

void DumpPacketHex(const EQApplicationPacket* app);
void DumpPacketAscii(const EQApplicationPacket* app);
void DumpPacket(const EQApplicationPacket* app, bool iShowInfo = false);
void DumpPacketBin(const EQApplicationPacket* app);


#endif
