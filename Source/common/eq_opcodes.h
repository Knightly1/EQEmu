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
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  04111-1307  USA
*/
#ifndef EQ_OPCODES_H
#define EQ_OPCODES_H

// solar: updated 2/12/04
//
// Invalid opcodes have been \t'd out, confirmed have no \t
// all the ops have the first nibble as 0, so anything that's not 0x0???
// is just an invalid opcode.  if an opcode is known to be wrong set it to
// this to avoid conflicts with real ops
//
// codes preceded by '/**/' are ones that are known to be right but not used
// by any code yet
//
// codes preceded by '// not used' are ones that are probably wrong and not
// used anywhere
//

//////////////////////////////////////
// Zone.exe opcodes:
//////////////////////////////////////
//0x029f gives you money
//280 makes you stop receiving and sending chat messages
//260 send spell information
//45 logout
//72 player is in zone blah at dfdsfdsfg
//80 The door says
//89 Your expansion settings are:
//104 scribes a spell to your spellbook
//108,555 kicks player to server select
//123 money split from groups
//11 sense trap response
//296 name approval question
//338 failed to create new player guild
//401 pk question, 0x0191 response
//411 sacrifice question, 0x019b response
//193,213,243,244,431 chat messages
//425,499,505,506,525,526,596 crash
//451 translocate question, 0x01c3 response
//474,537 you escape from combat hiding yourself from view
//492 stat buffs
//496 your petition text is:
//581 you receive money msg with money
//562 you have control of yourself again
//613 CTD
//686 removes the unencumbered
#define OP_Heartbeat		0x0176	// client sends this periodically
#define OP_ReloadUI			0x02d7
#define OP_IncreaseStats	0x01eb
#define OP_ApproveZone	0x0134
#define OP_Dye			0x01d5

// not used	#define OP_ExpansionSetting	0x0203
// not used	#define OP_GainMoney		0x0209
// not used	#define OP_Sacrifice		0x019b
// not used	#define OP_BecomePK			0x0191
#define OP_Stamina		    0x0168
#define OP_ControlBoat		0x014d
#define OP_MobUpdate		0x003e
#define OP_ClientUpdate		0x0027
#define OP_ChannelMessage	0x0024
#define OP_SimpleMessage    0x01d7
#define OP_FormattedMessage 0x01d8
/**/ #define OP_RaidInvite		0x01e4	// from seq - not used by eqemu
/**/ #define OP_RaidJoin		0x01e5	// from seq - not used by eqemu
	// not used	#define OP_ApplyPoison		0x00b7
#define OP_TGB				0x01c6 // /targetgroupbuff
/**/ #define OP_CharInfo			0x0012 // /charinfo
/**/ #define OP_Movelog			0x0290 // /movelog
/**/ #define OP_Beta				0x02cb // /beta
/**/ #define OP_TestBuff			0x0285 // /testbuffme
/**/ #define OP_Key				0x01e2 // /keys
#define OP_Bind_Wound		0x012d
#define OP_Charm			0x01ab
#define OP_Begging			0x014c
#define OP_MoveCoin			0x0152
#define OP_SpawnDoor		0x0292
#define OP_Sneak			0x009d // Clicked sneak - Doodman 10/10/2003
#define OP_ExpUpdate		0x0079
#define OP_DumpName			0x027d //no idea what this is: just tired of looking at it as unknown; updated by Shawn319
/**/ #define OP_UpdateAA			0x0222
#define OP_RespondAA		0x01ea // AA table
#define OP_SendAAStats		0x01c9
#define OP_SendAATable		0x0366
#define OP_AAAction         0x01e9 // Used for changing percent, buying? and activating skills
#define OP_BoardBoat		0x00bb
#define OP_LeaveBoat		0x00bc

#define OP_FindRequest		0x02dc //Cofruben: used when you press find on a NPC.30/08/2004

