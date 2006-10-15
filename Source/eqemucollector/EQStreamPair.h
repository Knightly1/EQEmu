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
#ifndef _EQSTREAM_PAIR_H
#define _EQSTREAM_PAIR_H

#include "../common/EQStream.h"
#include "../common/timer.h"

#include <vector>
using namespace std;

#define CONNECTION_TIMEOUT 20000	//in ms

class StreamPacketHandler;
class EQRawApplicationPacket;
class OpcodeManager;

class EQStreamPair {
	public:
	unsigned long server_ip, client_ip;
	unsigned short server_port, client_port;
	unsigned long Key;
	bool first_packet;

	EQStream server, client;

	EQStreamPair(bool be_quiet, OpcodeManager **ops);
	~EQStreamPair();
	bool Process(const unsigned char *buffer, unsigned short length, unsigned long src_ip, unsigned short src_port, unsigned long dst_ip, unsigned short dst_port, unsigned long ts_sec, unsigned long ts_usec);
	
	bool IsWorldStream() const { return server.GetStreamType() == WorldStream; }
	bool IsZoneStream() const { return server.GetStreamType() == ZoneStream; }
	bool IsChatStream() const { return server.GetStreamType() == ChatStream; }
	bool IsMailStream() const { return server.GetStreamType() == MailStream; }
	bool IsLoginStream() const { return server.GetStreamType() == LoginStream; }

	void CheckQueues();
	void CheckTimeouts(unsigned long now);

	bool IsValid() const { return(valid); }
	
protected:
	bool quiet;
	bool valid;
	
	void CallCreateHandlers();
	void AddHandler(StreamPacketHandler *it);
	void NotifyHandlers(bool to_server, const EQRawApplicationPacket *app);
	vector<StreamPacketHandler *> handlers;
};



#endif
