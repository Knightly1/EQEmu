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
#include "PacketHandler.h"
#include <vector>
#ifndef WIN32
	#include <netinet/in.h>
	#include <arpa/inet.h>
#endif

extern int debug;


//global packet handlers:
vector<PacketHandler *> PacketHandlers;
vector<StreamCreateHandler> StreamCreateHandlers;
vector<StreamDestroyHandler> StreamDestroyHandlers;

PacketHandler::PacketHandler(PacketHandlerDeleteMethod del_proc) {
	_delete_proc = del_proc;
}

StreamPacketHandler::StreamPacketHandler(PacketHandlerDeleteMethod del_proc, const EQStreamPair *s)
: PacketHandler(del_proc)
{
	stream = s;
}

void ClearPacketHandlers() {
	//does not remove stream-specific handlers allready in place
	PacketHandlers.clear();	//should prolly delete these
	StreamCreateHandlers.clear();
	StreamDestroyHandlers.clear();
}

void AddPacketHandler(PacketHandler *a) {
	if(a == NULL)
		return;
	PacketHandlers.push_back(a);
}

void AddStreamCreateHandler(StreamCreateHandler p)
{
	if(p == NULL)
		return;
	StreamCreateHandlers.push_back(p);
}

void AddStreamDestroyHandler(StreamDestroyHandler p)
{
	if(p == NULL)
		return;
	StreamDestroyHandlers.push_back(p);
}

void GetHandlerCallbacks(HandlerCallbacks *it) {
	it->AddPacketHandler = AddPacketHandler;
	it->AddStreamCreateHandler = AddStreamCreateHandler;
	it->AddStreamDestroyHandler = AddStreamDestroyHandler;
}