#define OP_AdventureInfoRequest	0x02b8 //Cofruben:received when client click on the recruiter.
#define OP_AdventureInfo	0x02b9 //Cofruben:sent when client right click on the recruiter.
#define OP_AdventureRequest	0x02a6 //Cofruben: received when client press request button.1 normal,2 hard
#define OP_AdventureDetails 0x02a8 //Cofruben: sent when client press request button.
#define OP_LDoNButton		0x02a9 //Cofruben: Received button.(int8)00 decline,01 accept.
#define OP_AdventureData	0x02ba //Cofruben: Sent when client press accept button.

#define OP_AdventureFinish	0x02c9 //Cofruben:Used when you win/lose a dungeon
#define OP_LeaveAdventure	0x02c6 //Cofruben:received when client press leave adventure button
#define OP_AdventureUpdate	0x02ce


#define OP_SendExpZonein		0x002b	// 0 length packets
#define OP_ZoneInSendName		0x01e5
#define OP_ZoneInSendName2		0x01c0
	/*Guild Opcodes*/
#define OP_GuildLeader		 0x01bf // /guildleader, was 00a7
#define OP_GuildPeace		 0x009a // /guildpeace
#define OP_GuildRemove		 0x0132 // /guildremove
#define OP_GuildMemberList   0x0059 //fixed by Shawn319 (dec 18th patch)
#define OP_GuildMemberUpdate 0x026e
#define OP_GuildInvite		0x0130 // /guildinvite
#define OP_GuildMOTD		0x01c0	// /guildmotd, was 01bf
	// not used	#define OP_GuildManagement	0x005e	// LoY guild mgm't tool
#define OP_GuildPublicNote		0x003c
#define OP_GetGuildMOTD			0x027e	// /getguildmotd
#define OP_GuildDemote			0x0277
#define OP_GuildInviteAccept 0x0131
#define OP_GuildWar			0x00a4 // /guildwar
		#define OP_GuildUpdate		0x7b41
#define OP_GuildDelete			0x0133
#define OP_GuildManageRemove	0x0233
#define OP_GuildManageAdd		0x022d
#define OP_GuildManageStatus	0x0039
	/*Bazaar*/
#define OP_Trader			0x01e8 // /trader
#define OP_Bazaar			0x01e7 // /bazaar search

#define OP_BecomeTrader		0x01c4
#define	OP_BazaarInspect	0x01f4
#define OP_TraderItemUpdate	0x006e
#define	OP_TraderDelItem	0x017c
#define OP_TraderShop		0x01eb	// right-click on a trader in bazaar

#define OP_TraderBuy		0x01ca	// buy from a trader in bazaar
		
#define OP_PetCommands		0x01ac
#define OP_TradeSkillCombine 0x0042
#define OP_AugmentItem 0x02e5
#define OP_ItemName 0x0367



/*Shops*/
#define OP_ShopItem			0x02cd	// Send merchant item data to client (header = 0x64)
#define OP_ShopPlayerBuy	0x0065
	// not used	#define OP_ShopTakeMoney	0x0066
#define OP_ShopPlayerSell	0x006a
#define OP_ShopDelItem		0x006d
		#define OP_ShopEndConfirm	0x4f6d
#define OP_ShopRequest		0x00f7	// right-click on merchant
#define OP_ShopEnd			0x006c	// Finished shopping at merchant

#define	OP_AdventureMerchantRequest	0x02d1
#define	OP_AdventureMerchantResponse	0x02d2
#define	OP_AdventureMerchantPurchase	0x02d3
#define	OP_AdventurePointsUpdate	0x02e3

/**/ #define OP_LFPCommand		     0x0272	// Looking for player
/**/ #define OP_LFPGetMatchesRequest  0x0273
/**/ #define OP_LFPGetMatchesResponse 0x0275

/**/ #define OP_LFGGetMatchesRequest  0x0271
/**/ #define OP_LFGGetMatchesResponse 0x0274
#define OP_LFGCommand		     0x0270	// When client issues /LFG command
/**/ #define OP_LFGResponse           0x01b1
#define OP_LFGAppearance	0x01d0	// Some other char in zone turns LFG on/off

#define	OP_MoneyUpdate		0x01b5

		#define OP_GroupDelete		0x9721
#define	OP_GroupAcknowledge	0x0272

