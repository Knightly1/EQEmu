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
#ifndef CLIENT_H
#define CLIENT_H

#include "../common/EQNetwork.h"
#include "../common/linked_list.h"
#include "../common/timer.h"
#include "zoneserver.h"

#define CLIENT_TIMEOUT 30000

class Client;

class Client {
public:
	Client(EQNetworkConnection* ieqnc);
    ~Client();
	
	bool	Process();
	void	ReceiveData(uchar* buf, int len);
	void	SendCharInfo();
	void	EnterWorld(bool TryBootup = true);
	void	ZoneUnavail();
	void	QueuePacket(const APPLAYER* app, bool ack_req = true);
	void	Clearance(sint8 response);
	void	SendGuildList();
	void	SendApproveWorld();
	bool	GenPassKey(char* key);

	inline int32		GetIP()				{ return ip; }
	inline int16		GetPort()			{ return port; }
	inline int32		GetZoneID()			{ return zoneID; }
	inline int32		WaitingForBootup()	{ return pwaitingforbootup; }
	inline sint16		GetAdmin()			{ if (cle) { return cle->Admin(); } return 0; }
	inline int32		GetAccountID()		{ if (cle) { return cle->AccountID(); } return 0; }
	inline int32		GetWID()			{ if (cle) { return cle->GetID(); } return 0; }
	inline int32		GetLSID()			{ if (cle) { return cle->LSID(); } return 0; }
	inline const char*	GetLSKey()			{ if (cle) { return cle->GetLSKey(); } return 0; }
	inline int32		GetCharID()			{ return charid; }
	inline const char*	GetCharName()		{ return char_name; }
	inline ClientListEntry* GetCLE()		{ return cle; }
	inline void			SetCLE(ClientListEntry* iCLE)			{ cle = iCLE; }
private:
	int32	ip;
	int16	port;
	int32	charid; 
	char	char_name[64];
	int32	zoneID;
	Timer*	autobootup_timeout;
	int32	pwaitingforbootup;

	bool OPCharCreate(CharCreate_Struct *cc);

	void SetClassStartingSkills( PlayerProfile_Struct *pp );
	void SetRaceStartingSkills( PlayerProfile_Struct *pp );
	void SetRacialLanguages( PlayerProfile_Struct *pp );

	ClientListEntry* cle;
	Timer*	CLE_keepalive_timer;
	Timer*	connect;
	bool firstlogin;
	bool seencharsel;
	bool realfirstlogin;
	bool HandlePacket(const APPLAYER *app);
	EQNetworkConnection* eqnc;
};

class ClientList {
public:
	ClientList();
	~ClientList();
	
	void	Add(Client* client);
	Client*	Get(int32 ip, int16 port);
	Client* FindByAccountID(int32 account_id);
	Client* FindByName(char* charname);
	void	Process();

	void	ZoneBootup(ZoneServer* zs);
	void	RemoveCLEReferances(ClientListEntry* cle);
private:
	LinkedList<Client*> list;
};


bool CheckCharCreateInfo(CharCreate_Struct *cc);

#endif
