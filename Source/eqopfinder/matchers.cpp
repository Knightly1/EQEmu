#include "structmatch.h"
#include "../common/emu_opcodes.h"
#include "../common/eq_packet_structs.h"
#include "../common/MiscFunctions.h"
#include "../common/eq_constants.h"
#include "../common/classes.h"
#include "../common/skills.h"
#include "../EMuShareMem/Items.h"
#include "../zone/StringIDs.h"

#ifndef WIN32
#include <netinet/in.h>
#endif

/*

The point of each of these matchers is to detect a unique signature
of each packet in order to identify it. The first thing each one checks
is length, because thats the easiest way to filter most of them.

Once you have met an exact or minimum length, then the contents
and context of the packet is analyzed. Each field should be tested
against reasonable bounds for that field. Very often, expecially on
short packets, this will not be enough to uniquely identify packets,
so they will also need to look through the packets which came before
them. This should provide enough info to match a lot of packets.

In the case where multiple matchers match a packet, warnings will be
printed and it will be identified as the first one to match it, but
there is a variable named 'im_sure' which can be set to true by a 
later matcher in order to override a conflict.


*/


//configurable packet gap lengths for checking history
//should be a count of the number of other packets which might
//come in between the client's request and the server's response
//and vise-versa
//these were arbitrarily chosen for now
#define CLOSE 10
#define REGULAR 30
#define DISTANT 80
#define COMPLETE 100

#define MAX_MANA 12000
#define MAX_STAMINA 12000
#define MAX_HP 25000
#define HIGHEST_LIVE_LEVEL 99
#define MAX_GUILD_ID 1500
#define MAX_AA_SKILL 1000
#define MAX_STRING_ID 47000
#define MAX_MESSAGE_COLOR 330
#define MAX_RECIPE_ID 80000

//too general
bool StructMatcher::Match_OP_TargetReject(const SMPacket *p) {
/*	if(p->size == sizeof(TargetReject_Struct)) {
//		TargetReject_Struct *s = (TargetReject_Struct *) p->pBuffer;
		//unknown00 skipped
		
		CheckHistoryExists(OP_TargetMouse, REGULAR);
		
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_SimpleMessage(const SMPacket *p) {
	if(p->size == sizeof(SimpleMessage_Struct)) {
		SimpleMessage_Struct *s = (SimpleMessage_Struct *) p->pBuffer;
		
		//require this to only happen during play
		CheckHistoryExists(OP_MobHealth, DISTANT);
		
		//could get more complex and match it against eqstr_us.txt
		CheckUnsignedLimit(s->string_id, MAX_STRING_ID);
		CheckUnsignedLimit(s->color, MAX_MESSAGE_COLOR);
		//unknown8 skipped
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_SkillUpdate(const SMPacket *p) {
/*	if(p->size == sizeof(SkillUpdate_Struct)) {
		SkillUpdate_Struct *s = (SkillUpdate_Struct *) p->pBuffer;
		CheckSkillID(s->skillId);
		CheckUnsigned(s->value, 1, 512);	//could prolly make this tighter
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_Sneak(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
	return(false);
}

bool StructMatcher::Match_OP_ZoneEntry(const SMPacket *p) {
	if(p->size == sizeof(ClientZoneEntry_Struct)) {
		ClientZoneEntry_Struct *s = (ClientZoneEntry_Struct *) p->pBuffer;
		//unknown00 skipped
		CheckString(s->char_name, 64);
		return(true);
	} else if(p->size == sizeof(ServerZoneEntry_Struct)) {
//		ServerZoneEntry_Struct *s = (ServerZoneEntry_Struct *) p->pBuffer;
//TODO: match this based on the spawn struct
		//NO_MATCHER(s->player);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SendGuildTributes(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
	return(false);
}

bool StructMatcher::Match_OP_FinishTrade(const SMPacket *p) {
	if(p->size == 0) {
		CheckHistoryExists(OP_TradeAcceptClick, CLOSE);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_StartTribute(const SMPacket *p) {
	if(p->size == sizeof(StartTribute_Struct)) {
		StartTribute_Struct *s = (StartTribute_Struct *) p->pBuffer;
		
		CheckSelf(s->client_id);
		
		CheckMobClass(s->tribute_master_id, TRIBUTE_MASTER);
		
		//CheckUnsignedLimit(s->response, ULONG_MAX);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ShopPlayerSell(const SMPacket *p) {
	if(p->size == sizeof(Merchant_Purchase_Struct)) {
		Merchant_Purchase_Struct *s = (Merchant_Purchase_Struct *) p->pBuffer;
		
		CheckMobClass(s->npcid, MERCHANT);
		CheckItemSlot(s->itemslot);
		CheckUnsignedLimit(s->quantity, 20);
		//CheckUnsignedLimit(s->price, ULONG_MAX);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GMEndTrainingResponse(const SMPacket *p) {
	if(p->size == 0) {
		CheckHistoryExists(OP_GMTraining, DISTANT);
		return(true);
	}
	return(false);
}

//not 100% accurate
bool StructMatcher::Match_OP_GroupDisband(const SMPacket *p) {
	if(p->size == sizeof(GroupGeneric_Struct)) {
		CheckHistoryExists(OP_GroupInvite, COMPLETE);
		
		GroupGeneric_Struct *s = (GroupGeneric_Struct *) p->pBuffer;
		CheckString(s->name1, 64);
		CheckString(s->name2, 64);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_DuelResponse(const SMPacket *p) {
	if(p->size == sizeof(DuelResponse_Struct)) {
		CheckHistoryExists(OP_RequestDuel, DISTANT);
		
		DuelResponse_Struct *s = (DuelResponse_Struct *) p->pBuffer;
		CheckMobExists(s->target_id);
		CheckMobExists(s->entity_id);
		//unknown skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Buff(const SMPacket *p) {
	if(p->size == sizeof(SpellBuffFade_Struct)) {
		SpellBuffFade_Struct *s = (SpellBuffFade_Struct *) p->pBuffer;
		CheckMobExists(s->entityid);
		//I think this is wrong
		CheckUnsignedLimit(s->slot, BUFF_COUNT);
		CheckUnsignedLimit(s->level, HIGHEST_LIVE_LEVEL);
		//CheckUnsignedLimit(s->effect, ULONG_MAX);
		//unknown7 skipped
		CheckSpellID(s->spellid);
		//CheckUnsignedLimit(s->duration, ULONG_MAX);
		//unknown016 skipped
		CheckUnsignedLimit(s->slotid, BUFF_COUNT);
		//CheckUnsignedLimit(s->bufffade, ULONG_MAX);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_MoneyUpdate(const SMPacket *p) {
	if(p->size == sizeof(MoneyUpdate_Struct)) {
		MoneyUpdate_Struct *s = (MoneyUpdate_Struct *) p->pBuffer;
		CheckSigned(s->platinum, 0, 20000);
		CheckSigned(s->gold, 0, 20000);
		CheckSigned(s->silver, 0, 20000);
		CheckSigned(s->copper, 0, 20000);
		
		CheckHistoryExists(OP_SaveOnZoneReq, CLOSE);
		im_sure = true;
		
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_PetCommands(const SMPacket *p) {
/*	if(p->size == sizeof(PetCommand_Struct)) {
		PetCommand_Struct *s = (PetCommand_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->command, 18);
		//unknownpcs000 skipped
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_GMNameChange(const SMPacket *p) {
	if(p->size == sizeof(GMName_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

//needs a matcher
bool StructMatcher::Match_OP_PetitionUpdate(const SMPacket *p) {
/*	if(p->size == sizeof(PetitionUpdate_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_AAExpUpdate(const SMPacket *p) {
	if(p->size == sizeof(AltAdvStats_Struct)) {
		AltAdvStats_Struct *s = (AltAdvStats_Struct *) p->pBuffer;

		CheckUnsignedLimit(s->percentage, 100);
		CheckUnsignedLimit(s->experience, 330);
		
		//not actual limit, but reasonable... who carries around 50+ unspent aas?
		CheckUnsignedLimit(s->unspent, 30);
		
		CheckHistoryExists(OP_SendExpZonein, CLOSE);
		
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_AAAction(const SMPacket *p) {
	if(p->size == sizeof(UseAA_Struct)) {
		UseAA_Struct *s = (UseAA_Struct *) p->pBuffer;
		
		//could improve this check
		CheckUnsignedLimit(s->ability, 1000);
		
		//checking timers in a fixed interval
		uint32 curtime = time(NULL);
		if(s->ability != 0 || s->begin != 0)
			CheckUnsigned(s->begin, curtime-10000000, curtime+100000);
		CheckUnsigned(s->end, curtime-10000000, curtime+100000);
		if(s->end < s->begin) return(false);
		
		CheckHistoryExists(OP_SendExpZonein, CLOSE);
		
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ManaChange(const SMPacket *p) {
	if(p->size == sizeof(ManaChange_Struct)) {
		ManaChange_Struct *s = (ManaChange_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->new_mana, MAX_MANA);
		CheckUnsignedLimit(s->stamina, MAX_STAMINA);
		CheckSpellID(s->spell_id);
		//unknown12 skipped
		return(true);
	} else if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
	return(false);
}

bool StructMatcher::Match_OP_GuildDemote(const SMPacket *p) {
	if(p->size == sizeof(GuildDemoteStruct)) {
//		GuildDemoteStruct *s = (GuildDemoteStruct *) p->pBuffer;
//this packet is too general to possibly match accurately
//		CheckString(s->name, 64);
//		CheckString(s->target, 64);
//		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SomeItemPacketMaybe(const SMPacket *p) {
	if(p->size == sizeof(Arrow_Struct)) {
		Arrow_Struct *s = (Arrow_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		//unknown004 skipped
		CheckFloat(s->src_y, -1e7, 1e7, -5, 7);
		CheckFloat(s->src_x, -1e7, 1e7, -5, 7);
		CheckFloat(s->src_z, -1e7, 1e7, -5, 7);
		//unknown028 skipped
		CheckFloat(s->velocity, -1e7, 1e7, -5, 7);
		CheckFloat(s->launch_angle, -1e7, 1e7, -5, 7);
		CheckFloat(s->tilt, -1e7, 1e7, -5, 7);
		//unknown052 skipped
		CheckFloat(s->arc, -1e7, 1e7, -5, 7);
		//unknown064 skipped
		CheckMobExists(s->source_id);
		CheckMobExists(s->target_id);
		CheckItemID(s->item_id);
		//unknown088 skipped
		//unknown092 skipped
		//unknown096 skipped
		CheckString(s->model_name, 16);
		if(s->model_name[0] != 'I' || s->model_name[1] != 'T') return(false);
		//unknown117 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TraderShop(const SMPacket *p) {
	if(p->size == sizeof(TraderClick_Struct)) {
		TraderClick_Struct *s = (TraderClick_Struct *) p->pBuffer;
		CheckPlayer(s->traderid);
		//unknown4 skipped
		CheckUnsignedLimit(s->approval, 1);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GroupUpdate(const SMPacket *p) {
	if(p->size == sizeof(GroupJoin_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	} else if(p->size == sizeof(GroupUpdate2_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	} else if(p->size == sizeof(GroupUpdate_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

//this prolly isnt the most accurate match
bool StructMatcher::Match_OP_LootRequest(const SMPacket *p) {
	if(p->size == sizeof(EntityId_Struct)) {
		EntityId_Struct *s = (EntityId_Struct *) p->pBuffer;
		
		CheckCorpse(s->entity_id);
		
		const SMPacket *death;
		uint32 maxdist = DISTANT;
		CheckHistory(OP_Death, maxdist, death);
		Death_Struct *ds = (Death_Struct *) death->pBuffer;
		if(ds->spawn_id != s->entity_id) return(false);
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GMSummon(const SMPacket *p) {
	if(p->size == sizeof(GMSummon_Struct)) {
		GMSummon_Struct *s = (GMSummon_Struct *) p->pBuffer;
		CheckString(s->charname, 64);
		//assume we will be set as the summoned char... prolly wrong
		if(name != s->charname) return(false);
		CheckString(s->gmname, 64);
		CheckBool(s->success);
		CheckZoneID(s->zoneID);
		CheckCoord(s->y);
		CheckCoord(s->x);
		CheckZCoord(s->z);
		//unknown2 skipped
		return(true);
	}
	return(false);
}

//this packet is too general to possibly classify
bool StructMatcher::Match_OP_AutoAttack2(const SMPacket *p) {
/*	if(p->size == sizeof(uint32)) {
		uint32 *s = (uint32 *) p->pBuffer;
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_GuildInviteAccept(const SMPacket *p) {
	if(p->size == sizeof(GuildInviteAccept_Struct)) {
		GuildInviteAccept_Struct *s = (GuildInviteAccept_Struct *) p->pBuffer;
		CheckString(s->inviter, 64);
		CheckString(s->newmember, 64);
		CheckUnsignedLimit(s->response, 8);
		CheckUnsignedLimit(s->guildeqid, MAX_NUMBER_GUILDS);
		return(true);
	}
	return(false);
}

//this packet is too general to possibly classify
bool StructMatcher::Match_OP_FeignDeath(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);

		return(true);
	}
*/
	return(false);
}