#define OP_GroupUpdate		0x024a
#define OP_GroupInvite		0x025f
#define OP_GroupDisband		0x00ff
#define OP_GroupInvite2		0x00d5
#define OP_GroupFollow		0x025e
#define OP_GroupFollow2		0x00d7

#define OP_GroupCancelInvite		0x00d6

#define OP_Split			0x0156	// /split
#define OP_Jump				0x00d8	// not used atm but will be when stamina is fixed
#define OP_ConsiderCorpse	0x01d6
#define OP_SkillUpdate		0x0064

#define OP_GMEndTrainingResponse 0x0178
#define OP_GMEndTraining	0x013c
#define OP_GMTrainSkill		0x0175
#define OP_GMTraining		0x013b

#define OP_ConsumeAmmo		0x017b
#define OP_CombatAbility	0x0171

#define OP_TrackUnknown		0x009c
#define OP_TrackTarget		0x0234
#define OP_Track			0x0286
#define	OP_ReadBook			0x0297

#define OP_ItemLinkClick	0x001f
#define OP_ItemLinkResponse	0x01f4
#define OP_ItemLinkText		0x01d9

		#define OP_RezzRequest		0x2a41
#define OP_RezzAnswer		0x00e5
#define OP_RezzComplete		0x019b

#define	OP_MoveDoor			0x0128
#define	OP_ClickDoor		0x0127	// Click door
#define OP_SendZonepoints	0x0247	// Coords in a zone that will port you to another zone
#define OP_SetRunMode		0x008c	// Client hit the "run" button (or control+r)
#define OP_InspectRequest	0x0248
#define OP_InspectAnswer	0x0249
#define OP_SenseTraps		0x0187  // Clicked sense traps - @Doodman 10/10/2003
#define OP_DisarmTraps		0x018e  // Clicked disarm traps - @Doodman 10/10/2003
#define OP_Assist			0x01bc
#define OP_PickPocket		0x0240

#define OP_LootRequest		0x0119
#define OP_EndLootRequest	0x011a
#define OP_MoneyOnCorpse	0x011b
#define OP_LootComplete		0x0179
#define OP_LootItem			0x013f
	// solar: there was an OP_PlaceItem synonym for this
#define OP_MoveItem		0x0151	// Client moving an item from one slot to another (user action)

#define OP_WhoAllRequest   	0x0056
#define OP_WhoAllResponse   0x0229
#define OP_Consume			0x0167
#define OP_AutoAttack		0x0172
#define OP_AutoAttack2		0x0186
#define OP_TargetMouse		0x0173	// mouse targetting a person (also: Pressing F* key to target)
#define OP_TargetCommand	0x01ba	// /target user
#define OP_TargetReject		0x01d8	// When /target fails (// solar: untested)
#define OP_Hide				0x009e
#define OP_Forage			0x012e // Clicked forage  - @Doodman 10/10/2003
#define OP_Fishing 			0x0077	
/**/ #define OP_Adventure		0x02d0 // /adventure
/**/ #define OP_Feedback			0x0161	// /feedback
#define OP_Bug				0x0246	// /bug
#define OP_Emote			0x00f2	// /me goes blah
#define OP_EmoteAnim		0x0140 // solar: untested
#define OP_Consider			0x015c
#define OP_FaceChange		0x01cb	// /face
	// not used	#define OP_Report			0x41e0
#define OP_RandomReq		0x0197
#define OP_RandomReply		0x0087
#define OP_Camp				0x01c3
#define OP_YellForHelp		0x0192
#define OP_SafePoint		0x00ef

#define OP_Buff				0x0157
#define OP_BuffFadeMsg		0x00c0
		#define OP_MultiLineMsg		0x1440	// is this still good for anything?
#define OP_SpecialMesg		0x021c	// Communicate textual info to client
#define OP_Consent			0x0013	// /consent
#define OP_ConsentResponse	0x029d
#define OP_Deny				0x02d4
#define OP_Stun				0x016c
#define OP_BeginCast		0x0021
#define OP_CastSpell		0x00be
#define OP_InterruptCast	0x01a8
#define OP_Death			0x0105
#define OP_FeignDeath		0x023f
#define OP_Illusion			0x012b
#define OP_LevelUpdate		0x0078
#define OP_LevelAppearance	0x0371
	// not used	#define OP_LocateCorpse     0x00d1  //Sent when a client casts Locate Corpse spells?

