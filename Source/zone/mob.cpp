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
#include "../common/debug.h"
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include "masterentity.h"
#include "zone.h"
#include "spdat.h"
#include "../common/skills.h"
#include "map.h"
#include "StringIDs.h"

#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
#include "../common/Guilds.h"
extern GuildRanks_Struct guilds[512];
extern GuildWars guildwars;
#endif

#include "../common/eq_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "../common/database.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"

#include <stdio.h>
extern EntityList entity_list;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern bool spells_loaded;
extern Database database;
extern Zone* zone;

Mob::Mob(const char*   in_name,
         const char*   in_lastname,
         sint32  in_cur_hp,
         sint32  in_max_hp,
         int8    in_gender,
         uint16    in_race,
         int8    in_class,
         int8    in_bodytype,   // neotokyo targettype support 17-Nov-02
         int8    in_deity,
         int8    in_level,
         int32	 in_npctype_id, // rembrant, Dec. 20, 2001
         const int8*	 in_skills, // socket 12-29-01
		 float	in_size,
		 float	in_walkspeed,
		 float	in_runspeed,
         float	in_heading,
         float	in_x_pos,
         float	in_y_pos,
         float	in_z_pos,

         int8    in_light,
         const int8*   in_equipment,
		 int8	 in_texture,
		 int8	 in_helmtexture,
		 int16	 in_ac,
		 int16	 in_atk,
		 int8	 in_str,
		 int8	 in_sta,
		 int8	 in_dex,
		 int8	 in_agi,
		 int8	 in_int,
		 int8	 in_wis,
		 int8	 in_cha,
		 int8	in_haircolor,
		 int8	in_beardcolor,
		 int8	in_eyecolor1, // the eyecolors always seem to be the same, maybe left and right eye?
		 int8	in_eyecolor2,
		 int8	in_hairstyle,
// vesuvias - appearence fix
		 int8	in_luclinface,
		 int8	in_beard,

		 int8	in_aa_title,
		 float	in_fixed_z,
		 int16	in_d_meele_texture1,
		 int16	in_d_meele_texture2,
		 int8	in_see_invis,			// Mongrel: see through invis/ivu
		 int8  in_see_invis_undead,
		 int8	in_qglobal

		 )
{
	tar_ndx=0;
	tar_vector=0;
	tar_vx=0;
	tar_vy=0;
	tar_vz=0;
	tarx=0;
	tary=0;
	tarz=0;
	AI_Init();
	SetMoving(false);
	moved=false;
// for quest signal() command
	signaled=false;
	adverrorinfo = 0;
	name[0]=0;
	clean_name[0]=0;
	lastname[0]=0;
	if(in_name)
		strncpy(name,in_name,64);
	if(in_lastname)
		strncpy(lastname,in_lastname,64);
	cur_hp		= in_cur_hp;
	max_hp		= in_max_hp;
	base_hp		= in_max_hp;
	gender		= in_gender;
	race		= in_race;
	base_gender	= in_gender;
	base_race	= in_race;
	class_		= in_class;
    bodytype    = in_bodytype;
	deity		= in_deity;
	level		= in_level;
	npctype_id	= in_npctype_id; // rembrant, Dec. 20, 2001
	size		= in_size;
	walkspeed   = in_walkspeed; 
	runspeed   = in_runspeed;

	
    // neotokyo: sanity check
    if (walkspeed < 0 || walkspeed > 10)
        walkspeed = 0.7f;
    if (runspeed < 0 || runspeed > 20)
        runspeed = 1.25f;
	
	heading		= in_heading;
	x_pos		= in_x_pos;
	y_pos		= in_y_pos;
	z_pos		= in_z_pos;
//	fixedZ		= in_fixed_z;
	light		= in_light;
	texture		= in_texture;
	helmtexture	= in_helmtexture;
	d_meele_texture1 = in_d_meele_texture1;
	d_meele_texture2= in_d_meele_texture2;
	haircolor	= in_haircolor;
	beardcolor	= in_beardcolor;
	eyecolor1	= in_eyecolor1;
	eyecolor2	= in_eyecolor2;
	hairstyle	= in_hairstyle;
// vesuvias - appearence fix
	luclinface	= in_luclinface;
	beard		= in_beard;

	if(in_aa_title>0)
		aa_title	= in_aa_title;
	else
		aa_title	=0xFF;
	AC		= in_ac;
	ATK		= in_atk;
	STR		= in_str;
	STA		= in_sta;
	DEX		= in_dex;
	AGI		= in_agi;
	if (in_int<=75)
		INT 	= 105;
	else
		INT		= in_int;
	WIS		= in_wis;
	CHA		= in_cha;
	MR = CR = FR = DR = PR = 0;

	NPCTypedata = 0;
	ExtraHaste = 0;
	bEnraged = false;

	cur_mana = 0;
	max_mana = 0;
	hp_regen = 0;
	mana_regen = 0;
	invisible = false;
	invisible_undead = false;
	sneaking = false;
	qglobal=0;

	int i = 0;

	for (i=0; i<9; i++)
	{
		if (in_equipment == 0)
		{
			equipment[i] = 0;
		}
		else
		{
			equipment[i] = in_equipment[i];
		}
	}

	/*	for (i=0; i<74; i++) { // socket 12-29-01
	if (in_skills == 0) {
	skills[i] =0;
	}
	else {
	skills[i] = in_skills[i];
	}
		 }*/ // Quag: dont understand why a memcpy wont do the job here =P
	if (in_skills) {
		memcpy(skills, in_skills, sizeof(skills));
	}
	else {
		memset(skills, 0, sizeof(skills));
	}
	for (int j = 0; j < BUFF_COUNT; j++) {
		buffs[j].spellid = 0xFFFF;
	}

    // clear the proc array
    RemoveProcFromWeapon(0, true);
#ifdef WIN32
	for (j = 0; j < MAX_PROCS; j++)
#else
    for (int j = 0; j < MAX_PROCS; j++)
#endif
    {
        PermaProcs[j].spellID = 0xFFFF;
        PermaProcs[j].chance = 0;
        PermaProcs[j].pTimer = NULL;
    }

	delta_heading = 0;
	delta_x = 0;
	delta_y = 0;
	delta_z = 0;

	invulnerable = false;
	isgrouped = false;
	appearance = 0;
	pRunAnimSpeed = 0;
	guildeqid = GUILD_NONE;

	attack_timer = new Timer(2000);
	attack_timer_dw = new Timer(2000);
	tic_timer = new Timer(6000);
	mana_timer = new Timer(5000);
	spellend_timer = new Timer(0);
    spellend_timer->Disable();
	casting_spell_id = 0;
	target = 0;

	itembonuses = new StatBonuses;
	spellbonuses = new StatBonuses;
	memset(itembonuses, 0, sizeof(StatBonuses));
	memset(spellbonuses, 0, sizeof(StatBonuses));
	spellbonuses->ArrgoRange = -1;
	spellbonuses->AssistRange = -1;
	pLastChange = 0;
	this->SetPetID(0);
	this->SetOwnerID(0);
	typeofpet = 0xFF;  // means is not a pet; 0xFF means charmed
    this->SetFamiliarID(0);
	SaveSpawnSpot();

	isattacked = false;
	attacked_count = 0;
	mezzed = false;
	stunned = false;
	stunned_timer = new Timer(0);
	rune = 0;
    magicrune = 0;
	for (int m = 0; m < 60; m++) {
		flag[m]=0;
	}
	for (i=0; i<SPECATK_MAXNUM ; i++) {
		SpecAttacks[i] = false;
		SpecAttackTimers[i] = 0;
	}
    memset( RampageArray, 0, sizeof(RampageArray));
	wandertype=0;
	pausetype=0;
	max_wp=0;
	cur_wp=0;
	patrol=0;
	follow=0;

	permarooted = ( walkspeed == 0 ) && ( runspeed == 0 );

	movetimercompleted = false;
	roamer = false;
	rooted = false;
    guard_x = 0;
	guard_y = 0;
	guard_z = 0;
	guard_heading = 0;
    spawn_x = 0;
	spawn_y = 0;
	spawn_z = 0;
	spawn_heading = 0;
	pStandingPetOrder = SPO_Follow;

	see_invis = in_see_invis;
	see_invis_undead = in_see_invis_undead;
	qglobal=in_qglobal;
	
	// Bind wound
	bindwound_timer = new Timer(10000);
	bindwound_timer->Disable();
	bindwound_target = 0;
	
	trade = new Trade(this);
	// hp event
	nexthpevent = 0;
}

Mob::~Mob()
{
	AI_Stop();
	safe_delete(stunned_timer);
	SetPet(0);
	safe_delete(itembonuses);
	safe_delete(spellbonuses);
	safe_delete(spellend_timer);
	safe_delete(tic_timer);
	for (int i=0; i<SPECATK_MAXNUM ; i++) {
		safe_delete(SpecAttackTimers[i]);
	}
	if (attack_timer != 0) {
		safe_delete(attack_timer);
		attack_timer = 0;
	}
	if (mana_timer != 0) {
		safe_delete(mana_timer);
		mana_timer = 0;
	}
	// Kaiyodo - destruction of dual wield attack timer
	if(attack_timer_dw != 0)
	{
		safe_delete(attack_timer_dw);
		attack_timer_dw = 0;
	}
	safe_delete(bindwound_timer);
	APPLAYER app;
	CreateDespawnPacket(&app);
	entity_list.QueueClients(this, &app, true);
	entity_list.RemoveFromTargets(this);
	
	safe_delete(trade);
}

int32 Mob::GetAppearanceValue(int8 iAppearance) {
	switch (iAppearance) {
		// 0 standing, 1 sitting, 2 ducking, 3 lieing down, 4 looting
		case 0: {
			return 100;
		}
		case 1: {
			return 110;
		}
		case 2: {
			return 111;
		}
		case 3: {
			return 115;
		}
		case 4: {
			return 105;
		}
		default: {
			return 100;
		}
	}
}
int32 Mob::GetPRange(float x, float y, float z){
	return 0;
}
void Mob::SetInvisible(bool state)
{
	invisible = state;
	SendAppearancePacket(AT_Invis, invisible);
}

bool Mob::IsInvisible(Mob* other)
{
	// TC - removing until SeeInvisible is implemented.
	// Mongrel: Reimplementing see invis
	if (other && invisible && !other->SeeInvisible())
	{
		return true;
	}
	if (other && invisible_undead && !other->SeeInvisibleUndead())
	{
		return true;
	}
	if (other && !sneaking)
	{
		return false;
	}
	if (sneaking && BehindMob(other, GetX(), GetY()) )
	{
		return true;
	}
	else
	{
		return invisible;
	}
} 

float Mob::GetWalkspeed()
{
    float aa_speed = 1.0f;
    if (IsClient()){
        uint8 *aa_item = &(((uint8 *)&this->CastToClient()->aa)[13]);
        if (*aa_item > 0 && *aa_item < 4){
            aa_speed += (float) ((100+*aa_item)/100);
        }
    }
    if (IsRooted())
        return 0.0f;
    if (spellbonuses->movementspeed || itembonuses->movementspeed)
        return (walkspeed * (100+spellbonuses->movementspeed+itembonuses->movementspeed)) / 100;
    else
        return (walkspeed * aa_speed);
}

float Mob::GetRunspeed()
{
    float aa_speed = 1.0f;
    if (IsClient()){
        uint8 *aa_item = &(((uint8 *)&this->CastToClient()->aa)[13]);
        if (*aa_item > 0 && *aa_item < 4){
            aa_speed += (float) ((100+*aa_item)/100);
        }
    }
    if (IsRooted())
        return 0.0f;
    if (spellbonuses->movementspeed || itembonuses->movementspeed)
        return (runspeed * (100+spellbonuses->movementspeed+itembonuses->movementspeed)) / 100;
    else
        return (runspeed * aa_speed);
}

