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
#include "StdAfx.h"
#include "EQStreamPair.h"
#include "../common/op_codes.h"
#include "../common/misc.h"
#include "PacketHandler.h"
#include <vector>
#include <iomanip>
#include <iostream>
#ifndef WIN32
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

extern int debug;


//global packet handlers, in PacketHandler.cpp
extern vector<PacketHandler *> PacketHandlers;
extern vector<StreamCreateHandler> StreamCreateHandlers;
extern vector<StreamDestroyHandler> StreamDestroyHandlers;



EQStreamPair::EQStreamPair(bool be_quiet, OpcodeManager **ops)
{
	quiet = be_quiet;
	valid = true;
	first_packet=true;
	server.SetOpcodeManager(ops);
	client.SetOpcodeManager(ops);
}

EQStreamPair::~EQStreamPair() {
	if(!quiet) {
		string sips = inet_ntoa(*((struct in_addr *)&client_ip));
		string dips = inet_ntoa(*((struct in_addr *)&server_ip));
		printf("Destroying stream pair 0x%x: %s:%d -> %s:%d\n", this, sips.c_str(), client_port, dips.c_str(), server_port);
	}

	//notify anybody interested in stream destruction
	vector<StreamDestroyHandler>::iterator curf, endf;
	curf = StreamDestroyHandlers.begin();
	endf = StreamDestroyHandlers.end();
	for(; curf != endf; curf++) {
		StreamDestroyHandler proc = *curf;
		proc(this);
	}
	
	//destroy all my stream handlers
	vector<StreamPacketHandler *>::iterator curh, endh;
	curh = handlers.begin();
	endh = handlers.end();
	for(; curh != endh; curh++) {
		StreamPacketHandler *hand = *curh;
		
		//this is effectively signaling them of my demise
		//fun fun dll stuff, cant delete from the main app
		PacketHandlerDeleteMethod delproc = hand->GetDeleteMethod();
		if(delproc != NULL) {
			delproc(hand);
		} else {
			fprintf(stderr, "ERROR: Stream Handler provided NULL delete method!\n");
		}
//		safe_delete(*curh);
	}
}

void EQStreamPair::CheckTimeouts(unsigned long now) {
	if(!valid)
		return;
	//see if either of the connections are active.
	server.CheckTimeout(now, CONNECTION_TIMEOUT);
	client.CheckTimeout(now, CONNECTION_TIMEOUT);
	
	if(server.CheckState(CLOSED) 
	&& client.CheckState(CLOSED)) {
		valid = false;
	}
}

void EQStreamPair::AddHandler(StreamPacketHandler *it) {
	//I own this stream handler now.
	handlers.push_back(it);
}

void EQStreamPair::NotifyHandlers(bool to_server, const EQRawApplicationPacket *app) {
	
	EQStreamType type= client.GetStreamType();
	EmuOpcode emu_op = app->GetOpcode();
	uint16 eq_opcode = app->GetRawOpcode();
	
	//notify our stream-specific handlers
	vector<StreamPacketHandler *>::iterator curs, ends;
	curs = handlers.begin();
	ends = handlers.end();
	for(; curs != ends; curs++) {
		StreamPacketHandler *it = *curs;
		if(to_server) {
			it->ToServerPacket(type, eq_opcode, emu_op, app);
		} else {
			it->ToClientPacket(type, eq_opcode, emu_op, app);
		}
	}
	
	//notify global handlers
	vector<PacketHandler *>::iterator curg, endg;
	curg = PacketHandlers.begin();
	endg = PacketHandlers.end();
	for(; curg != endg; curg++) {
		PacketHandler *it = *curg;
		if(to_server) {
			it->ToServerPacket(type, eq_opcode, emu_op, app);
		} else {
			it->ToClientPacket(type, eq_opcode, emu_op, app);
		}
	}
}

void EQStreamPair::CallCreateHandlers() {
	vector<StreamCreateHandler>::iterator cur,end;
	cur = StreamCreateHandlers.begin();
	end = StreamCreateHandlers.end();
	StreamPacketHandler *res;
	StreamCreateHandler proc;
	for(; cur != end; cur++) {
		proc = *cur;
		
		res = proc(this->client.GetStreamType(),this);
		if(res != NULL) {
			AddHandler(res);
		}
	}
}

