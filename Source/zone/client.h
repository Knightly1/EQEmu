/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2003  EQEMu Development Team (http://eqemulator.net)

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
class Client;

#include "../common/timer.h"
#include "../common/ptimer.h"
#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "../common/EQNetwork.h"
#include "../common/linked_list.h"
#include "../common/database.h"
#include "errno.h"
#include "../common/classes.h"
#include "../common/races.h"
#include "../common/deity.h"
#include "mob.h"
#include "npc.h"
#include "zone.h"
#include "AA.h"
#include "../common/seperator.h"
#include "../common/Item.h"

#define ITEM_MAX_STACK 20

#define CLIENT_TIMEOUT		90000
#define CLIENT_LD_TIMEOUT	30000 // length of time client stays in zone after LDing
#define TARGETING_RANGE		200	// range for /assist and /target
extern Zone* zone;

class CLIENTPACKET
{
public:
    CLIENTPACKET();
    ~CLIENTPACKET();
    APPLAYER *app;
    bool ack_req;
};

enum {	//Type arguments to the Message* routines.
	//all not explicitly listed are the same grey color
	clientMessageWhite0 = 0,
	clientMessageLoot = 2,	//dark green
	clientMessageTradeskill = 4,	//light blue
	clientMessageTell = 5,		//magenta
	clientMessageWhite = 7,
	clientMessageWhite2 = 10,
	clientMessageLightGrey = 12,
	clientMessageError = 13,	//red
	clientMessageGreen = 14,
	clientMessageYellow = 15,
	clientMessageBlue = 16,
	clientMessageGroup = 18,	//cyan
	clientMessageWhite3 = 20,
};

typedef enum {	//focus types
	focusSpellHaste = 1,
	focusSpellDuration,
	focusRange,
	focusReagentCost,
	focusManaCost,
	focusImprovedHeal,
	focusImprovedDamage,
	focusImprovedDOT,		//i dont know about this...
	focusImprovedCritical,
	focusImprovedUndeadDamage
} focusType;

#define SPELLBAR_UNLOCK 0x2bc
enum {	//scribing argument to MemorizeSpell
	memSpellScribing = 0,
	memSpellMemorize = 1,
	memSpellForget = 2,
	memSpellSpellbar = 3
};

#define DISCIPLINE_SPELL_SLOT 10
#define ABILITY_SPELL_SLOT 9

typedef enum {	//disciplines for disc_inuse
	discNone			= 0,
	//general
	discResistant		= 30,
	discFearless		= 31,
	discWhirlwind		= 6,	//Counterattack/Whirlwind/Furious
	discFellstrike		= 14,	//Duelist/Innerflame/Fellstrike
	discBlindingSpeed	= 15,	//Blindingspeed/Hundredfist		haste unknown
	discDeadeye			= 16,	//Deadeye/Charge
	//warrior
	discEvasive			= 4,
	discMightystrike	= 17,
	discDefensive		= 3,	//values unknown
	discPrecise			= 2,	//values unknown
	discAggressive		= 1,	//values unknown
	//monk
	discStonestance		= 11,
	discThunderkick		= 12,	//values unknown
	discVoidance		= 13,
	discSilentfist		= 20,	//values unknown
	discAshenhand		= 5,	//values unknown
	//rogue
	discNimble			= 19,
	discKinesthetics	= 21,
	//paladin
	discHolyforge		= 22,
	discSanctification	= 23,	//not sure of exact effect
	//ranger
	discTrueshot		= 24,
	discWeaponshield	= 25,
	//bard
	discDeftdance		= 28,
	discPuretone		= 29,	//exact value unknown
	//SK
	discUnholyAura		= 26,	//dosent make un-resistable yet
	discLeechCurse		= 27
};

class Client : public Mob
{
public:
	
	PRange_Struct* pr;
	Client(EQNetworkConnection* ieqnc);
    ~Client();
	
//	void	Discipline(ClientDiscipline_Struct* disc_in, Mob* tar);
	void	AI_Init();
	void	AI_Start(int32 iMoveDelay = 0);
	void	AI_Stop();
	void	Trader_ShowItems();
	void	Trader_EndTrader();
	void	Trader_StartTrader();
	bool	Trader;
	int8	WithCustomer();
	bool	cheater;
	bool	CheckCheat();
	float	cheat_x;
	float	cheat_y;
	bool	AbilityTimer;
	int8	cheatcount;
	virtual bool IsClient() { return true; }
	virtual void DBAWComplete(int8 workpt_b1, DBAsyncWork* dbaw);
	bool	FinishConnState2(DBAsyncWork* dbaw);
	void	CompleteConnect();
	bool	IsOnBoat;
	bool	IsTracking;
	bool	withcustomer;
	void	SendGuildJoin(GuildJoin_Struct* gj);
	void	SendTraderPacket(Client* trader);
	GetItems_Struct* GetTraderItems(); 
	void	SendBazaarWelcome();
	void	DyeArmor(DyeStruct* dye);
	int8	SlotConvert(int8 slot,bool bracer=false);
	void	Message_StringID(int32 type, int32 string_id, int32 distance = 0);
	void	Message_StringID(int32 type, int32 string_id, const char* message,const char* message2=0,const char* message3=0,const char* message4=0,const char* message5=0,const char* message6=0,const char* message7=0,const char* message8=0,const char* message9=0, int32 distance = 0);
	void	SendBazaarResults(int32 trader_id,int32 class_,int32 race,int32 stat,int32 slot,int32 type,char name[64],int32 minprice,int32 maxprice);
	void	SendTraderItem(int32 item_id,int16 quantity);
	int16	FindTraderItemCharges(int32 item_id);
	int16	FindTraderItem(int32 item_id,int16 quantity);
	void	FindAndNukeTraderItem(int32 item_id,int16 quantity,Client* customer,int16 traderslot);
	void	NukeTraderItem(int16 slot,int16 charges,int16 quantity,Client* customer,int16 traderslot);
	void	ReturnTraderReq(const APPLAYER* app,int16 traderitemcharges);
	void	BuyTraderItem(TraderBuy_Struct* tbs,Client* trader,const APPLAYER* app);
	void	TraderUpdate(int16 slot_id,int32 trader_id);
	void	FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho);
	virtual bool Process();
	void	ReceiveData(uchar* buf, int len);
	void	RemoveData();
	void	LogMerchant(Client* player, Mob* merchant, Merchant_Sell_Struct* mp, const Item_Struct* item, bool buying);
	void	LogMerchant(Client* player, Mob* merchant, Merchant_Purchase_Struct* mp, const Item_Struct* item, bool buying);
	void	SendPacketQueue(bool Block = true);
	void	QueuePacket(const APPLAYER* app, bool ack_req = true, CLIENT_CONN_STATUS = CLIENT_CONNECTINGALL,int8 filter=0);
	void	FastQueuePacket(APPLAYER** app, bool ack_req = true, CLIENT_CONN_STATUS = CLIENT_CONNECTINGALL);
	void	ChannelMessageReceived(int8 chan_num, int8 language, const char* message, const char* targetname=NULL);
	void	ChannelMessageSend(const char* from, const char* to, int8 chan_num, int8 language, const char* message, ...);
	void	Message(uint32 type, const char* message, ...);
	void	operator<<(const char* message)		{ Message(0, "%s", message); }
	void	SendSound();
	APPLAYER*	ReturnItemPacket(sint16 slot_id, const ItemInst* inst, ItemPacketType packet_type);
	
	bool	GetRevoked() { return revoked; }
	void	SetRevoked(bool rev) { revoked = rev; }
	inline int32	GetIP()			{ return ip; }
	inline bool	GetHideMe()			{ return gmhideme; }
	void	SetHideMe(bool hm);
	inline int16	GetPort()		{ return port; }
	bool	berserk;
	bool	dead;
	
	virtual bool	Save() { return Save(0); }
			bool	Save(int8 iCommitNow); // 0 = delayed, 1=async now, 2=sync now
			void	SaveBackup();
	
	inline bool ClientDataLoaded() { return client_data_loaded; }
	inline bool	Connected()		{ return (client_state == CLIENT_CONNECTED); }
	inline bool	InZone()		{ return (client_state == CLIENT_CONNECTED || client_state == CLIENT_LINKDEAD); }
	inline void	Kick()			{ client_state = CLIENT_KICKED; }
	inline void	Disconnect()	{ eqnc->Close(); client_state = DISCONNECTED; }
	inline bool IsLD()			{ return (bool) (client_state == CLIENT_LINKDEAD); }
	void	WorldKick();
	inline int8	GetAnon()		{  return m_pp.anon; }
	inline PlayerProfile_Struct& GetPP()	{ return m_pp; }
	inline Inventory& GetInv()				{ return m_inv; }
	bool	CheckAccess(sint16 iDBLevel, sint16 iDefaultLevel);
	
	bool IsEnd(char* string);
	bool IsCommented(char* string);
	char * rmnl(char* nstring);
	void CheckQuests(const char* zonename, const char* message, uint32 npc_id, uint32 item_id, Mob* other);
	char * strreplace(const char* searchstring, const char* searchquery, const char* replacement);
	void LogLoot(Client* player,Corpse* corpse,const Item_Struct* item);
	bool	AutoAttackEnabled() { return auto_attack; }
	bool	AttackFlag() { return attack_flag; }
	bool	Attack(Mob* other, int Hand = 13, bool bRiposte = false);	// 13 = Primary (default), 14 = secondary
	void	Damage(Mob* other, sint32 damage, int16 spell_id, int8 attack_skill = 0x04, bool avoidable = true, sint8 buffslot = -1, bool iBuffTic = false);
	void	Death(Mob* other, sint32 damage, int16 spell_id = 0xFFFF, int8 attack_skill = 0x04);
	void	MakeCorpse(int32 exploss);
	
	bool	ChangeFirstName(const char* in_firstname,const char* gmname);
	
	void	MakeHorseSpawnPacket(int16 spell_id);
	
	void	Duck();
	void	Stand();
	
	virtual void	SetMaxHP();
	sint32	LevelRegen();
	void	HPTick();
	void	SetGM(bool toggle);
	void	SetPVP(bool toggle);

	inline bool	GetPVP()	{ return zone->GetZoneID() == 77 ? true : m_pp.pvp; }
	inline bool	GetGM()		{ return (bool) m_pp.gm; }
	
	inline void	SetBaseClass(uint32 i) { m_pp.class_=i; }
	inline void	SetBaseRace(uint32 i) { m_pp.race=i; }
	inline void	SetBaseGender(uint32 i) { m_pp.gender=i; }

	inline int16	GetBaseRace()	{ return m_pp.race; }
	inline int8	GetBaseGender()	{ return m_pp.gender; }
	inline int8	GetBaseFace()	{ return m_pp.face; }
	sint32	CalcMaxMana();
	const sint32&	SetMana(sint32 amount);
	
	void	ServerFilter(SetServerFilter_Struct* filter);
	void	BulkSendTraderInventory(int32 char_id);
	void	BulkSendMerchantInventory(int merchant_id, int16 npcid);
	
	inline int8	GetLanguageSkill(int16 n)	{ return m_pp.languages[n]; }

	
	inline char*	GetLastName()	{ return lastname; }
	inline int32	GetLDoNPoints() { return m_pp.ldon_available_points; }
	
	
	/*
		Begin client modifiers
	*/
	
	virtual void CalcBonuses();
	//these are all precalculated now
	inline virtual int16	GetAC()		{ return AC; }
	inline virtual int16	GetATK()	{ return ATK; }
	inline virtual int	GetHaste() { return Haste; }
	
	inline virtual sint16	GetSTR()	{ return STR; }
	inline virtual sint16	GetSTA()	{ return STA; }
	inline virtual sint16	GetDEX()	{ return DEX; }
	inline virtual sint16	GetAGI()	{ return AGI; }
	inline virtual sint16	GetINT()	{ return INT; }
	inline virtual sint16	GetWIS()	{ return WIS; }
	inline virtual sint16	GetCHA()	{ return CHA; }
	inline virtual sint16	GetMR() { return MR; }
	inline virtual sint16	GetFR()	{ return FR; }
	inline virtual sint16	GetDR()	{ return DR; }
	inline virtual sint16	GetPR()	{ return PR; }
	inline virtual sint16	GetCR() { return CR; }
	
	int16    CalcAC();
	int16    CalcATK();
	int      CalcHaste();
	int Haste;  //precalced value
	
	sint16   CalcSTR();
	sint16   CalcSTA();
	sint16   CalcDEX();
	sint16   CalcAGI();
	sint16   CalcINT();
	sint16   CalcWIS();
	sint16   CalcCHA();
	
    sint16	CalcMR();
	sint16	CalcFR();
	sint16	CalcDR();
	sint16	CalcPR();
	sint16	CalcCR();
    
	sint16	GetMaxStat();
	sint16  GetMaxSTR();
    sint16  GetMaxSTA();
    sint16  GetMaxDEX();
    sint16  GetMaxAGI();
    sint16  GetMaxINT();
    sint16  GetMaxWIS();
    sint16  GetMaxCHA();
	inline int8	GetBaseSTR()	{ return m_pp.STR; }
	inline int8	GetBaseSTA()	{ return m_pp.STA; }
	inline int8	GetBaseCHA()	{ return m_pp.CHA; }
	inline int8	GetBaseDEX()	{ return m_pp.DEX; }
	inline int8	GetBaseINT()	{ return m_pp.INT; }
	inline int8	GetBaseAGI()	{ return m_pp.AGI; }
	inline int8	GetBaseWIS()	{ return m_pp.WIS; }
	
	float  GetActSpellRange(int16 spell_id, float range);
	sint32  GetActSpellDamage(int16 spell_id, sint32 value);
	sint32  GetActSpellHealing(int16 spell_id, sint32 value);
	sint32  GetActSpellCost(int16 spell_id, sint32);
	sint32  GetActSpellDuration(int16 spell_id, sint32);
	sint32  GetActSpellCasttime(int16 spell_id, sint32);
	sint32  GetDotFocus(int16 spell_id, sint32 value);
	
	inline const sint32&	GetHP()			{ return cur_hp; }
	inline const sint32&	GetMaxHP()		{ return max_hp; }
	inline const sint32&	GetBaseHP()		{ return base_hp; }
	sint32	CalcMaxHP();
	sint32	CalcBaseHP();
	void DoHPRegen(/*SpawnAppearance_Struct* sa*/);
	void DoManaRegen();
	
	int16 GetWeight() const { return(weight); }
	inline void RecalcWeight() { weight = CalcCurrentWeight(); }
	int16 CalcCurrentWeight();	
	
	
    bool Flurry();
    bool Rampage();
	
	inline uint32	GetEXP()		{ return m_pp.exp; }
	
	
	bool	UpdateLDoNPoints(sint32 points, int32 theme);
	inline  void SetDeity(uint32 i) {m_pp.deity=i;}

	void	AddEXP(uint32 add_exp, int8 conlevel = 0xFF, bool resexp = false);
	void	SetEXP(uint32 set_exp, uint32 set_aaxp, bool resexp=false);
	virtual void SetLevel(uint8 set_level, bool command = false);
	
	void	GoToBind();
	void	Gate();
	void	SetBindPoint(int to_zone = -1, float new_x = 0.0f, float new_y = 0.0f, float new_z = 0.0f);
	void	MovePC(const char* zonename, float x, float y, float z, int8 ignorerestrictions = 0, bool summoned = false);
	void	MovePC(int32 zoneID, float x, float y, float z, int8 ignorerestrictions = 0, bool summoned = false);
	void	WhoAll();
	bool	CheckLoreConflict(const Item_Struct* item);
	void	ChangeLastName(const char* in_lastname);
	
	FACTION_VALUE	GetFactionCon(Mob* iOther);
    FACTION_VALUE   GetFactionLevel(int32 char_id, int32 npc_id, int32 p_race, int32 p_class, int32 p_deity, sint32 pFaction, Mob* tnpc);
	
	void	SetFactionLevel(int32 char_id, int32 npc_id, int8 char_class, int8 char_race, int8 char_deity);
	void    SetFactionLevel2(int32 char_id, sint32 faction_id, int8 char_class, int8 char_race, int8 char_deity, sint32 value);
	void SetSkill(int skill_num, int8 skill_id); // socket 12-29-01
	void	AddSkill(int skillid, int8 value);
	sint16	GetRawItemAC();
	int16	GetCombinedAC_TEST();
	
	inline int32	LSAccountID()	{ return lsaccountid; }
	inline int32	GetWID()		{ return WID; }
	inline void		SetWID(int32 iWID) { WID = iWID; }
	inline int32	AccountID()		{ return account_id; }
	inline char*	AccountName()	{ return account_name; }
	inline sint16	Admin()			{ return admin; }
	inline int32	CharacterID()	{ return character_id; }
	void	UpdateAdmin(bool iFromDB = true);
	void	UpdateWho(int8 remove = 0);
	bool	GMHideMe(Client* client = 0);
	
	inline int32	GuildEQID()		{ return guildeqid; }
	inline int32	GuildDBID()		{ return guilddbid; }
	inline int8	GuildRank()		{ return guildrank; }
	bool	SetGuild(int32 in_guilddbid, int8 in_rank);
	void	GuildChangeRank(int32 guildid,int32 oldrank,int32 newrank);
	void	GuildChangeRank(const char* name,int32 guildid,int32 oldrank,int32 newrank);
	void	SendManaUpdatePacket();
	void	SendGuildMembers(int32 guildid);
    // Disgrace: currently set from database.CreateCharacter. 
	// Need to store in proper position in PlayerProfile...
	int8	GetFace()		{ return m_pp.face; } 
	int32	PendingGuildInvite; // Used for /guildinvite
	void	WhoAll(Who_All_Struct* whom);
	
	void	Stun(int duration);
	void	ReadBook(char txtfile[20]);
	void	SendClientMoneyUpdate(int8 type,int32 amount);
	bool	TakeMoneyFromPP(uint32 copper);
	void	AddMoneyToPP(uint32 copper,bool updateclient);
	void	AddMoneyToPP(uint32 copper, uint32 silver, uint32 gold,uint32 platinum,bool updateclient);
	
	bool	CheckIncreaseSkill(int skillid, int chancemodi = 0);
//	bool	SimpleCheckIncreaseSkill(int16 skillid,sint16 chancemodi = 0);
	void	FinishTrade(Client* with);
	void	FinishTrade(NPC* with);
	bool	TGB() {return tgb;}  
	
	int16	GetSkillPoints() {return m_pp.points;}
	void	SetSkillPoints(int inp) {m_pp.points = inp;}
	void	IncreaseSkill(int skill_id, int value = 1) { if (skill_id <= HIGHEST_SKILL) { m_pp.skills[skill_id + 1] += value; } }
	void	IncreaseLanguageSkill(int skill_id, int value = 1) { if (skill_id < 26) { m_pp.languages[skill_id] += value; } }
	uint32		GetSkill(int skill_id) { if (skill_id <= HIGHEST_SKILL) { return((itembonuses.skillmod[skill_id] > 0)? m_pp.skills[skill_id + 1]*(100 + itembonuses.skillmod[skill_id])/100 : m_pp.skills[skill_id + 1]); } return 0; }
	uint32		GetRawSkill(int skill_id) { if (skill_id <= HIGHEST_SKILL) { return(m_pp.skills[skill_id + 1]); } return 0; }
	
	//Father Nitwit's Tradeskill Rework:
	void TradeskillSearchResults(const char *query, unsigned long qlen, unsigned long objtype, unsigned long someid);
	void SendTradeskillDetails(unsigned long  recipe_id);
	void TradeskillExecute(DBTradeskillRecipe_Struct *spec, uint16 tradeskill);
	
	void	SetZoneSummonCoords(float x, float y, float z) {zonesummon_x = x; zonesummon_y = y; zonesummon_z = z;}
	int32	pendingrezzexp;
	void	GMKill();
	inline bool	IsMedding()	{return medding;}
	inline int16	GetDuelTarget() { return duel_target; }
	inline bool	IsDueling() { return duelaccepted; }
	inline bool	GetMount() { return hasmount; }
	inline void	SetMount(bool mount) { hasmount = mount; }
	inline void	SetDuelTarget(int16 set_id) { duel_target=set_id; }
	inline void	SetDueling(bool duel) { duelaccepted = duel; }
	void  SendAAList();
	void  SendAA(int32 id, int seq=1, bool update=false);
	void  BuyAA(AA_Action* action);
	// solar: this function is used by some AA stuff
	void MemorizeSpell(int32 slot,int32 spellid,int32 scribing);
	// use this one instead
	void MemSpell(int16 spell_id, int slot, bool update_client = true);
	void UnmemSpell(int slot, bool update_client = true);
	void UnmemSpellAll(bool update_client = true);
	void ScribeSpell(int16 spell_id, int slot, bool update_client = true);
	void UnscribeSpell(int slot, bool update_client = true);
	void UnscribeSpellAll(bool update_client = true);
	
	inline bool	IsSitting() {return (playeraction == 1);}
	inline bool	IsBecomeNPC() { return npcflag; }
	inline int8	GetBecomeNPCLevel() { return npclevel; }
	inline void	SetBecomeNPC(bool flag) { npcflag = flag; }
	inline void	SetBecomeNPCLevel(int8 level) { npclevel = level; }
	bool	LootToStack(uint32 itemid);
	void	SetFeigned(bool in_feigned);
	inline bool    GetFeigned()	{ return(appearance != 3 ? false : feigned); }
	EQNetworkConnection* Connection() { return eqnc; }
	int8	guildchange;
	int16	otherleaderid;
	sint32 GetEquipment(int8 material_slot);	// returns item id
	//sint32 GetEquipmentMaterial(int8 material_slot);
	sint32 GetEquipmentColor(int8 material_slot);
	
	inline bool AutoSplitEnabled() { return(auto_split); }
	
/*    bool GetReduceManaCostItem(int16 &spell_id, char *itemname);
    bool GetExtendedRangeItem(int16 &spell_id, char *itemname);
    bool GetIncreaseSpellDurationItem(int16 &spell_id, char *itemname);
    bool GetReduceCastTimeItem(int16 &spell_id, char *itemname);
    bool GetImprovedHealingItem(int16 &spell_id, char *itemname);
    bool GetImprovedDamageItem(int16 &spell_id, char *itemname);
    sint32 GenericFocus(int16 spell_id, int16 modspellid);
*/
	void SetHorseId(int16 horseid_in) { horseId = horseid_in; }
	int16 GetHorseId() { return horseId; }
	void SetHasMount(bool hasmount_in) { hasmount = hasmount_in; }
	bool GetHasMount() { return hasmount; }
	// solar: for command_guild
	bool GetIsSettingGuildDoor(void) { return IsSettingGuildDoor; }
	void SetIsSettingGuildDoor(bool isgd) { IsSettingGuildDoor=isgd; }
	int16 GetSetGuildDoorID(void) { return SetGuildDoorID; }
	void SetSetGuildDoorID(int16 sgdid) { SetGuildDoorID=sgdid; }
	
	bool BindWound(Mob* bindmob, bool start, bool fail = false);
	void SetTradeskillObject(Object* object) { m_tradeskill_object = object; }
	Object* GetTradeskillObject() { return m_tradeskill_object; }
	void	SendTribute();
	
	inline PTimerList &GetPTimers() { return(p_timers); }
	
	//AA Methods
	void	ChangeAATitle(int8 in_aa_title) { this->aa_title = in_aa_title; }
	inline int32	GetMaxAAXP(void) { return max_AAXP; }
	inline uint32  GetAAXP()   { return m_pp.expAA; }
	void SendAAStats();
	void SendAATable();
	void SendAATimers();
	void ActivateAA(aaID activate);
	void SendAATimer(int32 ability, int32 begin, int32 end);
	void EnableAAEffect(aaEffectType type, int32 duration = 0);
	void DisableAAEffect(aaEffectType type);
	bool CheckAAEffect(aaEffectType type);
	void HandleAAAction(aaID activate);
	PlayerAA_Struct *GetAAStruct(void) { return &aa; }
	int32 GetAA(int32 aa_id);
	bool SetAA(int32 aa_id, int32 new_value);
	void TemporaryPets(int16 spell_id);
	
	sint16 acmod();
	
	// Item methods
	uint32	NukeItem(uint32 itemnum);
	void	SetTint(sint16 slot_id, uint32 color);
	void	SetTint(sint16 slot_id, Color_Struct& color);
	void	SetMaterial(sint16 slot_id, uint32 item_id);
	void	Undye();
	uint32	GetItemIDAt(sint16 slot_id);
	bool	PutItemInInventory(sint16 slot_id, const ItemInst& inst, bool client_update = false);
	void	DeleteItemInInventory(sint16 slot_id, sint8 quantity = 0, bool client_update = false);
	bool	SwapItem(MoveItem_Struct* move_in);
	void	PutLootInInventory(sint16 slot_id, const ItemInst &inst, ServerLootItem_Struct** bag_item_data = 0);
	bool	AutoPutLootInInventory(ItemInst& inst, bool try_worn = false, bool try_cursor = true, ServerLootItem_Struct** bag_item_data = 0);
	void	SummonItem(uint32 item_id, sint8 charges = 0);
	void	SetStats(int8 type,sint16 increase_val);
	void	DropItem(sint16 slot_id);
	void	SendItemLink(const ItemInst* inst, bool sendtoall=false);
	void	SendLootItemInPacket(const ItemInst* inst, sint16 slot_id);
	void	SendItemPacket(sint16 slot_id, const ItemInst* inst, ItemPacketType packet_type);
	
	int8 guildfaction; // 0 = Peace, 1 = War
	Client* guildtarget;
	int8	GetFilter(int8 filter_id) { return ClientFilters[filter_id]; }
	void	SetFilter(int8 filter_id,int8 value) { ClientFilters[filter_id]=value; }

	void	BreakInvis();
	Group*	GetGroup() { return entity_list.GetGroupByClient(this); }
	void	LeaveGroup();
		
	bool	Hungry() {if (GetGM()) return false; return m_pp.hunger_level <= 3000;}
	bool	Thirsty() {if (GetGM()) return false; return m_pp.thirst_level <= 3000;}
	
	bool	CheckTradeLoreConflict(Client* other);
	void	LinkDead();
	int16	GetInstrumentMod(int16 spell_id);
	bool	CanUseSkill(uint8 skillid) { if (GetSkill(skillid) < 254) return true; return false; }
	void	Insight(int32 t_id);
	bool	CheckDoubleAttack(bool AAadd = false, bool Triple = false);
	//remove charges/multiple objects from inventory:
	//bool	DecreaseByType(int32 type, int8 amt);
	bool	DecreaseByID(int32 type, int8 amt);
	int8	SlotConvert2(int8 slot);	//Maybe not needed.
	void	Escape(); //AA Escape
	void    RemoveNoRent();
	void	RangedAttack(Mob* other);
	void	ThrowingAttack(Mob* other);
	
	void	GoFish();
	void	ForageItem();
	//Calculate vendor price modifier based on CHA: (reverse==selling)
	float	CalcPriceMod(Mob* other = 0, bool reverse = false);
	void	ResetTrade();
	void	DropInst(const ItemInst* inst);
	//This function needs to be eliminated, it isnt doing anything:
	bool	CheckDiscipline(int8 type, bool onetime = false);	//true if discipline is active.
	bool	TrainDiscipline(int32 itemid);
	void	SendDisciplineUpdate();
	bool	UseDiscipline(int32 spell_id, int32 target);

	void    SetLanguageSkill(int langid, int value); // bUsh

#ifdef GUILDWARS
	int32 profit;
	bool permitflag;
	float meleepercentbonus;
	float castpercentbonus;
#endif

protected:
	friend class Mob;
	void CalcItemBonuses(StatBonuses* newbon);
	void AddItemBonuses(const ItemInst *inst, StatBonuses* newbon);
	int  CalcRecommendedLevelBonus(int8 level, uint8 reclevel, int basestat);
	void CalcEdibleBonuses(StatBonuses* newbon);
	void MakeBuffFadePacket(int16 spell_id, int slot_id, bool send_message = true);
	bool client_data_loaded;
	
	sint16	GetFocusEffect(focusType type, int16 spell_id);
	sint16	CalcFocusEffect(focusType type, int16 focus_id, int16 spell_id);
private:
	int8 ClientFilters[21];
	sint32	HandlePacket(const APPLAYER *app);
	void	OPTGB(const APPLAYER *app);
	void	OPRezzAnswer(const APPLAYER *app);
	void	OPMemorizeSpell(const APPLAYER *app);
	void	OPMoveCoin(const APPLAYER* app);
	void	MoveLootCharges(ItemInst &from, sint16 to_slot);
	void	OPGMTraining(const APPLAYER *app);
	void	OPGMEndTraining(const APPLAYER *app);
	void	OPGMTrainSkill(const APPLAYER *app);
	void	OPGMSummon(const APPLAYER *app);
	void	OPCombatAbility(const APPLAYER *app);

	int32 pLastUpdate;
	int32 pLastUpdateWZ;
	int8  playeraction;
	
	EQNetworkConnection* eqnc;
	
	int32				ip;
	int16				port;
    CLIENT_CONN_STATUS  client_state;
	int32				character_id;
	int32				WID;
	int32				account_id;
	char				account_name[30];
	int32				lsaccountid;
	char				lskey[30];
	sint16				admin;
	int32				guilddbid; // guild's ID in the database
	int8				guildrank; // player's rank in the guild, 0-GUILD_MAX_RANK
	int16				duel_target;
	bool				duelaccepted;
	bool				tellsoff;	// GM /toggle
	bool				gmhideme;
	bool				LFG;
	bool				AFK;
	bool				auto_attack;
	bool				attack_flag;
	int8				gmspeed;
	bool				medding;
	bool				hasmount;
	int16				horseId;
	bool				revoked;
	int32				pQueuedSaveWorkID;
	int16				pClientSideTarget;
	bool				auto_split;
	int16				weight;

	PlayerProfile_Struct		m_pp;
	Inventory					m_inv;
	ServerSideFilters_Struct	ssfs;
	Object*						m_tradeskill_object;
	
	void NPCSpawn(const Seperator* sep);
	uint32 GetEXPForLevel(uint16 level);
	
    bool    AddPacket(const APPLAYER *, bool);
    bool    AddPacket(APPLAYER**, bool);
    bool    SendAllPackets();
	LinkedList<CLIENTPACKET *> clientpackets;
	
	char	zonesummon_name[32];
	float	zonesummon_x;
	float	zonesummon_y;
	float	zonesummon_z;
	int8	zonesummon_ignorerestrictions;
	
	Timer	position_timer;
	int8	position_timer_counter;
	
	PTimerList p_timers;		//persistent timers
	Timer	hpregen_timer;
	Timer	camp_timer;
	Timer	process_timer;
	Timer	disc_timer;		//only for avaliable message, pTimerDisciplineReuse enforces reuse time
	Timer	disc_elapse;
	Timer	stamina_timer;
//	Timer	LDTimer;
	Timer	linkdead_timer;
	Timer	dead_timer;
	Timer	ooc_timer;
	Timer	shield_timer;
	Timer	fishing_timer;

	int8 disc_inuse;	//obsoleted by spell-based disciplines, not yet removed
	
	void	BulkSendInventoryItems();
	
	LinkedList<FactionValue*> factionvalue_list;
	sint32	GetCharacterFactionLevel(sint32 faction_id);
	
	bool IsSettingGuildDoor;
	int16 SetGuildDoorID;
	
	int32       max_AAXP;
	int32		staminacount;
	PlayerAA_Struct aa; // Alternate Advancement!
	bool npcflag;
	int8 npclevel;
	bool feigned;
	bool zoning;
	bool tgb;
	bool instalog;
};

#include "parser.h"
#endif