sint32 Mob::CalcMaxMana()
{
	switch (GetCasterClass()) {
		case 'I':
			max_mana = (((GetINT()/5)+2) * GetLevel()) + spellbonuses->Mana + itembonuses->Mana;
			break;
		case 'W':
			max_mana = (((GetWIS()/5)+2) * GetLevel()) + spellbonuses->Mana + itembonuses->Mana;
			break;
		case 'N':
		default:
#ifdef GUILDWARS
			max_mana = GetLevel()*50;
#else
			max_mana = 0;
#endif
			break;
	}

#if EQDEBUG >= 11
	if(IsClient())
		LogFile->write(EQEMuLog::Debug, "Mob::CalcMaxMana() called for %s - returning %d", GetName(), max_mana);
#endif

	return max_mana;
}

char Mob::GetCasterClass() {
	switch(class_)
	{
	case CLERIC:
	case PALADIN:
	case RANGER:
	case DRUID:
	case SHAMAN:
	case BEASTLORD:
	case CLERICGM:
	case PALADINGM:
	case RANGERGM:
	case DRUIDGM:
	case SHAMANGM:
	case BEASTLORDGM:
		return 'W';
		break;

	case SHADOWKNIGHT:
	case BARD:
	case NECROMANCER:
	case WIZARD:
	case MAGICIAN:
	case ENCHANTER:
	case SHADOWKNIGHTGM:
	case BARDGM:
	case NECROMANCERGM:
	case WIZARDGM:
	case MAGICIANGM:
	case ENCHANTERGM:
		return 'I';
		break;

	default:
		return 'N';
		break;
	}
}

void Mob::CreateSpawnPacket(APPLAYER* app, Mob* ForWho) {
	app->opcode = OP_NewSpawn;
	app->size = sizeof(NewSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);	
	NewSpawn_Struct* ns = (NewSpawn_Struct*)app->pBuffer;
	FillSpawnStruct(ns, ForWho);
}

void Mob::CreateHorseSpawnPacket(APPLAYER* app, const char* ownername, uint16 ownerid, Mob* ForWho) {
	app->opcode = OP_NewSpawn;
	app->pBuffer = new uchar[sizeof(NewSpawn_Struct)];
	app->size = sizeof(NewSpawn_Struct);
	memset(app->pBuffer, 0, sizeof(NewSpawn_Struct));
	NewSpawn_Struct* ns = (NewSpawn_Struct*)app->pBuffer;
	FillSpawnStruct(ns, ForWho);
#if (EQDEBUG >= 11)
	printf("Horse Spawn Packet - Owner: %s\n", ownername);
	DumpPacket(app);
#endif
}


void Mob::CreateSpawnPacket(APPLAYER* app, NewSpawn_Struct* ns) {
	app->opcode = OP_NewSpawn;
	app->size = sizeof(NewSpawn_Struct);
	
	app->pBuffer = new uchar[sizeof(NewSpawn_Struct)];
	
	// Copy ns directly into packet
	memcpy(app->pBuffer, ns, sizeof(NewSpawn_Struct));
	
	// Custom packet data
	NewSpawn_Struct* ns2 = (NewSpawn_Struct*)app->pBuffer;
	strcpy(ns2->spawn.name, ns->spawn.name);
	/*if (ns->spawn.class_==MERCHANT)
		strcpy(ns2->spawn.last_name, "EQEmu Shopkeeper");
	else*/ if (ns->spawn.class_==TRIBUTE_MASTER)
		strcpy(ns2->spawn.last_name, "Tribute Master");
	else if (ns->spawn.class_==BANKER)
		strcpy(ns2->spawn.last_name, "EQEmu Banker");
	else if (ns->spawn.class_==ADVENTUREMERCHANT)
#ifdef GUILDWARS
		strcpy(ns->spawn.last_name,"GuildWars Merchant");
#else
		strcpy(ns->spawn.last_name,"Adventure Merchant");
#endif
	else
		strcpy(ns2->spawn.last_name, ns->spawn.last_name);
}

void Mob::FillSpawnStruct(NewSpawn_Struct* ns, Mob* ForWho)
{
	int i;

	strcpy(ns->spawn.name, name);
	if(IsClient())
		strncpy(ns->spawn.last_name,lastname,32);
	ns->spawn.heading	= FloatToEQ19(heading);
	ns->spawn.x			= FloatToEQ19(x_pos);//((sint32)x_pos)<<3;
	ns->spawn.y			= FloatToEQ19(y_pos);//((sint32)y_pos)<<3;
	ns->spawn.z			= FloatToEQ19(z_pos);//((sint32)z_pos)<<3;
	ns->spawn.spawn_id	= GetID();
	ns->spawn.cur_hp	= (sint16)GetHPRatio();
	ns->spawn.max_hp	= 100;
	ns->spawn.race		= race;
	ns->spawn.runspeed	= runspeed;
	ns->spawn.walkspeed	= walkspeed;
	ns->spawn.class_	= class_;
	ns->spawn.gender	= gender;
	ns->spawn.level		= level;
	ns->spawn.deity		= deity;
	ns->spawn.animation	= 0;
// vesuvias - appearence fix
//	ns->spawn.light		= light; //not really sure where light is now in the struct


	ns->spawn.invis		= invisible;	// TODO: load this before spawning players
	ns->spawn.npc		= IsClient() ? 0 : 1;
	ns->spawn.pet_owner_id	= ownerid;

	ns->spawn.haircolor = haircolor ? haircolor : 0xFF;
	ns->spawn.beardcolor = beardcolor ? beardcolor : 0xFF;
	ns->spawn.eyecolor1 = eyecolor1 ? eyecolor1 : 0xFF;
	ns->spawn.eyecolor2 = eyecolor2 ? eyecolor2 : 0xFF;
	ns->spawn.hairstyle = hairstyle ? hairstyle : 0xFF;
	ns->spawn.face = luclinface;
	ns->spawn.beard = beard ? beard : 0xFF;

	ns->spawn.unknown143[0] = 0xff;//this used to be labeled beard.. if its not FF it will turn
								   //mob invis

	if(helmtexture && helmtexture != 0xFF)
	{
		ns->spawn.helm=helmtexture;
	}
	ns->spawn.equip_chest2  = texture;

	ns->spawn.guild_rank	= 0xFF;
	ns->spawn.size			= size;
	ns->spawn.bodytype = bodytype;

	//ns->spawn.unknown251[2] = helmtexture;
	//ns->spawn.trap_type	= bodytype;
	//ns->spawn.walkspeed	= walkspeed;
	//ns->spawn.runspeed	= runspeed;

	if(IsNPC())
		ns->spawn.aa_title = 0xFF;
	
	if (ns->spawn.class_==MERCHANT)
		strcpy(ns->spawn.last_name, "EQEmu Shopkeeper");
	else if (ns->spawn.class_==BANKER)
		strcpy(ns->spawn.last_name, "EQEmu Banker");
	else if (ns->spawn.class_==ADVENTUREMERCHANT)
#ifdef GUILDWARS
		strcpy(ns->spawn.last_name,"GuildWars Merchant");
#else
		strcpy(ns->spawn.last_name,"Adventure Merchant");
#endif
	else
		strncpy(ns->spawn.last_name, lastname, 32);

	// This portion of packet is always fixed with these values
	//0xff,0x33,0x33,0x33,0x3f                  A0,3f
	//value of FF in chMemCpy222 causes npcs to wear helms

	for(i = 0; i < 9; i++)
	{
		ns->spawn.equipment[i] = GetEquipmentMaterial(i);
		ns->spawn.dye_rgb[i].color = GetEquipmentColor(i);
	}
}

void Mob::CreateDespawnPacket(APPLAYER* app)
{
	app->opcode = OP_DeleteSpawn;
	app->size = sizeof(DeleteSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);
	DeleteSpawn_Struct* ds = (DeleteSpawn_Struct*)app->pBuffer;
	ds->spawn_id = GetID();
}

void Mob::CreateHPPacket(APPLAYER* app)
{ 
	this->IsFullHP=(cur_hp>=max_hp); 
	app->opcode = OP_SendHPTarget; 
	app->size = sizeof(SpawnHPUpdate_Struct2); 
	app->pBuffer = new uchar[app->size]; 
	memset(app->pBuffer, 0, sizeof(SpawnHPUpdate_Struct2)); 
	SpawnHPUpdate_Struct2* ds = (SpawnHPUpdate_Struct2*)app->pBuffer; 

	ds->spawn_id = GetID(); 
	// they don't need to know the real hp
	ds->hp = (int)GetHPRatio();
 
	// hp event 
	if ( IsNPC() && ( GetNextHPEvent() > 0 ) ) { 
		if ( ds->hp < GetNextHPEvent() ) { 
			int lasthpevent = nexthpevent;
			parse->Event(EVENT_HP, GetNPCTypeID(), 0, this, 0);
			if ( lasthpevent == nexthpevent ) {
				SetNextHPEvent(0);
			}
		} 
	}  

#if 0	// solar: old stuff, leaving while testing changes
	// we dont give the actual hp of npcs
	if(IsNPC() || GetMaxHP() > 30000)
	{
		ds->cur_hp = (int)GetHPRatio();
		ds->max_hp = 100;
	}
	else
	{
		if(IsClient())
		{
			ds->cur_hp = CastToClient()->GetHP() - itembonuses->HP;
			ds->max_hp = CastToClient()->GetMaxHP() - itembonuses->HP;
#ifdef SOLAR
			Message(0, "HP: %d/%d", ds->cur_hp, ds->max_hp);
#endif
		}
		else
		{
			ds->cur_hp = GetHP();
			ds->max_hp = GetMaxHP();
		}
	}
#endif
} 

// sends hp update of this mob to people who might care
void Mob::SendHPUpdate()
{
	APPLAYER hp_app;
	Group *group;
	
	// destructor will free the pBuffer
 	CreateHPPacket(&hp_app);

	// send to people who have us targeted
 	entity_list.QueueClientsByTarget(this, &hp_app, false, 0, false);

	// send to group
	if(IsGrouped())
	{
		group = entity_list.GetGroupByMob(this);
		if(group)
			group->QueuePacket(&hp_app, false);
	}	

	// send to master
	if(GetOwner() && GetOwner()->IsClient())
	{
		GetOwner()->CastToClient()->QueuePacket(&hp_app, false);
	}

	// send to pet
	if(GetPet() && GetPet()->IsClient())
	{
		GetPet()->CastToClient()->QueuePacket(&hp_app, false);
	}

	// send to self - we need the actual hps here
	if(IsClient())
	{
		APPLAYER* hp_app2 = new APPLAYER(OP_HPUpdate,sizeof(SpawnHPUpdate_Struct));
		SpawnHPUpdate_Struct* ds = (SpawnHPUpdate_Struct*)hp_app2->pBuffer; 
		ds->cur_hp = CastToClient()->GetHP() - itembonuses->HP;
		ds->spawn_id = GetID();
		ds->max_hp = CastToClient()->GetMaxHP() - itembonuses->HP;
		CastToClient()->QueuePacket(hp_app2);
		safe_delete(hp_app2);
	}
}

// this one just warps the mob to the current location
void Mob::SendPosition(){
	APPLAYER* app = new APPLAYER(OP_MobUpdate, sizeof(SpawnPositionUpdate_Struct));
	SpawnPositionUpdate_Struct* spu = (SpawnPositionUpdate_Struct*)app->pBuffer;	
	MakeSpawnUpdate(spu);
	entity_list.QueueCloseClients(0, app, true, 800);
	safe_delete(app);
}

// this one is for mobs on the move, with deltas - this makes them walk
void Mob::SendPosUpdate(int8 iSendToSelf) {
	APPLAYER* app = new APPLAYER(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)app->pBuffer;
	MakeSpawnUpdate(spu);
	if (iSendToSelf == 2) {
		if (this->IsClient())
			this->CastToClient()->FastQueuePacket(&app);
	}
	else
		entity_list.QueueCloseClients(this, app, (iSendToSelf==0), 800);
	safe_delete(app);
}

