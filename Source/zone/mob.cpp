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
#include "features.h"
#include <math.h>
#include <stdlib.h>
#include <stdarg.h>
#include "masterentity.h"
#include "pathing.h"
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

#include "../common/emu_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "../common/database.h"
#include "../common/packet_dump.h"
#include "../common/packet_functions.h"
#include "../common/bodytypes.h"
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
         bodyType in_bodytype,   // neotokyo targettype support 17-Nov-02
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
         const int32*   in_equipment,
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

		 ) : 
		attack_timer(2000),
		attack_dw_timer(2000),
		ranged_timer(2000),
		tic_timer(6000),
		mana_timer(2000),
		spellend_timer(0),
		stunned_timer(0),
		bardsong_timer(6000),
#ifdef FLEE_HP_RATIO
		flee_timer(FLEE_CHECK_TIMER),
#endif
		bindwound_timer(10000)
	//	mezzed_timer(0)
{
	targeted = false;
	logpos = false;
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
	signaled = false;
	signal_id = 0;

	_egnode = NULL;
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
	attack_speed= 0;
	findable	= false;

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
	
	ExtraHaste = 0;
	bEnraged = false;

	shield_target = NULL;
	cur_mana = 0;
	max_mana = 0;
	hp_regen = 0;
	mana_regen = 0;
	invisible = false;
	invisible_undead = false;
	sneaking = false;
	invulnerable = false;
	qglobal=0;

	int i = 0;

	for (i=0; i < MAX_MATERIALS; i++)
	{
		if (in_equipment == NULL)
		{
			equipment[i] = 0;
		}
		else
		{
			equipment[i] = in_equipment[i];
		}
	}
/*
	I dont think this is right, many places use the
	equipment array as an item id.
	if(in_d_meele_texture1)
		equipment[MATERIAL_PRIMARY] = in_d_meele_texture1;
	if(in_d_meele_texture2)
		equipment[MATERIAL_SECONDARY] = in_d_meele_texture2;
*/

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
	int j;
	for (j = 0; j < BUFF_COUNT; j++) {
		buffs[j].spellid = SPELL_UNKNOWN;
	}

    // clear the proc array
    RemoveProcFromWeapon(0, true);
	for (j = 0; j < MAX_PROCS; j++)
    {
        PermaProcs[j].spellID = 0xFFFF;
        PermaProcs[j].chance = 0;
        PermaProcs[j].pTimer = NULL;
    }

	delta_heading = 0;
	delta_x = 0;
	delta_y = 0;
	delta_z = 0;

	isgrouped = false;
	appearance = 0;
	pRunAnimSpeed = 0;
	guildeqid = GUILD_NONE;
	
    spellend_timer.Disable();
	
	bardsong_timer.Disable();
	bardsong = 0;
	bardsong_target = NULL;
	casting_spell_id = 0;
	target = 0;
	
	memset(&itembonuses, 0, sizeof(StatBonuses));
	memset(&spellbonuses, 0, sizeof(StatBonuses));
	spellbonuses.AggroRange = -1;
	spellbonuses.AssistRange = -1;
	pLastChange = 0;
	SetPetID(0);
	SetOwnerID(0);
	typeofpet = 0xFF;  // means is not a pet; 0xFF means charmed
    SetFamiliarID(0);
	SaveSpawnSpot();

	isattacked = false;
	attacked_count = 0;
	mezzed = false;
	stunned = false;
	rune = 0;
    magicrune = 0;
    int m;
	for (m = 0; m < 60; m++) {
		flag[m]=0;
	}
	for (m = 0; m < MAX_SHIELDERS; m++)
	{
		shielder[m].shielder_id = 0;
		shielder[m].shielder_bonus = 0;
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
#ifdef ENABLE_FEAR_PATHING
	fear_state = fearStateNotFeared;
	fear_path_state = NULL;
	flee_mode = false;
#ifdef FLEE_HP_RATIO
	flee_timer.Start();
#endif
#endif
	permarooted = ( walkspeed == 0 ) && ( runspeed == 0 );

	movetimercompleted = false;
	roamer = false;
	rooted = false;
	charmed = false;
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
	bindwound_timer.Disable();
	bindwound_target = 0;
	
	trade = new Trade(this);
	// hp event
	nexthpevent = 0;
	
	fix_pathing = false;
}

Mob::~Mob()
{
	AI_Stop();
	if (GetPet()) {
		if (GetPet()->Charmed())
			GetPet()->BuffFadeByEffect(SE_Charm);
		else
			SetPet(0);
	}
	for (int i=0; i<SPECATK_MAXNUM ; i++) {
		safe_delete(SpecAttackTimers[i]);
	}
	EQApplicationPacket app;
	CreateDespawnPacket(&app);
	Corpse* corpse = entity_list.GetCorpseByID(GetID());
	if(!corpse || (corpse && !corpse->IsPlayerCorpse()))
		entity_list.QueueClients(this, &app, true);
	
	entity_list.RemoveFromTargets(this);
	
	safe_delete(trade);
#ifdef ENABLE_FEAR_PATHING
	safe_delete(fear_path_state);
#endif
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
	
	if (other && other->IsNPC() && other->CastToNPC()->IsAnimal() && !other->SeeInvisible()) {
		int l;
		for (l = 0; l < BUFF_COUNT; l++) {
			if(buffs[l].spellid > (uint32)SPDAT_RECORDS) {
				buffs[l].spellid = SPELL_UNKNOWN;
			}
			if (buffs[l].spellid != SPELL_UNKNOWN) {
				for (int k = 0; k < 12; k++) {
					if (spells[buffs[l].spellid].effectid[k] == SE_InvisVsAnimals || spells[buffs[l].spellid].effectid[k] == SE_Invisibility) {
						return true;
					}
				}
			}
		}
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
    if (IsRooted())
        return 0.0f;
    float aa_speed = 1.0f;
    if (IsClient()){
        int32 aa_item = CastToClient()->GetAA(aaInnateRunSpeed);
        if (aa_item > 0 && aa_item < 4){
            aa_speed += aa_item/100;
        }
        //partial implementation of Fleet of Foot
        aa_item = CastToClient()->GetAA(aaFleetofFoot);
        if (aa_item > 0 && aa_item < 4){
            aa_speed += aa_item * 4/100;
        }
        
    }
    if (spellbonuses.movementspeed || itembonuses.movementspeed)
        aa_speed += (spellbonuses.movementspeed+itembonuses.movementspeed) / 100.0f;

	return (walkspeed * aa_speed);
}

float Mob::GetRunspeed()
{
    float aa_speed = 1.0f;
    if (IsClient()){
        int32 aa_item = CastToClient()->GetAA(aaInnateRunSpeed);
        if (aa_item > 0 && aa_item < 4){
            aa_speed += aa_item/100;
        }
        //partial implementation of Fleet of Foot
        aa_item = CastToClient()->GetAA(aaFleetofFoot);
        if (aa_item > 0 && aa_item < 4){
            aa_speed += aa_item * 4/100;
        }
        
    }
    if (IsRooted())
        return 0.0f;
    if (spellbonuses.movementspeed || itembonuses.movementspeed)
        aa_speed += (spellbonuses.movementspeed+itembonuses.movementspeed) / 100.0f;

        return (runspeed * aa_speed);
}

sint32 Mob::CalcMaxMana()
{
	switch (GetCasterClass()) {
		case 'I':
			max_mana = (((GetINT()/5)+2) * GetLevel()) + spellbonuses.Mana + itembonuses.Mana;
			break;
		case 'W':
			max_mana = (((GetWIS()/5)+2) * GetLevel()) + spellbonuses.Mana + itembonuses.Mana;
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

void Mob::CreateSpawnPacket(EQApplicationPacket* app, Mob* ForWho) {
	app->SetOpcode(OP_NewSpawn);
	app->size = sizeof(NewSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);	
	NewSpawn_Struct* ns = (NewSpawn_Struct*)app->pBuffer;
	FillSpawnStruct(ns, ForWho);
}

void Mob::CreateSpawnPacket(EQApplicationPacket* app, NewSpawn_Struct* ns) {
	app->SetOpcode(OP_NewSpawn);
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
	else if (ns->spawn.class_==ADVENTURERECRUITER)
		strcpy(ns2->spawn.last_name, "Adventure Recruiter");
	else if (ns->spawn.class_==BANKER)
		strcpy(ns2->spawn.last_name, "EQEmu Banker");
	else if (ns->spawn.class_==ADVENTUREMERCHANT)
#ifdef GUILDWARS
		strcpy(ns->spawn.last_name,"GuildWars Merchant");
#else
		strcpy(ns->spawn.last_name,"Adventure Merchant");
#endif
	else if (ns->spawn.class_==WARRIORGM)
		strcpy(ns2->spawn.last_name, "GM Warrior");
	else if (ns->spawn.class_==PALADINGM)
		strcpy(ns2->spawn.last_name, "GM Paladin");
	else if (ns->spawn.class_==RANGERGM)
		strcpy(ns2->spawn.last_name, "GM Ranger");
	else if (ns->spawn.class_==SHADOWKNIGHTGM)
		strcpy(ns2->spawn.last_name, "GM ShadowKnight");
	else if (ns->spawn.class_==DRUIDGM)
		strcpy(ns2->spawn.last_name, "GM Druid");
	else if (ns->spawn.class_==BARDGM)
		strcpy(ns2->spawn.last_name, "GM Bard");
	else if (ns->spawn.class_==ROGUEGM)
		strcpy(ns2->spawn.last_name, "GM Rogue");
	else if (ns->spawn.class_==SHAMANGM)
		strcpy(ns2->spawn.last_name, "GM Shaman");
	else if (ns->spawn.class_==NECROMANCERGM)
		strcpy(ns2->spawn.last_name, "GM Necromancer");
	else if (ns->spawn.class_==WIZARDGM)
		strcpy(ns2->spawn.last_name, "GM Wizard");
	else if (ns->spawn.class_==MAGICIANGM)
		strcpy(ns2->spawn.last_name, "GM Magician");
	else if (ns->spawn.class_==ENCHANTERGM)
		strcpy(ns2->spawn.last_name, "GM Enchanter");
	else if (ns->spawn.class_==BEASTLORDGM)
		strcpy(ns2->spawn.last_name, "GM Beastlord");
	else if (ns->spawn.class_==BERSERKERGM)
		strcpy(ns2->spawn.last_name, "GM Berserker");
	else
		strcpy(ns2->spawn.last_name, ns->spawn.last_name);
	memset(&app->pBuffer[sizeof(Spawn_Struct)-7],0xFF,7);
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
	ns->spawn.findable	= findable?1:0;
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

	ns->spawn.invis2 = 0xff;//this used to be labeled beard.. if its not FF it will turn
								   //mob invis

	if(helmtexture && helmtexture != 0xFF)
	{
		//ns->spawn.equipment[MATERIAL_HEAD] = helmtexture;
		ns->spawn.helm=helmtexture;
	} else {
		//ns->spawn.equipment[MATERIAL_HEAD] = 0;
		ns->spawn.helm = 0;
	}
	
	ns->spawn.equip_chest2  = texture;

	ns->spawn.guild_rank	= 0xFF;
	ns->spawn.size			= size;
	ns->spawn.bodytype = bodytype;

	//ns->spawn.unknown251[2] = helmtexture;
	//ns->spawn.trap_type	= bodytype;
	//ns->spawn.walkspeed	= walkspeed;
	//ns->spawn.runspeed	= runspeed;
	ns->spawn.last_name[0] = '\0';

	if(IsNPC())
		ns->spawn.aa_title = 0xFF;
	
/*	if (ns->spawn.class_==MERCHANT)
		strcpy(ns->spawn.last_name, "EQEmu Shopkeeper");
	else*/ if (ns->spawn.class_==BANKER)
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

	for(i = 0; i < 7; i++)
	{
		ns->spawn.equipment[i] = GetEquipmentMaterial(i);
		ns->spawn.dye_rgb[i].color = GetEquipmentColor(i);
	}
}

void Mob::CreateDespawnPacket(EQApplicationPacket* app)
{
	app->SetOpcode(OP_DeleteSpawn);
	app->size = sizeof(DeleteSpawn_Struct);
	app->pBuffer = new uchar[app->size];
	memset(app->pBuffer, 0, app->size);
	DeleteSpawn_Struct* ds = (DeleteSpawn_Struct*)app->pBuffer;
	ds->spawn_id = GetID();
}

void Mob::CreateHPPacket(EQApplicationPacket* app)
{ 
	this->IsFullHP=(cur_hp>=max_hp); 
	app->SetOpcode(OP_SendHPTarget); 
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
			parse->Event(EVENT_HP, GetNPCTypeID(), 0, CastToNPC(), NULL);
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
			ds->cur_hp = CastToClient()->GetHP() - itembonuses.HP;
			ds->max_hp = CastToClient()->GetMaxHP() - itembonuses.HP;
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
	EQApplicationPacket hp_app;
	Group *group;
	
	// destructor will free the pBuffer
 	CreateHPPacket(&hp_app);

#ifdef MANAGE_HP_UPDATES
	entity_list.QueueManaged(this, &hp_app, true);
#else
	// send to people who have us targeted
 	entity_list.QueueClientsByTarget(this, &hp_app, false, 0, false);

	// send to group
	if(IsGrouped())
	{
		group = entity_list.GetGroupByMob(this);
		group->SendHPPacketsFrom(this);
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
#endif	//MANAGE_HP_PACKETS

	// send to self - we need the actual hps here
	if(IsClient())
	{
		EQApplicationPacket* hp_app2 = new EQApplicationPacket(OP_HPUpdate,sizeof(SpawnHPUpdate_Struct));
		SpawnHPUpdate_Struct* ds = (SpawnHPUpdate_Struct*)hp_app2->pBuffer; 
		ds->cur_hp = CastToClient()->GetHP() - itembonuses.HP;
		ds->spawn_id = GetID();
		ds->max_hp = CastToClient()->GetMaxHP() - itembonuses.HP;
		CastToClient()->QueuePacket(hp_app2);
		safe_delete(hp_app2);
	}
}

// this one just warps the mob to the current location
void Mob::SendPosition() {
	EQApplicationPacket* app = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)app->pBuffer;	
	MakeSpawnUpdateNoDelta(spu);
//?	spu->heading *= 8;
#ifdef PACKET_UPDATE_MANAGER
	entity_list.QueueManaged(this, app, true);
#else
	entity_list.QueueCloseClients(this, app, true, 800);
#endif
	safe_delete(app);
}

// this one just warps the mob to the current location
void Mob::SendAllPosition() {
	EQApplicationPacket* app = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)app->pBuffer;	
	MakeSpawnUpdateNoDelta(spu);
//?	spu->heading *= 8;
	entity_list.QueueClients(this, app, true);
	safe_delete(app);
}

// this one is for mobs on the move, with deltas - this makes them walk
void Mob::SendPosUpdate(int8 iSendToSelf) {
	EQApplicationPacket* app = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
	PlayerPositionUpdateServer_Struct* spu = (PlayerPositionUpdateServer_Struct*)app->pBuffer;
	MakeSpawnUpdate(spu);
	if (iSendToSelf == 2) {
		if (this->IsClient())
			this->CastToClient()->FastQueuePacket(&app,false);
	}
	else
#ifdef PACKET_UPDATE_MANAGER
		entity_list.QueueManaged(this, app, (iSendToSelf==0),false);
#else
		entity_list.QueueCloseClients(this, app, (iSendToSelf==0), 800, NULL, false);
#endif
	safe_delete(app);
}

// this is for SendPosition()
void Mob::MakeSpawnUpdateNoDelta(PlayerPositionUpdateServer_Struct *spu){
	spu->spawn_id	= GetID();
	spu->y_pos		= FloatToEQ19(x_pos);
	spu->x_pos		= FloatToEQ19(y_pos);
	spu->z_pos		= FloatToEQ19(z_pos);
	spu->delta_x	= FloatToEQ13(0);
	spu->delta_y	= FloatToEQ13(0);
	spu->delta_z	= FloatToEQ13(0);
	spu->heading	= FloatToEQ19(heading);
	spu->animation	= 0;
	spu->delta_heading = FloatToEQ13(0);
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
	spu->delta_heading = FloatToEQ13(delta_heading);
}

void Mob::ShowStats(Client* client) {
	client->Message(0, "Name: %s %s", GetName(), lastname);
	client->Message(0, "  Level: %i  MaxHP: %i  CurHP: %i  AC: %i  Class: %i", GetLevel(), GetMaxHP(), GetHP(), GetAC(), GetClass());
	client->Message(0, "  MaxMana: %i  CurMana: %i  ATK: %i  Size: %1.1f", GetMaxMana(), GetMana(), GetATK(), GetSize());
	client->Message(0, "  STR: %i  STA: %i  DEX: %i  AGI: %i  INT: %i  WIS: %i  CHA: %i", GetSTR(), GetSTA(), GetDEX(), GetAGI(), GetINT(), GetWIS(), GetCHA());
	client->Message(0, "  MR: %i  PR: %i  FR: %i  CR: %i  DR: %i", GetMR(), GetPR(), GetFR(), GetCR(), GetDR());
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
			client->Message(0, "  NPCID: %u  SpawnGroupID: %u LootTable: %u  FactionID: %i  SpellsID: %u MerchantID: %i", this->GetNPCTypeID(),spawngroupid, this->CastToNPC()->GetLoottableID(), this->CastToNPC()->GetNPCFactionID(), this->GetNPCSpellsID(),this->CastToNPC()->MerchantType);
		}
		if (this->IsAIControlled()) {
			client->Message(0, "  AIControlled: AggroRange: %1.0f  AssistRange: %1.0f", this->GetAggroRange(), this->GetAssistRange());
		}
	}
}

void Mob::DoAnim(const int animnum, int type, bool ackreq) {
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_EmoteAnim, sizeof(EmoteAnim_Struct));
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
		if (buffs[i].spellid != SPELL_UNKNOWN) {
			if (buffs[i].durationformula == DF_Permanent)
				client->Message(0, "  %i: %s: Permanent", i, spells[buffs[i].spellid].name);
			else
				client->Message(0, "  %i: %s: %i tics left", i, spells[buffs[i].spellid].name, buffs[i].ticsremaining);

		}
	}
	if (IsClient()){
		client->Message(0, "itembonuses:");
		client->Message(0, "Atk:%i Ac:%i HP(%i):%i Mana:%i", itembonuses.ATK, itembonuses.AC, itembonuses.HPRegen, itembonuses.HP, itembonuses.Mana);
		client->Message(0, "Str:%i Sta:%i Dex:%i Agi:%i Int:%i Wis:%i Cha:%i",
			itembonuses.STR,itembonuses.STA,itembonuses.DEX,itembonuses.AGI,itembonuses.INT,itembonuses.WIS,itembonuses.CHA);
		client->Message(0, "SvMagic:%i SvFire:%i SvCold:%i SvPoison:%i SvDisease:%i",
				itembonuses.MR,itembonuses.FR,itembonuses.CR,itembonuses.PR,itembonuses.DR);
		client->Message(0, "DmgShield:%i Haste:%i", itembonuses.DamageShield, itembonuses.haste );
		client->Message(0, "spellbonuses:");
		client->Message(0, "Atk:%i Ac:%i HP(%i):%i Mana:%i", spellbonuses.ATK, spellbonuses.AC, spellbonuses.HPRegen, spellbonuses.HP, spellbonuses.Mana);
		client->Message(0, "Str:%i Sta:%i Dex:%i Agi:%i Int:%i Wis:%i Cha:%i",
			spellbonuses.STR,spellbonuses.STA,spellbonuses.DEX,spellbonuses.AGI,spellbonuses.INT,spellbonuses.WIS,spellbonuses.CHA);
		client->Message(0, "SvMagic:%i SvFire:%i SvCold:%i SvPoison:%i SvDisease:%i",
				spellbonuses.MR,spellbonuses.FR,spellbonuses.CR,spellbonuses.PR,spellbonuses.DR);
		client->Message(0, "DmgShield:%i Haste:%i", spellbonuses.DamageShield, spellbonuses.haste );
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
	SendAllPosition();
	//SendPosUpdate(1);
#ifdef PACKET_UPDATE_MANAGER
	if(IsClient()) {
		CastToClient()->GetUpdateManager()->FlushQueues();
	}
#endif
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
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_Illusion, sizeof(Illusion_Struct));
	memset(outapp->pBuffer, 0, sizeof(outapp->pBuffer));
	Illusion_Struct* is = (Illusion_Struct*) outapp->pBuffer;
	is->spawnid = this->GetID();
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
	DumpPacket(outapp);
	entity_list.QueueClients(this, outapp);
	safe_delete(outapp);
}

