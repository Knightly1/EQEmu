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
#ifndef ZONESERVER_H
#define ZONESERVER_H

#include "../common/linked_list.h"
#include "../common/timer.h"
#include "../common/queue.h"
#include "../common/servertalk.h"
#include "../common/eq_packet_structs.h"
#include "../common/TCPConnection.h"
#include "WorldTCPConnection.h"
#include "console.h"
#include "../common/Mutex.h"
#include "../common/eqtime.h"
#include "../common/md5.h"

class ClientListEntry;

#ifdef WIN32
	void ZoneServerLoop(void *tmp);
#else
	void *ZoneServerLoop(void *tmp);
#endif

class ZoneServer : public WorldTCPConnection {
public:
	ZoneServer(TCPConnection* itcpc);
    ~ZoneServer();
	virtual inline bool IsZoneServer() { return true; }
	
	bool		Process();
	bool		SendPacket(ServerPacket* pack) { return tcpc->SendPacket(pack); }
	bool		SendPacket(TCPConnection::TCPNetPacket_Struct* tnps) { return tcpc->SendPacket(tnps); }
	void		SendEmoteMessage(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message, ...);
	void		SendEmoteMessageRaw(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message);
	bool		SetZone(int32 iZoneID, bool iStaticZone = false);
	bool		SetConnectInfo(const char* in_address, int16 in_port);
	void		TriggerBootup(int32 iZoneID = 0, const char* iAdminName = 0, bool iMakeStatic = false);
	void		Disconnect() { tcpc->Disconnect(); }
	void		IncommingClient(Client* client);
	void		ChangeWID(int32 iCharID, int32 iWID);
	void		SendGroupIDs();

	inline const char*	GetZoneName()	{ return zone_name; }
	inline int32		GetZoneID()		{ return zoneID; }
	inline int32		GetIP()			{ return tcpc->GetrIP(); }
	inline int16		GetPort()		{ return tcpc->GetrPort(); }
	inline const char*	GetCAddress()	{ return clientaddress; }
	inline int16		GetCPort()		{ return clientport; }
	inline int32		GetID()			{ return ID; }
	inline bool			IsBootingUp()	{ return BootingUp; }
	inline bool			IsStaticZone()	{ return staticzone; }
	inline int32&		NumPlayers()	{ return pNumPlayers; }
private:
	TCPConnection* tcpc;

	int32	ID;
	char	clientaddress[250];
	int16	clientport;
	bool	BootingUp;
	bool	staticzone;
	bool	authenticated;
	int32	pNumPlayers;
	
	char	zone_name[16];
	int32	zoneID;
};

class ZSList
{
public:
	static void ShowUpTime(WorldTCPConnection* con, const char* adminname = 0);

	ZSList();
	~ZSList();
	ZoneServer* FindByName(const char* zonename);
	ZoneServer* FindByID(int32 ZoneID);
	ZoneServer* FindByZoneID(int32 ZoneID);
	ZoneServer*	FindByPort(int16 port);
	
	void	SendChannelMessage(const char* from, const char* to, int8 chan_num, int8 language, const char* message, ...);
	void	SendChannelMessageRaw(const char* from, const char* to, int8 chan_num, int8 language, const char* message);
	void	SendEmoteMessage(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message, ...);
	void	SendEmoteMessageRaw(const char* to, int32 to_guilddbid, sint16 to_minstatus, int32 type, const char* message);

	void	ClientUpdate(ZoneServer* zoneserver, ServerClientList_Struct* scl);
	void	CLERemoveZSRef(ZoneServer* iZS);
	ClientListEntry* CheckAuth(int32 iLSID, const char* iKey);
	ClientListEntry* CheckAuth(const char* iName, const char* iPassword);
	ClientListEntry* CheckAuth(int32 id, const char* iKey, int32 ip);
	ClientListEntry* FindCharacter(const char* name);
	ClientListEntry* FindCLEByAccountID(int32 iAccID);
	ClientListEntry* GetCLE(int32 iID);
	void	CLCheckStale();
	void	CLEKeepAlive(int32 numupdates, int32* wid);
	void	CLEAdd(int32 iLSID, const char* iLoginName, const char* iLoginKey, sint16 iWorldAdmin = 0, int32 ip = 0);

	void	SendWhoAll(int32 fromid,const char* to, sint16 admin, Who_All_Struct* whom, WorldTCPConnection* connection);
	void    ConsoleSendWhoAll(const char* to, sint16 admin, Who_All_Struct* whom, WorldTCPConnection* connection);
	void	SendZoneStatus(const char* to, sint16 admin, WorldTCPConnection* connection);
	void	SendCLEList(const sint16& admin, const char* to, WorldTCPConnection* connection, const char* iName = 0);

