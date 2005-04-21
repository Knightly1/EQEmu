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
#ifndef EQ_CONSTANTS_H
#define EQ_CONSTANTS_H 

/*
** Item packet types
**
*/
enum ItemPacketType
{
	ItemPacketViewLink			= 0x00,
	ItemPacketTradeView			= 0x65,
	ItemPacketLoot				= 0x66,
	ItemPacketTrade				= 0x67,
	ItemPacketCharInventory		= 0x69,
	ItemPacketSummonItem		= 0x6A,
	ItemPacketTributeItem		= 0x6C,
	ItemPacketMerchant			= 0x64,
	ItemPacketWorldContainer	= 0x6B
};

/*
** Item attributes
**
*/
enum ItemAttrib
{
	ItemAttribLore			= (1 << 0),
	ItemAttribArtifact		= (1 << 1),
	ItemAttribSummoned		= (1 << 2),
	ItemAttribMagic			= (1 << 3),
	ItemAttribAugment		= (1 << 4),
	ItemAttribPendingLore	= (1 << 5),
	ItemAttribNone			= 0,
	ItemAttribUnknown		= 0xFFFFFFFF
};

/*
** Item types
**
*/
enum ItemClass
{
	ItemClassCommon		= 0,
	ItemClassContainer	= 1,
	ItemClassBook		= 2
};

/*
** Item uses
**
*/
enum ItemTypes
{
	ItemType1HS			= 0,
	ItemType2HS			= 1,
	ItemTypePierce		= 2,
	ItemType1HB			= 3,
	ItemType2HB			= 4,
	ItemTypeBow			= 5,
	//6
	ItemTypeThrowing		= 7,
	ItemTypeShield		= 8,
	//9
	ItemTypeArmor		= 10,
	ItemTypeUnknon		= 11,	//A lot of random crap has this item use.
	ItemTypeLockPick		= 12,
	ItemTypeFood			= 14,
	ItemTypeDrink		= 15,
	ItemTypeLightSource	= 16,
	ItemTypeStackable	= 17,	//Not all stackable items are this use...
	ItemTypeBandage		= 18,
	ItemTypeThrowingv2	= 19,
	ItemTypeSpell		= 20,	//spells and tomes
	ItemTypePotion		= 21,
	ItemTypeWindInstr	= 23,
	ItemTypeStringInstr	= 24,
	ItemTypeBrassInstr	= 25,
	ItemTypeDrumInstr	= 26,
	ItemTypeArrow		= 27,
	ItemTypeJewlery		= 29,
	ItemTypeSkull		= 30,
	ItemTypeTome			= 31,
	ItemTypeNote			= 32,
	ItemTypeKey			= 33,
	ItemTypeCoin			= 34,
	ItemType2HPierce		= 35,
	ItemTypeFishingPole	= 36,
	ItemTypeFishingBait	= 37,
	ItemTypeAlcohol		= 38,
	ItemTypeCompass		= 40,
	ItemTypePoison		= 42,	//might be wrong, but includes poisons
	ItemTypeHand2Hand	= 45,
	ItemUseSinging		= 50,
	ItemUseAllInstruments	= 51,
	ItemTypeCharm		= 52,
	ItemTypeAugment		= 54,
	ItemTypeAugmentSolvent	= 55,
	ItemTypeAugmentDistill	= 56
};

/*
** Item Effect Types
**
*/
enum {
	ET_CombatProc = 0,
	ET_ClickEffect = 1,
	ET_WornEffect = 2,
	ET_Expendable = 3,
	ET_EquipClick = 4,
	ET_ClickEffect2 = 5,	//name unknown
	ET_Focus = 6,
	ET_Scroll = 7
};


#define AT_Die				0	// this causes the client to keel over and zone to bind point
#define AT_WhoLevel		1	// the level that shows up on /who
#define AT_Invis			3	// 0 = visible, 1 = invisible
#define AT_PVP				4	// 0 = blue, 1 = pvp (red)
#define AT_Light			5	// light type emitted by player (lightstone, shiny shield)
#define AT_Anim				14	// 100=standing, 110=sitting, 111=ducking, 115=feigned, 105=looting
#define AT_Sneak			15	// 0 = normal, 1 = sneaking
#define AT_SpawnID		16	// server to client, sets player spawn id
#define AT_HP					17	// Client->Server, my HP has changed (like regen tic)
#define AT_Linkdead		18	// 0 = normal, 1 = linkdead
#define AT_Levitate		19	// 0=off, 1=flymode, 2=levitate
#define AT_GM					20	// 0 = normal, 1 = GM - all odd numbers seem to make it GM
#define AT_Anon				21	// 0 = normal, 1 = anon, 2 = roleplay
#define AT_GuildID		22
#define AT_GuildRank	23	// 0=member, 1=officer, 2=leader
#define AT_AFK				24	// 0 = normal, 1 = afk
#define AT_Split			28	// 0 = normal, 1 = autosplit on
#define AT_Size				29	// spawn's size
#define AT_NPCName		31	// change PC's name's color to NPC color 0 = normal, 1 = npc name
//#define AT_Trader			300  // Bazzar Trader Mode