#define OP_MemorizeSpell	0x00c2	// Memming a spell from book to spell slot
#define OP_HPUpdate			0x0244	// Update HP % of a PC or NPC
/**/ #define OP_SendHPTarget		0x022e
#define OP_Mend				0x007d
	#define OP_MendHPUpdate		0x009b
#define OP_Taunt			0x0160

/**/ #define OP_Summoncorpse		0x02b5 // /summoncorpse
/**/ #define OP_GMSearchCorpse	0x0097	// GM /searchcorpse	- Search all zones for named corpse
/**/ #define OP_SearchCorpse		OP_GMSearchCorpse	// /searchcorpse
#define OP_GMDelCorpse		0x0199 // /delcorpse
#define OP_GMFind			0x0047	// GM /find			- ?
// not used	#define OP_FindResponse		0x02cc
#define OP_GMServers		0x0020	// GM /servers		- ?
#define OP_GMGoto			0x010b	// GM /goto			- Transport to another loc
#define OP_GMSummon			0x028c	// GM /summon		- Summon PC to self
#define	OP_GMKick			0x010a	// GM /kick			- Boot player
#define OP_GMKill			0x0109	// GM /kill			- Insta kill mob/pc
		#define OP_GMNameChange		0xcb40 // /name
#define OP_GMLastName		0x00a3	// GM /lastname		- Change user lastname
#define OP_GMToggle			0x01b3	// GM /toggle		- Toggle ability to receive tells from other PC's
#define OP_GMEmoteZone		0x028f	// GM /emotezone	- Send zonewide emote
#define OP_GMBecomeNPC		0x0074	// GM /becomenpc	- Become an NPC
// (TODO: Use opcode 0x012d, which is also sent with OP_GMBecomeNPC to create correct npc
/**/ #define OP_GMApproval		0x01b0	// GM /approval		- Name approval duty?
// not used	#define OP_NameApproval		0x011f //Name approval
#define OP_GMHideMe			0x00de	// GM /hideme		- Remove self from spawn lists and make invis
/**/ #define OP_GMInquire		0x00da	// GM /inquire		- Search soulmark data
/**/ #define	OP_GMSoulmark		0x00dc	// GM /praise /warn	- Add soulmark comment to user file
#define OP_GMZoneRequest	0x0184	// GM /zone			- Transport to another zone
#define OP_GMZoneRequest2	0x0239	// GM /zone 2

#define OP_Petition			0x0068
#define OP_PetitionRefresh	0x0085
#define OP_PDeletePetition	0x01ee
#define OP_PetitionBug		0x0092	// 0094 feedback 0095 guide
/**/ #define OP_PViewPetition	0x01ef
#define OP_PetitionUpdate	0x0069	// Updates the Petitions in the Que
#define OP_PetitionCheckout	0x0076	// Petition Checkout
#define OP_PetitionCheckout2	0x0056 //Also sent when a player checks out a petition Possibly requesting who all
#define OP_PetitionDelete	0x0091	// Client Petition Delete Request
#define OP_PetitionResolve	0x02b4	// Client Petition Resolve Request
#define OP_PetitionCheckIn	0x007e	// Petition Checkin
#define OP_PetitionUnCheckout 0x0090

#define OP_PetitionQue		0x01ec //GM looking at petitions


#define OP_SetServerFilter	0x01bb
	// not used	#define OP_SetServerFilterAck 0xc341

#define OP_NewSpawn			0x0218	// New NPC or PC entering zone
#define OP_Animation		0x0140
/**/ #define OP_MobHealth		0x022e	// health sent when a player clicks on the mob
#define OP_ZoneChange		0x0142	// Client requesting transfer to a different zone
#define OP_DeleteSpawn		0x00f3	// Remove a spawn from the current zone
	// not used	#define OP_ConfirmDelete	0x4178	//Client sends this to server to confirm op_deletespawn
	// not used	#define OP_NewCorpse		0x00da
		#define OP_CrashDump		0x4265
	// not used	#define OP_CastOn			0x0119
