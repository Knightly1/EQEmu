/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2004  EQEMu Development Team (http://eqemu.org)

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
#ifndef FEATURES_H
#define FEATURES_H

/*

	This file defines many optional features for the emu
	as well as various parameters used by the emu.
	
	If ambitious, most of these could prolly be turned into
	database variables, but the really frequently run pieces
	of code, should not be done that way for speed reasons IMO

*/

/*

Core Zone features

*/


#define ZONE_AUTOSHUTDOWN_DELAY		5000

//Uncomment this to cause a zone to basically idle
//when there are no players in it, mobs stop wandering, etc..
#define IDLE_WHEN_EMPTY


/*

Map Configuration
In general, these computations are expensive, so if you have performance
problems, consider turning them off.

*/

//uncomment this to make the LOS code say all mobs can see all others
//when no map file is loaded, opposed to the default nobody-sees-anybody
//#define LOS_DEFAULT_CAN_SEE

//THESE ARE BROKEN RIGHT NOW:
//enable these to prevent mob hopping when they are pathing
//#define FIX_PATHING_WHEN_MOVING // expensive but accurate
//fix Z on sendto as well, if you still have hopping problems.
//#define FIX_SENDTO_Z

/*

Zone extensions and features

*/


//Uncomment this to enable group linking:
#define ENABLE_GROUP_LINKING 1

//Uncomment this to enable the Winter's Roar pool looting system
//#define POOLLOOTING

//Uncomment this to scale XP gained from MOBs based on their CON
//#define CON_XP_SCALING

//Uncomment to make group buffs affect group pets
#define GROUP_BUFF_PETS


/*

Zone Numerical configuration

*/

//Reuse times for various skills, here for convenience, in sec
//set to 0 to disable server side checking of timers.
enum {	//reuse times
	FeignDeathReuseTime = 10,
	SneakReuseTime = 8,
	HideReuseTime = 10,
	TauntReuseTime = 6,
	InstillDoubtReuseTime = 10,
	FishingReuseTime = 12,
	ForagingReuseTime = 75,		//this is wrong
	MendReuseTime = 300,
	TrackingReuseTime = 150,
	BashReuseTime = 8,
	BackstabReuseTime = 10,
	KickReuseTime = 8,
	TailRakeReuseTime = 6,
	EagleStrikeReuseTime = 6,
	RoundKickReuseTime = 9,
	TigerClawReuseTime = 6,
	FlyingKickReuseTime = 8,
	SenseTrapsReuseTime = 10,
	DisarmTrapsReuseTime = 10,
	HarmTouchReuseTime = 4320,
	LayOnHandsReuseTime = 4320
};

enum {	//various hard caps
	eqManaRegenItemCap = 15,
	eqHPRegenItemCap = 15
};
	

//max number of people per group.
//is pretty much tied to what the client supports
#define MAX_GROUP_MEMBERS 6

//Max number of groups you can link with. Not tied to the client.
//if group linking is enabled above
#define MAX_GROUP_LINKS 8

//this is the number of levels below the thief's level that
//an npc must be before they can steal from them.
#define THIEF_PICKPOCKET_UNDER 5

//this is the % chance that an NPC will dual weild a 2nd weapon 
//in its loot table, if it is able to.
#define NPC_DW_CHANCE 5

//This is the entry in npc_types to spawn for trap damagaes
#define TRAP_NPC_TYPE 1586

//this defines the level difference at which things auto-resist spells
#define AUTO_RESIST_LEVEL_DIFF 15

//This is the multiplier of eqemu speed to get client speed
//tweak this if pathing mobs seem to jump forward or backwards
//this should prolly be dynamic based on ping time or something.. who knows
//Values found in the emu somewhere at one point in time: 36, 43
#define NPC_RUNANIM_RATIO 34

//minimum level to do alchemy
#define MIN_LEVEL_ALCHEMY 25

#endif

