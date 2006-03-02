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
#include "debug.h"
#include <stdio.h>
#include <iostream>
#include <iomanip>
#include "EQPacket.h"
#include "misc.h"
#include "op_codes.h"
#include "CRC16.h"
#include "opcodemgr.h"
#include "packet_dump.h"

using namespace std;

OpcodeManager *RawOpcodeManager=NULL;
extern OpcodeManager *WorldOpcodeManager;

EQPacket::EQPacket(OpcodeManager **om, const uint16 op, const unsigned char *buf, uint32 len)
{
	this->opcode=op;
	this->pBuffer=NULL;
	this->size=0;
	if (len>0) {
		this->size=len;
		pBuffer= new unsigned char[len];
		if (buf) {
			memcpy(this->pBuffer,buf,len);
		}  else {
			memset(this->pBuffer,0,len);
		}
	}
	this->OpMgr=om;
	this->emu_opcode = OP_Unknown;

}

uint32 EQProtocolPacket::serialize(unsigned char *dest) const
{
	if (opcode>0xff)  {
		*(uint16 *)dest=opcode;
	} else {
		*(dest)=0;
		*(dest+1)=opcode;
	}
	memcpy(dest+2,pBuffer,size);

	return size+2;
}

uint32 EQApplicationPacket::serialize(unsigned char *dest) const
{
	if (app_opcode_size==1)
		*(unsigned char *)dest=opcode;
	else
		*(uint16 *)dest=opcode;

	memcpy(dest+app_opcode_size,pBuffer,size);

	return size+app_opcode_size;
}

EQPacket::~EQPacket()
{
	if (pBuffer)
		delete[] pBuffer;
	pBuffer=NULL;
}

void EQPacket::DumpRawHeader(uint16 seq, FILE *to) const
{
	if (timestamp.tv_sec) {
		char temp[20];
		strftime(temp,20,"%F %T",localtime((const time_t *)&timestamp.tv_sec));
		fprintf(to, "%s.%06lu ",temp,timestamp.tv_usec);
	}
	if (src_ip) {
		string sIP,dIP;;
		sIP=long2ip(src_ip);
		dIP=long2ip(dst_ip);
		fprintf(to, "[%s:%d->%s:%d]\n",sIP.c_str(),src_port,dIP.c_str(),dst_port);
	}
	if (seq != 0xffff)
		fprintf(to, "[Seq=%u] ",seq);
	string name;
	if(OpMgr != NULL && *OpMgr != NULL)
		name = (*OpMgr)->EQToName(opcode);
	fprintf(to, "[OpCode 0x%04x (%s) Size=%lu]\n",opcode,name.c_str(),size);
}

void EQPacket::DumpRawHeaderNoTime(uint16 seq, FILE *to) const
{
	if (src_ip) {
		string sIP,dIP;;
		sIP=long2ip(src_ip);
		dIP=long2ip(dst_ip);
		fprintf(to, "[%s:%d->%s:%d] ",sIP.c_str(),src_port,dIP.c_str(),dst_port);
	}
	if (seq != 0xffff)
		fprintf(to, "[Seq=%u] ",seq);
	
	string name;
	if(OpMgr != NULL && *OpMgr != NULL)
		name = (*OpMgr)->EQToName(opcode);
	
	fprintf(to, "[OpCode 0x%04x (%s) Size=%lu]\n",opcode,name.c_str(),size);
}

void EQPacket::DumpRaw(FILE *to) const
{
	DumpRawHeader();
	if (pBuffer && size)
		dump_message_column(pBuffer, size, " ", to);
	fprintf(to, "\n");
}

EQProtocolPacket::EQProtocolPacket(const unsigned char *buf, uint32 len)
{
uint32 offset;
	opcode=ntohs(*(const uint16 *)buf);
	offset=2;

	if (len-offset) {
		pBuffer= new unsigned char[len-offset];
		memcpy(pBuffer,buf+offset,len-offset);
		size=len-offset;
	} else {
		pBuffer=NULL;
		size=0;
	}
	OpMgr=&RawOpcodeManager;
}

