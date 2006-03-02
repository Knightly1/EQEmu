	void SignalNPC(int _signal_id);
	FACTION_VALUE CheckNPCFactionAlly(sint32 other_faction);
	void	AddItem(int32 itemid, int8 charges, int8 slot = 0);
	void	AddLootTable();
	bool	IsRanger() { return rangerstance; }
	void    RemoveItem(uint16 item_id, int16 quantity = 0, int16 slot = 0);
	void	ClearItemList();
	void	AddCash(int16 in_copper, int16 in_silver, int16 in_gold, int16 in_platinum);
	void	RemoveCash();
	int32	CountLoot();
	inline int32	GetLoottableID()	{ return loottable_id; }
	void	SetPetType(int16 in_type)	{ typeofpet = in_type; } // put this here because only NPCs can be anything but charmed pets
	inline uint32	GetCopper()		{ return copper; }
	inline uint32	GetSilver()		{ return silver; }
	inline uint32	GetGold()		{ return gold; }
	inline uint32	GetPlatinum()	{ return platinum; }

	inline void	SetCopper(uint32 amt)		{ copper = amt; }
	inline void	SetSilver(uint32 amt)		{ silver = amt; }
	inline void	SetGold(uint32 amt)			{ gold = amt; }
	inline void	SetPlatinum(uint32 amt)		{ platinum = amt; }
	void SetGrid(int16 grid_){ grid=grid_; }
	void SetSp2(int32 sg2){ spawn_group=sg2; }
	int16 GetWaypointMax(){ return wp_m; }
	sint16 GetGrid(){ return grid; }
	int32 GetSp2(){ return spawn_group; }
   inline bool	IsPVP() { return pvp; }
	inline int8	CurrentPosition() { return position; }
	inline int8	HasBanishCapability() { return banishcapability; }

	inline const sint32&	GetNPCFactionID()	{ return npc_faction_id; }
	inline sint32			GetPrimaryFaction()	{ return primary_faction; }
	inline Mob*	GetIgnoreTarget() { return ignore_target; }
	inline void	SetIgnoreTarget(Mob* mob) {ignore_target = mob; }
	sint32	GetNPCHate(Mob* in_ent)  {return hate_list.GetEntHate(in_ent);}
    bool    IsOnHatelist(Mob*p) { return hate_list.IsOnHateList(p);}

	void	SetNPCFactionID(sint32 in) { npc_faction_id = in; database.GetFactionIdsForNPC(npc_faction_id, &faction_list, &primary_faction); }
	void	SetFeignMemory(const char* num) {feign_memory = num;}
	inline Const_char *    GetFeignMemory()	{ return feign_memory; }
	int16	GetMaxDMG() {return max_dmg;}
	bool	IsAnimal() { return(bodytype == 21); }
	int16   GetPetSpellID() {return pet_spell_id;}
	void    SetPetSpellID(int16 amt) {pet_spell_id = amt;}
	int32	GetMaxDamage(int8 tlevel);
	void    SetTaunting(bool tog) {taunting = tog;}
	void	PickPocket(Client* thief);
	void	StartSwarmTimer(int32 duration) { swarm_timer.Start(duration); }
	void	DoClassAttacks(Mob *target);


