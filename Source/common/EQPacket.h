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

#ifdef WIN32
	#include <time.h>
	#include <winsock2.h>
#else
	#include <sys/time.h>
	#include <netinet/in.h>
#endif

#include "eq_opcodes.h"
#include "op_codes.h"

class OpcodeManager;
extern OpcodeManager *EQOpcodeManager;

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

	~EQPacket();
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

	inline bool operator<(const EQPacket &rhs) {
		return (timestamp.tv_sec < rhs.timestamp.tv_sec || (timestamp.tv_sec==rhs.timestamp.tv_sec && timestamp.tv_usec < rhs.timestamp.tv_usec));
	}
	
protected:
	uint16 opcode;

	EQPacket(const uint16 op, const unsigned char *buf, const uint32 len);
	EQPacket(const EQPacket &p) { }
	EQPacket() { opcode=0; pBuffer=NULL; size=0; }

};

class EQApplicationPacket;

class EQProtocolPacket : public EQPacket {
public:
	EQProtocolPacket(uint16 op, const unsigned char *buf, uint32 len) : EQPacket(op,buf,len) { } 
	EQProtocolPacket(const unsigned char *buf, uint32 len);
	bool combine(const EQProtocolPacket *rhs);
	uint32 serialize (unsigned char *dest) const;
	static bool ValidateCRC(const unsigned char *buffer, int length, uint32 Key);
	static uint32 Decompress(const unsigned char *buffer, const uint32 length, unsigned char *newbuf, uint32 newbufsize);
	static uint32 Compress(const unsigned char *buffer, const uint32 length, unsigned char *newbuf, uint32 newbufsize);
	static void ChatDecode(unsigned char *buffer, int size, int DecodeKey);
	static void ChatEncode(unsigned char *buffer, int size, int EncodeKey);
	EQProtocolPacket *Copy() { return new EQProtocolPacket(opcode,pBuffer,size); }
	EQApplicationPacket *MakeApplicationPacket(uint8 opcode_size=0) const;
	
private:
	EQProtocolPacket(const EQProtocolPacket &p) { }
};

class EQApplicationPacket : public EQPacket {
	friend class EQProtocolPacket;
	friend class EQStream;
public:
	EQApplicationPacket() : EQPacket(0,NULL,0) { emu_opcode = OP_Unknown; app_opcode_size=default_opcode_size; }
	EQApplicationPacket(const EmuOpcode op) : EQPacket(0,NULL,0) { SetOpcode(op); app_opcode_size=default_opcode_size; }
	EQApplicationPacket(const EmuOpcode op, const uint32 len) : EQPacket(0,NULL,len) { SetOpcode(op); app_opcode_size=default_opcode_size; }
	EQApplicationPacket(const EmuOpcode op, const unsigned char *buf, const uint32 len) : EQPacket(0,buf,len) { SetOpcode(op); app_opcode_size=default_opcode_size; }
	bool combine(const EQApplicationPacket *rhs);
	uint32 serialize (unsigned char *dest) const;
	uint32 Size() const { return size+app_opcode_size; }
	EQApplicationPacket *Copy() const {
		EQApplicationPacket *it = new EQApplicationPacket;
		it->pBuffer= new unsigned char[size];
		memcpy(it->pBuffer,pBuffer,size);
		it->size=size;
		it->opcode = opcode;
		it->emu_opcode = emu_opcode;
		return(it);
	}
	
	void SetOpcodeSize(uint8 s) { app_opcode_size=s; }
	void SetOpcode(EmuOpcode op);
	const EmuOpcode GetOpcodeConst() const;
	inline const EmuOpcode GetOpcode() const { return(GetOpcodeConst()); }
	//caching version of get
	inline const EmuOpcode GetOpcode() { EmuOpcode r = GetOpcodeConst(); emu_opcode = r; return(r); }

	static uint8 default_opcode_size;

protected:
	//this is just a cache so we dont look it up several times on Get()
	EmuOpcode emu_opcode;

private:
	//this constructor should only be used by EQProtocolPacket, as it
	//assumes the first two bytes of buf are the opcode.
	EQApplicationPacket(const unsigned char *buf, uint32 len, uint8 opcode_size=0);
	EQApplicationPacket(const EQApplicationPacket &p) { emu_opcode = OP_Unknown; app_opcode_size=default_opcode_size; }

	uint8 app_opcode_size;
};

#endif