bool EQProtocolPacket::combine(const EQProtocolPacket *rhs)
{
bool result=false;
	if (opcode==OP_Combined && size+rhs->size+5<256) {
		unsigned char *tmpbuffer=new unsigned char [size+rhs->size+3];
		memcpy(tmpbuffer,pBuffer,size);
		uint32 offset=size;
		tmpbuffer[offset++]=rhs->Size();
		offset+=rhs->serialize(tmpbuffer+offset);
		size=offset;
		delete[] pBuffer;
		pBuffer=tmpbuffer;
		result=true;
	} else if (size+rhs->size+7<256) {
		unsigned char *tmpbuffer=new unsigned char [size+rhs->size+6];
		uint32 offset=0;
		tmpbuffer[offset++]=Size();
		offset+=serialize(tmpbuffer+offset);
		tmpbuffer[offset++]=rhs->Size();
		offset+=rhs->serialize(tmpbuffer+offset);
		size=offset;
		delete[] pBuffer;
		pBuffer=tmpbuffer;
		opcode=OP_Combined;
		result=true;
	}

	return result;

}

bool EQApplicationPacket::combine(const EQApplicationPacket *rhs)
{
uint32 newsize=0, offset=0;
unsigned char *tmpbuffer=NULL;

	if (opcode!=OP_AppCombined) {
		newsize=app_opcode_size+size+(size>254?3:1)+app_opcode_size+rhs->size+(rhs->size>254?3:1);
		tmpbuffer=new unsigned char [newsize];
		offset=0;
		if (size>254) {
			tmpbuffer[offset++]=0xff;
			*(uint16 *)(tmpbuffer+offset)=htons(size);
			offset+=1;
		} else {
			tmpbuffer[offset++]=size;
		}
		offset+=serialize(tmpbuffer+offset);
	} else {
		newsize=size+app_opcode_size+rhs->size+(rhs->size>254?3:1);
		tmpbuffer=new unsigned char [newsize];
		memcpy(tmpbuffer,pBuffer,size);
		offset=size;
	}

	if (rhs->size>254) {
		tmpbuffer[offset++]=0xff;
		*(uint16 *)(tmpbuffer+offset)=htons(rhs->size);
		offset+=1;
	} else {
		tmpbuffer[offset++]=rhs->size;
	}
	offset+=rhs->serialize(tmpbuffer+offset);

	size=offset;
	opcode=OP_AppCombined;

	delete[] pBuffer;
	pBuffer=tmpbuffer;

	return true;
}

bool EQProtocolPacket::ValidateCRC(const unsigned char *buffer, int length, uint32 Key)
{
bool valid=false;
	// OP_SessionRequest, OP_SessionResponse, OP_OutOfSession are not CRC'd
	if (buffer[0]==0x00 && (buffer[1]==OP_SessionRequest || buffer[1]==OP_SessionResponse || buffer[1]==OP_OutOfSession)) {
		valid=true;
	} else {
		uint16 comp_crc=CRC16(buffer,length-2,Key);
		uint16 packet_crc=ntohs(*(const uint16 *)(buffer+length-2));
#ifdef EQN_DEBUG
		if (packet_crc && comp_crc != packet_crc) {
			cout << "CRC mismatch: comp=" << hex << comp_crc << ", packet=" << packet_crc << dec << endl;
		}
#endif
		valid = (!packet_crc || comp_crc == packet_crc);
	}
	return valid;
}

uint32 EQProtocolPacket::Decompress(const unsigned char *buffer, const uint32 length, unsigned char *newbuf, uint32 newbufsize)
{
uint32 newlen=0;
uint32 flag_offset=0;
	newbuf[0]=buffer[0];
	if (buffer[0]==0x00) {
		flag_offset=2;
		newbuf[1]=buffer[1];
	} else
		flag_offset=1;

	if (length>2 && buffer[flag_offset]==0x5a)  {
		newlen=Inflate(const_cast<unsigned char *>(buffer+flag_offset+1),length-(flag_offset+1)-2,newbuf+flag_offset,newbufsize-flag_offset)+2;
		newbuf[newlen++]=buffer[length-2];
		newbuf[newlen++]=buffer[length-1];
	} else if (length>2 && buffer[flag_offset]==0xa5) {
		memcpy(newbuf+flag_offset,buffer+flag_offset+1,length-(flag_offset+1));
		newlen=length-1;
	} else {
		memcpy(newbuf,buffer,length);
		newlen=length;
	}

	return newlen;
}