#define OP_EnvDamage		0x00e8

#define OP_Action			0x0101
#define OP_Damage			0x00e2	// seq calls this Action2
#define OP_ManaChange		0x00bf
#define OP_ClientError		0x027c
/**/ #define OP_LoadSpellSet		0x02a4
#define OP_Save				0x00fb	// Client asking server to save user state
#define OP_LocInfo			0x0316

#define OP_Surname			0x0188
#define OP_SwapSpell		0x018f	// Swapping spell positions within book
#define OP_DeleteSpell			0x01db
#define OP_CloseContainer	0x029f	//Client closing world container (i.e., forge)
#define OP_ClickObjectAck	0x029f	//Client closing world container (i.e., forge)
#define OP_CreateObject		0x00fa	//Zone objects (pok books, objects on ground, etc)
#define	OP_ClickObject		0x00f9	//Client clicking on object
#define OP_ClearObject		0x01c1
#define OP_ZoneUnavail		0x0265
/**/ #define OP_FlashMessage		0x02cd //wierd opcode that flashes message on screen
#define OP_ItemPacket		0x02e0	// Variety of ways for sending out item data
		//0x0283 hmm
#define OP_TradeRequest		0x029a	// Client request trade session
#define OP_TradeRequestAck	0x0037	// Trade request recipient is acknowledging they are able to trade
#define OP_TradeAcceptClick	0x002d
	// not used	#define OP_ItemToTrade		0x0031
#define OP_TradeMoneyUpdate	0x0162
#define OP_TradeCoins		0x0036
#define OP_CancelTrade		0x002e
#define OP_FinishTrade		0x002f
/**/ #define OP_Translocate		0x01c5
	// not used	#define OP_WebUpdate		0x01f2
#define OP_SaveOnZoneReq    0x00a1
#define OP_Logout           0x0185  // Last opcode seny by server when you zone or camp
#define	OP_RequestDuel		0x0298 //Shawn319: Fixed 1/3/04
	// OP_DeclineDuel	0x29c
		#define OP_DuelResponse		0x4a5d
#define OP_DuelResponse2	0x016e
#define OP_InstillDoubt		0x0022

#define OP_SafeFallSuccess	0x00ac
#define OP_DisciplineUpdate	0x02fb	

//Tribute Master

//send initial tribute update right before inventory
//send the OP_TributeInfo's in response to OP_ReqClientSpawn
#define OP_TributeUpdate	0x02f2
#define OP_TributeItem		0x02f3
#define OP_TributePointUpdate   0x02f4
#define OP_SendTributes		0x02f5
#define OP_TributeInfo		0x02f6
#define OP_SelectTribute	0x02f7
#define OP_TributeID		0x02f8		//4 bytes, seems to be from offset 5760 in the PlayerProfile
#define OP_StartTribute		0x02f9
#define OP_TributeNPC		0x02fa		//no idea what this is
#define OP_TributeMoney		0x02fe
#define OP_TributeToggle	0x0364

//Father Nitwit OpCodes:
//New Tradeskill Interface
#define OP_RecipesFavorite	0x0322
#define OP_RecipesSearch	0x01f9
#define OP_RecipeReply	0x01fa
#define OP_RecipeDetails	0x01fb
#define OP_RecipeAutoCombine 0x01fc

//for the 'find' command, still missing ones sent on zone-in
	#define OP_FindPersonRequest	0x02db
	#define OP_FindPersonReply	0x02dc

#define OP_Shielding		0x01dd		//this is prolly wrong

	//////////////////////////////////////
	// Zone.exe opcodes for login sequence:
	//////////////////////////////////////