// solar: animations for AT_Anim
#define ANIM_FREEZE	102
#define	ANIM_STAND		0x64
#define	ANIM_SIT		0x6e
#define	ANIM_CROUCH		0x6f
#define	ANIM_DEATH		0x73
#define ANIM_LOOT		0x69

/* 
** Diety List
*/
#define DEITY_UNKNOWN			0
#define DEITY_AGNOSTIC			396
#define DEITY_BRELL				202
#define DEITY_CAZIC				203
#define DEITY_EROL				204
#define DEITY_BRISTLE			205
#define DEITY_INNY				206
#define DEITY_KARANA			207
#define DEITY_MITH				208
#define DEITY_PREXUS			209
#define DEITY_QUELLIOUS			210
#define DEITY_RALLOS			211
#define DEITY_SOLUSEK			213
#define DEITY_TRIBUNAL			214
#define DEITY_TUNARE			215

//Guessed:
#define DEITY_BERT				201	
#define DEITY_RODCET			212
#define DEITY_VEESHAN			216

// msg_type's for custom usercolors 
#define MT_Say					256
#define MT_Tell					257
#define MT_Group				258
#define MT_Guild				259
#define MT_OOC					260
#define MT_Auction				261
#define MT_Shout				262
#define MT_Emote				263
#define MT_Spells				264
#define MT_YouHitOther			265
#define MT_OtherHitsYou			266
#define MT_YouMissOther			267
#define MT_OtherMissesYou		268
#define MT_Broadcasts			269
#define MT_Skills				270
#define MT_Disciplines			271
#define MT_CritMelee			301
#define	MT_Unused1				272
#define MT_DefaultText			273
#define MT_Unused2				274
#define MT_MerchantOffer		275
#define MT_MerchantBuySell		276
#define	MT_YourDeath			277
#define MT_OtherDeath			278
#define MT_OtherHits			279
#define MT_OtherMisses			280
#define	MT_Who					281
#define MT_YellForHelp			282
#define MT_NonMelee				283
#define MT_WornOff				284
#define MT_MoneySplit			285
#define MT_LootMessages			286
#define MT_DiceRoll				287
#define MT_OtherSpells			288
#define MT_Fizzles				289
#define MT_Chat					290
#define MT_Channel1				291
#define MT_Channel2				292
#define MT_Channel3				293
#define MT_Channel4				294
#define MT_Channel5				295
#define MT_Channel6				296
#define MT_Channel7				297
#define MT_Channel8				298
#define MT_Channel9				299
#define MT_Channel10			300
#define MT_CritMelee			301
#define MT_SpellCrits			302
#define MT_TooFarAway			303
#define MT_Rampage				304
#define MT_Flurry				305
#define MT_Enrage				306
#define MT_SayEcho				307
#define MT_TellEcho				308
#define MT_GroupEcho			309
#define MT_GuildEcho			310
#define MT_OOCEcho				311
#define MT_AuctionEcho			312
#define MT_ShoutECho			313
#define MT_EmoteEcho			314
#define MT_Chat1Echo			315
#define MT_Chat2Echo			316
#define MT_Chat3Echo			317
#define MT_Chat4Echo			318
#define MT_Chat5Echo			319
#define MT_Chat6Echo			320
#define MT_Chat7Echo			321
#define MT_Chat8Echo			322
#define MT_Chat9Echo			323
#define MT_Chat10Echo			324
#define MT_DoTDamage			315
#define MT_ItemLink				316
#define MT_RaidSay				317
#define MT_MyPet				318
#define MT_DS					320

//ZoneChange_Struct->success values
#define ZONE_ERROR_NOMSG 0
#define ZONE_ERROR_NOTREADY -1
#define ZONE_ERROR_VALIDPC -2
#define ZONE_ERROR_STORYZONE -3
#define ZONE_ERROR_NOEXPANSION -6
#define ZONE_ERROR_NOEXPERIENCE -7


#define FILTER_DAMAGESHIELD 0
#define FILTER_NPCSPELLS	1
#define FILTER_PCSPELLS		2
#define FILTER_BARDSONGS	3
#define FILTER_GUILDSAY		4
#define FILTER_SOCIALS		5
#define FILTER_GROUP		6
#define FILTER_SHOUT		7
#define FILTER_AUCTION		8
#define FILTER_OOC			9
#define FILTER_MYMISSES		10
#define FILTER_OTHERMISSES	11
#define FILTER_OTHERHITS	12
#define FILTER_ATKMISSESME	13
#define FILTER_CRITSPELLS	14
#define FILTER_CRITMELEE	15
#define FILTER_SPELLDAMAGE	16
#define FILTER_DOTDAMAGE	17
#define FILTER_MYPETHITS	18
#define FILTER_MYPETMISSES	19



#define	STAT_STR		0
#define	STAT_STA		1
#define	STAT_AGI		2
#define	STAT_DEX		3
#define	STAT_INT		4
#define	STAT_WIS		5
#define	STAT_CHA		6
#define	STAT_FIRE		7
#define	STAT_COLD		8
#define	STAT_MAGIC		9
#define	STAT_POISON		10
#define	STAT_DISEASE	11

#endif