// this is for SendPosition()
void Mob::MakeSpawnUpdate(SpawnPositionUpdate_Struct *spu){
	spu->spawn_id	= GetID();
	spu->y		= FloatToEQ19(y_pos);
	spu->x		= FloatToEQ19(x_pos);
	spu->z		= FloatToEQ19(z_pos);
	spu->heading	= FloatToEQ19(heading);
	spu->u3=0;
	spu->unused2=15;
}

// this is for SendPosUpdate()
void Mob::MakeSpawnUpdate(PlayerPositionUpdateServer_Struct* spu) {
	spu->spawn_id	= GetID();
	spu->y_pos		= FloatToEQ19(x_pos);
	spu->x_pos		= FloatToEQ19(y_pos);
	spu->z_pos		= FloatToEQ19(z_pos);
	spu->delta_x	= FloatToEQ13(delta_x);
	spu->delta_y	= FloatToEQ13(delta_y);
	spu->delta_z	= FloatToEQ13(delta_z);
	spu->heading	= FloatToEQ19(heading);
	if(this->IsClient())
		spu->animation=animation;
	else
		spu->animation	= pRunAnimSpeed;//animation;
	spu->delta_heading = delta_heading;
}

void Mob::ShowStats(Client* client) {
	client->Message(0, "Name: %s %s", GetName(), lastname);
	client->Message(0, "  Level: %i  MaxHP: %i  CurHP: %i  AC: %i  Class: %i", GetLevel(), GetMaxHP(), GetHP(), GetAC(), GetClass());
	client->Message(0, "  MaxMana: %i  CurMana: %i  ATK: %i  Size: %1.1f", GetMaxMana(), GetMana(), GetATK(), GetSize());
	client->Message(0, "  STR: %i  STA: %i  DEX: %i  AGI: %i  INT: %i  WIS: %i  CHA: %i", GetSTR(), GetSTA(), GetDEX(), GetAGI(), GetINT(), GetWIS(), GetCHA());
	client->Message(0, "  Race: %i  BaseRace: %i  Texture: %i  HelmTexture: %i  Gender: %i  BaseGender: %i", GetRace(), GetBaseRace(), GetTexture(), GetHelmTexture(), GetGender(), GetBaseGender());
	if (client->Admin() >= 100) {
		client->Message(0, "  EntityID: %i  PetID: %i  OwnerID: %i  AIControlled: %i", this->GetID(), this->GetPetID(), this->GetOwnerID(), this->IsAIControlled());
		if (this->IsClient()) {
			client->Message(0, "  CharID: %i  PetID: %i", this->CastToClient()->CharacterID(), this->GetPetID());
		}
		else if (this->IsCorpse()) {
			if (this->IsPlayerCorpse()) {
				client->Message(0, "  CharID: %i  PlayerCorpse: %i", this->CastToCorpse()->GetCharID(), this->CastToCorpse()->GetDBID());
			}
			else {
				client->Message(0, "  NPCCorpse", this->GetID());
			}
		}
		else if (this->IsNPC()) {
			int32 spawngroupid = 0;
			if(this->CastToNPC()->respawn2 != 0)
				spawngroupid = this->CastToNPC()->respawn2->SpawnGroupID();
			client->Message(0, "  NPCID: %u  SpawnGroupID: %u LootTable: %u  FactionID: %i  SpellsID: %u", this->GetNPCTypeID(),spawngroupid, this->CastToNPC()->GetLoottableID(), this->CastToNPC()->GetNPCFactionID(), this->GetNPCSpellsID());
		}
		if (this->IsAIControlled()) {
			client->Message(0, "  AIControlled: ArrgoRange: %1.0f  AssistRange: %1.0f", this->GetArrgoRange(), this->GetAssistRange());
		}
	}
}

void Mob::DoAnim(const int animnum, int type, bool ackreq) {
	APPLAYER* outapp = new APPLAYER(OP_EmoteAnim, sizeof(EmoteAnim_Struct));
	EmoteAnim_Struct* anim = (EmoteAnim_Struct*)outapp->pBuffer;
	anim->spawnid = GetID();
	if(type==1){
		anim->action = 10;
		anim->value=animnum;
	}
	else{
		anim->action = animnum;
		anim->value=type;
	}
	entity_list.QueueCloseClients(this, outapp, false, 200, 0, ackreq);
	safe_delete(outapp);
}

void Mob::ShowBuffs(Client* client) {
	if (!spells_loaded)
		return;
	client->Message(0, "Buffs on: %s", this->GetName());
	for (int i=0; i < BUFF_COUNT; i++) {
		if (buffs[i].spellid != 0xFFFF) {
			if (buffs[i].durationformula == DF_Permanent)
				client->Message(0, "  %i: %s: Permanent", i, spells[buffs[i].spellid].name);
			else
				client->Message(0, "  %i: %s: %i tics left", i, spells[buffs[i].spellid].name, buffs[i].ticsremaining);

		}
	}
	if (IsClient()){
		client->Message(0, "itembonuses:");
		client->Message(0, "Atk:%i Ac:%i HP(%i):%i Mana:%i", itembonuses->ATK, itembonuses->AC, itembonuses->HPRegen, itembonuses->HP, itembonuses->Mana);
		client->Message(0, "Str:%i Sta:%i Dex:%i Agi:%i Int:%i Wis:%i Cha:%i",
			itembonuses->STR,itembonuses->STA,itembonuses->DEX,itembonuses->AGI,itembonuses->INT,itembonuses->WIS,itembonuses->CHA);
		client->Message(0, "SvMagic:%i SvFire:%i SvCold:%i SvPoison:%i SvDisease:%i",
				itembonuses->MR,itembonuses->FR,itembonuses->CR,itembonuses->PR,itembonuses->DR);
		client->Message(0, "DmgShield:%i Haste:%i", itembonuses->DamageShield, itembonuses->haste );
		client->Message(0, "spellbonuses:");
		client->Message(0, "Atk:%i Ac:%i HP(%i):%i Mana:%i", spellbonuses->ATK, spellbonuses->AC, spellbonuses->HPRegen, spellbonuses->HP, spellbonuses->Mana);
		client->Message(0, "Str:%i Sta:%i Dex:%i Agi:%i Int:%i Wis:%i Cha:%i",
			spellbonuses->STR,spellbonuses->STA,spellbonuses->DEX,spellbonuses->AGI,spellbonuses->INT,spellbonuses->WIS,spellbonuses->CHA);
		client->Message(0, "SvMagic:%i SvFire:%i SvCold:%i SvPoison:%i SvDisease:%i",
				spellbonuses->MR,spellbonuses->FR,spellbonuses->CR,spellbonuses->PR,spellbonuses->DR);
		client->Message(0, "DmgShield:%i Haste:%i", spellbonuses->DamageShield, spellbonuses->haste );
	}
}

void Mob::GMMove(float x, float y, float z, float heading) {
	x_pos = x;
	y_pos = y;
	z_pos = z;
	if (heading != 0.01)
		this->heading = heading;
	if(IsNPC())
		SaveGuardSpot(true);
	SendPosition();
	//SendPosUpdate(1);
}



