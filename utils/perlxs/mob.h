
//BEGIN ENTITY METHODS

	virtual bool IsClient()			{ return false; }
	virtual bool IsNPC()			{ return false; }
	virtual bool IsMob()			{ return false; }
	virtual bool IsCorpse()			{ return false; }
	virtual bool IsPlayerCorpse()	{ return false; }
	virtual bool IsNPCCorpse()		{ return false; }
	virtual bool IsObject()			{ return false; }
//	virtual bool IsGroup()			{ return false; }
	virtual bool IsDoor()			{ return false; }
	virtual bool IsTrap()			{ return false; }
	virtual bool IsBeacon()			{ return false; }
	
	Client* CastToClient();
	NPC*    CastToNPC();
	Mob*    CastToMob();
	Corpse*	CastToCorpse();
//	Object* CastToObject();
//	Group*	CastToGroup();
//	Doors*	CastToDoors();
//	Trap*	CastToTrap();
//	Beacon*	CastToBeacon();
	
//BEGIN MOB METHODS
	int16& GetID();
	virtual Const_char * GetName();
	void Depop(bool StartSpawnTimer = true);
	
	void SetSkill(int in_skill_num, int8 in_skill_value);
	uint32 GetSkill(int skill_num);
	void Gate();
	void Heal();
	void SetMaxHP();
	int32 GetLevelCon(int8 iOtherLevel);
	void SetHP(sint32 hp);
	void DoAnim(const int animnum, int type=1);
	void ChangeSize(float in_size, bool bNoRestriction = false);
	void GMMove(float x, float y, float z, float heading = 0.01);
	void SendPosUpdate(int8 iSendToSelf = 0);
	void SendPosition();
	bool Rezzed();
	bool HasProcs();
	//bool CheckLos(Mob* other);
	bool IsInvisible();
	void SetInvisible(bool state);
	bool FindBuff(int16 spellid);
	bool	FindType(int8 type, bool bOffensive = false, int16 threshold = 100);
	sint8	GetBuffSlotFromType(int8 type);
	void	MakePetType(int16 spell_id, const char* pettype);
	void	MakePet(int16 spell_id, int8 in_level, int8 in_class, int16 in_race, int8 in_texture = 0, int8 in_pettype = 0, float in_size = 0, int8 type = 0, int32 min_dmg = 0, int32 max_dmg = 0);
	virtual inline int16	GetBaseRace();
	virtual inline int8	GetBaseGender();
	virtual inline int8	GetDeity();
	inline const int16&	GetRace();
	inline const int8&	GetGender();
	inline const int8&	GetTexture();
	inline const int8&	GetHelmTexture();
	inline const int8&	GetClass();
	inline const uint8&	GetLevel();
	Const_char * GetCleanName();
	inline Mob*			GetTarget();
	virtual inline void	SetTarget(Mob* mob);
	virtual inline float		GetHPRatio();

	bool IsWarriorClass();
	virtual inline const sint32&	GetHP();
	virtual inline const sint32&	GetMaxHP();
	virtual float GetWalkspeed();
	virtual float GetRunspeed();
	virtual int GetCasterLevel(int16 spell_id);
	virtual inline const sint32&	GetMaxMana();
	virtual inline const sint32&	GetMana();
	virtual void SetMana(sint32 amount);
	virtual inline float			GetManaRatio();

	inline virtual int16	GetAC();
	inline virtual int16	GetATK();
	inline virtual sint16	GetSTR();
	inline virtual sint16	GetSTA();
	inline virtual sint16	GetDEX();
	inline virtual sint16	GetAGI();
	inline virtual sint16	GetINT();
	inline virtual sint16	GetWIS();
	inline virtual sint16	GetCHA();
	inline virtual sint16	GetMR();
	inline virtual sint16	GetFR();
	inline virtual sint16	GetDR();
	inline virtual sint16	GetPR();
	inline virtual sint16	GetCR();

	inline virtual sint16  GetMaxSTR();
	inline virtual sint16  GetMaxSTA();
	inline virtual sint16  GetMaxDEX();
	inline virtual sint16  GetMaxAGI();
	inline virtual sint16  GetMaxINT();
	inline virtual sint16  GetMaxWIS();
	inline virtual sint16  GetMaxCHA();

	virtual float GetActSpellRange(int16 spell_id, float range);
	virtual sint32  GetActSpellDamage(int16 spell_id, sint32 value);
	virtual sint32  GetActSpellHealing(int16 spell_id, sint32 value);
	virtual sint32 GetActSpellCost(int16 spell_id, sint32 cost);
	virtual sint32 GetActSpellDuration(int16 spell_id, sint32 duration);
	virtual sint32 GetActSpellCasttime(int16 spell_id, sint32 casttime);
	double ResistSpell(uint8 ressit_type, int16 spell_id, Mob *caster);
	int GetSpecializeSkill(int16 spell_id);

	int32 GetNPCTypeID();
	const int32& GetNPCSpellsID();
	bool IsTargeted();

	inline const float&	GetX();
	inline const float&	GetY();
	inline const float&	GetZ();
	inline const float&	GetHeading();
	inline const float&	GetSize();

	void	SetFollowID(int32 id);
	int32	GetFollowID();

	virtual void	Message(int32 type, const char* message, ...);
	virtual void	Message_StringID(int32 type, int32 string_id, int32 distance = 0);
	void Say(const char *format, ...);
	void Shout(const char *format, ...);
	void Emote(const char *format, ...);
	void InterruptSpell(int16 spellid = 0xFFFF);

	virtual void	CastSpell(int16 spell_id, int16 target_id, int16 slot = 10, sint32 casttime = -1, sint32 mana_cost = -1);
	bool	IsImmuneToSpell(int16 spell_id, Mob *caster);
	void	BuffFadeBySpellID(int16 spell_id);
	void	BuffFadeByEffect(int effectid, int skipslot = -1);
	void	BuffFadeAll();
	void	BuffFadeBySlot(int slot, bool iRecalcBonuses = true);

	int	CanBuffStack(int16 spellid, int8 caster_level, bool iFailIfOverwrite = false);
	inline bool	IsCasting();
	int16	CastingSpellID();

	void	SetAppearance(int8 app, bool iIgnoreSelf = true);

	inline const int8&	GetAppearance();
	inline const int8&	GetRunAnimSpeed();
	inline void			SetRunAnimSpeed(sint8 in);

	void	SetPetID(int16 NewPetID);
	inline const	int16&	GetPetID();
	void	SetFamiliarID(int16 NewPetID);
	inline const	int16&	GetFamiliarID();
	void	SetOwnerID(int16 NewOwnerID);
	inline const	int16&	GetOwnerID();
	inline const	int16&	GetPetType();

	inline const	int8&	GetBodyType();
	virtual void    Stun(int duration);

	void	Spin();
	void	Kill();

	inline void	SetInvul(bool invul);
	inline bool	GetInvul();
	inline void	SetExtraHaste(int Haste);
	virtual int GetHaste();
	int		GetMonkHandToHandDamage(void);

	bool	CanThisClassDoubleAttack(void);
	bool	CanThisClassDualWield(void);
	bool	CanThisClassRiposte(void);
	bool	CanThisClassDodge(void);
	bool	CanThisClassParry(void);

	int	GetMonkHandToHandDelay(void);
	int8	GetClassLevelFactor();
	void	Mesmerize();
	inline bool	IsMezzed();
	inline bool	IsStunned();
	inline int16	GetRune();
	inline void	SetRune(int16 in_rune);
	inline const int16& GetMagicRune();
	void	SetMagicRune(int16 in_rune);

	void StartEnrage();
	bool IsEnraged();

	void				StopWandering();
	void				ResumeWandering();
	void				PauseWandering(int pausetime);
	void				MoveTo(float mtx, float mty, float mtz);
	FACTION_VALUE GetFactionCon(Mob* iOther);
	inline const bool&	IsAIControlled();
	inline const float&	GetGuardX();
	inline const float&	GetGuardY();
	inline const float&	GetGuardZ();

	void SetGuardXYZ(float x, float y, float z);
	inline const float&	GetGuardHeading();
	inline const float&	GetSpawnX();
	inline const float&	GetSpawnY();
	inline const float&	GetSpawnZ();
	inline const float&	GetSpawnHeading();
	inline const float& GetAggroRange();
	inline const float& GetAssistRange();
	void				SaveGuardSpot(bool iClearGuardSpot = false);
	void				SaveSpawnSpot();
	void				UpdateWaypoint(int wp_index);

	inline void			SetPetOrder(Mob::eStandingPetOrder i);
	inline const Mob::eStandingPetOrder& GetPetOrder();
	inline const bool&	IsRoamer();
	inline const bool   IsRooted();

	void AddToHateList(Mob* other, sint32 hate = 0, sint32 damage = 0, bool iYellForHelp = true, bool bFrenzy = false, bool iBuffTic = false);
	void SetHate(Mob* other, sint32 hate = 0, sint32 damage = 0);
	int32 GetHateAmount(Mob* tmob, bool is_dam = false);
	int32 GetDamageAmount(Mob* tmob);
	Mob* GetHateTop();
	Mob* GetHateDamageTop(Mob* other);
	Mob* GetHateRandom();
	bool IsEngaged();
	bool HateSummon();
	void FaceTarget(Mob* MobToFace = 0, bool update = false);
	void SetHeading(float iHeading);
	void WhipeHateList();

	inline bool	CheckAggro(Mob* other);

	sint8				CalculateHeadingToTarget(float in_x, float in_y);
	bool				CalculateNewPosition(float x, float y, float z, float speed, bool checkZ = false);
	bool				CalculateNewPosition2(float x, float y, float z, float speed, bool checkZ = false);
	float				CalculateDistance(float x, float y, float z);
	void				CalculateNewWaypoint();
	int8				CalculateHeadingToNextWaypoint();
	//	float				CalculateDistanceToNextWaypoint();
	void				AssignWaypoints(int16 grid);
	void				SendTo(float new_x, float new_y, float new_z);
	void				SendToFixZ(float new_x, float new_y, float new_z);
	void				NPCSpecialAttacks(const char* parse, int permtag);
	inline int32&		DontHealMeBefore();
	inline int32&		DontBuffMeBefore();
	inline int32&		DontDotMeBefore();
	inline int32&		DontRootMeBefore();
	inline int32&		DontSnareMeBefore();

	sint16	GetResist(int8 type);
	void	StopSong();
	Mob*	GetShieldTarget();
	void	SetShieldTarget(Mob* mob);
	Mob*	GetSongTarget();
	void	SetSongTarget(Mob* mob);
	bool	Charmed();
	int32	GetLevelHP(int8 tlevel);
	int16	CheckAggroAmount(int16 spellid);
	int16	CheckHealAggroAmount(int16 spellid);
	virtual uint16 GetAA(uint8 aa_id);
	bool DivineAura();
	
//END MOB METHODS





	
	