int8 Mob::GetDefaultGender(int16 in_race, int8 in_gender) {
//cout << "Gender in:  " << (int)in_gender << endl;
	if ((in_race > 0 && in_race <= GNOME )
		|| in_race == IKSAR || in_race == VAHSHIR || in_race == FROGLOK
		|| in_race == 15 || in_race == 50 || in_race == 57 || in_race == 70 || in_race == 98 || in_race == 118) {
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
		if (buffs[buffs_i].spellid != SPELL_UNKNOWN) {
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
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_SpawnAppearance, sizeof(SpawnAppearance_Struct));
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
	sint32 mmana = GetMaxMana();
	cur_mana = amount < 0 ? 0 : (amount > mmana ? mmana : amount);
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

Mob* Mob::GetOwnerOrSelf() {
	if (!GetOwnerID())
		return this;
	Mob* owner = entity_list.GetMob(this->GetOwnerID());
	if (!owner) {
		SetOwnerID(0);
		return(this);
	}
	if (owner->GetPetID() == this->GetID()) {
		return owner;
	}
    if (owner->GetFamiliarID() == this->GetID()) {
        return owner;
    }
    if (GetBodyType() == BT_SwarmPet) {		//Dook- swarm pets
		return(owner);
	}
	SetOwnerID(0);
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
    if (GetBodyType() == BT_SwarmPet) {		//Dook- swarm pets
    	return(owner);
   	}
	SetOwnerID(0);
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
	float heading = other->GetHeading();	// mob heading
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
	Save();
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
	
	//default value for attack timer in case they have
	//an invalid weapon equipped:
	attack_timer.SetAtTrigger(4000, true);
	
	Timer* TimerToUse = NULL;
	const Item_Struct* PrimaryWeapon = NULL;
	
	for (int i=SLOT_RANGE; i<=SLOT_SECONDARY; i++) {
		
		//pick a timer
		if (i == SLOT_PRIMARY)
			TimerToUse = &attack_timer;
		else if (i == SLOT_RANGE)
			TimerToUse = &ranged_timer;
		else if(i == SLOT_SECONDARY)
			TimerToUse = &attack_dw_timer;
		else	//invalid slot (hands will always hit this)
			continue;
		
		const Item_Struct* ItemToUse = NULL;
		
		//find our item
		if (IsClient()) {
			ItemInst* ci = CastToClient()->GetInv().GetItem(i);
			if (ci)
				ItemToUse = ci->GetItem();
		} else {
			if(equipment[i] != 0)
				ItemToUse = database.GetItem(equipment[i]);
		}
		
		//special offhand stuff
		if(i == SLOT_SECONDARY) {
			//if we have a 2H weapon in our main hand, no dual
			if(PrimaryWeapon != NULL) {
				if(	PrimaryWeapon->ItemClass == ItemClassCommon
					&& (PrimaryWeapon->Common.ItemType == ItemType2HS
					||	PrimaryWeapon->Common.ItemType == ItemType2HB
					||	PrimaryWeapon->Common.ItemType == ItemType2HPierce)) {
					attack_dw_timer.Disable();
					continue;
				}
			}

			//clients must have the skill to use it...
			if(IsClient()) {
				int8 tmp = GetSkill(DUAL_WIELD);
				
				//if we cant dual weild, skip it
				if (tmp == 0 || tmp > 252 || !CanThisClassDualWield()) {
					attack_dw_timer.Disable();
					continue;
				}
			} else {
				//NPCs get it for free at 13
				if(GetLevel() < 13) {
					attack_dw_timer.Disable();
					continue;
				}
			}
		}
		
		if(i == SLOT_RANGE) {
			if(!ItemToUse) {
				//no item, no timer.
				ranged_timer.Disable();
				continue;
			}
			
			//dont enforce skill requirement, I dont think its required.
			uint8 skill = 0;
			if(ItemToUse->Common.ItemType == ItemTypeBow) {
				skill = ARCHERY;
			} else if(ItemToUse->Common.ItemType == ItemTypeThrowing || ItemToUse->Common.ItemType == ItemTypeThrowingv2) {
				skill = THROWING;
			} else {
				//not a throwing weapon, no timer.
				ranged_timer.Disable();
				continue;
			}
		}
		
		//see if we have a valid weapon
		if(ItemToUse != NULL) {
			//check type and damage/delay
			if(ItemToUse->ItemClass != ItemClassCommon 
				|| ItemToUse->Common.Damage == 0 
				|| ItemToUse->Common.Delay == 0) {
				//no weapon
				ItemToUse = NULL;
			}
			// Check to see if skill is valid
			else if((ItemToUse->Common.ItemType > ItemType2HB) && (ItemToUse->Common.ItemType != ItemTypeHand2Hand) && (ItemToUse->Common.ItemType != ItemType2HPierce)) {
				//no weapon
				ItemToUse = NULL;
			}
		}
		
		//if we have no weapon..
		if (ItemToUse == NULL) {
			// Work out if we're a monk
			if ((GetClass() == MONK) || (GetClass() == BEASTLORD)) {
				//we are a monk, use special delay
				int speed = (int)(GetMonkHandToHandDelay()*(100.0f+attack_speed)*PermaHaste);
				// neotokyo: 1200 seemed too much, with delay 10 weapons available
				if(speed < 500)	//lower bound
					speed = 500;
				TimerToUse->SetAtTrigger(speed, true);	// Hand to hand, delay based on level or epic
			} else {
				//not a monk... using fist, regular delay
				int speed = (int)(36*(100.0f+attack_speed)*PermaHaste);
				if(speed < 1800 && IsClient())	//lower bound
					speed = 1800;
				TimerToUse->SetAtTrigger(speed, true); 	// Hand to hand, non-monk 2/36
			}
		} else {
			//we have a weapon, use its delay
			// Convert weapon delay to timer resolution (milliseconds)
			//delay * 100
			int speed = (int)(ItemToUse->Common.Delay*(100.0f+attack_speed)*PermaHaste);
			if(speed < 500)
				speed = 500;
			TimerToUse->SetAtTrigger(speed, true);
		}
		
		if(i == SLOT_PRIMARY)
			PrimaryWeapon = ItemToUse;
	}
}

bool Mob::CanThisClassDualWield(void) //Dual wield not Duel, busy someone else fix it (fixed! bUsh)
{
	// All npcs over level 13 can dual wield
	if (this->IsNPC() && (this->GetLevel() >= 13))
		return true;
	
	// Kaiyodo - Check the classes that can DW, and make sure we're not using a 2 hander
	bool dh2h = false;
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
		{
			if(GetLevel() < 17)
				return false;
			break;
		}
	case BEASTLORD:
		{
			if(GetLevel() < 17)
				return false;
			dh2h = true;
			break;
		}
	case MONK:
		{
			dh2h = true;
			break;
		}
	default:
		{
			return false;
		}
	}
	
	if (IsClient()) {
		const ItemInst* inst = CastToClient()->GetInv().GetItem(SLOT_PRIMARY);
		// 2HS, 2HB, or 2HP
		if (inst && inst->IsType(ItemClassCommon)) {
			const Item_Struct* item = inst->GetItem();
			if ((item->Common.ItemType == ItemType2HB) || (item->Common.ItemType == ItemType2HS) || (item->Common.ItemType == ItemType2HPierce))
				return false;
		} else {
			//No weapon in hand... using hand-to-hand...
			//only monks and beastlords? can dual weild their fists.
			return(dh2h);
		}
		
		return (this->CastToClient()->GetSkill(DUAL_WIELD) != 0);	// No skill = no chance
	}
	else
		return true;	//if we get here, we are the right class
						//and are at the right level, and are NPC
}

bool Mob::CanThisClassDoubleAttack(void)
{
    // All npcs over level 26 can double attack
    if (IsNPC() && GetLevel() >= 26)
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

	if (IsClient())
		return(CastToClient()->GetSkill(DOUBLE_ATTACK) != 0);	// No skill = no chance
	else
		return true;	//if we get here, we are the right class
						//and are at the right level, and are NPC
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

float Mob::Dist(const Mob &other) {
	_ZP(Mob_Dist);
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	float zDiff = other.z_pos - z_pos;

	return sqrt( (xDiff * xDiff) 
	           + (yDiff * yDiff) 
		       + (zDiff * zDiff) );
}

float Mob::DistNoZ(const Mob &other) {
	_ZP(Mob_DistNoZ);
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	
	return sqrt( (xDiff * xDiff) 
		       + (yDiff * yDiff) );
}

float Mob::DistNoRoot(const Mob &other) {
	_ZP(Mob_DistNoRoot);
	float xDiff = other.x_pos - x_pos;
	float yDiff = other.y_pos - y_pos;
	float zDiff = other.z_pos - z_pos;

	return ( (xDiff * xDiff)  
	       + (yDiff * yDiff)  
	       + (zDiff * zDiff) );
}

float Mob::DistNoRootNoZ(const Mob &other) {
	_ZP(Mob_DistNoRootNoZ);
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

		// RangerDown - GMMove doesn't seem to be working well with players, so use MovePC for them, GMMove for NPC's
		if (target->IsClient())
			target->CastToClient()->MovePC(zone->GetZoneID(),x_pos,y_pos,z_pos,0,true);
		else
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
//	EQApplicationPacket* outapp = new EQApplicationPacket(OP_ClientUpdate, sizeof(PlayerPositionUpdateServer_Struct));
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

int32 Mob::RandomTimer(int min,int max) {
    int r = 14000;
	if(min != 0 && max != 0 && min < max)
	{
	    r = (rand()  % (max - min)) + min;
	}
	return r;
}

sint32 Mob::GetEquipment(int8 material_slot)
{
	if(material_slot > 8)
		return -1;

	return equipment[material_slot];
}

void Mob::SendWearChange(int8 material_slot)
{
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_WearChange, sizeof(WearChange_Struct));
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
	for (int l = 0; l < BUFF_COUNT; l++)
	{
		if (buffs[l].spellid != SPELL_UNKNOWN)
		{
			for (int k = 0; k < EFFECT_COUNT; k++)
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



/*bool Mob::SeeInvisibleUndead()
{
	if (IsNPC() && CastToNPC()->Undead() && CastToNPC()->immunities[0] == 0)
		return false;
	return true;
}

bool Mob::SeeInvisible()
{
	if (IsNPC() && CastToNPC()->immunities[0] > 0)
		return true;
	int l;
	for (l = 0; l < BUFF_COUNT; l++)
	{
		if (buffs[l].spellid != SPELL_UNKNOWN)
		{
			for (int k = 0; k < EFFECT_COUNT; k++)
			{
				if (spells[buffs[l].spellid].effectid[k] == SE_SeeInvis)
				{
					return true;
				}
			}
		}
	}
	return false;
}*/

sint16 Mob::GetResist(int8 type)
{
	if (IsNPC())
	{
		if (type == 1)
			return MR + spellbonuses.MR + itembonuses.MR;
		else if (type == 2)
			return FR + spellbonuses.FR + itembonuses.FR;
		else if (type == 3)
			return CR + spellbonuses.CR + itembonuses.CR;
		else if (type == 4)
			return PR + spellbonuses.PR + itembonuses.PR;
		else if (type == 5)
			return DR + spellbonuses.DR + itembonuses.DR;
	}
	else if (IsClient())
	{
		if (type == 1)
			return CastToClient()->GetMR();
		else if (type == 2)
			return CastToClient()->GetFR();
		else if (type == 3)
			return CastToClient()->GetCR();
		else if (type == 4)
			return CastToClient()->GetPR();
		else if (type == 5)
			return CastToClient()->GetDR();
	}
	return 25;
}

int32 Mob::GetLevelHP(int8 tlevel)
{
	//cout<<"Tlevel: "<<(int)tlevel<<endl;
	int multiplier = 0;
	if (tlevel < 10)
	{
		multiplier = tlevel*20;
	}
	else if (tlevel < 20)
	{
		multiplier = tlevel*25;
	}
	else if (tlevel < 40)
	{
		multiplier = tlevel*tlevel*12*((tlevel*2+60)/100)/10;
	}
	else if (tlevel < 45)
	{
		multiplier = tlevel*tlevel*15*((tlevel*2+60)/100)/10;
	}
	else if (tlevel < 50)
	{
		multiplier = tlevel*tlevel*175*((tlevel*2+60)/100)/100;
	}
	else
	{
		multiplier = tlevel*tlevel*2*((tlevel*2+60)/100)*(1+((tlevel-50)*20/10));
	}
	return multiplier;
}

void Mob::StopSong()
{
	if (IsClient() && (bardsong || IsBardSong(casting_spell_id)))
	{
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ManaChange, sizeof(ManaChange_Struct));
		ManaChange_Struct* manachange = (ManaChange_Struct*)outapp->pBuffer;
		manachange->new_mana = cur_mana;
		if (!bardsong)
			manachange->spell_id = casting_spell_id;
		else
			manachange->spell_id = bardsong;
		manachange->stamina = 6000;
		if (CastToClient()->Hungry())
			manachange->stamina = 0;
		CastToClient()->QueuePacket(outapp);
		delete outapp;
	}
	bardsong = 0;
	bardsong_target = 0;
	bardsong_slot = 0;
	bardsong_timer.Disable();
}

sint32 Mob::GetActSpellCasttime(int16 spell_id, sint32 casttime) {
	if (level >= 60 && casttime > 1000)
	{
		casttime = casttime / 2;
		if (casttime < 1000)
			casttime = 1000;
	} else if (level >= 50 && casttime > 1000) {
		sint32 cast_deduction = (casttime*(level - 49))/5;
		if (cast_deduction > casttime/2)
			casttime /= 2;
		else
			casttime -= cast_deduction;
	}
	return(casttime);
}

void Mob::TryWeaponProc(const ItemInst* weapon_g, Mob *on) {
	if(!weapon_g || !weapon_g->IsType(ItemClassCommon)) {
		TryWeaponProc((const Item_Struct*) NULL, on);
		return;
	}
	const ItemCommonInst* weapon = (const ItemCommonInst *) weapon_g;
	
	//do main procs
	TryWeaponProc(weapon->GetItem(), on);
	
	//we have to calculate these again, oh well
	int ourlevel = GetLevel();
	float ProcChance, ProcBonus;
	GetProcChances(ProcChance, ProcBonus);
	
	//do augment procs
	int r;
	for(r = 0; r < MAX_AUGMENT_SLOTS; r++) {
		const ItemCommonInst* aug_i = weapon->GetAugment(r);
		if(!aug_i)
			continue;
		const Item_Struct* aug = aug_i->GetItem();
		if(!aug)
			continue;
		
		if (IsValidSpell(aug->Common.Proc.Effect) 
			&& (aug->Common.Proc.Type == ET_CombatProc)) {
			if (MakeRandomFloat(0, 1) < ProcChance) {	// 255 dex = 0.084 chance of proc. No idea what this number should be really.
				if(aug->Common.Proc.Level > ourlevel) {
					Mob * own = GetOwner();
					if(own != NULL) {
						own->Message_StringID(13,PROC_PETTOOLOW);
					} else {
						Message_StringID(13,PROC_TOOLOW);
					}
				} else {
					ExecWeaponProc(aug->Common.Proc.Effect, on);
				}
			}
		}
	}
}

//proc chance includes proc bonus
float Mob::GetProcChances(float &ProcBonus, float &ProcChance) {
	int mydex = GetDEX();
	ProcBonus = 0;
	if(IsClient()) {
		//increases based off 1 guys observed results.
		switch(CastToClient()->GetAA(aaWeaponAffinity)) {
			case 1:
				ProcBonus += 0.10f;
				break;
			case 2:
				ProcBonus += 0.15f;
				break;
			case 3:
				ProcBonus += 0.25f;
				break;
		}
	}
	ProcBonus += float(itembonuses.ProcChance + spellbonuses.ProcChance) / 1000.0f;
	
	ProcChance = float(mydex) / 3020.0f + ProcBonus;
	return ProcChance;
}

void Mob::TryWeaponProc(const Item_Struct* weapon, Mob *on) {
	
	int ourlevel = GetLevel();
	float ProcChance, ProcBonus;
	GetProcChances(ProcChance, ProcBonus);
	
	//give weapon a chance to proc first.
	if(weapon != NULL) {
		if (IsValidSpell(weapon->Common.Proc.Effect) && (weapon->Common.Proc.Type == ET_CombatProc)) {
			if (MakeRandomFloat(0, 1) < ProcChance) {	// 255 dex = 0.084 chance of proc. No idea what this number should be really.
				if(weapon->Common.Proc.Level > ourlevel) {
					Mob * own = GetOwner();
					if(own != NULL) {
						own->Message_StringID(13,PROC_PETTOOLOW);
					} else {
						Message_StringID(13,PROC_TOOLOW);
					}
				} else {
					ExecWeaponProc(weapon->Common.Proc.Effect, on);
				}
			}
		}
	}
	
	int ourclass = GetClass();
	
	//now try our proc arrays
	float procmod =  float(GetDEX()) / 100.0f + ProcBonus;
    for(int i = 0; i < MAX_PROCS; i++) {
		if (PermaProcs[i].spellID != SPELL_UNKNOWN) {
			if(MakeRandomInt(0,99) < (PermaProcs[i].chance * procmod)) {
				int spelllevel = spells[PermaProcs[i].spellID].classes[ourclass-1];
				//pets must be high enough to cast the spell..?
				if(GetOwner() != NULL && ourlevel < spelllevel) {
					GetOwner()->Message_StringID(13,PROC_PETTOOLOW);
				} else {
					ExecWeaponProc(PermaProcs[i].spellID, on);
				}
				break;
			}
		}
		if (SpellProcs[i].spellID != SPELL_UNKNOWN) {
			if(MakeRandomInt(0,99) < (SpellProcs[i].chance * procmod)) {
				int spelllevel = spells[SpellProcs[i].spellID].classes[ourclass-1];
				//pets must be high enough to cast the spell..?
				if(GetOwner() != NULL && ourlevel < spelllevel) {
					GetOwner()->Message_StringID(13,PROC_PETTOOLOW);
				} else {
					ExecWeaponProc(SpellProcs[i].spellID, on);
				}
			}
		}
	}
}

void Mob::ExecWeaponProc(uint16 spell_id, Mob *on) {
	// Trumpcard: Changed proc targets to look up based on the spells goodEffect flag.
	// This should work for the majority of weapons.
	if(spell_id == SPELL_UNKNOWN)
		return;
	if ( IsBeneficialSpell(spell_id) )
		SpellFinished(spell_id, GetID(), 10, 0);
	else if(!(on->IsClient() && on->CastToClient()->dead))	//dont proc on dead clients
		SpellFinished(spell_id, on->GetID(), 10, 0);
}

int Mob::GetHaste() {
	int h = spellbonuses.haste + itembonuses.haste;
	int cap = 0;
	int level = GetLevel();

	if(level < 30) {
		cap = 50;
	} else if(level < 50) {
		cap = 74;
	} else if(level < 55) {
		cap = 84;
	} else if(level < 60) {
		cap = 94;
	} else {
		cap = 100;
	}
	
	//todo: handle spells like warsong of the vah shir, etc...
	
	if(h > cap)
		h = cap;
	
	//for now we will let hundred hands exceed the cap
	if(spellbonuses.HundredHands || itembonuses.HundredHands)
		h += 20;	//TODO: put a real number here... I just made this up
	
	h += ExtraHaste;	//GM granted haste.
	
	return(h); 
}