// vesuvias - appearence fix
void Mob::SendIllusionPacket(int16 in_race, int8 in_gender, int16 in_texture, int16 in_helmtexture, int8 in_haircolor, int8 in_beardcolor, int8 in_eyecolor1, int8 in_eyecolor2, int8 in_hairstyle, int8 in_luclinface, int8 in_beard,int8 in_aa_title) {

	if (in_race == 0) {
		this->race = GetBaseRace();
		if (in_gender == 0xFF)
			this->gender = GetBaseGender();
		else
			this->gender = in_gender;
	}
	else {
		this->race = in_race;
		if (in_gender == 0xFF) {
			int8 tmp = Mob::GetDefaultGender(this->race, gender);
			if (tmp == 2)
				gender = 2;
			else if (gender == 2 && GetBaseGender() == 2)
				gender = tmp;
			else if (gender == 2)
				gender = GetBaseGender();
		}
		else
			gender = in_gender;
	}
	if (in_texture == 0xFFFF) {
		if ((race == 0 || race > 12) && race != 128 && race != 130) {
			if (GetTexture() == 0xFF)
				this->texture = 0;
		}
		else if (in_race == 0)
			this->texture = 0xFF;
	}
	else if (in_texture != 0xFF || this->IsClient() || this->IsPlayerCorpse()) {
		this->texture = in_texture;
	}
	else
		this->texture = 0;
	if (in_helmtexture == 0xFFFF) {
		if (in_texture != 0xFFFF)
			this->helmtexture = this->texture;
		else if ((race == 0 || race > 12) && race != 128 && race != 130) {
			if (GetHelmTexture() == 0xFF)
				this->helmtexture = 0;
		}
		else if (in_race == 0)
			this->helmtexture = 0xFF;
		else
			this->helmtexture = 0;
	}
	else if (in_helmtexture != 0xFF || this->IsClient() || this->IsPlayerCorpse()) {
		this->helmtexture = in_helmtexture;
	}
	else
		this->texture = 0;
	if ((race == 0 || race > 12) && race != 128 && race != 130) {
		this->haircolor = in_haircolor;
		this->beardcolor = in_beardcolor;
		this->eyecolor1 = in_eyecolor1;
		this->eyecolor2 = in_eyecolor2;
		this->hairstyle = in_hairstyle;
		this->luclinface = in_luclinface;
// vesuvias - appearence fix
		this->beard = in_beard;

		this->aa_title = in_aa_title;
	}
	else {
		this->hairstyle = 0xFF;
		this->beardcolor = 0xFF;
		this->eyecolor1 = 0xFF;
		this->eyecolor2 = 0xFF;
		this->hairstyle = 0xFF;
		this->luclinface = 0xFF;
// vesuvias - appearence fix
		this->beard	= 0xFF;

		this->aa_title = 0xFF;
	}
	APPLAYER* outapp = new APPLAYER(OP_Illusion, sizeof(Illusion_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	Illusion_Struct* is = (Illusion_Struct*) outapp->pBuffer;
	strcpy(is->charname, GetCleanName());
	is->race = this->race;
	is->gender = this->gender;
	is->texture = this->texture;
	is->helmtexture = this->helmtexture;
	/*is->haircolor = this->haircolor;
	is->beardcolor = this->beardcolor;
	is->eyecolor1 = this->eyecolor1;
	is->eyecolor2 = this->eyecolor2;
	is->hairstyle = this->hairstyle;
	is->luclinface = this->luclinface;
	is->aa_title = this->aa_title;
	is->unknown_26 = 26;
	is->unknown016 = 0xffffffff;
	*/
	//DumpPacket(outapp);
	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

int8 Mob::GetDefaultGender(int16 in_race, int8 in_gender) {
//cout << "Gender in:  " << (int)in_gender << endl;
	if ((in_race > 0 && in_race <= 12 ) || in_race == 128 || in_race == 130 || in_race == 15 || in_race == 50 || in_race == 57 || in_race == 70 || in_race == 98 || in_race == 118) {
		if (in_gender >= 2) {
			// Female default for PC Races
			return 1;
		}
		else
			return in_gender;
	}
	else if (in_race == 44 || in_race == 52 || in_race == 55 || in_race == 65 || in_race == 67 || in_race == 88 || in_race == 117 || in_race == 127 ||
		in_race == 77 || in_race == 78 || in_race == 81 || in_race == 90 || in_race == 92 || in_race == 93 || in_race == 94 || in_race == 106 || in_race == 112) {
		// Male only races
		return 0;

	}
	else if (in_race == 25 || in_race == 56) {
		// Female only races
		return 1;
	}
	else {
		// Neutral default for NPC Races
		return 2;
	}
}

void Mob::TicProcess() {
	for (int buffs_i=0; buffs_i<BUFF_COUNT; buffs_i++) {
		if (buffs[buffs_i].spellid != 0xFFFF) {
			DoBuffTic(buffs[buffs_i].spellid, buffs[buffs_i].ticsremaining, buffs[buffs_i].casterlevel, entity_list.GetMob(buffs[buffs_i].casterid));
			if (buffs[buffs_i].durationformula != 50) {
				buffs[buffs_i].ticsremaining--;
				if (buffs[buffs_i].ticsremaining <= 0) {
					BuffFadeBySpellID(buffs[buffs_i].spellid);
				}
			}
		}
	}
}

void Mob::SendAppearancePacket(int32 type, int32 value, bool WholeZone, bool iIgnoreSelf) {
	if (!GetID())
		return;
	APPLAYER* outapp = new APPLAYER(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
	SpawnAppearance_Struct* appearance = (SpawnAppearance_Struct*)outapp->pBuffer;
	appearance->spawn_id = this->GetID();
	appearance->type = type;
	appearance->parameter = value;
	if (WholeZone)
		entity_list.QueueClients(this, outapp, iIgnoreSelf);
	else if (this->IsClient())
		this->CastToClient()->QueuePacket(outapp);
	safe_delete(outapp);
}

const sint32& Mob::SetMana(sint32 amount)
{
	CalcMaxMana();
	cur_mana = amount < 0 ? 0 : (amount > GetMaxMana() ? GetMaxMana() : amount);
/*
	if(IsClient())
		LogFile->write(EQEMuLog::Debug, "Setting mana for %s to %d (%4.1f%%)", GetName(), amount, GetManaRatio());
*/

	return cur_mana;
}


void Mob::SetAppearance(int8 app, bool iIgnoreSelf) {
	if (appearance != app) {
		appearance = app;
		SendAppearancePacket(AT_Anim, GetAppearanceValue(app), true, iIgnoreSelf);
		if (this->IsClient() && this->IsAIControlled())
			SendAppearancePacket(AT_Anim, ANIM_FREEZE, false, false);
	}
}

void Mob::ChangeSize(float in_size = 0, bool bNoRestriction) {
	//Neotokyo's Size Code
	if (!bNoRestriction)
	{
		if (this->IsClient() || this->petid != 0)
			if (in_size < 3.0)
				in_size = 3.0;


			if (this->IsClient() || this->petid != 0)
				if (in_size > 15.0)
					in_size = 15.0;
	}


	if (in_size < 1.0)
		in_size = 1.0;

	if (in_size > 255.0)
		in_size = 255.0;
	//End of Neotokyo's Size Code
	this->size = in_size;
	SendAppearancePacket(AT_Size, (int32) in_size);
}

Mob* Mob::GetFamiliar() {
	Mob* tmp = entity_list.GetMob(this->GetFamiliarID());

	if (tmp) {
		if (tmp->GetOwnerID() == this->GetID()) {
			return tmp;
		}
		else {
			this->SetFamiliarID(0);
			return 0;
		}
	}
	return 0;
}

Mob* Mob::GetPet() {
	Mob* tmp = entity_list.GetMob(this->GetPetID());

	if (tmp) {
		if (tmp->GetOwnerID() == this->GetID()) {
			return tmp;
		}
		else {
			this->SetPetID(0);
			return 0;
		}
	}
	return 0;
}

void Mob::SetPet(Mob* newpet) {
	Mob* oldpet = GetPet();
	if (oldpet) {
		oldpet->SetOwnerID(0);
	}
	if (newpet == 0) {
		SetPetID(0);
	}
	else {
		SetPetID(newpet->GetID());
		Mob* oldowner = entity_list.GetMob(newpet->GetOwnerID());
		if (oldowner)
			oldowner->SetPetID(0);
		newpet->SetOwnerID(this->GetID());
	}
}

void Mob::SetPetID(int16 NewPetID) {
	if (NewPetID == GetID() && NewPetID != 0)
		return;
	petid = NewPetID;
}

void Mob::SetFamiliarID(int16 NewPetID) {
	if (NewPetID == GetID() && NewPetID != 0)
		return;
	familiarid = NewPetID;
}



Mob* Mob::GetOwnerOrSelf() {
	if (!GetOwnerID())
		return this;
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (!owner) {
		SetOwnerID(0);
	}
	else if (owner->GetPetID() == this->GetID()) {
		return owner;
	}
    else if (owner->GetFamiliarID() == this->GetID()) {
        return owner;
    }
	else {
		SetOwnerID(0);
	}
	return this;
}

Mob* Mob::GetOwner() {
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (owner && owner->GetPetID() == this->GetID()) {

		return owner;
	}
    if (owner && owner->GetFamiliarID() == this->GetID())
    {
        return owner;
    }
	this->SetOwnerID(0);
	return 0;
}

void Mob::SetOwnerID(int16 NewOwnerID) {
	if (NewOwnerID == GetID() && NewOwnerID != 0) // ok, no charming yourself now =p
		return;
	ownerid = NewOwnerID;
	if (ownerid == 0 && this->IsNPC() && this->GetPetType() != 0xFF)
		this->Depop();
}

//heko: for backstab
bool Mob::BehindMob(Mob* other, float playerx, float playery) {
    if (!other)
        return true; // sure your behind your invisible friend?? (fall thru for sneak)
	//see if player is behind mob
	float angle, lengthb, vectorx, vectory;
	float mobx = -(other->GetX());	// mob xlocation (inverse because eq is confused)
	float moby = other->GetY();		// mobylocation
	float heading = (int8)other->GetHeading();	// mob heading
	heading = (heading * 360.0)/256.0;	// convert to degrees
	if (heading < 270)
		heading += 90;
	else
		heading -= 270;
	heading = heading*3.1415/180.0;	// convert to radians
	vectorx = mobx + (10.0 * cosf(heading));	// create a vector based on heading
	vectory = moby + (10.0 * sinf(heading));	// of mob length 10

	//length of mob to player vector
	//lengthb = (float)sqrt(pow((-playerx-mobx),2) + pow((playery-moby),2));
	lengthb = (float) sqrt( ( (-playerx-mobx) * (-playerx-mobx) ) + ( (playery-moby) * (playery-moby) ) );

	// calculate dot product to get angle
	angle = acosf(((vectorx-mobx)*(-playerx-mobx)+(vectory-moby)*(playery-moby)) / (10 * lengthb));
	angle = angle * 180 / 3.1415;
	if (angle > 90.0) //not sure what value to use (90*2=180 degrees is front)
		return true;
	else
		return false;
}

void Mob::SetZone(int32 zone_id)
{
	if(IsClient())
		CastToClient()->GetPP().zone_id = zone_id;
}

void Mob::Kill() {
	Death(this, 0, 0xffff, 0x04);
}

void Mob::SetAttackTimer() {
	float PermaHaste;
	if (GetHaste() == -100)
		PermaHaste = 10.0f;   // 10x as slow as normal for 100% slowed mobs
	else
		PermaHaste = 100.0f / (100.0f + GetHaste()); //PercentageHaste);  	// use #haste to set haste level
	
	Timer* TimerToUse = NULL;
	for (int i=SLOT_PRIMARY; i<=SLOT_SECONDARY; i++) {
		ItemCommonInst ItemToUse;
		
		if (i==13)
			TimerToUse = attack_timer;
		else
			TimerToUse = attack_timer_dw;
		
		if (IsClient()) {
			if (CastToClient()->GetInv().GetItem(i))
				ItemToUse.SetItem(CastToClient()->GetInv().GetItem(i)->GetItem());
		}
    	else if (IsNPC())
			ItemToUse.SetItem(database.GetItem(CastToNPC()->equipment[i]));
		
		if ((i==SLOT_SECONDARY) && IsClient()) {
			int8 tmp = this->CastToClient()->GetSkill(DUAL_WIELD);
			if ((tmp == 0) || (tmp > 252) || !ItemToUse.IsWeapon() ) {
				if ( !ItemToUse.IsWeapon() && !CanThisClassDualWield() ) {
//					if(EQDEBUG>=1 && CastToClient()->GetGM())
//						Message(0, "Debug Mode: Dual wield disabled (secondary weapon is not a weapon)");
					attack_timer_dw->Disable();
					break;
				}
				else if (ItemToUse.IsType(ItemTypeCommon)) {
					const Item_Struct* item = ItemToUse.GetItem();
					if (item->Common.Skill != 45) {
//						if(EQDEBUG>=1 && CastToClient()->GetGM())
//							Message(0, "Debug Mode: Dual wield disabled (secondary weapon is not a hand to hand weapon)");
						attack_timer_dw->Disable();
						break;
					}
				}
				else {
					const ItemInst* MainWeapon = CastToClient()->GetInv().GetItem(SLOT_PRIMARY);
					if (MainWeapon && MainWeapon->IsWeapon()) {
						const Item_Struct* item = MainWeapon->GetItem();
						if ((item->Common.Skill == 1) || (item->Common.Skill == 4) || (item->Common.Skill == 35)) {
//							if (EQDEBUG >=1 && CastToClient()->GetGM())
//								Message(0, "Debug Mode: Dual wield disabled (Primary weapon is a two handed weapon)");
							attack_timer_dw->Disable();
							break;
						}
					}
				}
			}
		}
		
//		if ((EQDEBUG>=1) && IsClient() && CastToClient()->GetGM())
//			Message(0, "Debug mode: Dual wield enabled (GM Only)");
		
		if (!ItemToUse.IsType(ItemTypeCommon)) {
			// Work out if we're a monk
			if ((GetClass() == MONK) || (GetClass() == BEASTLORD)) {
				int speed = (int)(GetMonkHandToHandDelay()*100.0f*PermaHaste);
				// neotokyo: 1200 seemed too much, with delay 10 weapons available
            	if(speed < 500)
					speed = 500;
				TimerToUse->SetAtTrigger(speed, true);	// Hand to hand, delay based on level or epic
			}
			else {
				int speed = (int)(3600*PermaHaste);
				if(speed < 1800 && this->IsClient())
					speed = 1800;
				TimerToUse->SetAtTrigger(speed, true); 	// Hand to hand, non-monk 2/36
			}
		}
		else {
			const Item_Struct* item = ItemToUse.GetItem();
			if ((item->Common.Skill > 4) && (item->Common.Skill != 45) && (item->Common.Skill != 23)) { // Check skill is valid
				// item info is invalid, but ignore that for npcs
				if ((i == 13) || (GetLevel() >= 13)) // make npcs auto have dual wield at lvl 13
					TimerToUse->SetAtTrigger(2000, true);
				else
					TimerToUse->Disable();		// Disable timer if primary item uses a non-weapon skill
        	}
			else {
				int speed = (int)(item->Common.Delay*(100.0f*PermaHaste));
				if(speed < 500)
					speed = 500;
				TimerToUse->SetAtTrigger(speed, true);	// Convert weapon delay to timer resolution (milliseconds)
			}
		}
	}
}

bool Mob::CanThisClassDualWield(void) //Dual wield not Duel, busy someone else fix it (fixed! bUsh)
{
	// All npcs over level 13 can dual wield
	if (this->IsNPC() && (this->GetLevel() >= 13))
		return true;
	
	// Kaiyodo - Check the classes that can DW, and make sure we're not using a 2 hander
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
	case ROGUE:
		{
			if(GetLevel() < 13)
				return false;
			break;
		}
	case BARD:
	case RANGER:
	case BEASTLORD:
		{
			if(GetLevel() < 17)
				return false;
			break;
		}
	case MONK:
		{
			break;
		}
	default:
		{
			return false;
		}
	}
	
	if (IsClient()) {
		const ItemInst* inst = CastToClient()->GetInv().GetItem(SLOT_PRIMARY);
		// 2HS, 2HB or 2HP
		if (inst && inst->IsType(ItemTypeCommon)) {
			const Item_Struct* item = inst->GetItem();
			if ((item->Common.Skill == 0x01) || (item->Common.Skill == 0x23) || (item->Common.Skill == 0x04))
				return false;
		}
		
		return (this->CastToClient()->GetSkill(DUAL_WIELD) != 0);	// No skill = no chance
	}
	else
		return false;
}

bool Mob::CanThisClassDoubleAttack(void)
{
    // All npcs over level 26 can double attack
    if (this->IsNPC() && this->GetLevel() >= 26)
        return true;
	// Kaiyodo - Check the classes that can DA
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
	case MONK:
		{
			if(GetLevel() < 15)
				return false;
			break;
		}
	case ROGUE:
		{
			if(GetLevel() < 16)
				return false;
			break;
		}
	case RANGER:
	case PALADIN:
	case SHADOWKNIGHT:
		{
			if(GetLevel() < 20)
				return false;
			break;
		}
	default:
		{
			return false;
		}
	}

	if (this->IsClient())
		return(this->CastToClient()->GetSkill(DOUBLE_ATTACK) != 0);	// No skill = no chance
	else
		return false;
}

bool Mob::IsWarriorClass(void)
{
	switch(GetClass())
	{
	case WARRIOR:
	case WARRIORGM:
	case ROGUE:
	case ROGUEGM:
	case MONK:
	case MONKGM:
	case PALADIN:
	case PALADINGM:
	case SHADOWKNIGHT:
	case SHADOWKNIGHTGM:
	case RANGER:
	case RANGERGM:
	case BEASTLORD:
	case BEASTLORDGM:
	case BARD:
	case BARDGM:
		{
			return true;
		}
	default:
		{
			return false;
		}
	}

}

bool Mob::CanThisClassParry(void)
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
		{
		if(GetLevel() < 10)
			return false;
		break;
		}
	case ROGUE:
		{
		if(GetLevel() < 12)
			return false;
		break;
		}
	case BARD:
		{
		if(GetLevel() < 53)
			return false;
		break;
		}
	case RANGER:
		{
		if(GetLevel() < 18)
			return false;
		break;
		}
	case SHADOWKNIGHT:
	case PALADIN:
		{
		if(GetLevel() < 17)
			return false;
		break;
		}
	default:
		{
			return false;
		}
	}

	if (this->IsClient())
		return(this->CastToClient()->GetSkill(PARRY) != 0);	// No skill = no chance
	else
		return false;
}