#define OP_SetDataRate		0x0198	// Client sending datarate.txt value
#define OP_ZoneEntry		0x023b	// Info about char entering zone..
#define OP_PlayerProfile	0x006b	// Basic player info (no inventory)
#define OP_CharInventory	0x0291	// Full inventory of player
#define OP_ZoneSpawns		0x0170	// All spawns in current zone
#define	OP_TimeOfDay		0x0026	// Notify client of current time
#define OP_Weather			0x015b	// Weather update
#define OP_ReqNewZone		0x00ec	// Client requesting NewZone_Struct
#define OP_NewZone			0x00eb	// Info about zone being loaded (exp multiplier, etc)
#define OP_ReqClientSpawn	0x00fd	// Client requesting spawn data
#define OP_SpawnAppearance	0x012F	// Sets spawnid/animation/equipment
#define OP_ClientReady		0x0086	// Client fully connected, finished loading
#define OP_ZoneComplete			OP_ClientReady //Client sends upon successful zone in

//////////////////////////////////////
// World.exe opcodes:
//////////////////////////////////////
/**/ #define OP_LoginComplete		0x02db
#define OP_ApproveWorld			0x0195
#define OP_LogServer			0x035f
#define OP_MOTD					0x01b2	// Server message of the day
#define OP_SendLoginInfo		0x0251
#define OP_DeleteCharacter		0x00ea	// Delete character @ char select
#define OP_SendCharInfo			0x0102	// Send all chars visible @ char select
#define OP_ExpansionInfo		0x00e1	// Which expansions user has
#define OP_CharacterCreate		0x0104	// Create character @ char select
#define OP_RandomNameGenerator	0x02ab	// Returns a random name
#define OP_GuildsList			0x005d	// Server sending client list of guilds
#define OP_ApproveName			0x0125	// Approving new character name @ char creation
#define OP_EnterWorld			0x0261	// Server approval for client to enter world
#define OP_World_Client_CRC1	0x015a	// Contains a snippet of spell data
#define OP_World_Client_CRC2	0x015e	// Second client verification packet
#define OP_SetChatServer		0x0269	// Chatserver? IP,Port,servername.Charname,password(?)
#define OP_ZoneServerInfo		0x0264	// Zone server? IP,0's,int16?
// not used	#define OP_UserCompInfo		0x02a5	// User submitting computer information

//////////////////////////////////////
// Zone.exe/World.exe shared opcodes:
//////////////////////////////////////
#define OP_AckPacket			0x0017	// Appears to be generic ack at the presentation level
#define OP_WearChange			0x012c	// Client texture/color update


////////////////////////////
// OLD opcodes:
////////////////////////////

// not used	#define OP_SummonedItem		0x7841
// not used	#define OP_HarmTouch		0x7E41
// not used	#define OP_Drink			0x4641
// not used	#define OP_Medding			0x5841
// not used	#define OP_SenseHeading		0x00c3	// Clicked sense heading button - solar: not used anymore

	#define PET_BACKOFF			1
	#define PET_GETLOST			2
	#define PET_HEALTHREPORT	4
	#define PET_GUARDHERE		5
	#define PET_GUARDME			6
	#define PET_ATTACK			7
	#define PET_FOLLOWME		8
	#define PET_SITDOWN			9
	#define PET_STANDUP			10
	#define PET_TAUNT			11
	#define PET_HOLD			12
	#define PET_NOTAUNT			14
	#define PET_LEADER			16
	#define	PET_SLUMBER			17


// Agz: The following is from the old source I used as base
/************ ENUMERATED PACKET OPCODES ************/
	#define ALL_FINISH                  0x0005
	#define LS_REQUEST_VERSION          0x0059
	#define LS_SEND_VERSION             0x0059
	#define LS_SEND_LOGIN_INFO          0x0001
	#define LS_SEND_SESSION_ID          0x0004
	#define LS_REQUEST_UPDATE           0x0052
	#define LS_SEND_UPDATE              0x0052

	#define LS_REQUEST_SERVERLIST       0x0046
	#define LS_SEND_SERVERLIST          0x0046

	#define LS_REQUEST_SERVERSTATUS     0x0048
	#define LS_SEND_SERVERSTATUS        0x004A
	#define LS_GET_WORLDID              0x0047
	#define LS_SEND_WORLDID             0x0047
	#define WS_SEND_LOGIN_INFO          0x5818
	#define WS_SEND_LOGIN_APPROVED      0x0710
	#define WS_SEND_LOGIN_APPROVED2     0x0180
	#define WS_SEND_CHAR_INFO           0x4720
#endif