bool EQStreamPair::Process(const unsigned char *buffer, unsigned short length, unsigned long src_ip, unsigned short src_port, unsigned long dst_ip, unsigned short dst_port, unsigned long ts_sec, unsigned long ts_usec)
{
static unsigned char newbuffer[2048];
unsigned long newlength;
	//dump_message_column(const_cast<unsigned char*>(buffer),length,"Raw: ");
	if (EQProtocolPacket::ValidateCRC(buffer,length,Key)) {
		EQStreamType type=client.GetStreamType();
		if (type==LoginStream || buffer[1]==0x01 || buffer[1]==0x02) {
			memcpy(newbuffer,buffer,length);
			newlength=length;
		} else if (type==ChatStream || type==MailStream || type==ChatOrMailStream ) {
			memcpy(newbuffer,buffer,length);
			newlength=length;
			if (newbuffer[1] != 0x01 && newbuffer[1] != 0x02) {
				EQProtocolPacket::ChatDecode(newbuffer,newlength-2,Key);
			}
			//dump_message_column(newbuffer,newlength,"");
			//cout << endl;
		} else if (type==WorldStream || type==ZoneStream) {
			newlength=EQProtocolPacket::Decompress(buffer,length,newbuffer,2048);
		} else{
			cout << "***** Unknown stream type!" << endl;
			return false;
		}

	} else {
		if(debug > 1) {
			cout << "*******" << endl << "Packet failed CRC check" << endl;
			dump_message_column(const_cast<unsigned char *>(buffer),length,"RAW(client->server): ");
		}
		return false;
	}
	
	int16 proto_opcode = ntohs(*(const uint16 *)newbuffer);
	
	if (proto_opcode != OP_SessionRequest && newbuffer[1] != OP_SessionResponse && newbuffer[1] != OP_OutOfSession && newlength>4)
		newlength-=2;

	//dump_message_column(const_cast<unsigned char*>(newbuffer),newlength,"Dec: ");
	EQProtocolPacket *p=new EQProtocolPacket(proto_opcode, newbuffer+2, newlength-2);
	p->setSrcInfo(src_ip,src_port);
	p->setDstInfo(dst_ip,dst_port);
	p->setTimeInfo(ts_sec,ts_usec);
	//p->DumpRaw();
	if (p->GetRawOpcode() == OP_SessionRequest) {
		server_ip=dst_ip;
		server_port=dst_port;
		client_ip=src_ip;
		client_port=src_port;
		if(!quiet)
			cout << "New Stream: Server=" << long2ip(server_ip) << ":" << server_port << ", Client=" << long2ip(client_ip) << ":" << client_port << " Detected!" << endl;
	}

	if (server_ip==dst_ip && server_port==dst_port && client_ip==src_ip && client_port==src_port) {
		server.ProcessPacket(p);
		server.SetLastPacketTime(Timer::GetCurrentTime());
		server.ProcessQueue();
	} else if (client_ip==dst_ip && client_port==dst_port && server_ip==src_ip && server_port==src_port){
		client.ProcessPacket(p);
		client.SetLastPacketTime(Timer::GetCurrentTime());
		client.ProcessQueue();
		if (p->GetRawOpcode() == OP_SessionResponse) {
			EQStreamType Type;
			Key=client.GetKey();
			server.SetKey(Key);
//			if(!quiet)
//				cout << "Stream: Server=" << long2ip(server_ip) << ":" << server_port << ", Client=" << long2ip(client_ip) << ":" << client_port << " Key=" << hex << Key << dec << endl;
			Type=client.GetStreamType();
			server.SetStreamType(Type);
			CallCreateHandlers();
		}
	}
	if(p->GetRawOpcode() == OP_SessionDisconnect) {
		valid = false;
	}
	
	safe_delete(p);
	
	return true;
}


void EQStreamPair::CheckQueues() {
	EQRawApplicationPacket *c_app,*s_app,*app;
	while(1) {
		c_app=client.PeekPacket();
		s_app=server.PeekPacket();

		if ((c_app || s_app) && first_packet) {
			if(!quiet) {
				cout << "Stream: Server=" << long2ip(server_ip) << ":" << server_port << ", Client=" << long2ip(client_ip) << ":" << client_port << " Key=" << hex << Key << dec;
				switch (client.GetStreamType()) {
					case LoginStream:
						cout << " (Login)";
						break;
					case WorldStream:
						cout << " (World)";
						break;
					case ZoneStream:
						cout << " (Zone)";
						break;
					case ChatOrMailStream:
						cout << " (Chat/Mail)";
						break;
					case ChatStream:
						cout << " (Chat)";
						break;
					case MailStream:
						cout << " (Mail)";
						break;
					case UnknownStream:
						cout << " (Unknown)";
						break;
				}
				cout << endl;
			}
			first_packet=false;
		}
		if (c_app && (!s_app || (*c_app) < (*s_app) ) ) {
			app=client.PopRawPacket();
			NotifyHandlers(false, app);
		} else if (s_app && (!c_app || (*s_app) < (*c_app) ) ) {
			app=server.PopRawPacket();
			NotifyHandlers(true, app);
		} else {
			break;
		}

		safe_delete(app);
	}
}