bool Mob::CanThisClassDodge(void)
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
		{
			if(GetLevel() < 6)
				return false;
			break;
		}
	case MONK:
		{
			break;
		}
	case ROGUE:
		{
			if(GetLevel() < 4)
				return false;
			break;
		}
	case RANGER:
		{
			if(GetLevel() < 8)
				return false;
			break;
		}
	case BARD:
	case BEASTLORD:
	case SHADOWKNIGHT:
	case PALADIN:
		{
			if(GetLevel() < 10)
				return false;
			break;
		}
	case CLERIC:
	case SHAMAN:
	case DRUID:
		{
			if( GetLevel() < 15 )
				return false;
			break;
		}
	case NECROMANCER:
	case ENCHANTER:
	case WIZARD:
	case MAGICIAN:
		{
			if( GetLevel() < 22 )
				return false;
			break;
		}
	default:
		{
			return false;
		}
	}
	
	if (this->IsClient())
		return(this->CastToClient()->GetSkill(DODGE) != 0);	// No skill = no chance
	else
		return false;
}

bool Mob::CanThisClassRiposte(void) //Could just check if they have the skill?
{
	// Trumpcard
	switch(GetClass()) // Lets make sure they are the right level! -image
	{
	case WARRIOR:
		{
			if(GetLevel() < 25)
				return false;
			break;
		}
	case ROGUE:
	case RANGER:
	case SHADOWKNIGHT:
	case PALADIN:
		{
			if(GetLevel() < 30)
				return false;
			break;
		}
	case MONK:
		{
			if(GetLevel() < 35)
				return false;
			break;
		}
	case BEASTLORD:
		{
			if(GetLevel() < 40)
				return false;
			break;
		}
	case BARD:
		{
			if(GetLevel() < 58)
				return false;
			break;
		}
	default:
		{
			return false;
		}
	}
	
	if (this->IsClient())
		return(this->CastToClient()->GetSkill(RIPOSTE) != 0);	// No skill = no chance
	else
		return false;
}

int8 Mob::GetClassLevelFactor(){
	int8 multiplier = 0;
	int8 mlevel=GetLevel();
	switch(GetClass())
	{
		case WARRIOR:{
			if (mlevel < 20)
				multiplier = 22;
			else if (mlevel < 30)
				multiplier = 23;
			else if (mlevel < 40)
				multiplier = 25;
			else if (mlevel < 53)
				multiplier = 27;
			else if (mlevel < 57)
				multiplier = 28;
			else
				multiplier = 30;
			break;
		}
		case DRUID:
		case CLERIC:
		case SHAMAN:{
			multiplier = 15;
			break;
		}
		case PALADIN:
		case SHADOWKNIGHT:{
			if (mlevel < 35)
				multiplier = 21;
			else if (mlevel < 45)
				multiplier = 22;
			else if (mlevel < 51)
				multiplier = 23;
			else if (mlevel < 56)
				multiplier = 24;
			else if (mlevel < 60)
				multiplier = 25;
			else
				multiplier = 26;
			break;
		}
		case MONK:
		case BARD:
		case ROGUE:
		case BEASTLORD:{
			if (mlevel < 51)
				multiplier = 18;
			else if (mlevel < 58)
				multiplier = 19;
			else
				multiplier = 20;
			break;
		}
		case RANGER:{
			if (mlevel < 58)
				multiplier = 20;
			else
				multiplier = 21;
			break;
		}
		case MAGICIAN:
		case WIZARD:
		case NECROMANCER:
		case ENCHANTER:{
			multiplier = 12;
			break;
		}
		default:{
			//cerr << "Unknown/invalid class in Client::CalcBaseHP" << endl;
			if (mlevel < 35)
				multiplier = 21;
			else if (mlevel < 45)
				multiplier = 22;
			else if (mlevel < 51)
				multiplier = 23;
			else if (mlevel < 56)
				multiplier = 24;
			else if (mlevel < 60)
				multiplier = 25;
			else
				multiplier = 26;
			break;
		}
	}
	return multiplier;
}

/*
solar: returns false if attack should not be allowed
I try to list every type of conflict that's possible here, so it's easy
to see how the decision is made.  Yea, it could be condensed and made
faster, but I'm doing it this way to make it readable and easy to modify
*/