bool StructMatcher::Match_OP_TributeUpdate(const SMPacket *p) {
	if(p->size == sizeof(TributeInfo_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}


//this packet is too general to possibly classify
bool StructMatcher::Match_OP_InstillDoubt(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_BecomeTrader(const SMPacket *p) {
/*	if(p->size == sizeof(BecomeTrader_Struct)) {
		BecomeTrader_Struct *s = (BecomeTrader_Struct *) p->pBuffer;
		CheckPlayer(s->id);
		CheckUnsignedLimit(s->code, 5);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_SendAATable(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	} else if(p->size >= sizeof(SendAA_Struct)) {
		SendAA_Struct *s = (SendAA_Struct *) p->pBuffer;
		CheckUnsigned(s->id, 1, ULONG_MAX);
		if(s->hotkey_sid != 0xFFFFFFFF)
			CheckUnsigned(s->hotkey_sid, 3200, 47000);
		if(s->hotkey_sid2 != 0xFFFFFFFF)
			CheckUnsigned(s->hotkey_sid2, 3200, 47000);
		if(s->title_sid != 0xFFFFFFFF)
			CheckUnsigned(s->title_sid, 3200, 47000);
		if(s->desc_sid != 0xFFFFFFFF)
			CheckUnsigned(s->desc_sid, 3200, 47000);
		//CheckUnsigned(s->class_type, 0, ULONG_MAX);
		//CheckUnsigned(s->cost, 0, ULONG_MAX);
		//CheckUnsigned(s->seq, 0, ULONG_MAX);
		CheckUnsignedLimit(s->current_level, 15);
		if(s->prereq_skill != 0xFFFFFFFF) {
			CheckUnsignedLimit(s->prereq_skill, 1000);
			CheckUnsignedLimit(s->prereq_minpoints, 15);
		}
		//CheckUnsigned(s->type, 0, ULONG_MAX);
		if(s->spellid != 0xFFFFFFFF)
			CheckSpellID(s->spellid);
		//CheckUnsigned(s->spell_type, 0, ULONG_MAX);
		//CheckUnsigned(s->spell_refresh, 0, ULONG_MAX);
		//CheckUnsigned(s->classes, 0, ULONG_MAX);
		//CheckUnsigned(s->berserker, 0, ULONG_MAX);
		//CheckUnsigned(s->max_level, 0, ULONG_MAX);
		if(s->last_id != 0xFFFFFFFF)
			CheckUnsignedLimit(s->last_id, 1000);
		if(s->next_id != 0xFFFFFFFF)
			CheckUnsignedLimit(s->next_id, 1000);
		//CheckUnsigned(s->cost2, 0, ULONG_MAX);
		//unknown80 skipped
		CheckUnsignedLimit(s->total_abilities, 30);
		
		//should verify these too
/*		uint32 idx_abilities;
		for(idx_abilities = 0; idx_abilities < 0; idx_abilities++) {
			//NO_MATCHER(s->abilities[idx_abilities]);
		}
*/
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_MoveCoin(const SMPacket *p) {
	if(p->size == sizeof(MoveCoin_Struct)) {
		MoveCoin_Struct *s = (MoveCoin_Struct *) p->pBuffer;
		CheckSigned(s->from_slot, -1, 4);
		CheckSigned(s->to_slot, -1, 4);
		CheckSigned(s->cointype1, COINTYPE_CP, COINTYPE_PP);
		CheckSigned(s->cointype2, COINTYPE_CP, COINTYPE_PP);
		CheckSigned(s->amount, 0, 20000);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_DeleteSpell(const SMPacket *p) {
	if(p->size == sizeof(DeleteSpell_Struct)) {
		DeleteSpell_Struct *s = (DeleteSpell_Struct *) p->pBuffer;
		CheckSigned(s->spell_slot, 0, 10);
		//unknowndss002 skipped
		CheckBool(s->success);
		//unknowndss006 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Death(const SMPacket *p) {
	if(p->size == sizeof(Death_Struct)) {
		Death_Struct *s = (Death_Struct *) p->pBuffer;
		CheckMobExists(s->spawn_id);
		CheckMobExists(s->killer_id);
		//corpse ID always seems to be 0
		if(s->corpseid != 0) return(false);
		
		if(s->spell_id != 0xFFFFFFFF) {
			CheckSpellID(s->spell_id);
		} else {
			CheckSkillID(s->attack_skill);
		}
		CheckZoneID(s->bindzoneid);
		
		im_sure = true;
		
		//unknown028 skipped
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Consent(const SMPacket *p) {
/*	if(p->size >= sizeof(Consent_Struct)) {
		Consent_Struct *s = (Consent_Struct *) p->pBuffer;
		CheckString(
		
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_RezzAnswer(const SMPacket *p) {
	if(p->size == sizeof(Resurrect_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

//this might be too general
bool StructMatcher::Match_OP_ItemName(const SMPacket *p) {
	if(p->size == sizeof(ItemNamePacket_Struct)) {
		ItemNamePacket_Struct *s = (ItemNamePacket_Struct *) p->pBuffer;
		CheckItemID(s->item_id);
		//CheckUnsignedLimit(s->unkown004, ULONG_MAX);
		CheckString(s->name, 64);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_ConsentDeny(const SMPacket *p) {
/*	if(p->size >= sizeof(Consent_Struct)) {
		
		return(true);
	}
*/	return(false);
}

//this packet is too general to match
bool StructMatcher::Match_OP_GMKick(const SMPacket *p) {
/*	if(p->size == sizeof(GMKick_Struct)) {
		GMKick_Struct *s = (GMKick_Struct *) p->pBuffer;
		CheckString(s->name, 64);
		CheckString(s->gmname, 64);
		//unknown skipped
		return(true);
	} else if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
*/
	return(false);
}

//might be too general
bool StructMatcher::Match_OP_TributeNPC(const SMPacket *p) {
	if(p->size == sizeof(int32)) {
		int32 *s = (int32 *) p->pBuffer;
		CheckMobClass(*s, TRIBUTE_MASTER);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Stun(const SMPacket *p) {
/*	if(p->size == sizeof(Stun_Struct)) {
		Stun_Struct *s = (Stun_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->duration, 3600);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_DisarmTraps(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);

		return(true);
	}
*/
	return(false);
}

bool StructMatcher::Match_OP_GuildMOTD(const SMPacket *p) {
	if(p->size == sizeof(GuildMOTD_Struct)) {
		GuildMOTD_Struct *s = (GuildMOTD_Struct *) p->pBuffer;
		//unknown0 skipped
		CheckString(s->name, 64);
		CheckString(s->setby_name, 64);
		//unknown132 skipped
		CheckString(s->motd, 512);
		return(true);
	}
	return(false);
}

//not sure if this is exact enough
bool StructMatcher::Match_OP_EndLootRequest(const SMPacket *p) {
	if(p->size == sizeof(EntityId_Struct)) {
		EntityId_Struct *s = (EntityId_Struct *) p->pBuffer;
		CheckHistoryExists(OP_LootRequest, DISTANT);
		CheckCorpse(s->entity_id);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_FaceChange(const SMPacket *p) {
	if(p->size == sizeof(FaceChange_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TimeOfDay(const SMPacket *p) {
	if(p->size == sizeof(TimeOfDay_Struct)) {
		TimeOfDay_Struct *s = (TimeOfDay_Struct *) p->pBuffer;
		CheckUnsigned(s->hour, 1, 24);
		CheckUnsignedLimit(s->minute, 59);
		CheckUnsigned(s->day, 1, 28);
		CheckUnsigned(s->month, 1, 12);
		CheckUnsigned(s->year, 3000, 4000);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GuildWar(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
	return(false);
}

bool StructMatcher::Match_OP_RespondAA(const SMPacket *p) {
	if(p->size == sizeof(AATable_Struct)) {
		AATable_Struct *s = (AATable_Struct *) p->pBuffer;
		int r;
		for(r = 0; r < MAX_PP_AA_ARRAY; r++) {
			CheckUnsignedLimit(s->aa_list[0].aa_skill, MAX_AA_SKILL);
			CheckUnsignedLimit(s->aa_list[0].aa_value, 10);
		}
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_GuildLeader(const SMPacket *p) {
/*	if(p->size == sizeof(GuildMakeLeader)) {
		GuildMakeLeader *s = (GuildMakeLeader *) p->pBuffer;
		CheckString(s->name, 64);
		CheckString(s->target, 64);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_LFGCommand(const SMPacket *p) {
	if(p->size == sizeof(LFG_Struct)) {
		LFG_Struct *s = (LFG_Struct *) p->pBuffer;
		//unknown000 skipped
		//CheckUnsignedLimit(s->value, ULONG_MAX);
		//unknown008 skipped
		//unknown012 skipped
		CheckString(s->name, 64);
		return(true);
	}
	return(false);
}

//too unknown
bool StructMatcher::Match_OP_GMToggle(const SMPacket *p) {
/*	if(p->size == sizeof(GMToggle_Struct)) {
		GMToggle_Struct *s = (GMToggle_Struct *) p->pBuffer;
		//unknown0 skipped
		//CheckUnsignedLimit(s->toggle, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

//too general for now
bool StructMatcher::Match_OP_Shielding(const SMPacket *p) {
/*	if(p->size == sizeof(Shielding_Struct)) {
		if(_class != WARRIOR) return(false);
		Shielding_Struct *s = (Shielding_Struct *) p->pBuffer;
		CheckPlayer(s->target_id);
		return(true);
	}
*/	
	return(false);
}

bool StructMatcher::Match_OP_LeadershipExpToggle(const SMPacket *p) {
	if(p->size == sizeof(uint8)) {
//		uint8 *s = (uint8 *) p->pBuffer;
		//I think this is the only 1 byte packet...
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TradeRequestAck(const SMPacket *p) {
	if(p->size == sizeof(TradeRequest_Struct)) {
		
		CheckHistoryExists(OP_TradeRequest, DISTANT);
		
		TradeRequest_Struct *s = (TradeRequest_Struct *) p->pBuffer;
		CheckMobExists(s->to_mob_id);
		CheckMobExists(s->from_mob_id);
		
		//could check distance
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ClientError(const SMPacket *p) {
	if(p->size == sizeof(ClientError_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_RecipesFavorite(const SMPacket *p) {
	if(p->size == sizeof(TradeskillFavorites_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_RandomReply(const SMPacket *p) {
	if(p->size == sizeof(RandomReply_Struct)) {
		RandomReply_Struct *s = (RandomReply_Struct *) p->pBuffer;
		if(s->low >= s->high || s->result < s->low || s->result > s->high) return(false);
		//CheckUnsignedLimit(s->low, ULONG_MAX);
		//restrict range since most people dont rand() huge numbers
		CheckUnsignedLimit(s->high, 2000);
		//CheckUnsignedLimit(s->result, ULONG_MAX);
		CheckString(s->name, 64);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Action(const SMPacket *p) {
	if(p->size == sizeof(Action_Struct)) {
		Action_Struct *s = (Action_Struct *) p->pBuffer;
		CheckMobExists(s->target);
		CheckMobExists(s->source);
		CheckUnsignedLimit(s->level, HIGHEST_LIVE_LEVEL);
		//unknown06 skipped
		//unknown08 skipped
		//unknown16 skipped
		//CheckUnsignedLimit(s->sequence, ULONG_MAX);
		//unknown18 skipped
		CheckUnsigned(s->type, 1, 232);
		//unknown23 skipped
		CheckSkillID(s->spell);
		//unknown29 skipped
		//buff_unknown skipped
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_TributeToggle(const SMPacket *p) {
/*	if(p->size == sizeof(int32)) {
		int32 *s = (int32 *) p->pBuffer;
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_TributeTimer(const SMPacket *p) {
	if(p->size == sizeof(int32)) {
		int32 *s = (int32 *) p->pBuffer;
		return(*s == 600000);
	}
	return(false);
}

bool StructMatcher::Match_OP_ShopRequest(const SMPacket *p) {
	if(p->size == sizeof(Merchant_Click_Struct)) {
		Merchant_Click_Struct *s = (Merchant_Click_Struct *) p->pBuffer;
		CheckMobClass(s->npcid, MERCHANT);
		CheckSelf(s->playerid);
		//unknown skipped
		return(true);
	}
	return(false);
}

//this is prolly too general
bool StructMatcher::Match_OP_ItemLinkClick(const SMPacket *p) {
	if(p->size == sizeof(ItemViewRequest_Struct)) {
		ItemViewRequest_Struct *s = (ItemViewRequest_Struct *) p->pBuffer;
		CheckItemID(s->item_id);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SaveOnZoneReq(const SMPacket *p) {
	if(p->size == sizeof(Save_Struct)) {
		CheckHistoryExists(OP_ZoneChange, CLOSE);
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Animation(const SMPacket *p) {
	if(p->size == sizeof(Animation_Struct)) {
		
		CheckHistoryExists(OP_Emote, CLOSE);
		Animation_Struct *s = (Animation_Struct *) p->pBuffer;
		CheckMobExists(s->spawnid);
		CheckUnsigned(s->action, 4, 15);
		CheckUnsigned(s->value, 1, 47);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_FormattedMessage(const SMPacket *p) {
	if(p->size >= sizeof(FormattedMessage_Struct)) {
		FormattedMessage_Struct *s = (FormattedMessage_Struct *) p->pBuffer;
		//unknown0 skipped
		CheckUnsigned(s->string_id, 1, MAX_STRING_ID);
		CheckUnsignedLimit(s->type, MAX_MESSAGE_COLOR);
		CheckString(s->message, p->size-sizeof(FormattedMessage_Struct));
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_CancelTrade(const SMPacket *p) {
	if(p->size == sizeof(CancelTrade_Struct)) {
		CheckHistoryExists(OP_TradeRequest, CLOSE);
		CancelTrade_Struct *s = (CancelTrade_Struct *) p->pBuffer;
		CheckMobExists(s->fromid);
		CheckUnsignedLimit(s->action, 1);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SetServerFilter(const SMPacket *p) {
/*	if(p->size == sizeof(SetServerFilter_Struct)) {
		SetServerFilter_Struct *s = (SetServerFilter_Struct *) p->pBuffer;
		uint32 * eles = s->filters;
		int r;
		for(r = 0; r < sizeof(*s)/sizeof(uint32); r++, eles++) {
			CheckUnsignedLimit(*eles, 100);
		}

		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_SelectTribute(const SMPacket *p) {
	if(p->size == sizeof(SelectTributeReq_Struct)) {
		SelectTributeReq_Struct *s = (SelectTributeReq_Struct *) p->pBuffer;
		CheckSelf(s->client_id);
		CheckMobClass(s->tribute_id, TRIBUTE_MASTER);
		//unknown8 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TradeCoins(const SMPacket *p) {
	if(p->size == sizeof(TradeCoin_Struct)) {
		TradeCoin_Struct *s = (TradeCoin_Struct *) p->pBuffer;
		CheckPlayer(s->trader);
		CheckUnsignedLimit(s->slot, 80);
		//unknown5 skipped
		//unknown7 skipped
		CheckUnsignedLimit(s->amount, 1000000);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_RequestClientZoneChange(const SMPacket *p) {
	if(p->size == sizeof(RequestClientZoneChange_Struct)) {
		RequestClientZoneChange_Struct *s = (RequestClientZoneChange_Struct *) p->pBuffer;
		CheckZoneID(s->zone_id);
		//CheckUnsignedLimit(s->instance_id, ULONG_MAX);
		CheckCoord(s->y);
		CheckCoord(s->x);
		CheckZCoord(s->z);
		CheckHCoord(s->heading);
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Hide(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_AdventurePointsUpdate(const SMPacket *p) {
/*	if(p->size == sizeof(AdventurePoints_Update_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_Consider(const SMPacket *p) {
	if(p->size == sizeof(Consider_Struct)) {
		Consider_Struct *s = (Consider_Struct *) p->pBuffer;
		CheckPlayer(s->playerid);
		CheckMobExists(s->targetid);
		CheckUnsigned(s->faction, 1, 9);
		CheckUnsigned(s->level, 2, 20);
		CheckSigned(s->cur_hp, -100, MAX_HP);
		CheckSigned(s->max_hp, -100, MAX_HP);
		//CheckUnsignedLimit(s->pvpcon, ULONG_MAX);
		//unknown3 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_MoveDoor(const SMPacket *p) {
	if(p->size == sizeof(MoveDoor_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_GMZoneRequest2(const SMPacket *p) {
/*	if(p->size == sizeof(int32)) {
		int32 *s = (int32 *) p->pBuffer;
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_ReqClientSpawn(const SMPacket *p) {
	if(p->size == 0) {
		CheckHistoryExists(OP_AckPacket, 1);
		im_sure = true;

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GuildPublicNote(const SMPacket *p) {
	if(p->size == sizeof(GuildUpdate_PublicNote)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_LootComplete(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TraderBuy(const SMPacket *p) {
	if(p->size == sizeof(TraderBuy_Struct)) {
		TraderBuy_Struct *s = (TraderBuy_Struct *) p->pBuffer;
		//unknown0 skipped
		CheckPlayer(s->traderid);
		CheckItemID(s->itemid);
		//unknown8 skipped
		//CheckUnsignedLimit(s->price, ULONG_MAX);
		CheckUnsigned(s->quantity, 1, 20);
		CheckUnsignedLimit(s->slot_num, 80);
		CheckString(s->itemname, 60);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_LFGAppearance(const SMPacket *p) {
/*	if(p->size == sizeof(LFG_Appearance_Struct)) {
		LFG_Appearance_Struct *s = (LFG_Appearance_Struct *) p->pBuffer;
		CheckPlayer(s->spawn_id, 0, ULONG_MAX);
		//CheckUnsignedLimit(s->lfg, ULONG_MAX);
		//unknown0005 skipped
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_TributeItem(const SMPacket *p) {
	if(p->size == sizeof(TributeItem_Struct)) {
		TributeItem_Struct *s = (TributeItem_Struct *) p->pBuffer;
		CheckItemSlot(s->slot);
		CheckUnsigned(s->quantity, 1, 20);
		CheckMobClass(s->tribute_master_id, TRIBUTE_MASTER);
		CheckSigned(s->tribute_points, 0, 10000);
		return(true);
	}
	return(false);
}

//todo: implement tracking of ground spawns
//too general without it
bool StructMatcher::Match_OP_ClickObject(const SMPacket *p) {
/*	if(p->size == sizeof(ClickObject_Struct)) {
		ClickObject_Struct *s = (ClickObject_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->drop_id, ULONG_MAX);
		CheckPlayer(s->player_id);
		return(true);
	}
*/	return(false);
}

//might be too general, but we'll give it a shot
bool StructMatcher::Match_OP_InspectRequest(const SMPacket *p) {
	if(p->size == sizeof(Inspect_Struct)) {
		Inspect_Struct *s = (Inspect_Struct *) p->pBuffer;
		CheckPlayer(s->TargetID);
		CheckPlayer(s->PlayerID);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_GetGuildsList(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_Emote(const SMPacket *p) {
	if(p->size == sizeof(Emote_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_MoneyOnCorpse(const SMPacket *p) {
	if(p->size == sizeof(moneyOnCorpseStruct)) {
		moneyOnCorpseStruct *s = (moneyOnCorpseStruct *) p->pBuffer;
		CheckUnsignedLimit(s->response, 2);
		//unknown1 skipped
		//unknown2 skipped
		//unknown3 skipped
		CheckUnsignedLimit(s->platinum, 5000);
		CheckUnsignedLimit(s->gold, 10);
		CheckUnsignedLimit(s->silver, 10);
		CheckUnsignedLimit(s->copper, 10);
		
		//will not match if no loot on mob, but makes us much more sure
		CheckFutureExists(OP_ItemPacket, 1);
		
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_FindPersonRequest(const SMPacket *p) {
	if(p->size == sizeof(FindPersonRequest_Struct)) {
		FindPersonRequest_Struct *s = (FindPersonRequest_Struct *) p->pBuffer;
		CheckMobExists(s->npc_id);
		CheckCoord(s->client_pos.x);
		CheckCoord(s->client_pos.y);
		CheckZCoord(s->client_pos.z);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_GMBecomeNPC(const SMPacket *p) {
/*	if(p->size == sizeof(BecomeNPC_Struct)) {
		BecomeNPC_Struct *s = (BecomeNPC_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->id, ULONG_MAX);
		//CheckSigned(s->maxlevel, LONG_MIN, LONG_MAX);
		return(true);
	}
*/
	return(false);
}

bool StructMatcher::Match_OP_ApproveZone(const SMPacket *p) {
	if(p->size == sizeof(ApproveZone_Struct)) {
		ApproveZone_Struct *s = (ApproveZone_Struct *) p->pBuffer;
		CheckString(s->name, 64);
		CheckZoneID(s->zoneid);
		//CheckUnsignedLimit(s->approve, ULONG_MAX);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Charm(const SMPacket *p) {
	if(p->size == sizeof(Charm_Struct)) {
		Charm_Struct *s = (Charm_Struct *) p->pBuffer;
		CheckMobExists(s->owner_id);
		CheckNPC(s->pet_id);
		CheckUnsignedLimit(s->command, 1);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_PurchaseLeadershipAA(const SMPacket *p) {
/*	if(p->size == sizeof(uint32)) {
		uint32 *s = (uint32 *) p->pBuffer;
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_PetitionBug(const SMPacket *p) {
	if(p->size == sizeof(PetitionBug_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GroupFollow(const SMPacket *p) {
	if(p->size == sizeof(GroupGeneric_Struct)) {
		
		GroupGeneric_Struct *s = (GroupGeneric_Struct *) p->pBuffer;
		CheckString(s->name1, 64);
		CheckString(s->name2, 64);
		
		const SMPacket *old;
		uint32 maxdist = CLOSE;
		CheckHistory(OP_GroupInvite, maxdist, old);
		GroupInvite_Struct *s2 = (GroupInvite_Struct *) old->pBuffer;
		
		//not sure what order they come back in
		if(strcasecmp(s->name1, s2->invitee_name) && strcasecmp(s->name1, s2->inviter_name))
			return(false);
		if(strcasecmp(s->name2, s2->invitee_name) && strcasecmp(s->name2, s2->inviter_name))
			return(false);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_LevelAppearance(const SMPacket *p) {
	if(p->size == sizeof(LevelAppearance_Struct)) {
		LevelAppearance_Struct *s = (LevelAppearance_Struct *) p->pBuffer;
		CheckMobExists(s->spawn_id);
		CheckUnsigned(s->parm1, 0x4D, 0x52);
		CheckUnsigned(s->value1a, 1, 2);
		CheckUnsigned(s->value1b, 1, 2);
		CheckUnsigned(s->parm2, 0x4D, 0x52);
		CheckUnsigned(s->value2a, 1, 2);
		CheckUnsigned(s->value2b, 1, 2);
		CheckUnsigned(s->parm3, 0x4D, 0x52);
		CheckUnsigned(s->value3a, 1, 2);
		CheckUnsigned(s->value3b, 1, 2);
		CheckUnsigned(s->parm4, 0x4D, 0x52);
		CheckUnsigned(s->value4a, 1, 2);
		CheckUnsigned(s->value4b, 1, 2);
		CheckUnsigned(s->parm5, 0x4D, 0x52);
		CheckUnsigned(s->value5a, 1, 2);
		CheckUnsigned(s->value5b, 1, 2);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_IncreaseStats(const SMPacket *p) {
	if(p->size == sizeof(IncreaseStat_Struct)) {
		IncreaseStat_Struct *s = (IncreaseStat_Struct *) p->pBuffer;
		
		//unknown0 skipped
		CheckUnsigned(s->str, 1, 250);
		CheckUnsigned(s->sta, 1, 250);
		CheckUnsigned(s->agi, 1, 250);
		CheckUnsigned(s->dex, 1, 250);
		CheckUnsigned(s->int_, 1, 250);
		CheckUnsigned(s->wis, 1, 250);
		CheckUnsigned(s->cha, 1, 250);
		CheckUnsigned(s->fire, 1, 250);
		CheckUnsigned(s->cold, 1, 250);
		CheckUnsigned(s->magic, 1, 250);
		CheckUnsigned(s->poison, 1, 250);
		CheckUnsigned(s->disease, 1, 250);
		//unknown13 skipped
		CheckUnsigned(s->str2, 1, 250);
		CheckUnsigned(s->sta2, 1, 250);
		CheckUnsigned(s->agi2, 1, 250);
		CheckUnsigned(s->dex2, 1, 250);
		CheckUnsigned(s->int_2, 1, 250);
		CheckUnsigned(s->wis2, 1, 250);
		CheckUnsigned(s->cha2, 1, 250);
		CheckUnsigned(s->fire2, 1, 250);
		CheckUnsigned(s->cold2, 1, 250);
		CheckUnsigned(s->magic2, 1, 250);
		CheckUnsigned(s->poison2, 1, 250);
		CheckUnsigned(s->disease2, 1, 250);
		return(true);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Illusion(const SMPacket *p) {
	if(p->size == sizeof(Illusion_Struct)) {
		Illusion_Struct *s = (Illusion_Struct *) p->pBuffer;
		
		CheckMobExists(s->spawnid);
		CheckString(s->charname, 64);
		CheckUnsigned(s->race, 1, 1000);
		//unknown006 skipped
		CheckUnsignedLimit(s->gender, 2);
		//CheckUnsigned(s->texture, 0, ULONG_MAX);
		//CheckUnsigned(s->helmtexture, 0, ULONG_MAX);
		//unknown011 skipped
		//CheckUnsigned(s->face, 0, ULONG_MAX);
		//unknown020 skipped
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GMFind(const SMPacket *p) {
	if(p->size == sizeof(GMSummon_Struct)) {
		GMSummon_Struct *s = (GMSummon_Struct *) p->pBuffer;
		CheckString(s->charname, 64);
		CheckString(s->gmname, 64);
		CheckBool(s->success);
		CheckZoneID(s->zoneID);
		CheckCoord(s->y);
		CheckCoord(s->x);
		CheckZCoord(s->z);
		//unknown2 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ZoneSpawns(const SMPacket *p) {
	if(p->size >= sizeof(Spawn_Struct)) {
		//make sure the length is a multiple of the size
		if ((p->size % sizeof(NewSpawn_Struct)) != 0)
			return(false);
		
		//just look at the first entry, fake the packet
		SMPacket p2(p->eq_opcode, p->opcode, 0);
		p2.size = sizeof(Spawn_Struct);
		p2.pBuffer = p->pBuffer;
		bool res = Match_OP_NewSpawn(&p2);
		p2.pBuffer = NULL;
		return(res);
	}
	return(false);
}

bool StructMatcher::Match_OP_NewSpawn(const SMPacket *p) {
	if(p->size == sizeof(NewSpawn_Struct)) {
		NewSpawn_Struct *s2 = (NewSpawn_Struct *) p->pBuffer;
		Spawn_Struct *s = &s2->spawn;
		
		CheckUnsignedLimit(s->NPC, 5);
		//CheckUnsignedLimit(s->beard, ULONG_MAX);
		//CheckUnsignedLimit(s->beardcolor, ULONG_MAX);
		//CheckUnsignedLimit(s->aa_title, ULONG_MAX);
		//uint32 idx_dye_rgb;
		//for(idx_dye_rgb = 0; idx_dye_rgb < 7; idx_dye_rgb++) {
			//NO_MATCHER(s->dye_rgb[idx_dye_rgb]);
		//}
		//unknown032 skipped
		CheckUnsignedLimit(s->class_, 64);
		//unknown044 skipped
		CheckUnsignedLimit(s->curHp, 100);
//		CheckBool(s->afk);
		//CheckUnsignedLimit(s->texture, ULONG_MAX);
		CheckUnsignedLimit(s->race, 1000);
		//CheckUnsignedLimit(s->eyecolor1, ULONG_MAX);
		CheckString(s->name, 64);
		//CheckUnsignedLimit(s->eyecolor2, ULONG_MAX);
		//CheckUnsignedLimit(s->face, ULONG_MAX);
		CheckBool(s->invis);
		//CheckUnsignedLimit(s->max_hp, ULONG_MAX);
		//unknown122 skipped
		CheckUnsigned(s->level, 1, HIGHEST_LIVE_LEVEL);
		CheckBool(s->lfg);
		CheckHCoord13(s->heading);
		//CheckSigned(s->deltaX, LONG_MIN, LONG_MAX);
		CheckCoord19(s->x);
		//CheckSigned(s->deltaZ, LONG_MIN, LONG_MAX);
		CheckCoord19(s->y);
		//CheckSigned(s->deltaY, LONG_MIN, LONG_MAX);
		CheckZCoord19(s->z);
		//CheckUnsignedLimit(s->hairstyle, ULONG_MAX);
		//CheckUnsignedLimit(s->haircolor, ULONG_MAX);
//		CheckBool(s->invis2);
		//unknown144 skipped
//		CheckBool(s->pvp);
		//CheckUnsignedLimit(s->light, ULONG_MAX);
		CheckFloat(s->size, -1e7, 1e7, -5, 7);
		//CheckUnsignedLimit(s->helm, ULONG_MAX);
		CheckFloat(s->runspeed, 0, 1e3, -3, 3);
//		CheckUnsignedLimit(s->gm, 1);
		CheckFloat(s->walkspeed, 0, 1e3, -3, 3);
		CheckUnsignedLimit(s->guildID, ULONG_MAX);
		//CheckUnsignedLimit(s->anon, MAX_GUILD_ID);
		CheckUnsignedLimit(s->gender, 2);
		//CheckUnsignedLimit(s->spawn_id, ULONG_MAX);
		//unknown173 skipped
		CheckStringEmpty(s->lastName, 32);
		uint32 idx_equipment;
		for(idx_equipment = 0; idx_equipment < 9; idx_equipment++) {
			//CheckUnsignedLimit(s->equipment[idx_equipment], ULONG_MAX);
		}
//		CheckUnsignedLimit(s->linkdead, 1);
		CheckUnsignedLimit(s->bodytype, 67);
		//CheckUnsignedLimit(s->guild_rank, ULONG_MAX);
		//unknown249 skipped
		//CheckUnsignedLimit(s->pet_owner_id, ULONG_MAX);
		CheckUnsignedLimit(s->deity, 400);
		//unknown260 skipped
		//CheckUnsignedLimit(s->findable, ULONG_MAX);
		//unknown_float skipped
		//unknown272 skipped
		CheckStringEmpty(s->title, 48);
		//unknown355 skipped
		//unknown367 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_BankerChange(const SMPacket *p) {
	if(p->size == sizeof(BankerChange_Struct)) {
		BankerChange_Struct *s = (BankerChange_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->platinum, 20000);
		CheckUnsignedLimit(s->gold, 1000);
		CheckUnsignedLimit(s->silver, 1000);
		CheckUnsignedLimit(s->copper, 1000);
		//CheckUnsignedLimit(s->platinum_bank, ULONG_MAX);
		//CheckUnsignedLimit(s->gold_bank, ULONG_MAX);
		//CheckUnsignedLimit(s->silver_bank, ULONG_MAX);
		//CheckUnsignedLimit(s->copper_bank, ULONG_MAX);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_CloseContainer(const SMPacket *p) {
	if(p->size == sizeof(ClickObjectAck_Struct)) {
		CheckHistoryExists(OP_ClickObject, DISTANT);
		
		ClickObjectAck_Struct *s = (ClickObjectAck_Struct *) p->pBuffer;
		CheckSelf(s->player_id);
		//CheckUnsignedLimit(s->drop_id, ULONG_MAX);
		CheckBool(s->open);
		CheckUnsigned(s->type, 1, 0x35);
		//unknown16 skipped
		CheckUnsigned(s->icon, 1, 1200);
		//unknown24 skipped
		CheckString(s->object_name, 32);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Damage(const SMPacket *p) {
	if(p->size == sizeof(CombatDamage_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ClientReady(const SMPacket *p) {
	if(p->size == 0) {
		CheckHistoryExists(OP_SetServerFilter, CLOSE);

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SetGuildMOTD(const SMPacket *p) {
	if(p->size == sizeof(GuildMOTD_Struct)) {
		//always will happen after initial get motd from zone in
		
		CheckHistoryExists(OP_GuildMOTD, COMPLETE);
		
		GuildMOTD_Struct *s = (GuildMOTD_Struct *) p->pBuffer;
		//unknown0 skipped
		CheckString(s->name, 64);
		CheckString(s->setby_name, 64);
		//unknown132 skipped
		CheckString(s->motd, 512);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GroupInvite2(const SMPacket *p) {
	if(p->size == sizeof(GroupInvite_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_AdventureData(const SMPacket *p) {
	if(p->size == sizeof(AdventureRequestResponse_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TrackUnknown(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
	return(false);
}

bool StructMatcher::Match_OP_SendAAStats(const SMPacket *p) {
	if(p->size == 0) {
		CheckHistoryExists(OP_ReqNewZone, CLOSE);

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_BeginCast(const SMPacket *p) {
	if(p->size == sizeof(BeginCast_Struct)) {
		BeginCast_Struct *s = (BeginCast_Struct *) p->pBuffer;
		CheckMobExists(s->caster_id);
		CheckSpellID(s->spell_id);
		CheckUnsignedLimit(s->cast_time, 3600);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_AutoAttack(const SMPacket *p) {
/*	if(p->size == sizeof(uint32)) {
		uint32 *s = (uint32 *) p->pBuffer;
		return(true);
	}
*/	return(false);
}

//chances of seeing this in a regular log are slim to none
//and it is exactly the same as the more useful MobRename
bool StructMatcher::Match_OP_GMLastName(const SMPacket *p) {
/*	if(p->size == sizeof(GMLastName_Struct)) {
		GMLastName_Struct *s = (GMLastName_Struct *) p->pBuffer;
		CheckString(s->name, 64);
		CheckString(s->gmname, 64);
		CheckString(s->lastname, 64);
		//unknown skipped
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_MobRename(const SMPacket *p) {
	if(p->size == sizeof(MobRename_Struct)) {
		MobRename_Struct *s = (MobRename_Struct *) p->pBuffer;
		CheckString(s->old_name, 64);
		CheckString(s->old_name_again, 64);
		CheckString(s->new_name, 64);
		
		if(s->unknown192 != 0) return(false);
		if(s->unknown196 != 1) return(false);
		//unknown skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_RecipesSearch(const SMPacket *p) {
	if(p->size == sizeof(RecipesSearch_Struct)) {
		CheckHistoryExists(OP_RecipesFavorite, COMPLETE);
		
		RecipesSearch_Struct *s = (RecipesSearch_Struct *) p->pBuffer;
		CheckUnsigned(s->object_type, 1, 0x35);
		//CheckUnsignedLimit(s->some_id, ULONG_MAX);
		CheckUnsignedLimit(s->mintrivial, 500);
		CheckUnsignedLimit(s->maxtrivial, 500);
		CheckString(s->query, 56);
		//unknown4 skipped
		//unknown5 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GMTraining(const SMPacket *p) {
	if(p->size == sizeof(GMTrainee_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TradeSkillCombine(const SMPacket *p) {
	if(p->size == sizeof(NewCombine_Struct)) {
		//only checks for world containers to be less general
		
		CheckHistoryExists(OP_RecipesFavorite, DISTANT);
		
		NewCombine_Struct *s = (NewCombine_Struct *) p->pBuffer;
//		if(s->container_slot != SLOT_TRADESKILL && (s->container_slot < SLOT_PERSONAL_BEGIN || s->container_slot > SLOT_PERSONAL_END)
		if(s->container_slot != SLOT_TRADESKILL)
			return(false);
		//unknown02 skipped
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_RandomReq(const SMPacket *p) {
/*	if(p->size == sizeof(RandomReq_Struct)) {
		RandomReq_Struct *s = (RandomReq_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->low, ULONG_MAX);
		//CheckUnsignedLimit(s->high, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_GMEmoteZone(const SMPacket *p) {
	if(p->size == sizeof(GMEmoteZone_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SpawnAppearance(const SMPacket *p) {
	if(p->size == sizeof(SpawnAppearance_Struct)) {
		SpawnAppearance_Struct *s = (SpawnAppearance_Struct *) p->pBuffer;
		if(s->spawn_id == 0) {
			CheckHistoryExists(OP_ReqNewZone, CLOSE);
			im_sure = true;
			return(true);
		}
		
//		CheckMobExists(s->spawn_id);
		//these will most likely be from players
		//also solves ambiguity with OP_AnnoyingZoneUnknown
		CheckPlayer(s->spawn_id);
		
		CheckUnsignedLimit(s->type, 30);
		CheckUnsignedLimit(s->parameter, 350);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_TraderDelItem(const SMPacket *p) {
/*	if(p->size == sizeof(TraderDelItem_Struct)) {
		TraderDelItem_Struct *s = (TraderDelItem_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->slotid, ULONG_MAX);
		//CheckUnsignedLimit(s->quantity, ULONG_MAX);
		//unknown skipped
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_GroupFollow2(const SMPacket *p) {
	if(p->size == sizeof(GroupGeneric_Struct)) {
		GroupGeneric_Struct *s = (GroupGeneric_Struct *) p->pBuffer;
		CheckString(s->name1, 64);
		CheckString(s->name2, 64);
		
		const SMPacket *old;
		uint32 maxdist = CLOSE;
		CheckHistory(OP_GroupInvite2, maxdist, old);
		GroupInvite_Struct *s2 = (GroupInvite_Struct *) old->pBuffer;
		
		//not sure what order they come back in
		if(strcasecmp(s->name1, s2->invitee_name) && strcasecmp(s->name1, s2->inviter_name))
			return(false);
		if(strcasecmp(s->name2, s2->invitee_name) && strcasecmp(s->name2, s2->inviter_name))
			return(false);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_BoardBoat(const SMPacket *p) {
/*	if(p->size == sizeof(EntityId_Struct)) {
		EntityId_Struct *s = (EntityId_Struct *) p->pBuffer;
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_Camp(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
	return(false);
}

bool StructMatcher::Match_OP_LeadershipExpUpdate(const SMPacket *p) {
	if(p->size == sizeof(LeadershipExpUpdate_Struct)) {
		CheckHistoryExists(OP_Death, CLOSE);
		LeadershipExpUpdate_Struct *s = (LeadershipExpUpdate_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->group_leadership_exp, 1000);
		CheckUnsignedLimit(s->group_leadership_points, 10);
		CheckUnsignedLimit(s->raid_leadership_exp, 2000);
		CheckUnsignedLimit(s->raid_leadership_points, 10);
		if(s->group_leadership_exp+s->raid_leadership_exp == 0) return(false);
		
		//prolly need to check more, this is still pretty general
		
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_SenseTraps(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_TradeMoneyUpdate(const SMPacket *p) {
/*	if(p->size == sizeof(TradeMoneyUpdate_Struct)) {
		TradeMoneyUpdate_Struct *s = (TradeMoneyUpdate_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->trader, ULONG_MAX);
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		//CheckUnsignedLimit(s->amount, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_Bug(const SMPacket *p) {
	if(p->size == sizeof(BugStruct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Stamina(const SMPacket *p) {
	if(p->size == sizeof(Stamina_Struct)) {
		//attempt at seperating this, it almost ALWAYS follows a HP Update
		CheckHistoryExists(OP_HPUpdate, REGULAR);
		
		Stamina_Struct *s = (Stamina_Struct *) p->pBuffer;
		CheckUnsigned(s->food, 3000, 18000);
		CheckUnsigned(s->water, 3000, 18000);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GMZoneRequest(const SMPacket *p) {
	if(p->size == sizeof(GMZoneRequest_Struct)) {
		GMZoneRequest_Struct *s = (GMZoneRequest_Struct *) p->pBuffer;
		CheckString(s->charname, 64);
		CheckZoneID(s->zone_id);
		CheckCoord(s->x);
		CheckCoord(s->y);
		CheckZCoord(s->z);
		//unknown0080 skipped
		//CheckUnsignedLimit(s->success, ULONG_MAX);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Taunt(const SMPacket *p) {
/*	if(p->size == sizeof(ClientTarget_Struct)) {
		ClientTarget_Struct *s = (ClientTarget_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->new_target, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_Fishing(const SMPacket *p) {
	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
	return(false);
}

bool StructMatcher::Match_OP_CombatAbility(const SMPacket *p) {
	if(p->size == sizeof(CombatAbility_Struct)) {
		CombatAbility_Struct *s = (CombatAbility_Struct *) p->pBuffer;
		CheckMobExists(s->m_target);
		CheckUnsignedLimit(s->m_atk, 101);
		CheckSkillID(s->m_skill);
		return(true);
	}
	return(false);
}

//needs a real matcher
bool StructMatcher::Match_OP_PetitionResolve(const SMPacket *p) {
/*	if(p->size == sizeof(PetitionUpdate_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_HPUpdate(const SMPacket *p) {
	if(p->size == sizeof(SpawnHPUpdate_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_LootItem(const SMPacket *p) {
	if(p->size == sizeof(LootingItem_Struct)) {
		LootingItem_Struct *s = (LootingItem_Struct *) p->pBuffer;
		CheckMobExists(s->lootee);
		CheckMobExists(s->looter);
		CheckUnsignedLimit(s->slot_id, 80);
		//unknown3 skipped
		CheckBool(s->auto_loot);
		return(true);
	}
	return(false);
}

//needs a matcher
bool StructMatcher::Match_OP_PetitionDelete(const SMPacket *p) {
/*	if(p->size == sizeof(PetitionUpdate_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_Logout(const SMPacket *p) {
	if(p->size == 0) {
		CheckHistoryExists(OP_SaveOnZoneReq, 5);

		return(false);
	}
	return(false);
}

bool StructMatcher::Match_OP_AdventureMerchantPurchase(const SMPacket *p) {
	if(p->size == sizeof(Adventure_Purchase_Struct)) {
		Adventure_Purchase_Struct *s = (Adventure_Purchase_Struct *) p->pBuffer;
		CheckMobClass(s->npcid, ADVENTUREMERCHANT);
		CheckItemID(s->itemid);
		//CheckUnsignedLimit(s->variable, ULONG_MAX);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Assist(const SMPacket *p) {
/*	if(p->size == sizeof(EntityId_Struct)) {
		EntityId_Struct *s = (EntityId_Struct *) p->pBuffer;
		CheckMobExists(s->entity_id, entity_id);
		return(true);
	}
*/	return(false);
}

//im too lazy to write a matcher for this
bool StructMatcher::Match_OP_Dye(const SMPacket *p) {
/*	if(p->size == sizeof(DyeStruct)) {
		DyeStruct *s = (DyeStruct *) p->pBuffer;
		return(true);
	} else if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_GuildManageRemove(const SMPacket *p) {
	if(p->size == sizeof(GuildManageRemove_Struct)) {
		GuildManageRemove_Struct *s = (GuildManageRemove_Struct *) p->pBuffer;
		CheckUnsigned(s->guildeqid, 1, MAX_GUILD_ID);
		CheckString(s->member, 64);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_PetitionCheckIn(const SMPacket *p) {
	if(p->size == sizeof(Petition_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_RaidUpdate(const SMPacket *p) {
	if(p->size == sizeof(ZoneInSendName_Struct)) {
		//inactive raid
		ZoneInSendName_Struct *s = (ZoneInSendName_Struct *) p->pBuffer;
		//unknown0 skipped
		CheckString(s->name, 64);
		CheckString(s->name2, 64);
		//unknown132 skipped
		return(true);
	} else if(p->size >= sizeof(RaidMembers_Struct)) {
		//active raid
		RaidMembers_Struct *s = (RaidMembers_Struct *) p->pBuffer;
		CheckUnsigned(s->details.action, 6, 20);
		CheckString(s->details.leader_name, 64);
		
		//should verify all the raid members
		CheckUnsignedLimit(s->members[0].group_number, 10);
		CheckString(s->members[0].member_name, 64);
	}
	return(false);
}

bool StructMatcher::Match_OP_GMKill(const SMPacket *p) {
	if(p->size == sizeof(GMKill_Struct)) {
		GMKill_Struct *s = (GMKill_Struct *) p->pBuffer;
		CheckString(s->name, 64);
		CheckString(s->gmname, 64);
		//unknown skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_EnvDamage(const SMPacket *p) {
	if(p->size == sizeof(EnvDamage2_Struct)) {
		EnvDamage2_Struct *s = (EnvDamage2_Struct *) p->pBuffer;
		CheckSelf(s->id);
		//unknown4 skipped
		CheckUnsignedLimit(s->damage, MAX_HP);
		//unknown10 skipped
		//CheckUnsignedLimit(s->dmgtype, ULONG_MAX);
		//unknown2 skipped
		if(s->constant != 0xFFFF) return(false);
		//unknown29 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GMTrainSkill(const SMPacket *p) {
	if(p->size == sizeof(GMSkillChange_Struct)) {
		GMSkillChange_Struct *s = (GMSkillChange_Struct *) p->pBuffer;
		
		const MobInfoStruct *mob;
		CheckMob(s->npcid, mob);
		
		if(mob->_class < WARRIORGM || mob->_class > BERSERKERGM) return(false);
		//unknown1 skipped
		CheckUnsignedLimit(s->skillbank, 1);
		//unknown2 skipped
		CheckSkillID(s->skill_id);
		//unknown3 skipped
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_TargetMouse(const SMPacket *p) {
/*	if(p->size == sizeof(ClientTarget_Struct)) {
//		ClientTarget_Struct *s = (ClientTarget_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->new_target, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_MoveItem(const SMPacket *p) {
	if(p->size == sizeof(MoveItem_Struct)) {
		MoveItem_Struct *s = (MoveItem_Struct *) p->pBuffer;
		CheckItemSlot(s->from_slot);
		CheckItemSlot(s->to_slot);
		CheckUnsigned(s->number_in_stack, 1, 20);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_WearChange(const SMPacket *p) {
	if(p->size == sizeof(WearChange_Struct)) {
		WearChange_Struct *s = (WearChange_Struct *) p->pBuffer;
		CheckMobExists(s->spawn_id);
		CheckUnsignedLimit(s->material, MAX_MATERIALS);
		//NO_MATCHER(s->color);
		CheckUnsignedLimit(s->wear_slot_id, SLOT_PERSONAL_BEGIN-1);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_RecipeAutoCombine(const SMPacket *p) {
	if(p->size == sizeof(RecipeAutoCombine_Struct)) {
		CheckHistoryExists(OP_RecipeDetails, DISTANT);
		
		RecipeAutoCombine_Struct *s = (RecipeAutoCombine_Struct *) p->pBuffer;
		CheckUnsigned(s->object_type, 1, 0x35);
		//CheckUnsignedLimit(s->some_id, ULONG_MAX);
		//unknown1 skipped
		CheckUnsigned(s->recipe_id, 500, MAX_RECIPE_ID);
		//CheckUnsignedLimit(s->reply_code, ULONG_MAX);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Bind_Wound(const SMPacket *p) {
/*	if(p->size == sizeof(BindWound_Struct)) {
		BindWound_Struct *s = (BindWound_Struct *) p->pBuffer;
		CheckMobExists(s->to);
		//unknown2 skipped
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		//unknown6 skipped
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_AdventureMerchantRequest(const SMPacket *p) {
	if(p->size == sizeof(AdventureMerchant_Struct)) {
		AdventureMerchant_Struct *s = (AdventureMerchant_Struct *) p->pBuffer;
		CheckMobClass(s->entity_id, ADVENTUREMERCHANT);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SendExpZonein(const SMPacket *p) {
	if(p->size == 0) {
		//should be the first 0 length packet after NewZone
		
		//will not find this in zones without zone points (guild hall)
		CheckHistoryExists(OP_SendZonepoints, CLOSE);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_WhoAllRequest(const SMPacket *p) {
	if(p->size == sizeof(Who_All_Struct)) {
		Who_All_Struct *s = (Who_All_Struct *) p->pBuffer;
		CheckString(s->whom, 64);
		CheckUnsigned(s->wrace, 1, 1024);	//arbitrary upper bound
		CheckUnsignedLimit(s->wclass, 64);
		CheckUnsignedLimit(s->lvllow, 0xFFFF);
		CheckUnsigned(s->lvlhigh, 1, 0xFFFF);
		CheckUnsignedLimit(s->gmlookup, 0xFFFF);
		//unknown074 skipped
		//unknown076 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ExpUpdate(const SMPacket *p) {
	if(p->size == sizeof(ExpUpdate_Struct)) {
		CheckHistoryExists(OP_SendExpZonein, CLOSE);
		
		ExpUpdate_Struct *s = (ExpUpdate_Struct *) p->pBuffer;
		
/*printf("eup\n");
		const SMPacket *old;
		uint32 maxdist = DISTANT;
		CheckHistory(OP_PlayerProfile, maxdist, old);
		PlayerProfile_Struct *pp = (PlayerProfile_Struct *) old->pBuffer;
		
printf("ppup\n");
printf("cexp %u %u\n", pp->exp, s->exp);
		if(pp->exp != s->exp) return(false);*/
		CheckUnsignedLimit(s->exp, 330);
		
		//not sure if this is valid
		if(s->aaxp != 0) return(false);	
		//CheckUnsignedLimit(s->aaxp, ULONG_MAX);
		
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_InspectAnswer(const SMPacket *p) {
/*	if(p->size == sizeof(Inspect_Struct)) {
		Inspect_Struct *s = (Inspect_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->TargetID, ULONG_MAX);
		//CheckUnsignedLimit(s->PlayerID, ULONG_MAX);
		return(true);
	}
*/
	return(false);
}

bool StructMatcher::Match_OP_ShopPlayerBuy(const SMPacket *p) {
	if(p->size == sizeof(Merchant_Sell_Struct)) {
		Merchant_Sell_Struct *s = (Merchant_Sell_Struct *) p->pBuffer;
		CheckMobClass(s->npcid, MERCHANT);
		CheckSelf(s->playerid);
		CheckUnsignedLimit(s->itemslot, 80);
		//unknown12 skipped
		CheckUnsigned(s->quantity, 1, 20);
		//Unknown016 skipped
		CheckUnsigned(s->price, 1, 1000000);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Forage(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
*/	return(false);
}

//too general, not sure if this is even used anymore
bool StructMatcher::Match_OP_Save(const SMPacket *p) {
/*	if(p->size == sizeof(Save_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_LDoNButton(const SMPacket *p) {
/*	if(p->size == sizeof(bool)) {
		bool *s = (bool *) p->pBuffer;
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_ReqNewZone(const SMPacket *p) {
	if(p->size == 0) {
		CheckHistoryExists(OP_Weather, REGULAR);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ClickObjectAck(const SMPacket *p) {
	if(p->size == sizeof(ClickObjectAck_Struct)) {
		ClickObjectAck_Struct *s = (ClickObjectAck_Struct *) p->pBuffer;
		CheckSelf(s->player_id);
		//CheckUnsignedLimit(s->drop_id, ULONG_MAX);
		//CheckUnsignedLimit(s->open, ULONG_MAX);
		CheckUnsigned(s->type, 1, 0x35);
		//unknown16 skipped
		CheckUnsigned(s->icon, 1, 1200);
		//unknown24 skipped
		CheckString(s->object_name, 32);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_RequestDuel(const SMPacket *p) {
/*	if(p->size == sizeof(Duel_Struct)) {
		Duel_Struct *s = (Duel_Struct *) p->pBuffer;
		CheckPlayer(s->duel_initiator);
		CheckPlayer(s->duel_target);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_SenseHeading(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);
	}
*/
	return(false);
}

//too general
bool StructMatcher::Match_OP_GroupCancelInvite(const SMPacket *p) {
/*	if(p->size == sizeof(GroupGeneric_Struct)) {
		GroupGeneric_Struct *s = (GroupGeneric_Struct *) p->pBuffer;
		CheckString(s->name1, 64);
		CheckString(s->name2, 64);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_Consume(const SMPacket *p) {
/*	if(p->size == sizeof(Consume_Struct)) {
		Consume_Struct *s = (Consume_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->slot, ULONG_MAX);
		//CheckUnsignedLimit(s->auto_consumed, ULONG_MAX);
		//c_unknown1 skipped
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		//unknown13 skipped
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_MemorizeSpell(const SMPacket *p) {
	if(p->size == sizeof(MemorizeSpell_Struct)) {
		MemorizeSpell_Struct *s = (MemorizeSpell_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->slot, 10);
		CheckSpellID(s->spell_id);
		CheckUnsignedLimit(s->scribing, 3);
		//unknown12 skipped
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Surname(const SMPacket *p) {
/*	if(p->size == sizeof(Surname_Struct)) {
		Surname_Struct *s = (Surname_Struct *) p->pBuffer;
		CheckString(s->name, 64);
		//unknown0064 skipped
		CheckString(s->lastname, 32);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_ClientUpdate(const SMPacket *p) {
	if(p->size == sizeof(PlayerPositionUpdateClient_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	} else if(p->size == sizeof(PlayerPositionUpdateServer_Struct)) {
		PlayerPositionUpdateServer_Struct *s = (PlayerPositionUpdateServer_Struct *) p->pBuffer;
		CheckMobExists(s->spawn_id);
		CheckHCoord13(s->heading);
		CheckCoord19(s->x_pos);
		CheckCoord19(s->y_pos);
		CheckZCoord19(s->z_pos);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_BazaarInspect(const SMPacket *p) {
	if(p->size == sizeof(BazaarInspect_Struct)) {
		BazaarInspect_Struct *s = (BazaarInspect_Struct *) p->pBuffer;
		CheckItemID(s->item_id);
		//unknown skipped
		CheckString(s->name, 64);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_BuffFadeMsg(const SMPacket *p) {
	if(p->size >= sizeof(BuffFadeMsg_Struct)) {
		if(p->size > 100) return(false);	//not likely at all
		BuffFadeMsg_Struct *s = (BuffFadeMsg_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->color, MAX_MESSAGE_COLOR);
		
		CheckStringExact(s->msg, p->size-sizeof(BuffFadeMsg_Struct)+1);
		
		//check for |'s to make sure its not an item
		CheckNotCharCount(s->msg, '|', 5);
		
		//force it to happen in the midst of normal play:
		CheckHistoryExists(OP_HPUpdate, CLOSE);
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_NewZone(const SMPacket *p) {
	if(p->size == sizeof(NewZone_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_DeleteSpawn(const SMPacket *p) {
	if(p->size == sizeof(EntityId_Struct)) {
		EntityId_Struct *s = (EntityId_Struct *) p->pBuffer;
		CheckMobExists(s->entity_id);
		CheckHistoryExists(OP_Death, 2);
		//also happens right after OP_SaveOnZoneReq
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GuildInvite(const SMPacket *p) {
	if(p->size == sizeof(GuildCommand_Struct)) {
		GuildCommand_Struct *s = (GuildCommand_Struct *) p->pBuffer;
		CheckString(s->othername, 64);
		CheckString(s->myname, 64);
		CheckUnsigned(s->guildeqid, 1, MAX_GUILD_ID);
		//unknown skipped
		//CheckUnsignedLimit(s->officer, ULONG_MAX);
		return(true);
	}
	return(false);
}

//too unknown
bool StructMatcher::Match_OP_AdventureFinish(const SMPacket *p) {
/*	if(p->size == sizeof(AdventureFinish_Struct)) {
		AdventureFinish_Struct *s = (AdventureFinish_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->win_lose, ULONG_MAX);
		//CheckUnsignedLimit(s->points, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_GMDelCorpse(const SMPacket *p) {
/*	if(p->size == sizeof(GMDelCorpse_Struct)) {
		GMDelCorpse_Struct *s = (GMDelCorpse_Struct *) p->pBuffer;
		CheckString(s->corpsename, 64);
		CheckString(s->gmname, 64);
		//unknown skipped
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_PickPocket(const SMPacket *p) {
	if(p->size == sizeof(PickPocket_Struct)) {
		PickPocket_Struct *s = (PickPocket_Struct *) p->pBuffer;
		CheckMobExists(s->to);
		CheckMobExists(s->from);
		CheckUnsigned(s->myskill, 1, 500);
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		//unknown1 skipped
		//CheckUnsignedLimit(s->coin, ULONG_MAX);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_AugmentItem(const SMPacket *p) {
/*	if(p->size == sizeof(AugmentItem_Struct)) {
		AugmentItem_Struct *s = (AugmentItem_Struct *) p->pBuffer;
		//CheckSigned(s->container_slot, LONG_MIN, LONG_MAX);
		//unknown02 skipped
		//CheckSigned(s->augment_slot, LONG_MIN, LONG_MAX);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_GuildManageAdd(const SMPacket *p) {
	if(p->size == sizeof(GuildJoin_Struct)) {
		GuildJoin_Struct *s = (GuildJoin_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->guildid, MAX_GUILD_ID);
		//unknown04 skipped
		CheckUnsigned(s->level, 1, HIGHEST_LIVE_LEVEL);
		CheckUnsignedLimit(s->class_, 64);
		//CheckUnsignedLimit(s->rank, ULONG_MAX);
		CheckZoneID(s->zoneid);
		//unknown24 skipped
		CheckString(s->name, 64);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_GuildPeace(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);

		return(true);
	}
*/	return(false);
}

//too unknown
bool StructMatcher::Match_OP_TraderItemUpdate(const SMPacket *p) {
/*	if(p->size == sizeof(TraderItemUpdate_Struct)) {
		TraderItemUpdate_Struct *s = (TraderItemUpdate_Struct *) p->pBuffer;
		//unknown0 skipped
		CheckPlayer(s->traderid, 0, ULONG_MAX);
		//not sure on the bounds of these:
		CheckUnsignedLimit(s->fromslot, 2500);
		CheckUnsignedLimit(s->toslot, 2500);
		CheckUnsigned(s->charges, 1, 20);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_GMHideMe(const SMPacket *p) {
/*	if(p->size == sizeof(SpawnAppearance_Struct)) {
		SpawnAppearance_Struct *s = (SpawnAppearance_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->spawn_id, ULONG_MAX);
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		//CheckUnsignedLimit(s->parameter, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_SendTributes(const SMPacket *p) {
/*	if(p->size == 0) {

		return(false);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_GroupInvite(const SMPacket *p) {
	if(p->size == sizeof(GroupInvite_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_PetitionCheckout(const SMPacket *p) {
	if(p->size == sizeof(Petition_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Track(const SMPacket *p) {
/*	if(p->size == sizeof(Track_Struct)) {
		Track_Struct *s = (Track_Struct *) p->pBuffer;
		CheckMobExists(s->entityid);
		CheckCoord(float(s->y));
		CheckCoord(float(s->x));
		CheckZCoord(float(s->z));
		return(true);
	}
*/	return(false);
}

//too general and im getting lazy
bool StructMatcher::Match_OP_DuelResponse2(const SMPacket *p) {
/*	if(p->size == sizeof(Duel_Struct)) {
		Duel_Struct *s = (Duel_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->duel_initiator, ULONG_MAX);
		//CheckUnsignedLimit(s->duel_target, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_ReadBook(const SMPacket *p) {
	if(p->size > sizeof(BookText_Struct)) {
		BookText_Struct *s = (BookText_Struct *) p->pBuffer;
		if(s->unknown0 != 0xFF) return(false);
		CheckUnsignedLimit(s->type, 5);
		CheckString(s->booktext, p->size - sizeof(BookText_Struct)+1);
		
		return(true);
	}
	return(false);
}

//could verify valid door id
bool StructMatcher::Match_OP_ClickDoor(const SMPacket *p) {
	if(p->size == sizeof(ClickDoor_Struct)) {
		ClickDoor_Struct *s = (ClickDoor_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->doorid, 350);
		//CheckUnsignedLimit(s->picklockskill, ULONG_MAX);
		//unknown005 skipped
		CheckItemID(s->item_id);
		CheckPlayer(s->player_id);
		//unknown014 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ConsiderCorpse(const SMPacket *p) {
	if(p->size == sizeof(Consider_Struct)) {
		Consider_Struct *s = (Consider_Struct *) p->pBuffer;
		CheckSelf(s->playerid);
		CheckCorpse(s->targetid);
		CheckUnsignedLimit(s->faction, 9);
		CheckUnsignedLimit(s->level, HIGHEST_LIVE_LEVEL);
		CheckSigned(s->cur_hp, -100, MAX_HP);
		CheckSigned(s->max_hp, -100, MAX_HP);
		//CheckUnsignedLimit(s->pvpcon, ULONG_MAX);
		//unknown3 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ShopDelItem(const SMPacket *p) {
	if(p->size == sizeof(Merchant_DelItem_Struct)) {
		CheckHistoryExists(OP_ShopRequest, DISTANT);
		Merchant_DelItem_Struct *s = (Merchant_DelItem_Struct *) p->pBuffer;
		CheckMobClass(s->npcid, MERCHANT);
		CheckSelf(s->playerid);
		CheckUnsignedLimit(s->itemslot, 80);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GMEndTraining(const SMPacket *p) {
	if(p->size == sizeof(GMTrainEnd_Struct)) {
		CheckHistoryExists(OP_GMTraining, DISTANT);
		GMTrainEnd_Struct *s = (GMTrainEnd_Struct *) p->pBuffer;
		const MobInfoStruct *mob;
		CheckMob(s->npcid, mob);
		
		if(mob->_class < WARRIORGM || mob->_class > BERSERKERGM) return(false);
		
		CheckSelf(s->playerid);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_LevelUpdate(const SMPacket *p) {
	if(p->size == sizeof(LevelUpdate_Struct)) {
		CheckHistoryExists(OP_Death, REGULAR);
		LevelUpdate_Struct *s = (LevelUpdate_Struct *) p->pBuffer;
		CheckUnsigned(s->level, 1, HIGHEST_LIVE_LEVEL);
		CheckUnsigned(s->level_old, 1, HIGHEST_LIVE_LEVEL);
		CheckUnsigned(s->exp, 1000, ULONG_MAX);
		if((s->level_old+1) != s->level) return(false);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_UpdateLeadershipAA(const SMPacket *p) {
/*	if(p->size == sizeof(UpdateLeadershipAA_Struct)) {
		UpdateLeadershipAA_Struct *s = (UpdateLeadershipAA_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->ability_id, ULONG_MAX);
		//CheckUnsignedLimit(s->new_rank, ULONG_MAX);
		//unknown08 skipped
		return(true);
	}
*/	return(false);
}

//im not sure which one of these (OP_GMSummon/OP_GMGoto) will get found first
bool StructMatcher::Match_OP_GMGoto(const SMPacket *p) {
	if(p->size == sizeof(GMSummon_Struct)) {
		GMSummon_Struct *s = (GMSummon_Struct *) p->pBuffer;
		CheckString(s->charname, 64);
		CheckString(s->gmname, 64);
		//assume we will be set as the GM
		if(name != s->gmname) return(false);
		CheckBool(s->success);
		CheckZoneID(s->zoneID);
		CheckCoord(s->y);
		CheckCoord(s->x);
		CheckZCoord(s->z);
		//unknown2 skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_CastSpell(const SMPacket *p) {
	if(p->size == sizeof(CastSpell_Struct)) {
		CastSpell_Struct *s = (CastSpell_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->slot, 10);
		CheckSpellID(s->spell_id);
		CheckUnsignedLimit(s->inventoryslot, 400);	//upper bound not tight
		CheckMobExists(s->target_id);
		//cs_unknown skipped
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ZoneChange(const SMPacket *p) {
	if(p->size == sizeof(ZoneChange_Struct)) {
		ZoneChange_Struct *s = (ZoneChange_Struct *) p->pBuffer;
		CheckString(s->char_name, 64);
		CheckZoneID(s->zoneID);
		CheckUnsignedLimit(s->zone_reason, 0xF);
		//unknown69 skipped
		CheckBool(s->success);
		return(true);
	}
	return(false);
}

//
bool StructMatcher::Match_OP_ItemPacket(const SMPacket *p) {
	if(p->size >= sizeof(ItemPacket_Struct)) {
		
		if(p->size > 550) return(false);	//should never be longer *450 is current max)
		
		ItemPacket_Struct *s = (ItemPacket_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->PacketType, 0x6C);
		CheckStringExact(s->SerializedItem, p->size - sizeof(ItemPacket_Struct)+1);
		
		//item packets should be at least this big
		if(strlen(s->SerializedItem) < 300) return(false);
		
		//count |'s
		CheckCharCount(s->SerializedItem, '|', 10);
		
		return(true);
	}
	return(false);
}

//
bool StructMatcher::Match_OP_ItemLinkResponse(const SMPacket *p) {
	if(p->size >= sizeof(ItemPacket_Struct)) {
		
		CheckHistoryExists(OP_ItemLinkClick, CLOSE);
		
		if(p->size > 550) return(false);	//should never be longer *450 is current max)
		
		ItemPacket_Struct *s = (ItemPacket_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->PacketType, 0x6C);
		CheckStringExact(s->SerializedItem, p->size - sizeof(ItemPacket_Struct)+1);
		
		//item packets should be at least this big
		if(strlen(s->SerializedItem) < 300) return(false);
		
		//count |'s
		CheckCharCount(s->SerializedItem, '|', 10);
		
		return(true);
	}
	return(false);
}

//
bool StructMatcher::Match_OP_CharInventory(const SMPacket *p) {
	if(p->size >= sizeof(ItemPacket_Struct)) {
		//should have at least a few items.
		if(p->size < 1500) return(false);
		
		char *s = (char *) p->pBuffer;
		//I guess each item dosent have to be delimited
		CheckString(s, 5550);
		
		//look for |s
		CheckCharCount(s, '|', 10);
		
		CheckHistoryExists(OP_AckPacket, REGULAR);
		
		im_sure = true;
		
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Split(const SMPacket *p) {
/*	if(p->size == sizeof(Split_Struct)) {
		Split_Struct *s = (Split_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->platinum, ULONG_MAX);
		//CheckUnsignedLimit(s->gold, ULONG_MAX);
		//CheckUnsignedLimit(s->silver, ULONG_MAX);
		//CheckUnsignedLimit(s->copper, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_TargetCommand(const SMPacket *p) {
/*	if(p->size == sizeof(ClientTarget_Struct)) {
		ClientTarget_Struct *s = (ClientTarget_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->new_target, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

//might be too general
bool StructMatcher::Match_OP_TradeRequest(const SMPacket *p) {
	if(p->size == sizeof(TradeRequest_Struct)) {
		TradeRequest_Struct *s = (TradeRequest_Struct *) p->pBuffer;
		CheckPlayer(s->to_mob_id);
		CheckPlayer(s->from_mob_id);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_InterruptCast(const SMPacket *p) {
	if(p->size >= sizeof(InterruptCast_Struct)) {
		InterruptCast_Struct *s = (InterruptCast_Struct *) p->pBuffer;
		CheckMobExists(s->spawnid);
		
		if(s->messageid != INTERRUPT_SPELL 
		&& s->messageid != INTERRUPT_SPELL_OTHER
		&& s->messageid != SONG_ENDS
		&& s->messageid != SONG_ENDS_OTHER
		&& s->messageid != SONG_ENDS_ABRUPTLY
		&& s->messageid != SONG_ENDS_ABRUPTLY_OTHER )
			return(false);
		
		//a spell needs to have been cast recently.
		//we cant really look at the caster of the last begin cast
		//since multiple spells could have been casted since then
		CheckHistoryExists(OP_BeginCast, REGULAR);
		
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_Mend(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);

		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_GuildRemove(const SMPacket *p) {
/*	if(p->size == sizeof(GuildCommand_Struct)) {
		GuildCommand_Struct *s = (GuildCommand_Struct *) p->pBuffer;
		CheckString(s->othername, 64);
		CheckString(s->myname, 64);
		CheckUnsigned(s->guildeqid, 1, MAX_GUILD_ID);
		//unknown skipped
		//CheckUnsignedLimit(s->officer, ULONG_MAX);
		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_LeaveBoat(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);

		return(true);
	}
*/	return(false);
}

//I dont wanna deal with this right now
bool StructMatcher::Match_OP_Trader(const SMPacket *p) {
/*	if(p->size == sizeof(Trader_ShowItems_Struct)) {
		Trader_ShowItems_Struct *s = (Trader_ShowItems_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->code, ULONG_MAX);
		//CheckUnsignedLimit(s->traderid, ULONG_MAX);
		//unknown08 skipped
		return(true);
	} else if(p->size == sizeof(TraderBuy_Struct)) {
		TraderBuy_Struct *s = (TraderBuy_Struct *) p->pBuffer;
		//unknown0 skipped
		//CheckUnsignedLimit(s->traderid, ULONG_MAX);
		//CheckUnsignedLimit(s->itemid, ULONG_MAX);
		//unknown8 skipped
		//CheckUnsignedLimit(s->price, ULONG_MAX);
		//CheckUnsignedLimit(s->quantity, ULONG_MAX);
		//CheckUnsignedLimit(s->slot_num, ULONG_MAX);
		CheckString(s->itemname, 60);
		return(true);
	} else if(p->size == sizeof(Trader_Struct)) {
		Trader_Struct *s = (Trader_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->code, ULONG_MAX);
		uint32 idx_itemid;
		for(idx_itemid = 0; idx_itemid < 160; idx_itemid++) {
			//CheckUnsignedLimit(s->itemid[idx_itemid], ULONG_MAX);
		}
		//unknown skipped
		uint32 idx_itemcost;
		for(idx_itemcost = 0; idx_itemcost < 80; idx_itemcost++) {
			//CheckUnsignedLimit(s->itemcost[idx_itemcost], ULONG_MAX);
		}
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_PlayerProfile(const SMPacket *p) {
	if(p->size == sizeof(PlayerProfile_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_RecipeReply(const SMPacket *p) {
	if(p->size == sizeof(RecipeReply_Struct)) {
		CheckHistoryExists2(OP_RecipesSearch, OP_RecipesFavorite, COMPLETE);
		
		RecipeReply_Struct *s = (RecipeReply_Struct *) p->pBuffer;
		CheckUnsigned(s->object_type, 1, 0x35);
		//CheckUnsignedLimit(s->some_id, ULONG_MAX);
		CheckUnsigned(s->component_count, 1, 10);
		CheckUnsigned(s->recipe_id, 1, MAX_RECIPE_ID);
		CheckUnsigned(s->trivial, 2, 500);
		CheckString(s->recipe_name, 64);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ConsentResponse(const SMPacket *p) {
	if(p->size == sizeof(ConsentResponse_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_RezzRequest(const SMPacket *p) {
	if(p->size == sizeof(Resurrect_Struct)) {
		//this struct's length is unique, a length check is adequate

		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ChannelMessage(const SMPacket *p) {
	if(p->size >= sizeof(ChannelMessage_Struct)) {
		ChannelMessage_Struct *s = (ChannelMessage_Struct *) p->pBuffer;
		
		CheckString(s->targetname, 64);
		CheckString(s->sender, 64);
		//ranged not accurate
		CheckUnsignedLimit(s->language, 100);
		CheckUnsigned(s->chan_num, 1, 75);
		//cm_unknown4 skipped
		CheckUnsignedLimit(s->skill_in_language, 250);
		
		CheckString(s->message, p->size - sizeof(ChannelMessage_Struct));
		
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_SwapSpell(const SMPacket *p) {
/*	if(p->size == sizeof(SwapSpell_Struct)) {
		SwapSpell_Struct *s = (SwapSpell_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->from_slot, 10);
		CheckUnsignedLimit(s->to_slot, 10);
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_TradeAcceptClick(const SMPacket *p) {
	if(p->size == sizeof(TradeAccept_Struct)) {
		
		TradeAccept_Struct *s = (TradeAccept_Struct *) p->pBuffer;
		CheckMobExists(s->from_mob_id);
		//unknown4 skipped
		
		const SMPacket *old;
		uint32 maxdist = DISTANT;
		CheckHistory(OP_TradeRequestAck, maxdist, old);
		TradeRequest_Struct *os = (TradeRequest_Struct *) old->pBuffer;
		
		if(s->from_mob_id != os->from_mob_id) return(false);
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TributeMoney(const SMPacket *p) {
	if(p->size == sizeof(TributeMoney_Struct)) {
		//todo: watch for tribute master open packet
		
		TributeMoney_Struct *s = (TributeMoney_Struct *) p->pBuffer;
		CheckUnsigned(s->platinum, 1, 2000);
		CheckMobClass(s->tribute_master_id, TRIBUTE_MASTER);
		CheckSigned(s->tribute_points, 0, 250000);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_ClearObject(const SMPacket *p) {
/*	if(p->size == 0) {
		//this struct is empty, need to add history checking for it to be useful.
		//CheckHistoryExists(OP_.., 5_);

		return(false);

		return(true);
	}
*/	return(false);
}

//too general
bool StructMatcher::Match_OP_GuildManageStatus(const SMPacket *p) {
/*	if(p->size == sizeof(GuildManageStatus_Struct)) {
		GuildManageStatus_Struct *s = (GuildManageStatus_Struct *) p->pBuffer;
		CheckUnsigned(s->guildid, 1, MAX_GUILD_ID);
		//CheckUnsignedLimit(s->oldrank, ULONG_MAX);
		//CheckUnsignedLimit(s->newrank, ULONG_MAX);
		CheckString(s->name, 64);
		return(true);
	}
*/	return(false);
}

//im lazy
bool StructMatcher::Match_OP_Bazaar(const SMPacket *p) {
/*	if(p->size == sizeof(BazaarSearch_Struct)) {
		BazaarSearch_Struct *s = (BazaarSearch_Struct *) p->pBuffer;
		//NO_MATCHER(s->beginning);
		//CheckUnsignedLimit(s->traderid, ULONG_MAX);
		//CheckUnsignedLimit(s->class_, ULONG_MAX);
		//CheckUnsignedLimit(s->race, ULONG_MAX);
		//CheckUnsignedLimit(s->stat, ULONG_MAX);
		//CheckUnsignedLimit(s->slot, ULONG_MAX);
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		CheckString(s->name, 64);
		//CheckUnsignedLimit(s->minprice, ULONG_MAX);
		//CheckUnsignedLimit(s->maxprice, ULONG_MAX);
		return(true);
	} else if(p->size == sizeof(BazaarWelcome_Struct)) {
		BazaarWelcome_Struct *s = (BazaarWelcome_Struct *) p->pBuffer;
		//NO_MATCHER(s->beginning);
		//CheckUnsignedLimit(s->traders, ULONG_MAX);
		//CheckUnsignedLimit(s->items, ULONG_MAX);
		//unknown1 skipped
		return(true);
	} else if(p->size == sizeof(BazaarReturnDone_Struct)) {
		BazaarReturnDone_Struct *s = (BazaarReturnDone_Struct *) p->pBuffer;
		//CheckUnsignedLimit(s->type, ULONG_MAX);
		//CheckUnsignedLimit(s->traderid, ULONG_MAX);
		//unknown8 skipped
		//unknown12 skipped
		//unknown16 skipped
		return(true);
	}
*/	return(false);
}

bool StructMatcher::Match_OP_RecipeDetails(const SMPacket *p) {
	if(p->size == sizeof(uint32)) {
		CheckHistoryExists(OP_RecipeReply, DISTANT);
		
		uint32 *s = (uint32 *) p->pBuffer;
		CheckUnsigned(*s, 500, MAX_RECIPE_ID);
		
		//there is also a complicated OP_RecipeDetails reply struct OUT bound
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_Weather(const SMPacket *p) {
	if(p->size == sizeof(Weather_Struct)) {
		Weather_Struct *s = (Weather_Struct *) p->pBuffer;
		if(s->val1 != 0xFF) return(false);
		CheckUnsigned(s->type, 0, 0x31);
		CheckUnsigned(s->mode, 0, 3);
		
		CheckHistoryExists(OP_CompletedTasks, 1);
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_MobHealth(const SMPacket *p) {
	if(p->size == sizeof(MobHealth_Struct)) {
		MobHealth_Struct *s = (MobHealth_Struct *) p->pBuffer;
		
		CheckMobExists(s->entity_id);
		if(s->hp != 100)  return(false);
		
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_CompletedTasks(const SMPacket *p) {
	if(p->size >= sizeof(TaskHistory_Struct)) {
		TaskHistory_Struct *s = (TaskHistory_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->completed_count, 1024);
		
		TaskHistoryEntry_Struct *c = s->entries;
		uint32 r;
		uint32 curtime = time(NULL);
		for(r = 0; r < s->completed_count; r++) {
			CheckUnsigned(c->task_id, 1, 0xFFFF);
			CheckString(c->name, 128);
			int len = strlen(c->name);
			
			c = (TaskHistoryEntry_Struct *) (((uint8 *) c) + len);
			//checking timers in a 10 month interval
			CheckUnsigned(c->completed_time, curtime-26297438, curtime+100000);

			//I really dont know why I need the +5 here
			c = (TaskHistoryEntry_Struct *) (((uint8 *) c) + sizeof(TaskHistory_Struct) + 5);
		}
		
		//task descriptions come before this, and is a variable number of
		//packets
		CheckHistoryExists(OP_CharInventory, DISTANT);
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_AckPacket(const SMPacket *p) {
	if(p->size == sizeof(uint32)) {
		uint32 *s = (uint32 *) p->pBuffer;
		if(*s != 0) return(false);
		
		CheckHistoryExists(OP_ZoneEntry, CLOSE);
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_ZoneInUnknown(const SMPacket *p) {
	if(p->size == sizeof(ZoneInUnknown_Struct)) {
//		ZoneInUnknown_Struct *s = (ZoneInUnknown_Struct *) p->pBuffer;
		
		//always follows zone in right away
		CheckHistoryExists(OP_ZoneEntry, 1);
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_CustomTitles(const SMPacket *p) {
	if(p->size >= sizeof(Titles_Struct)) {
		Titles_Struct *s = (Titles_Struct *) p->pBuffer;
		CheckUnsignedLimit(s->title_count, 50);
		
		TitleEntry_Struct *c = s->titles;
		uint32 r;
		for(r = 0; r < s->title_count; r++) {
			CheckUnsignedLimit(c->skill_id, HIGHEST_SKILL);
			CheckUnsignedLimit(c->skill_value, 500);
			CheckString(c->title, 64);	//artificial length
			int len = strlen(c->title);
			
			//no idea why +5
			c = (TitleEntry_Struct *) (((uint8 *) c) + len + sizeof(TitleEntry_Struct));
		}
		
		//should come right after new zone
		CheckHistoryExists(OP_NewZone, CLOSE);
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SpawnDoor(const SMPacket *p) {
	if(p->size >= sizeof(Door_Struct)) {
		
		//make sure the length is a multiple of the size
		if ((p->size % sizeof(Door_Struct)) != 0)
			return(false);
		
		int count = p->size / sizeof(Door_Struct);
		
		Door_Struct *s = (Door_Struct *) p->pBuffer;
		while(count > 0) {
			CheckString(s->name, 16);
			//unknown0016 skipped
			CheckCoord(s->yPos);
			CheckCoord(s->xPos);
			CheckZCoord(s->zPos);
			CheckHCoord(s->heading);
			//CheckUnsigned(s->incline, 0, ULONG_MAX);
			//CheckUnsigned(s->size, 0, ULONG_MAX);
			//unknown0038 skipped
			//CheckUnsigned(s->doorId, 0, ULONG_MAX);
			//CheckUnsigned(s->opentype, 0, ULONG_MAX);
			//CheckUnsigned(s->state_at_spawn, 0, ULONG_MAX);
			CheckBool(s->invert_state);
			//CheckUnsigned(s->door_param, 0, ULONG_MAX);
			//unknown0052 skipped
			
			s++;
			count--;
		}
		
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SendZonepoints(const SMPacket *p) {
	if(p->size >= sizeof(ZonePoints)) {
		
		//make sure the length is a multiple of the size
		if (((p->size-sizeof(ZonePoints)) % sizeof(ZonePoint_Entry)) != 0)
			return(false);
		
		
		ZonePoints *so = (ZonePoints *) p->pBuffer;
		int count = so->count;
		if(count == 0) return(false);	//not sent if empty
		ZonePoint_Entry *s = so->zpe;
		while(count > 0) {
			CheckUnsigned(s->iterator, 0, 200);
			CheckFloat(s->y, -999999, 999999, -5, 7);
			CheckFloat(s->x, -999999, 999999, -5, 7);
			CheckFloat(s->z, -999999, 999999, -5, 7);
			CheckFloat(s->heading, -1, 1e7, -5, 7);
			CheckZoneID(s->zoneid);
			//CheckUnsigned(s->zoneinstance, 0, ULONG_MAX);
			
			s++;
			count--;
		}
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GroundSpawn(const SMPacket *p) {
	if(p->size == sizeof(Object_Struct)) {
		Object_Struct *s = (Object_Struct *) p->pBuffer;
		
		//unknown008 skipped
		//CheckUnsigned(s->drop_id, 0, ULONG_MAX);
		CheckZoneID(s->zone_id);
		//CheckUnsigned(s->zone_instance, 0, ULONG_MAX);
		//unknown020 skipped
		CheckHCoord(s->heading);
		CheckZCoord(s->z);
		CheckCoord(s->y);
		CheckCoord(s->x);
		CheckString(s->object_name, 20);
		//unknown060 skipped
		//CheckUnsigned(s->object_type, 0, ULONG_MAX);
		//unknown084 skipped
		//CheckUnsigned(s->spawn_id, 0, ULONG_MAX);
		return(true);
	}
	return(false);
}


bool StructMatcher::Match_OP_GuildMemberUpdate(const SMPacket *p) {
	if(p->size == sizeof(GuildMemberUpdate_Struct)) {
		GuildMemberUpdate_Struct *s = (GuildMemberUpdate_Struct *) p->pBuffer;
		CheckUnsigned(s->guild_id, 1, MAX_GUILD_ID);
		CheckString(s->member_name, 64);
		CheckZoneID(s->zone_id);
		//CheckUnsigned(s->instance_id, 0, ULONG_MAX);
		//unknown072 skipped
		return(true);
	}
	return(false);
}


bool StructMatcher::Match_OP_AnnoyingZoneUnknown(const SMPacket *p) {
	if(p->size == sizeof(AnnoyingZoneUnknown_Struct)) {
		AnnoyingZoneUnknown_Struct *s = (AnnoyingZoneUnknown_Struct *) p->pBuffer;
		
		CheckNPC(s->entity_id);
		
		if(s->value != 4) return(false);
		
		return(true);
	}
	return(false);
}


bool StructMatcher::Match_OP_GuildMemberList(const SMPacket *p) {
	if(p->size >= (sizeof(uint32)+10)) {
		char *s = (char *) p->pBuffer;
		
		if(name != s) return(false);
		s += name.length() + 1;
		uint32 *n = (uint32 *) s;
		uint32 rn = htonl(*n);
		CheckUnsigned(rn, 2, 1000);	//artificial bounds
		s += sizeof(uint32);
		CheckString(s, 64);
		
		im_sure = true;
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_TributeInfo(const SMPacket *p) {
	if(p->size >= sizeof(TributeAbility_Struct)) {
		TributeAbility_Struct *s = (TributeAbility_Struct *) p->pBuffer;
		CheckUnsigned(s->tribute_id, 0, 200);
		uint32 idx_tiers;
		for(idx_tiers = 0; idx_tiers < htonl(s->tier_count); idx_tiers++) {
			TributeLevel_Struct *s2 = &s->tiers[idx_tiers];
			CheckUnsigned(htonl(s2->level), 10, HIGHEST_LIVE_LEVEL);
			CheckItemID(htonl(s2->tribute_item_id));
			CheckUnsigned(htonl(s2->cost), 1, 400);
		}
		//max length wrong
		CheckString(s->name, 64);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_GuildTributeInfo(const SMPacket *p) {
	if(p->size >= sizeof(GuildTributeAbility_Struct)) {
		GuildTributeAbility_Struct *s = (GuildTributeAbility_Struct *) p->pBuffer;
		CheckUnsigned(s->ability.tribute_id, 0, 200);
		uint32 idx_tiers;
		for(idx_tiers = 0; idx_tiers < htonl(s->ability.tier_count); idx_tiers++) {
			TributeLevel_Struct *s2 = &s->ability.tiers[idx_tiers];
			CheckUnsigned(htonl(s2->level), 10, HIGHEST_LIVE_LEVEL);
			CheckItemID(htonl(s2->tribute_item_id));
			CheckUnsigned(htonl(s2->cost), 1, 400);
		}
		//max length wrong
		CheckString(s->ability.name, 64);
		return(true);
	}
	return(false);
}

//too general
bool StructMatcher::Match_OP_RequestTitles(const SMPacket *p) {
//	if(p->size == 0) {
		//we would need future tracking to find this one.
//	}
	return(false);
}

//prolly too general without string verification
bool StructMatcher::Match_OP_SendTitleList(const SMPacket *p) {
	if(p->size >= sizeof(TitleList_Struct)+sizeof(TitleListEntry_Struct)) {
		TitleList_Struct *s = (TitleList_Struct *) p->pBuffer;
		
		CheckHistoryExists(OP_RequestTitles, CLOSE);
		
		CheckUnsigned(s->title_count, 0, ULONG_MAX);
		
		//todo: verify all the strings which follow
		//TitleListEntry_Struct *tmp = s->titles;
		//CheckStringEmpty(tmp->prefix, 64);
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SetTitle(const SMPacket *p) {
	if(p->size == sizeof(SetTitle_Struct)) {
		SetTitle_Struct *s = (SetTitle_Struct *) p->pBuffer;
		
		CheckHistoryExists(OP_SendTitleList, REGULAR);
		
		CheckUnsignedLimit(s->is_suffix, 1);
		CheckUnsigned(s->title_id, 100, 20000);	//arbitrary limits right now
		
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_SetTitleReply(const SMPacket *p) {
	if(p->size == sizeof(SetTitleReply_Struct)) {
		SetTitleReply_Struct *s = (SetTitleReply_Struct *) p->pBuffer;
		
		CheckHistoryExists(OP_SetTitle, CLOSE);
		
		CheckUnsignedLimit(s->is_suffix, 1);
		CheckString(s->title, 32);
		CheckMobExists(s->entity_id);
		return(true);
	}
	return(false);
}

bool StructMatcher::Match_OP_DeleteItem(const SMPacket *p) {
	return(false);
}




