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

#ifndef CORPSE_H
#define CORPSE_H

#include "mob.h"

class Client;
class NPC;

#define MAX_LOOTERS 6

class Corpse : public Mob
{
public:
	static void Corpse::SendEndLootErrorPacket(Client* client);
	static void Corpse::SendLootReqErrorPacket(Client* client, int8 response = 2);
	static Corpse* Corpse::LoadFromDBData(int32 in_corpseid, int32 in_charid, char* in_charname, uchar* in_data, int32 in_datasize, float in_x, float in_y, float in_z, float in_heading, char* timeofdeath, bool rezzed = false);

	Corpse::Corpse(NPC* in_npc, ItemList* in_itemlist, int32 in_npctypeid, const NPCType** in_npctypedata, int32 in_decaytime = 600000);
	Corpse::Corpse(Client* client, sint32 in_rezexp);
	Corpse::Corpse(int32 in_corpseid, int32 in_charid, char* in_charname, ItemList* in_itemlist, int32 in_copper, int32 in_silver, int32 in_gold, int32 in_plat, float in_x, float in_y, float in_z, float in_heading, float in_size, int8 in_gender, int16 in_race, int8 in_class, int8 in_deity, int8 in_level, int8 in_texture, int8 in_helmtexture,int32 in_rezexp);
	Corpse::~Corpse();

	bool	IsCorpse()			{ return true; }
	bool	IsPlayerCorpse()	{ return p_PlayerCorpse; }
	bool	IsNPCCorpse()		{ return !p_PlayerCorpse; }
	bool	IsBecomeNPCCorpse() { return become_npc; }
	bool	Process();
	bool	Save();
	int32	GetCharID()			{ return charid; }
	int32	SetCharID(int32 iCharID) { if (IsPlayerCorpse()) { return (charid=iCharID); } return 0xFFFFFFFF; };
	int32	GetDecayTime()		{ if (!corpse_decay_timer.Enabled()) return 0xFFFFFFFF; else return corpse_decay_timer.GetRemainingTime(); }
	void	CalcCorpseName();
	inline void		Lock()			{ pLocked = true; }
	inline void		UnLock()		{ pLocked = false; }
	inline bool		IsLocked()		{ return pLocked; }
	inline void		ResetLooter()	{ BeingLootedBy = 0xFFFFFFFF; }
	inline int32	GetDBID()		{ return dbid; }
	inline char*	GetOwnerName()	{ return orgname;}

	void	SetDecayTimer(int32 decaytime);
	bool	IsEmpty();
	void	AddItem(uint32 itemnum, int8 charges, sint16 slot = 0, uint32 aug1=0, uint32 aug2=0, uint32 aug3=0, uint32 aug4=0, uint32 aug5=0);
	uint32	GetWornItem(sint16 equipSlot);
	ServerLootItem_Struct* GetItem(int16 lootslot, ServerLootItem_Struct** bag_item_data = 0);
	void	RemoveItem(int16 lootslot);
	void	RemoveItem(ServerLootItem_Struct* item_data);
	void	AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum);
	void	RemoveCash();
	void	QueryLoot(Client* to);
	int32	CountItems();
	void	Delete();
	virtual void	Depop(bool StartSpawnTimer = true);

	uint32	GetCopper()		{ return copper; }
	uint32	GetSilver()		{ return silver; }
	uint32	GetGold()		{ return gold; }
	uint32	GetPlatinum()	{ return platinum; }

	void	FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	void	MakeLootRequestPackets(Client* client, const APPLAYER* app);
	void	LootItem(Client* client, const APPLAYER* app);
	void	EndLoot(Client* client, const APPLAYER* app);
	void	Summon(Client* client, bool spell);
	void	CastRezz(int16 spellid, Mob* Caster);
	void	CompleteRezz();
	void	SetPKItem(sint32 id) { pkitem = id; }
	sint32	GetPKItem() { return pkitem; }
	bool CanMobLoot(int charid);
	void AllowMobLoot(Mob *them, int8 slot);
	void AddLooter(Mob *who);

	char		orgname[64];
	bool IsRezzed() { return isrezzed; }
	sint32 GetEquipment(int8 material_slot);	// returns item id
	sint32 GetEquipmentColor(int8 material_slot);
protected:
	void MoveItemToCorpse(Client *client, ItemInst *item, sint16 equipslot);
private:
	bool		p_PlayerCorpse;
	bool		pIsChanged;
	bool		pLocked;
	sint32		pkitem;
	int32		dbid;
	int32		charid;
	ItemList	itemlist;
	uint32		copper;
	uint32		silver;
	uint32		gold;
	uint32		platinum;
	bool		p_depop;
	int32		BeingLootedBy;
	int32		rezzexp;
	bool		become_npc;
	bool		isrezzed;
	int			looters[MAX_LOOTERS]; // People allowed to loot the corpse, character id
	Timer		corpse_decay_timer;
	Timer		corpse_delay_timer;
	Color_Struct item_tint[9];
};

#endif