uint32 EQProtocolPacket::Compress(const unsigned char *buffer, const uint32 length, unsigned char *newbuf, uint32 newbufsize) {
uint32 flag_offset=1,newlength;
	//dump_message_column(buffer,length,"Before: ");
	newbuf[0]=buffer[0];
	if (buffer[0]==0) {
		flag_offset=2;
		newbuf[1]=buffer[1];
	}
	if (length>30) {
		newlength=Deflate(const_cast<unsigned char *>(buffer+flag_offset),length-flag_offset,newbuf+flag_offset+1,newbufsize);
		*(newbuf+flag_offset)=0x5a;
		newlength+=flag_offset+1;
	} else {
		memmove(newbuf+flag_offset+1,buffer+flag_offset,length-flag_offset);
		*(newbuf+flag_offset)=0xa5;
		newlength=length+1;
	}
	//dump_message_column(newbuf,length,"After: ");

	return newlength;
}

void EQProtocolPacket::ChatDecode(unsigned char *buffer, int size, int DecodeKey)
{
	if (buffer[1]!=0x01 && buffer[0]!=0x02 && buffer[0]!=0x1d) {
		int Key=DecodeKey;
		unsigned char *test=(unsigned char *)malloc(size);
		buffer+=2;
		size-=2;

        	int i;
		for (i = 0 ; i+4 <= size ; i+=4)
		{
			int pt = (*(int*)&buffer[i])^(Key);
			Key = (*(int*)&buffer[i]);
			*(int*)&test[i]=pt;
		}
		unsigned char KC=Key&0xFF;
		for ( ; i < size ; i++)
		{
			test[i]=buffer[i]^KC;
		}
		memcpy(buffer,test,size);	
		free(test);
	}
}

void EQProtocolPacket::ChatEncode(unsigned char *buffer, int size, int EncodeKey)
{
	if (buffer[1]!=0x01 && buffer[0]!=0x02 && buffer[0]!=0x1d) {
		int Key=EncodeKey;
		char *test=(char*)malloc(size);
		int i;
		buffer+=2;
		size-=2;
		for ( i = 0 ; i+4 <= size ; i+=4)
		{
			int pt = (*(int*)&buffer[i])^(Key);
			Key = pt;
			*(int*)&test[i]=pt;
		}
		unsigned char KC=Key&0xFF;
		for ( ; i < size ; i++)
		{
			test[i]=buffer[i]^KC;
		}
		memcpy(buffer,test,size);	
		free(test);
	}
}


void EQPacket::SetOpcode(EmuOpcode emu_op) {
	if(emu_op == OP_Unknown) {
		opcode = 0;
		emu_opcode = OP_Unknown;
		return;
	}

	if (OpMgr == NULL || *OpMgr == NULL) {
#if EQDEBUG >= 4
		LogFile->write(EQEMuLog::Debug, "OpMgr is NULL!");
		LogFile->write(EQEMuLog::Debug, "WorldOpMgr=0x%x\n!",WorldOpcodeManager);
#endif
		opcode = OP_Unknown;
	} else {
		opcode = (*OpMgr)->EmuToEQ(emu_op);
	}
	
#if EQDEBUG >= 4
	if(opcode == OP_Unknown) {
		LogFile->write(EQEMuLog::Debug, "Unable to convert Application opcode %s (%d) into an EQ opcode.", OpcodeNames[emu_op], emu_op);
	}
#endif

	//save the emu opcode we just set.
	emu_opcode = emu_op;
}

const EmuOpcode EQPacket::GetOpcode() const {
	if(emu_opcode != OP_Unknown) {
		return(emu_opcode);
	}
	if(opcode == 0) {
		return(OP_Unknown);
	}

	EmuOpcode emu_op;
	emu_op = (OpMgr && *OpMgr) ? (*OpMgr)->EQToEmu(opcode) : OP_Unknown;
#if EQDEBUG >= 4
	if(emu_op == OP_Unknown) {
		LogFile->write(EQEMuLog::Debug, "Unable to convert EQ opcode 0x%.4x to an Application opcode.", opcode);
	}
#endif
	
	emu_opcode = emu_op;
	
	return(emu_op);
}

void DumpPacketHex(const EQApplicationPacket* app)
{
	DumpPacketHex(app->pBuffer, app->size);
}

void DumpPacketAscii(const EQApplicationPacket* app)
{
	DumpPacketAscii(app->pBuffer, app->size);
}

void DumpPacket(const EQApplicationPacket* app, bool iShowInfo) {
	if (iShowInfo) {
		cout << "Dumping Applayer: 0x" << hex << setfill('0') << setw(4) << app->GetOpcode() << dec;
		cout << " size:" << app->size << endl;
	}
	DumpPacketHex(app->pBuffer, app->size);
//	DumpPacketAscii(app->pBuffer, app->size);
}

void DumpPacketBin(const EQApplicationPacket* app) {
	DumpPacketBin(app->pBuffer, app->size);
}