///////////////////////////////////////////////////////////////////////////////
//////////// real one is below this function, this is all ifdef'd GW //////////
///////////////////////////////////////////////////////////////////////////////
#ifdef GUILDWARS
bool Mob::IsAttackAllowed(Mob *target)
{
	if(!target)
        return 0;
	if(target->IsNPC() && target->CastToNPC()->GetOwner() != 0 && this == target->CastToNPC()->GetOwner())
		return false; // People killing charmed pets for XP
	if(target->IsClient() && target->CastToClient()->GetOwner() != 0 && this == target->CastToClient()->GetOwner())
		return false; // People killing charmed pets for XP

	/* self */
#ifdef GUILDWARS
	if((target->IsClient() && target->CastToClient()->GetGM()) || this == target) {
		return false; // GMs cant get attacked

	}
#else
	if(this == target) {
		return true; // Quag: sure, you can attack yourself, why not?
	}
#endif
	if (this->IsCorpse() || target->IsCorpse())
		return false;

//Begin of GuildWars rules
#ifndef GUILDWARS
goto normalrules;
#endif

#ifdef GUILDWARS

if(guildwars.GetPVPAbility() == 1)
return false;
else if(guildwars.GetPVPAbility() == 2 && ((_CLIENT(this) && _CLIENT(target)) || (_CLIENT(this) && _CLIENTPET(target)) || (_CLIENTPET(this) && _CLIENT(target)) || (_CLIENTPET(this) && _CLIENTPET(target))))
return false;
else if(guildwars.GetPVPAbility() == 3)
return guildwars.SpecialAttackPrivs(this,target);
else if(target->IsClient() && target->CastToClient()->GuildDBID() == 0 && IsClient() && target->CastToClient()->GuildDBID() == 0)
goto normalrules;
#endif

#ifdef GUILDWARS
	if(_CLIENT(this))
	{
		if(_CLIENT(target))
		{
		if(CastToClient()->GuildDBID() != target->CastToClient()->GuildDBID())
			return true;
		}
		else if(_CLIENTPET(target))
		{
		if(CastToClient()->GuildDBID() != target->GetOwner()->CastToClient()->GuildDBID())
			return true;
		}
	}
	else if(_CLIENTPET(this)) {
		if(_CLIENT(target))
		{
		if(GetOwner()->CastToClient()->GuildDBID() != target->CastToClient()->GuildDBID())
			return true;
		}
		else if(_CLIENTPET(target)) {
		if(GetOwner()->CastToClient()->GuildDBID() != target->GetOwner()->CastToClient()->GuildDBID())
			return true;
		}
	}
#endif
//End of Guildwars rules

normalrules:
	if(_CLIENT(this))
	{
		if(_CLIENT(target))
		{
			if(this->CastToClient()->GetPVP() != target->CastToClient()->GetPVP())
				return false;
			else if(this->CastToClient()->GetPVP() && target->CastToClient()->GetPVP())
				return true;
			else if(this->CastToClient()->GetDuelTarget() == target->GetID() && target->CastToClient()->GetDuelTarget() == GetID() && target->CastToClient()->IsDueling() && this->CastToClient()->IsDueling())
				return true;
		}
		else if(_NPC(target))
		{
			#ifdef IPC
			if(target->CastToNPC()->IsInteractive() && (!target->CastToNPC()->IsPVP() || !this->CastToClient()->GetPVP()))
				return false;
			#endif
				return true;
		}
		else if(_BECOMENPC(target))
		{
			if(this->CastToClient()->GetLevel() > target->CastToClient()->GetBecomeNPCLevel())
				return false;
			else
				return true;
		}
		else if(_CLIENTPET(target))
		{
			if(this->CastToClient()->GetDuelTarget() == target->CastToMob()->GetOwner()->GetID() && target->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == GetID() && target->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToClient()->IsDueling())
				return true;
			else if(this->CastToClient()->GetPVP() != target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return false;
			else if(this->CastToClient()->GetPVP() && target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return true;
		}
		else if(_NPCPET(target))
		{
			return true;
		}
		else if(_BECOMENPCPET(target))
		{
			if(this->CastToClient()->GetLevel() > target->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return false;
			else
				return true;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_NPC(this))
	{
		if(_CLIENT(target))
		{
			#ifdef IPC
			if(CastToNPC()->IsInteractive() && (!CastToNPC()->IsPVP() || !target->CastToClient()->GetPVP()))
				return false;
			#endif
			return true;
		}
		else if(_NPC(target))
		{
			if(target->CastToNPC()->GetPrimaryFaction() != 0 && target->CastToNPC()->GetPrimaryFaction() == CastToNPC()->GetPrimaryFaction())
				return false;
			if(CastToNPC()->GetPrimaryFaction() != 0 && target->CastToNPC()->GetPrimaryFaction() != 0 && CastToNPC()->GetPrimaryFaction() == target->CastToNPC()->GetPrimaryFaction() || CastToNPC()->IsFactionListAlly(target->CastToNPC()->GetPrimaryFaction()))
				return false;


			return 1;
		}
		else if(_BECOMENPC(target))
		{

			return 1;
		}
		else if(_CLIENTPET(target))
		{
			return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_BECOMENPC(this))
	{
		if(_CLIENT(target))
		{
			if(target->CastToClient()->GetLevel() > this->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target)) {
			if(target->CastToMob()->GetOwner()->CastToClient()->GetLevel() > this->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPCPET(target)) {
			return 1;
		}
		else if(_BECOMENPCPET(target)) {
			return 1;
		}
		else {
			goto nocase;
		}
	}
	else if(_CLIENTPET(this)) {
		if(_CLIENT(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == target->GetID() && target->CastToClient()->IsDueling() && target->CastToClient()->GetDuelTarget() == this->CastToMob()->GetOwner()->GetID())
				return 1;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() != target->CastToClient()->GetPVP())
				return 0;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() && target->CastToClient()->GetPVP())
				return 1;
		}
		else if(_NPC(target)) {
			return 1;

		}
		else if(_BECOMENPC(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->GetLevel() > target->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_CLIENTPET(target)) {
			if(this->CastToMob()->GetOwner()->CastToClient()->IsDueling() && this->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == target->CastToMob()->GetOwner()->GetID() && target->CastToMob()->GetOwner()->CastToClient()->IsDueling() && target->CastToMob()->GetOwner()->CastToClient()->GetDuelTarget() == this->CastToMob()->GetOwner()->GetID())
				return 1;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() != target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return 0;
			else if(this->CastToMob()->GetOwner()->CastToClient()->GetPVP() && target->CastToMob()->GetOwner()->CastToClient()->GetPVP())
				return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			if(this->CastToMob()->GetOwner()->CastToClient()->GetLevel() > target->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else
		{
			goto nocase;
		}
	}
	else if(_NPCPET(this))
	{
		if(_CLIENT(target))
		{
			return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target))
		{
			return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;

		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}



		else
		{
			goto nocase;
		}
	}
	else if(_BECOMENPCPET(this))
	{
		if(_CLIENT(target))
		{
			if(target->CastToClient()->GetLevel() > this->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPC(target))
		{
			return 1;
		}
		else if(_BECOMENPC(target))
		{
			return 1;
		}
		else if(_CLIENTPET(target))
		{
			if(target->CastToMob()->GetOwner()->CastToClient()->GetLevel() > this->CastToMob()->GetOwner()->CastToClient()->GetBecomeNPCLevel())
				return 0;
			else
				return 1;
		}
		else if(_NPCPET(target))
		{
			return 1;
		}
		else if(_BECOMENPCPET(target))
		{
			return 1;
		}
		else
		{
			goto nocase;
		}
	}

nocase:
	LogFile->write(EQEMuLog::Debug, "Mob::IsAttackAllowed: don't have a rule for this - %s vs %s\n", this->GetName(), target->GetName());
	return 0;
}

#else // not guildwars

bool Mob::IsAttackAllowed(Mob *target)
{
	Mob *mob1, *mob2, *tempmob;
	Client *c1, *c2, *becomenpc;
	NPC *npc1, *npc2;
	int reverse;

	// some special cases
	if(!target)
		return false;

	if(this == target)	// you can attack yourself
		return true;

	// can't damage own pet (applies to everthing)
	if(target->GetOwner() && target->GetOwner() == this)
		return false;
	else if(GetOwner() && GetOwner() == target)
		return false;

#ifdef SHAWN319
	if
	(
	(IsClient() && CastToClient()->GetGM()) ||
	(target->IsClient() && target->CastToClient()->GetGM())
	)
	return false;
#endif
	// solar: the format here is a matrix of mob type vs mob type.
	// redundant ones are omitted and the reverse is tried if it falls through.

	
	// first figure out if we're pets.  we always look at the master's flags.
	// no need to compare pets to anything
	mob1 = this->GetOwner() ? this->GetOwner() : this;
	mob2 = target->GetOwner() ? target->GetOwner() : target;

	reverse = 0;
	do
	{
		if(_CLIENT(mob1))
		{
			if(_CLIENT(mob2))					// client vs client
			{
				c1 = mob1->CastToClient();
				c2 = mob2->CastToClient();
	
				if	// if both are pvp they can fight
				(
					c1->GetPVP() &&
					c2->GetPVP()
				)
					return true;
				else if	// if they're dueling they can go at it
				(
					c1->IsDueling() &&
					c2->IsDueling() &&
					c1->GetDuelTarget() == c2->GetID() &&
					c2->GetDuelTarget() == c1->GetID()
				)
					return true;
				else
					return false;
			}
			else if(_NPC(mob2))				// client vs npc
			{	
				return true;
			}
			else if(_BECOMENPC(mob2))	// client vs becomenpc
			{
				c1 = mob1->CastToClient();
				becomenpc = mob2->CastToClient();
	
				if(c1->GetLevel() > becomenpc->GetBecomeNPCLevel())
					return false;
				else
					return true;
			}	
			else if(_CLIENTCORPSE(mob2))	// client vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client vs npc corpse
			{
				return false;
			}
		}
		else if(_NPC(mob1))
		{
			if(_NPC(mob2))						// npc vs npc
			{
				npc1 = mob1->CastToNPC();
				npc2 = mob2->CastToNPC();
				if
				(
					npc1->GetPrimaryFaction() != 0 &&
					npc2->GetPrimaryFaction() != 0 &&
					(
						npc1->GetPrimaryFaction() == npc2->GetPrimaryFaction() ||
						npc1->IsFactionListAlly(npc2->GetPrimaryFaction())
					)
				)
					return false;
				else
					return true;
			}
			else if(_BECOMENPC(mob2))	// npc vs becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// npc vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// npc vs npc corpse
			{
				return false;
			}
		}
		else if(_BECOMENPC(mob1))
		{
			if(_BECOMENPC(mob2))			// becomenpc vs becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// becomenpc vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// becomenpc vs npc corpse
			{
				return false;
			}
		}
		else if(_CLIENTCORPSE(mob1))
		{
			if(_CLIENTCORPSE(mob2))		// client corpse vs client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client corpse vs npc corpse
			{
				return false;
			}
		}
		else if(_NPCCORPSE(mob1))
		{
			if(_NPCCORPSE(mob2))			// npc corpse vs npc corpse
			{
				return false;
			}
		}

		// we fell through, now we swap the 2 mobs and run through again once more
		tempmob = mob1;
		mob1 = mob2;
		mob2 = tempmob;
	}
	while( reverse++ == 0 );

	LogFile->write(EQEMuLog::Debug, "Mob::IsAttackAllowed: don't have a rule for this - %s vs %s\n", this->GetName(), target->GetName());
	return false;
}

#endif // not guildwars

// solar: this is to check if non detrimental things are allowed to be done
// to the target.  clients cannot affect npcs and vice versa, and clients
// cannot affect other clients that are not of the same pvp flag as them.
// also goes for their pets
bool Mob::IsBeneficialAllowed(Mob *target)
{
	Mob *mob1, *mob2, *tempmob;
	Client *c1, *c2;
	int reverse;

	if(!target)
		return false;

	// solar: see IsAttackAllowed for notes
	
	// first figure out if we're pets.  we always look at the master's flags.
	// no need to compare pets to anything
	mob1 = this->GetOwner() ? this->GetOwner() : this;
	mob2 = target->GetOwner() ? target->GetOwner() : target;

	// if it's self target or our own pet it's ok
	if(mob1 == mob2)
		return true;

	reverse = 0;
	do
	{
		if(_CLIENT(mob1))
		{
			if(_CLIENT(mob2))					// client to client
			{
				c1 = mob1->CastToClient();
				c2 = mob2->CastToClient();

				if(c1->GetPVP() == c2->GetPVP())
					return true;
				else if	// if they're dueling they can heal each other too
				(
					c1->IsDueling() &&
					c2->IsDueling() &&
					c1->GetDuelTarget() == c2->GetID() &&
					c2->GetDuelTarget() == c1->GetID()
				)
					return true;
				else
					return false;
			}
			else if(_NPC(mob2))				// client to npc
			{
			}
			else if(_BECOMENPC(mob2))	// client to becomenpc
			{
				return false;
			}
			else if(_CLIENTCORPSE(mob2))	// client to client corpse
			{
				return true;
			}
			else if(_NPCCORPSE(mob2))	// client to npc corpse
			{
				return false;
			}
		}
		else if(_NPC(mob1))
		{
#ifdef GUILDWARS
			if(_CLIENT(mob2))
			{
				return true;
			}
			else if(_NPC(mob2))						// npc to npc
#else
			if(_NPC(mob2))						// npc to npc
#endif
			{
				return true;
			}
			else if(_BECOMENPC(mob2))	// npc to becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// npc to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// npc to npc corpse
			{
				return false;
			}
		}
		else if(_BECOMENPC(mob1))
		{
			if(_BECOMENPC(mob2))			// becomenpc to becomenpc
			{
				return true;
			}
			else if(_CLIENTCORPSE(mob2))	// becomenpc to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// becomenpc to npc corpse
			{
				return false;
			}
		}
		else if(_CLIENTCORPSE(mob1))
		{
			if(_CLIENTCORPSE(mob2))		// client corpse to client corpse
			{
				return false;
			}
			else if(_NPCCORPSE(mob2))	// client corpse to npc corpse
			{
				return false;
			}
		}
		else if(_NPCCORPSE(mob1))
		{
			if(_NPCCORPSE(mob2))			// npc corpse to npc corpse
			{
				return false;
			}
		}

		// we fell through, now we swap the 2 mobs and run through again once more
		tempmob = mob1;
		mob1 = mob2;
		mob2 = tempmob;
	}
	while( reverse++ == 0 );

	LogFile->write(EQEMuLog::Debug, "Mob::IsBeneficialAllowed: don't have a rule for this - %s to %s\n", this->GetName(), target->GetName());
	return false;
}

bool Mob::CombatRange(Mob* other)
{
    // neotokyo: some mobs have set size == -1; this caused a signed/unsigned overflow
	sint32 size_mod = (sint32)GetSize();
	if(GetRace() == 49 || GetRace() == 158 || GetRace() == 196) //For races with a fixed size
 		size_mod = 60;
	else if (size_mod < 6)
		size_mod = 8;
	if (other->GetSize() > size_mod)
		size_mod = (sint32)other->GetSize();
	if (DistNoZ(*other) <= size_mod*2)
		return true;
	return false;
}

float Mob::Dist(const Mob &other) {
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	float zDiff = other.z_pos - z_pos;

	return sqrt( (xDiff * xDiff) 
	           + (yDiff * yDiff) 
		       + (zDiff * zDiff) );
}

float Mob::DistNoZ(const Mob &other) {
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	
	return sqrt( (xDiff * xDiff) 
		       + (yDiff * yDiff) );
}

float Mob::DistNoRoot(const Mob &other) {
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	float zDiff = other.z_pos - z_pos;

	return ( (xDiff * xDiff)  
	       + (yDiff * yDiff)  
	       + (zDiff * zDiff) );
}

float Mob::DistNoRootNoZ(const Mob &other) {
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;

	return ( (xDiff * xDiff) + (yDiff * yDiff) );
}

bool Mob::HateSummon() {
    // check if mob has ability to summon
    // we need to be hurt and level 51+ or ability checked to continue
// Sandy - fix so not automatic summon
	if (GetHPRatio() >= 95 || SpecAttacks[SPECATK_SUMMON] == false)
        return false;

    // now validate the timer
    if (!SpecAttackTimers[SPECATK_SUMMON])
    {
        SpecAttackTimers[SPECATK_SUMMON] = new Timer(6000);
        SpecAttackTimers[SPECATK_SUMMON]->Start();
    }

    // now check the timer
    if (!SpecAttackTimers[SPECATK_SUMMON]->Check())
        return false;

    // get summon target
    target = GetHateTop();
    if( target)
    {
        if (target->IsClient())
	        target->CastToClient()->Message(15,"You have been summoned!");
		entity_list.MessageClose(this, true, 500, 10, "%s says,'You will not evade me, %s!' ", GetName(), GetHateTop()->GetName() );
		GetHateTop()->GMMove(x_pos, y_pos, z_pos, target->GetHeading());
		return true;
	}
	return false;
}

void Mob::FaceTarget(Mob* MobToFace, bool update) {
	if (MobToFace == 0)
		MobToFace = target;
	if (MobToFace == 0 || MobToFace == this)
		return;
	// TODO: Simplify?
	float oldheading = heading;
	heading=(CalculateHeadingToTarget(MobToFace->GetX(),MobToFace->GetY()));
//	APPLAYER* outapp = new APPLAYER(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
//	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)outapp->pBuffer;
//	MakeSpawnUpdate(spu);
//	entity_list.QueueCloseClients(this, outapp, true, 300);
//	safe_delete(outapp);
	/*float angle;
	
	if (MobToFace->GetX()-x_pos > 0)
		angle = - 90 + atan((double)(MobToFace->GetY()-y_pos) / (double)(MobToFace->GetX()-x_pos)) * 180 / M_PI;
	else if (MobToFace->GetX()-x_pos < 0)
		angle = + 90 + atan((double)(MobToFace->GetY()-y_pos) / (double)(MobToFace->GetX()-x_pos)) * 180 / M_PI;
	else // Added?
	{
		if (MobToFace->GetY()-y_pos > 0)
			angle = 0;
		else
			angle = 180;
	}
	//cout << "dX:" << MobToFace->GetX()-x_pos;
	//cout << "dY:" << MobToFace->GetY()-y_pos;
	//cout << "Angle:" << angle;
	
	if (angle < 0)
		angle += 360;
	if (angle > 360)
		angle -= 360;
	

	float oldheading = heading;
	heading = (sint8) (256*(360-angle)/360.0f);
	//	return angle;
	
	//cout << "Heading:" << (int)heading << endl;
	*/
	SendPosUpdate();
	if (update && oldheading != heading)
		pLastChange = Timer::GetCurrentTime();
}

bool Mob::RemoveFromHateList(Mob* mob) {
	SetRunAnimSpeed(0);
    bool bFound = false;
	if (this->IsEngaged())
	{
		bFound = hate_list.RemoveEnt(mob);	
		if  (!this->IsEngaged()){
			if (bFound)
				AI_Event_NoLongerEngaged();
			zone->DelAggroMob();
			//			cout << "Mobs currently Aggro: " << zone->MobsAggroCount() << endl; 
		}
	}
	if (target == mob)
		SetTarget(hate_list.GetTop());
    return bFound;
}
void Mob::WhipeHateList() {
	if (this->IsEngaged()) {
		AI_Event_NoLongerEngaged();
	}
	hate_list.Wipe();
}

// we need this for charmed NPCs
void Mob::SaveSpawnSpot() {
    spawn_x = x_pos;
    spawn_y = y_pos;
    spawn_z = z_pos;
    spawn_heading = heading;
}

void Mob::SaveGuardSpot(bool iClearGuardSpot) {
	if (iClearGuardSpot) {
		guard_x = 0;
		guard_y = 0;
		guard_z = 0;
		guard_heading = 0;

	}
	else {
		guard_x = x_pos;
		guard_y = y_pos;
		guard_z = z_pos;
		guard_heading = heading;
	}
}

void Mob::NPCSpecialAttacks(const char* parse, int permtag) {
    for(int i = 0; i < SPECATK_MAXNUM; i++)
	{
	    SpecAttacks[i] = false;
        SpecAttackTimers[i] = NULL;
    }

    while (*parse)
    {
        switch(*parse)
        {
        case 'Z':
//			if (this->IsNPC())
 //   			this->CastToNPC()->interactive = true;
            break;
        case 'X':
   // 		if (this->IsNPC())
    //			this->CastToNPC()->citycontroller = true;
            break;
        case 'Y':
    //		if (this->IsNPC())
    //			this->CastToNPC()->guildbank = true;
            break;
	    case 'E':
    	    SpecAttacks[SPECATK_ENRAGE] = true;
    		break;
	    case 'F':
    	    SpecAttacks[SPECATK_FLURRY] = true;
    		break;
/*		case 'G':
			{
	CastToNPC()->d_meele_texture1= 199;
	CastToNPC()->equipment[7]=199;
	CastToNPC()->rangerstance = true;
	CastToNPC()->pArrgoRange = 200;
	CastToNPC()->pAssistRange = 200;
	break;
			}*/
	    case 'R':
    	    SpecAttacks[SPECATK_RAMPAGE] = true;
    		break;

	    case 'S':
    	    SpecAttacks[SPECATK_SUMMON] = true;
            SpecAttackTimers[SPECATK_SUMMON] = new Timer(6000);
            SpecAttackTimers[SPECATK_SUMMON]->Start();
    		break;
	    case 'T':
            SpecAttacks[SPECATK_TRIPLE] = true;
            break;
		case 'U':
			SpecAttacks[UNSLOWABLE] = true;
			break;
		case 'M':
			SpecAttacks[UNMEZABLE] = true;
			break;
		case 'C':
			SpecAttacks[UNCHARMABLE] = true;
			break;
		case 'N':
			SpecAttacks[UNSTUNABLE] = true;
			break;
	    case 'Q':
            SpecAttacks[SPECATK_QUAD] = true;
            break;
        default:
            break;
        }
        parse++;
    }
	
	if(permtag == 1 && this->GetNPCTypeID() > 0){
		if(database.SetSpecialAttkFlag(this->GetNPCTypeID(),parse)) {
			LogFile->write(EQEMuLog::Normal, "NPCTypeID: %i flagged to '%s' for Special Attacks.\n",this->GetNPCTypeID(),parse);
		}
	}
}

int32 Mob::RandomTimer(int min,int max) {
    int r = 14000;
	if(min != 0 && max != 0 && min < max)
	{
	    r = (rand()  % (max - min)) + min;
	}
	return r;
}

// ##########################################
// Trade implementation
// ##########################################

Trade::Trade(Mob* in_owner)
{
	owner = in_owner;
	Reset();
}

Trade::~Trade()
{
	Reset();
}

void Trade::Reset()
{
	state = TradeNone;
	with_id = 0;
	pp=0; gp=0; sp=0; cp=0;
}

void Trade::SetTradeCash(uint32 in_pp, uint32 in_gp, uint32 in_sp, uint32 in_cp)
{
	pp=in_pp; gp=in_gp; sp=in_sp; cp=in_cp;
}

// Initiate a trade with another mob
// initiate_with specifies whether to start trade with other mob as well
void Trade::Start(uint32 mob_id, bool initiate_with)
{
	Reset();
	state = Trading;
	with_id = mob_id;
	
	// Autostart on other mob?
	if (initiate_with) {
		Mob* with = With();
		if (with)
			with->trade->Start(owner->GetID(), false);
	}
}

// Add item from a given slot to trade bucket (automatically does bag data too)
void Trade::AddEntity(int16 from_slot_id, int16 trade_slot_id)
{
	if (!owner || !owner->IsClient()) {
		// This should never happen
		LogFile->write(EQEMuLog::Debug, "Programming error: NPC's should not call Trade::AddEntity()");
		return;
	}
	
	// If one party accepted the trade then an item was added, their state needs to be reset
	owner->trade->state = Trading;
	Mob* with = With();
	if (with)
		with->trade->state = Trading;
	
	// Item always goes into trade bucket from cursor
	Client* client = owner->CastToClient();
	const ItemInst* inst = client->GetInv().GetItem(SLOT_CURSOR);
	if (!inst) {
		client->Message(13, "Error: Could not find item on your cursor!");
		return;
	}
	
	#if (EQDEBUG >= 9)
		LogFile->write(EQEMuLog::Debug, "%s added item '%s' to trade slot %i", owner->GetName(), item->Name, trade_slot_id);
	#endif
	
	// Send all item data to other client
	SendItemData(inst, trade_slot_id);
	
	// Move item on cursor to the trade slots
	client->PutItemInInventory(trade_slot_id, *inst);
	//ItemCommonInst trade_inst;
	client->DeleteItemInInventory(from_slot_id); //, (ItemInst&)trade_inst);
}

// Retrieve mob the owner is trading with
// Done like this in case 'with' mob goes LD and Mob* becomes invalid
Mob* Trade::With()
{
	return entity_list.GetMob(with_id);
}

// Private Method: Send item data for trade item to other person involved in trade
void Trade::SendItemData(const ItemInst* inst, sint16 dest_slot_id)
{
	// @merth: This needs to be redone with new item classes
	Mob* mob = With();
	if (!mob->IsClient())
		return; // Not sending packets to NPCs!
	
	Client* with = mob->CastToClient();
	Client* trader = owner->CastToClient();
	if (with && with->IsClient()) {
		with->SendItemPacket(dest_slot_id -IDX_TRADE,inst,ItemPacketTradeView);
		if (inst->GetItem()->ItemClass == 1) {
			for (int16 i=0; i<10; i++) {
				int16 bagslot_id = Inventory::CalcSlotId(dest_slot_id, i);
				const ItemInst* bagitem = trader->GetInv().GetItem(bagslot_id);
				if (bagitem) {
					with->SendItemPacket(bagslot_id-IDX_TRADE,bagitem,ItemPacketTradeView);
				}
			}
		}
		
		//safe_delete(outapp);
	}
}

// Audit trade: The part logged is what travels owner -> with
void Trade::LogTrade()
{
	Mob* with = With();
	if (!owner->IsClient() || !with)
		return; // Should never happen
	
	Client* trader = owner->CastToClient();
	bool logtrade = false;
	int admin_level = 0;
	uint8 item_count = 0;
	
	if (zone->tradevar != 0) {
		for (int16 i=3000; i<=3007; i++) {
			if (trader->GetInv().GetItem(i))
				item_count++;
		}
		
		if (((this->cp + this->sp + this->gp + this->pp)>0) || (item_count>0))
			admin_level = trader->Admin();
		else
			admin_level = 999;
		
		if (zone->tradevar == 7) {
			logtrade = true;
		}
		else if ((admin_level>=10) && (admin_level<20)) {
			if ((zone->tradevar<8) && (zone->tradevar>5))
				logtrade = true;
		}
		else if (admin_level<=20) {
			if ((zone->tradevar<8) && (zone->tradevar>4))
				logtrade = true;
		}
		else if (admin_level<=80) {
			if ((zone->tradevar<8) && (zone->tradevar>3))
				logtrade = true;
		}
		else if (admin_level<=100){
			if ((zone->tradevar<9) && (zone->tradevar>2))
				logtrade = true;
		}
		else if (admin_level<=150){
			if (((zone->tradevar<8) && (zone->tradevar>1)) || (zone->tradevar==9))
				logtrade = true;
		}
		else if (admin_level<=255){
			if ((zone->tradevar<8) && (zone->tradevar>0))
				logtrade = true;	
		}
	}
	
	if (logtrade == true) {
		char logtext[1000] = {0};
		uint32 cash = 0;
		bool comma = false;
		
		// Log items offered by owner
		cash = this->cp + this->sp + this->gp + this->pp;
		if ((cash>0) || (item_count>0)) {
			sprintf(logtext, "%s gave %s ", trader->GetName(), with->GetName());
			
			if (item_count > 0) {
				strcat(logtext, "items {");
				
				for (int16 i=3000; i<=3007; i++) {
					const ItemInst* inst = trader->GetInv().GetItem(i);
					
					if (!comma)
						comma = true;
					else {
						if (inst)
							strcat(logtext, ",");
					}
					
					if (inst) {
						char item_num[15] = {0};
						sprintf(item_num, "%i", inst->GetItem()->ItemNumber);
						strcat(logtext, item_num);
						
						if (inst->IsType(ItemTypeContainer)) {
							for (uint8 j=0; j<10; j++) {
								inst = trader->GetInv().GetItem(i, j);
								if (inst) {
									strcat(logtext, ",");
									sprintf(item_num, "%i", inst->GetItem()->ItemNumber);
									strcat(logtext, item_num);
								}
							}
						}
					}
				}
			}
			
			if (cash > 0) {	
				char money[100] = {0};
				sprintf(money, " %ipp, %igp, %isp, %icp", trader->trade->pp, trader->trade->gp, trader->trade->sp, trader->trade->cp);
				strcat(logtext, money);
			}
			
			database.logevents(trader->AccountName(), trader->AccountID(),
				trader->Admin(), trader->GetName(), with->GetName(), "Trade", logtext, 6);
		}
	}
}

#if (EQDEBUG >= 9)
void Trade::DumpTrade()
{
	Mob* with = With();
	LogFile->write(EQEMuLog::Debug, "Dumping trade data: '%s' in TradeState %i with '%s'",
		this->owner->GetName(), state, ((with==NULL)?"(null)":with->GetName()));
	
	if (!owner->IsClient())
		return;
	
	Client* trader = owner->CastToClient();
	for (int16 i=3000; i<=3007; i++) {
		const ItemInst* inst = trader->GetInv().GetItem(i);
		
		if (inst) {
			LogFile->write(EQEMuLog::Debug, "Item %i (Charges=%i, Slot=%i, IsBag=%s)",
				inst->GetItem()->ItemNumber, inst->GetCharges(),
				i, ((inst->IsType(ItemTypeContainer)) ? "True" : "False"));
			
			if (inst.IsType(ItemTypeContainer)) {
				for (uint8 j=0; j<10; j++) {
					inst = trader->GetInv().GetItem(i, j);
					if (inst) {
						LogFile->write(EQEMuLog::Debug, "\tBagItem %i (Charges=%i, Slot=%i)",
							inst->GetItem()->ItemNumber, inst->GetCharges(),
							Inventory::CalcBagSlotId(i, j));
					}
				}
			}
		}
	}
	
	LogFile->write(EQEMuLog::Debug, "\tpp:%i, gp:%i, sp:%i, cp:%i", pp, gp, sp, cp);
}
#endif

bool Mob::CheckLos(Mob* other) {
	if (zone->map == 0)
	{
		return true;
	}
	float tmp_x = GetX();
	float tmp_y = GetY();
	float tmp_z = GetZ();
	float trg_x = other->GetX();
	float trg_y = other->GetY();
	float trg_z = other->GetZ();
	float perwalk_x = 0.5;
	float perwalk_y = 0.5;
	float perwalk_z = 0.5;
	float dist_x = tmp_x - trg_x;
	if (dist_x < 0)
		dist_x *= -1;
	float dist_y = tmp_y - trg_y;
	if (dist_y < 0)
		dist_y *= -1;
	float dist_z = tmp_z - trg_z;
	if (dist_z < 0)
		dist_z *= -1;
	if (dist_x  < dist_y && dist_z < dist_y)
	{
		perwalk_x /= (dist_y/dist_x);
		perwalk_z /= (dist_y/dist_z);
	}
	else if (dist_y  < dist_x && dist_z < dist_x)
	{
		perwalk_y /= (dist_x/dist_y);
		perwalk_z /= (dist_x/dist_z);
	}
	else if (dist_x  < dist_z && dist_y < dist_z)
	{
		perwalk_x /= (dist_z/dist_x);
		perwalk_y /= (dist_z/dist_y);
	}
	float steps = (dist_x/perwalk_x + dist_y/perwalk_y + dist_z/perwalk_z)*10; //Just a safety check to prevent endless loops.
	while (steps > 0) {
		steps--;
		if (tmp_x < trg_x)
		{
			if (tmp_x + perwalk_x < trg_x)
				tmp_x += perwalk_x;
			else
				tmp_x = trg_x;
		}
		if (tmp_y < trg_y)
		{
			if (tmp_y + perwalk_y < trg_y)
				tmp_y += perwalk_y;
			else
				tmp_y = trg_y;
		}
		if (tmp_z < trg_z)
		{
			if (tmp_z + perwalk_z < trg_z)
				tmp_z += perwalk_z;
			else
				tmp_z = trg_z;
		}
		if (tmp_x > trg_x)
		{
			if (tmp_x - perwalk_x > trg_x)
				tmp_x -= perwalk_x;
			else
				tmp_x = trg_x;
		}
		if (tmp_y > trg_y)
		{
			if (tmp_y - perwalk_y > trg_y)
				tmp_y -= perwalk_y;
			else
				tmp_y = trg_y;
		}
		if (tmp_z > trg_z)
		{
			if (tmp_z - perwalk_z > trg_z)
				tmp_z -= perwalk_z;
			else
				tmp_z = trg_z;
		}
		if (tmp_y == trg_y && tmp_x == trg_x && tmp_z == trg_z)
		{
			return true;
		}

//I believe this is contributing to breaking mob spawns when a map is loaded
//		NodeRef pnode = zone->map->SeekNode( zone->map->GetRoot(), tmp_x, tmp_y );
		NodeRef pnode = NODE_NONE;
		if (pnode != NODE_NONE)
		{
			int *iface = zone->map->SeekFace( pnode, tmp_x, tmp_y );
			if (*iface == -1) {
				return false;
			}
			float temp_z = 0;
			float best_z = 999999;
			while(*iface != -1)
			{
				temp_z = zone->map->GetFaceHeight( *iface, x_pos, y_pos );
				float best_dist = sqrt((double)(pow(best_z-tmp_z, 2)));
				float tmp_dist = sqrt((double)(pow(tmp_z-tmp_z, 2)));
				if (tmp_dist < best_dist)
				{
					best_z = temp_z;
				}
				iface++;
			}
/*	solar: our aggro code isn't using this right now, just spells, so i'm
    taking out the +-10 check for now to make it work right on hills
			if (best_z - 10 > trg_z || best_z + 10 < trg_z)
			{
				return false;
			}
*/
		}
	}
	return true;
}


//Father Nitwit's LOS code
bool Mob::CheckLosFN(Mob* other) {
	if(other == NULL || zone->map == NULL) {
		return(false);	//not sure what the best return is on error
	}
	
	VERTEX myloc;
	VERTEX oloc;
	
#define LOS_DEFAULT_HEIGHT 6.0f
	
	myloc.x = GetX();
	myloc.y = GetY();
	myloc.z = GetZ() + (GetSize()==0.0?LOS_DEFAULT_HEIGHT:GetSize())/2 * HEAD_POSITION;
	
	oloc.x = other->GetX();
	oloc.y = other->GetY();
	oloc.z = other->GetZ() + (other->GetSize()==0.0?LOS_DEFAULT_HEIGHT:other->GetSize())/2 * SEE_POSITION;

#if EQDEBUG>=5
	LogFile->write(EQEMuLog::Debug, "LOS from (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f) sizes: (%.2f, %.2f)", myloc.x, myloc.y, myloc.z, oloc.x, oloc.y, oloc.z, GetSize(), other->GetSize());
#endif
	
	FACE *onhit;
	NodeRef mynode;
	NodeRef onode;
	
	VERTEX hit;
	//see if anything in our node is in the way
	mynode = zone->map->SeekNode( zone->map->GetRoot(), myloc.x, myloc.y);
	if(mynode != NODE_NONE) {
		if(zone->map->LineIntersectsNode(mynode, myloc, oloc, &hit, &onhit)) {
#if EQDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see.", GetName(), other->GetName() );
			LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				onhit->a.x, onhit->a.y, onhit->a.z,
				onhit->b.x, onhit->b.y, onhit->b.z, 
				onhit->c.x, onhit->c.y, onhit->c.z);
#endif
			return(false);
		}
	}
#if EQDEBUG>=5
	 else {
		LogFile->write(EQEMuLog::Debug, "WTF, I have no node, what am I standing on??? (%.2f, %.2f).", myloc.x, myloc.y);
	}
#endif
	
	//see if they are in a different node.
	//if so, see if anything in their node is blocking me.
	if(! zone->map->LocWithinNode(mynode, oloc.x, oloc.y)) {
		onode = zone->map->SeekNode( zone->map->GetRoot(), oloc.x, oloc.y);
		if(onode != NODE_NONE && onode != mynode) {
			if(zone->map->LineIntersectsNode(onode, myloc, oloc, &hit, &onhit)) {
#if EQDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see (2).", GetName(), other->GetName());
			LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				onhit->a.x, onhit->a.y, onhit->a.z,
				onhit->b.x, onhit->b.y, onhit->b.z, 
				onhit->c.x, onhit->c.y, onhit->c.z);
#endif
				return(false);
			}
		}
#if EQDEBUG>=5
		 else if(onode == NODE_NONE) {
			LogFile->write(EQEMuLog::Debug, "WTF, They have no node, what are they standing on??? (%.2f, %.2f).", myloc.x, myloc.y);
		}
#endif
	}
	
	/*
	if(zone->map->LineIntersectsZone(myloc, oloc, CHECK_LOS_STEP, &onhit)) {
#if EQDEBUG>=5
		LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, cannot see.", GetName(), other->GetName() );
		LogFile->write(EQEMuLog::Debug, "\tPoly: (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
			onhit->a.x, onhit->a.y, onhit->a.z,
			onhit->b.x, onhit->b.y, onhit->b.z, 
			onhit->c.x, onhit->c.y, onhit->c.z);
#endif
		return(false);
	}*/
	
#if EQDEBUG>=5
			LogFile->write(EQEMuLog::Debug, "Check LOS for %s target %s, CAN SEE.", GetName(), other->GetName());
#endif
	
	return(true);
}


sint32 Mob::GetEquipment(int8 material_slot)
{
	if(material_slot > 8)
		return -1;

	return equipment[material_slot];
}

void Mob::SendWearChange(int8 material_slot)
{
	APPLAYER* outapp = new APPLAYER(OP_WearChange, sizeof(WearChange_Struct));
	WearChange_Struct* wc = (WearChange_Struct*)outapp->pBuffer;

	wc->spawn_id = GetID();
	wc->material = GetEquipmentMaterial(material_slot);
	wc->color.color = GetEquipmentColor(material_slot);
	wc->wear_slot_id = material_slot;

	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

sint32 Mob::GetEquipmentMaterial(int8 material_slot)
{
	const Item_Struct *item;
	
	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		if	// for primary and secondary we need the model, not the material
		(
			material_slot == MATERIAL_PRIMARY ||
			material_slot == MATERIAL_SECONDARY
		)
		{
			if(strlen(item->IDFile) > 2)
				return atoi(&item->IDFile[2]);
		}
		else
		{
			return item->Common.Material;
		}
	}

	return 0;
}

sint32 Mob::GetEquipmentColor(int8 material_slot)
{
	const Item_Struct *item;
	
	item = database.GetItem(GetEquipment(material_slot));
	if(item != 0)
	{
		return item->Common.Color;
	}

	return 0;
}

//
// solar: works just like a printf
//
void Mob::Say(const char *format, ...)
{
	char buf[1000];
	va_list ap;
	
	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);
	
	entity_list.MessageClose_StringID(this, false, 200, 10,
		GENERIC_SAY, GetCleanName(), buf);
}

//
// solar: this is like the above, but the first parameter is a string id
//
void Mob::Say_StringID(int32 string_id, const char *message3, const char *message4, const char *message5, const char *message6, const char *message7, const char *message8, const char *message9)
{
	char string_id_str[10];
	
	snprintf(string_id_str, 10, "%d", string_id);

	entity_list.MessageClose_StringID(this, false, 200, 10,
		GENERIC_STRINGID_SAY, GetCleanName(), string_id_str, message3, message4, message5,
		message6, message7, message8, message9
	);
}

void Mob::Shout(const char *format, ...)
{
	char buf[1000];
	va_list ap;
	
	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);
	
	entity_list.Message_StringID(this, false, MT_Shout,
		GENERIC_SHOUT, GetCleanName(), buf);
}

void Mob::Emote(const char *format, ...)
{
	char buf[1000];
	va_list ap;
	
	va_start(ap, format);
	vsnprintf(buf, 1000, format, ap);
	va_end(ap);
	
	entity_list.MessageClose_StringID(this, false, 200, 10,
		GENERIC_EMOTE, GetCleanName(), buf);
}

const char *Mob::GetCleanName()
{
	if(!strlen(clean_name))
	{
		CleanMobName(GetName(), clean_name);
	}

	return clean_name;
}

// hp event 
void Mob::SetNextHPEvent( int hpevent ) 
{ 
	nexthpevent = hpevent; 
	if ( nexthpevent < 0 ) 
	{ 
		nexthpevent = 0; 
	} 
}
//warp for quest function,from sandy
void Mob::Warp( float x, float y, float z ) 
{ 
   x_pos = x; 
   y_pos = y; 
   z_pos = z; 

   Mob* target = GetTarget(); 
   if ( target ) { 
      FaceTarget( target, true ); 
   } 

   SendPosition(); 

}

bool Mob::DivineAura()
{
	for (int l = 0; l < 30; l++)
	{
		if (buffs[l].spellid < 0xFFFF && buffs[l].spellid != 0xFFFF)
		{
			for (int k = 0; k < 12; k++)
			{
				if (spells[buffs[l].spellid].effectid[k] == SE_DivineAura)
				{
					return true;
				}
			}
		}
	}
	return false;
}