	void	SendTimeSync();	
	void	Add(ZoneServer* zoneserver);
	void	Process();
	void	KillAll();
	bool	SendPacket(ServerPacket* pack);
	bool	SendPacket(const char* to, ServerPacket* pack);
	inline int32	GetNextID()		{ return NextID++; }
	void	RebootZone(const char* ip1,int16 port, const char* ip2, int32 skipid, int32 zoneid = 0);
	int32	TriggerBootup(int32 iZoneID);
	static void SOPZoneBootup(const char* adminname, int32 ZoneServerID, const char* zonename, bool iMakeStatic = false);
	EQTime worldclock;
	enum { MaxLockedZones = 10 };
	bool	SetLockedZone(int16 iZoneID, bool iLock);
	bool	IsZoneLocked(int16 iZoneID);
	void	ListLockedZones(const char* to, WorldTCPConnection* connection);
	Timer* shutdowntimer;
	Timer* reminder;
	void	NextGroupIDs(int32 &start, int32 &end);
protected:
	friend class ClientListEntry;
	inline int32	GetNextCLEID() { return NextCLEID++; }
private:
	int32 NextID;
	int32 NextCLEID;
	LinkedList<ZoneServer*> list;
	LinkedList<ClientListEntry*> clientlist;
	Timer*	CLStale_timer;
	int16	pLockedZones[MaxLockedZones];
	int32 CurGroupID;
};

#define CLE_Status_Never		-1
#define CLE_Status_Offline		0
#define CLE_Status_Online		1	// Will not overwrite more specific online status
#define CLE_Status_CharSelect	2
#define CLE_Status_Zoning		3
#define CLE_Status_InZone		4

class ClientListEntry {
public:
	ClientListEntry(int32 iLSID, const char* iLoginName, const char* iLoginKey, sint16 iWorldAdmin = 0, int32 ip = 0);
	ClientListEntry(int32 iAccID, const char* iAccName, MD5& iMD5Pass, sint16 iAdmin = 0);
	ClientListEntry(ZoneServer* iZS, ServerClientList_Struct* scl, sint8 iOnline);
	~ClientListEntry();
	bool	CheckStale();
	void	Update(ZoneServer* zoneserver, ServerClientList_Struct* scl, sint8 iOnline = CLE_Status_InZone);
	bool	CheckAuth(int32 iLSID, const char* key);
	bool	CheckAuth(const char* iName, MD5& iMD5Password);
	bool	CheckAuth(int32 id, const char* key, int32 ip);
	void	SetOnline(ZoneServer* iZS, sint8 iOnline);
	void	SetOnline(sint8 iOnline = CLE_Status_Online);
	void	SetChar(int32 iCharID, const char* iCharName);
	inline sint8		Online()		{ return pOnline; }
	inline const int32&	GetID()			{ return id; }
	inline const int32&	GetIP()			{ return pIP; }
	inline void			SetIP(const int32& iIP) { pIP = iIP; }
	inline void			KeepAlive()		{ stale = 0; }
	inline int8			GetStaleCounter() { return stale; }
	void	LeavingZone(ZoneServer* iZS = 0, sint8 iOnline = CLE_Status_Offline);
	void	Camp(ZoneServer* iZS = 0);

	// Login Server stuff
	inline int32		LSID()			{ return pLSID; }
	inline int32		LSAccountID()	{ return pLSID; }
	inline const char*	LSName()		{ return plsname; }
	inline sint16		WorldAdmin()	{ return pworldadmin; }
	inline const char*	GetLSKey()		{ return plskey; }

	// Account stuff
	inline int32		AccountID()		{ return paccountid; }
	inline const char*	AccountName()	{ return paccountname; }
	inline sint16		Admin()			{ return padmin; }
	inline void			SetAdmin(int16 iAdmin) { padmin = iAdmin; }

	// Character info
	inline ZoneServer*	Server()		{ return pzoneserver; }
	inline void			ClearServer()	{ pzoneserver = 0; }
	inline int32		CharID()		{ return pcharid; }
	inline const char*	name()			{ return pname; }
	inline int32		zone()			{ return pzone; }
	inline int8			level()			{ return plevel; }
	inline int8			class_()		{ return pclass_; }
	inline int16			race()			{ return prace; }
	inline int8			Anon()			{ return panon; }
	inline int8			TellsOff()		{ return ptellsoff; }
	inline int32		GuildDBID()		{ return pguilddbid; }
	inline int32		GuildEQID()		{ return pguildeqid; }
	inline bool			LFG()			{ return pLFG; }
	inline int8			GetGM()			{ return gm; }
	inline void			SetGM(int8 igm)	{ gm = igm; }
	inline void			SetZone(int32 zone) { pzone = zone; }

private:
	void	ClearVars(bool iAll = false);

	int32	id;
	int32	pIP;
	sint8	pOnline;
	int8	stale;

	// Login Server stuff
	int32	pLSID;
	char	plsname[32];
	char	plskey[16];
	sint16	pworldadmin;		// Login server's suggested admin status setting

	// Account stuff
	int32	paccountid;
	char	paccountname[32];
	MD5		pMD5Pass;
	sint16	padmin;

	// Character info
	ZoneServer* pzoneserver;
	int32	pzone;
	int32	pcharid;
	char	pname[64];
	int8	plevel;
	int8	pclass_;
	int16	prace;
	int8	panon;
	int8	ptellsoff;
	int32	pguilddbid;
	int32	pguildeqid;
	bool	pLFG;
	int8	gm;
};

#endif
