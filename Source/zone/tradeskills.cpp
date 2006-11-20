/*  EQEMu:  Everquest Server Emulator
	Copyright (C) 2001-2004  EQEMu Development Team (http://eqemulator.net)

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
#include <stdlib.h>

#ifndef WIN32
#include <netinet/in.h>	//for htonl
#endif

#include "masterentity.h"
#include "zonedb.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"
#include "StringIDs.h"
#include "../common/MiscFunctions.h"

static const SkillType TradeskillUnknown = _1H_BLUNT; /* an arbitrary non-tradeskill */

void Object::HandleAugmentation(Client* user, const AugmentItem_Struct* in_augment, Object *worldo)
{
	if (!user || !in_augment) {
		LogFile->write(EQEMuLog::Error, "Client or AugmentItem_Struct not set in Object::HandleAugmentation");
		return;
	}
	
	ItemInst *tobe_auged, *auged_with = NULL;
	sint8 slot=-1;
	ItemInst* container = worldo->m_inst;

	if (!(tobe_auged = container->GetItem(0))) {
		user->Message(13, "Error: No item in slot 0 of sealer");
		return;
	}
	if (tobe_auged->IsAugmentable()) {
		if (!(auged_with=container->GetItem(1))) {;
			user->Message(13, "Error: No item in slot 1 of sealer");
			return;
		}
	} else {
		auged_with=tobe_auged;
		if (!(tobe_auged=container->GetItem(1))) {
			user->Message(13, "Error: No item in slot 1 of sealer");
			return;
		}
	}

	// Adding augment
	if (in_augment->augment_slot == -1) {
		if ((slot=tobe_auged->AvailableAugmentSlot(auged_with->GetAugmentType()))!=-1) {
			tobe_auged->PutAugment(slot,*auged_with);
			user->PushItemOnCursor(*tobe_auged,true);
			container->Clear();
			EQApplicationPacket* outapp = new EQApplicationPacket(OP_ClearObject,0);
			user->QueuePacket(outapp);
			safe_delete(outapp);
			database.DeleteWorldContainer(worldo->m_id, zone->GetZoneID());
		} else {
			user->Message(13, "Error: No available slot for augment");
		}
	} else {
		ItemInst *old_aug=NULL;
		const uint32 id=auged_with->GetID();
		if (id==40408 || id==40409 || id==40410)
			tobe_auged->DeleteAugment(in_augment->augment_slot);
		else
			old_aug=tobe_auged->RemoveAugment(in_augment->augment_slot);

		user->PushItemOnCursor(*tobe_auged,true);
		if (old_aug)
			user->PushItemOnCursor(*old_aug,true);
		container->Clear();
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ClearObject,0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		database.DeleteWorldContainer(worldo->m_id, zone->GetZoneID());
	}
}

// Perform tradeskill combine
// complete tradeskill rewrite by father nitwit, 8/2004
void Object::HandleCombine(Client* user, const NewCombine_Struct* in_combine, Object *worldo)
{
	if (!user || !in_combine) {
		LogFile->write(EQEMuLog::Error, "Client or NewCombine_Struct not set in Object::HandleCombine");
		return;
	}
	
	Inventory& user_inv = user->GetInv();
	PlayerProfile_Struct& user_pp = user->GetPP();
	ItemInst* container = NULL;
	ItemInst* inst = NULL;
	int8 tstype = 0xE8;
	uint8 passtype = 0;
	bool worldcontainer=false;
	
	if (in_combine->container_slot == SLOT_TRADESKILL) {
		if(!worldo) {
			user->Message(13, "Error: Server is not aware of the tradeskill container you are attempting to use");
			return;
		}
		inst = worldo->m_inst;
		worldcontainer=true;
	}
	else {
		inst = user_inv.GetItem(in_combine->container_slot);
		if (inst) {
			const Item_Struct* item = inst->GetItem();
			if (item && inst->IsType(ItemClassContainer)) {
				tstype = item->BagType;
			}
		}
	}
	
	if (!inst || !inst->IsType(ItemClassContainer)) {
		user->Message(13, "Error: Server does not recognize specified tradeskill container");
		return;
	}
	
	container = inst;
	
	// Convert container type to tradeskill type
	SkillType tradeskill = TradeskillUnknown;
	switch (tstype)
	{
	case 16:
		tradeskill = TAILORING;
		break;
	case 0xE8: //Generic World Container
		if(!worldcontainer)	//just to garuntee that worldo is valid
			return;
		passtype = worldo->m_type;
		
		if(worldo->m_type == OT_MEDICINEBAG) {
			if ((user_pp.class_ == SHAMAN) & (user_pp.level >= MIN_LEVEL_ALCHEMY))
				tradeskill = ALCHEMY;
			else if (user_pp.class_ != SHAMAN)
				user->Message(13, "This tradeskill can only be performed by a shaman.");
			else if (user_pp.level < MIN_LEVEL_ALCHEMY)
				user->Message(13, "You cannot perform alchemy until you reach level %i.", MIN_LEVEL_ALCHEMY);
			break;
		} else {
			tradeskill = TypeToSkill(worldo->m_type);
		}
		break;
	case 18:
		tradeskill = FLETCHING;
		break;
	case 20:
		tradeskill = JEWELRY_MAKING;
		break;
	case 30: //Pottery Still needs completion
		tradeskill = POTTERY;
		break;
	case 14: // Baking 
	case 15:
		tradeskill = BAKING;
		break;
	case 9: //Alchemy Still needs completion
		if ((user_pp.class_ == SHAMAN) & (user_pp.level >= MIN_LEVEL_ALCHEMY))
			tradeskill = ALCHEMY;
		else if (user_pp.class_ != SHAMAN)
			user->Message(13, "This tradeskill can only be performed by a shaman.");
		else if (user_pp.level < MIN_LEVEL_ALCHEMY)
			user->Message(13, "You cannot perform alchemy until you reach level %i.", MIN_LEVEL_ALCHEMY);
		break;
	case 10: //Tinkering Still needs completion
		if (user_pp.race == GNOME)
			tradeskill = TINKERING;
		else
			user->Message(13, "Only gnomes can tinker.");
		break; 
	case 24: //Research Still needs completion
	case 25:
	case 26:
	case 27:
		tradeskill = RESEARCH;
		break;
	case 12:
		if (user_pp.class_ == ROGUE)
			tradeskill = MAKE_POISON;
		else
			user->Message(13, "Only rogues can mix poisons.");
		break;
	case 0x0D: //Quest Containers-Most use 1E but item 17111 uses this one, odd Still needs completion
		tradeskill = POTTERY;
		break;
	case 46: //Fishing Still needs completion
		tradeskill = FISHING;
		break;
	default:
		user->Message(13, "This tradeskill has not been implemented yet, if you get this message send a "
			"petition and let them know what tradeskill you were trying to use. and give them the following code: 0x%02X", tradeskill);
	}
	
	if (tradeskill == TradeskillUnknown) {
		return;
	}
	
	DBTradeskillRecipe_Struct spec;
	if (!database.GetTradeRecipe(container, passtype, tradeskill, &spec)) {
		user->Message_StringID(4,TRADESKILL_NOCOMBINE);
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	
	//do the check and send results...
	bool success = user->TradeskillExecute(&spec, tradeskill);
	
	// Send acknowledgement packets to client
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_TradeSkillCombine, 0);
	user->QueuePacket(outapp);
	safe_delete(outapp);
	
	//now clean out the containers.
	if(worldcontainer){
		container->Clear();
		outapp = new EQApplicationPacket(OP_ClearObject,0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		database.DeleteWorldContainer(worldo->m_id, zone->GetZoneID());
		if(success && spec.replace_container) {
			//should report this error, but we dont have the recipe ID, so its not very useful
			LogFile->write(EQEMuLog::Error, "Replace container combine executed in a world container.");
		}
	} else{
		for (uint8 i=0; i<10; i++){
			const ItemInst* inst = container->GetItem(i);
			if (inst) {
				user->DeleteItemInInventory(Inventory::CalcSlotId(in_combine->container_slot,i),0,true);
			}
		}
		container->Clear();
		if(success && spec.replace_container) {
			user->DeleteItemInInventory(in_combine->container_slot, 0, true);
		}
	}
}

void Object::HandleAutoCombine(Client* user, const RecipeAutoCombine_Struct* rac) {
	
	//get our packet ready, gotta send one no matter what...
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_RecipeAutoCombine, sizeof(RecipeAutoCombine_Struct));
	RecipeAutoCombine_Struct *outp = (RecipeAutoCombine_Struct *)outapp->pBuffer;
	outp->object_type = rac->object_type;
	outp->some_id = rac->some_id;
	outp->unknown1 = rac->unknown1;
	outp->recipe_id = rac->recipe_id;
	outp->reply_code = 0xFFFFFFF5;	//default fail.
	
	
	SkillType tskill = Object::TypeToSkill(rac->object_type);
	if(tskill == TradeskillUnknown) {
		LogFile->write(EQEMuLog::Error, "Unknown container type for HandleAutoCombine: %d\n", rac->object_type);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	
	//ask the database for the recipe to make sure it exists...
	DBTradeskillRecipe_Struct spec;
	if (!database.GetTradeRecipe(rac->recipe_id, rac->object_type, tskill, &spec)) {
		LogFile->write(EQEMuLog::Error, "Unknown recipe for HandleAutoCombine: %u\n", rac->recipe_id);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	
	uint32 qlen = 0;
	uint8 qcount = 0;

	//pull the list of components
	qlen = MakeAnyLenString(&query, "SELECT tre.item_id,tre.componentcount "
	 " FROM tradeskill_recipe_entries AS tre "
	 " WHERE tre.componentcount > 0 AND tre.recipe_id=%u", rac->recipe_id);

	if (!database.RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in HandleAutoCombine query '%s': %s", query, errbuf);
		safe_delete_array(query);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount < 1) {
		LogFile->write(EQEMuLog::Error, "Error in HandleAutoCombine: no components returned");
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	if(qcount > 10) {
		LogFile->write(EQEMuLog::Error, "Error in HandleAutoCombine: too many components returned (%u)", qcount);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	
	uint32 items[10];
	memset(items, 0, sizeof(items));
	uint8 counts[10];
	memset(counts, 0, sizeof(counts));
	
	
	//search for all the crap in their inventory
	Inventory& user_inv = user->GetInv();
	uint8 count = 0;
	uint8 needcount = 0;
	uint8 r,k;
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);
		uint32 item = (uint32)atoi(row[0]);
		uint8 num = (uint8) atoi(row[1]);
		
		needcount += num;
		
		//because a HasItem on items with num > 1 only returns the
		//last-most slot... the results of this are useless to us
		//when we go to delete them because we cannot assume it is in a single stack.
		if(user_inv.HasItem(item, num, invWherePersonal) != SLOT_INVALID)
			count += num;
		//dont start deleting anything until we have found it all.
		items[r] = item;
		counts[r] = num;
	}
	mysql_free_result(result);
	
	//make sure we found it all...
	if(count != needcount) {
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	
	//now we know they have everything...
	
	//remove all the items from the players inventory, with updates...
	sint16 slot;
	for(r = 0; r < qcount; r++) {
		if(items[r] == 0 || counts[r] == 0)
			continue;	//skip empties, could prolly break here
		
		//we have to loop here to delete 1 at a time in case its in multiple stacks.
		for(k = 0; k < counts[r]; k++) {
			slot = user_inv.HasItem(items[r], 1, invWherePersonal);
			if(slot == SLOT_INVALID) {
				//WTF... I just checked this above, but just to be sure...
				//we cant undo the previous deletes without a lot of work.
				//so just call it quits, this shouldent ever happen anyways.
				user->QueuePacket(outapp);
				safe_delete(outapp);
				return;
			}
			
			user->DeleteItemInInventory(slot, 1, true);
		}
	}
	
	//otherwise, we found it all...
	outp->reply_code = 0x00000000;	//success for finding it...
	
	user->QueuePacket(outapp);
	//DumpPacket(outapp);
	safe_delete(outapp);
	
	
	//now actually try to make something...
	
	bool success = user->TradeskillExecute(&spec, tskill);
	
	//TODO: find in-pack containers in inventory, make sure they are really
	//there, and then use that slot to handle replace_container too.
	if(success && spec.replace_container) {
//		user->DeleteItemInInventory(in_combine->container_slot, 0, true);
	}
	
}

SkillType Object::TypeToSkill(uint32 type) {
	SkillType tradeskill = TradeskillUnknown;
	switch (type) {
		case OT_MEDICINEBAG: {
			tradeskill = ALCHEMY;
			break;
		}
		case OT_SEWINGKIT: {
			tradeskill = TAILORING;
			break;
		}
		case OT_FORGE:
		case OT_TEIRDALFORGE:
		case OT_OGGOKFORGE:
		case OT_FIERDALFFORGE:
		case OT_STORMGUARDF: {
			tradeskill = BLACKSMITHING;
			break;
		}
		case OT_FLETCHINGKIT: {
			tradeskill = FLETCHING;
			break;
		}
		case OT_BREWBARREL: {
			tradeskill = BREWING;
			break;
		}
		case OT_JEWELERSKIT: {
			tradeskill = JEWELRY_MAKING;
			break;
		}
		case OT_POTTERYWHEEL:
		case OT_KILN: {
			tradeskill = POTTERY;
			break;
		}
		case OT_OVEN: {
			tradeskill = BAKING;
			break;
		}
		case OT_TACKLEBOX: {
			tradeskill = FISHING;
			break;
		}
		case OT_KEYMAKER: { //unknown for now...
			tradeskill = TradeskillUnknown;
			break;
		}
		case OT_TOOLBOX: {
			tradeskill = TINKERING;
			break;
		}
		case OT_WIZARDLEX:
		case OT_MAGELEX:
		case OT_NECROLEX:
		case OT_ENCHLEX: {
			tradeskill = RESEARCH;
			break;
		}
	}
	return(tradeskill);
}

void Client::TradeskillSearchResults(const char *query, unsigned long qlen, 
  unsigned long objtype, unsigned long someid) {
	
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    
//printf("TradeskillSearchResults query ' %s '\n", query);
	if (!database.RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in TradeskillSearchResults query '%s': %s", query, errbuf);
		return;
	}
	
	uint8 qcount = 0;
	
	qcount = mysql_num_rows(result);
	if(qcount < 1) {
		//search gave no results... not an error
		return;
	}
	if(mysql_num_fields(result) != 4) {
		LogFile->write(EQEMuLog::Error, "Error in TradeskillSearchResults query '%s': Invalid column count in result", query);
		return;		
	}
	
	uint8 r;
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);
		if(row == NULL || row[0] == NULL || row[1] == NULL || row[2] == NULL || row[3] == NULL)
			continue;
		uint32 recipe = (uint32)atoi(row[0]);
		const char *name = row[1];
		uint32 trivial = (uint32) atoi(row[2]);
		uint32 comp_count = (uint32) atoi(row[3]);
		
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_RecipeReply, sizeof(RecipeReply_Struct));
		RecipeReply_Struct *reply = (RecipeReply_Struct *) outapp->pBuffer;
		
		reply->object_type = objtype;
		reply->some_id = someid;
		reply->component_count = comp_count;
		reply->recipe_id = recipe;
		reply->trivial = trivial;
		strncpy(reply->recipe_name, name, 63);
		
		//DumpPacket(outapp);
		FastQueuePacket(&outapp);
	}
	mysql_free_result(result);
}

void Client::SendTradeskillDetails(unsigned long recipe_id) {

//from server in response to a 4 byte OP_RecipeDetails, just the item id
/*struct RecipeDetails_Struct {
	unsigned long recipe_id;	//backwards byte order from the Reply
	//dynamic part...
	// there are as many as 10 0xFFFFFFFF here in a row..
	// there are (10 - component count) of them...
	
	//then one of these for each component:
	// unsigned long item_id;	//in backwards byte order
	// unsigned long icon_id;	//in backwards byte order
	// NULL terminated name...
	
};*/

	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	
	uint32 qlen = 0;
	uint8 qcount = 0;

	//pull the list of components
	qlen = MakeAnyLenString(&query, "SELECT tre.item_id,tre.componentcount,i.icon,i.Name "
	 " FROM tradeskill_recipe_entries AS tre "
	 " LEFT JOIN items AS i ON tre.item_id = i.id "
	 " WHERE tre.componentcount > 0 AND tre.recipe_id=%u", recipe_id);

	if (!database.RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in SendTradeskillDetails query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return;
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount < 1) {
		LogFile->write(EQEMuLog::Error, "Error in SendTradeskillDetails: no components returned");
		return;
	}
	if(qcount > 10) {
		LogFile->write(EQEMuLog::Error, "Error in SendTradeskillDetails: too many components returned (%u)", qcount);
		return;
	}
	
	//biggest this packet can ever be:
	// 64 * 10 + 8 * 10 + 4 + 4 * 10 = 764
	char *buf = new char[775];	//dynamic so we can just give it to EQApplicationPacket
	uint8 r,k;
	
	unsigned long *header = (unsigned long *) buf;
	//Hell if I know why this is in the wrong byte order....
	*header = htonl(recipe_id);
	
	char *startblock = buf;
	startblock += sizeof(unsigned long);
	
	unsigned long *ffff_start = (unsigned long *) startblock;
	//fill in the FFFF's as if there were 0 items
	for(r = 0; r < 10; r++) {
		*ffff_start = 0xFFFFFFFF;
		ffff_start++;
	}
	char * datastart = (char *) ffff_start;
	char * cblock = (char *) ffff_start;
	
	unsigned long *itemptr;
	unsigned long *iconptr;
	uint32 len;
	uint32 datalen = 0;
	uint8 count = 0;
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);
		
		//watch for references to items which are not in the
		//items table, which the left join will make NULL...
		if(row[2] == NULL || row[3] == NULL) {
			continue;
		}
		
		uint32 item = (uint32)atoi(row[0]);
		uint8 num = (uint8) atoi(row[1]);
		
		
		uint32 icon = (uint32) atoi(row[2]);
		const char *name = row[3];
		len = strlen(name);
		if(len > 63)
			len = 63;
		
		//Hell if I know why these are in the wrong byte order....
		item = htonl(item);
		icon = htonl(icon);
		
		//if we get more than 10 items, just start skipping them...
		for(k = 0; k < num && count < 10; k++) {
			itemptr = (unsigned long *) cblock;
			cblock += sizeof(unsigned long);
			datalen += sizeof(unsigned long);
			iconptr = (unsigned long *) cblock;
			cblock += sizeof(unsigned long);
			datalen += sizeof(unsigned long);
			
			*itemptr = item;
			*iconptr = icon;
			strncpy(cblock, name, len);
			
			cblock[len] = '\0';	//just making sure.
			cblock += len + 1;	//get the null
			datalen += len + 1;	//gte the null
			count++;
		}
		
	}
	mysql_free_result(result);
	
	//now move the item data over top of the FFFFs
	uint8 dist = sizeof(unsigned long) * (10 - count);
	startblock += dist;
	memmove(startblock, datastart, datalen);
	
	uint32 total = sizeof(unsigned long) + dist + datalen;
	
	EQApplicationPacket* outapp = new EQApplicationPacket(OP_RecipeDetails);
	outapp->size = total;
	outapp->pBuffer = (uchar*) buf;
	QueuePacket(outapp);
	DumpPacket(outapp);
	safe_delete(outapp);
}

//returns true on success
bool Client::TradeskillExecute(DBTradeskillRecipe_Struct *spec, SkillType tradeskill) {
	if(spec == NULL || tradeskill == 0)
		return(false);
	
	int16 user_skill = GetSkill(tradeskill);
	float chance = 0;
	float skillup_modifier;
	sint16 thirdstat = 0;
	sint16 stat_modifier = 15;
	uint16 success_modifier;

	// Rework based on the info on eqtraders.com
	// http://mboards.eqtraders.com/eq/showthread.php?t=22246
	// 09/10/2006 v0.1 (eq4me)
	// 09/11/2006 v0.2 (eq4me)
	// Todo:
	//     Implementing AAs
	//     Success modifiers based on recipes
	//     Skillup modifiers based on the rarity of the ingredients

	// Some tradeskills are more eqal then others. ;-)
	// If you want to customize the stage1 success rate do it here.
    // Remember: skillup_modifier is (float). Lower is better
	switch(tradeskill) {
	case FLETCHING:
	case ALCHEMY:
	case JEWELRY_MAKING:
	case POTTERY:
		skillup_modifier = 4;
		break;
	case BAKING:
	case BREWING:
		skillup_modifier = 3;
		break;
	case RESEARCH:
		skillup_modifier = 1;
		break;
	default:
		skillup_modifier = 2;
		break;
	}

	// Some tradeskills take the higher of one additional stat beside INT and WIS
	// to determine the skillup rate. Additionally these tradeskills do not have an
	// -15 modifier on their statbonus.
	if (tradeskill ==  FLETCHING || tradeskill == MAKE_POISON) {
		thirdstat = GetDEX();
		stat_modifier = 0;
	} else if (tradeskill == BLACKSMITHING) {
		thirdstat = GetSTR();
		stat_modifier = 0;
	}
	
	sint16 higher_from_int_wis = (GetINT() > GetWIS()) ? GetINT() : GetWIS();
	sint16 bonusstat = (higher_from_int_wis > thirdstat) ? higher_from_int_wis : thirdstat;
	
	vector< pair<uint32,uint8> >::iterator itr;


    //calculate the base success chance
	// For trivials over 68 the chance is (skill - 0.75*trivial) +51.5
    // For trivial up to 68 the chance is (skill - trivial) + 66
	if (spec->trivial >= 68) {
		chance = (user_skill - (0.75*spec->trivial)) + 51.5;
	} else {
		chance = (user_skill - spec->trivial) + 66;
	}
	
	sint16 over_trivial = (sint16)user_skill - (sint16)spec->trivial;

	//handle caps
	if(spec->nofail) {
		chance = 100;	//cannot fail.
		_log(TRADESKILLS__TRACE, "...This combine cannot fail.");
	} else if(over_trivial > 0) {
		// At reaching trivial the chance goes to 95% going up an additional
		// percent for every 40 skillpoints above the trivial.
		// The success rate is not modified through stats.
		// Mastery AAs are unaccounted for so far.
		// chance_AA = chance + ((100 - chance) * mastery_modifier)
		// But the 95% limit with an additional 1% for every 40 skill points
		// above critical still stands.
		// Mastery modifier is: 10%/25%/50% for rank one/two/three
		chance = 95.0f + (float(user_skill - spec->trivial) / 40.0f);
		Message_StringID(4, TRADESKILL_TRIVIAL);
	} else if(chance < 5) {
		// Minimum chance is always 5
		chance = 5;
	} else if(chance > 95) {
		//cap is 95, shouldent reach this before trivial, but just in case.
		chance = 95;
	}
	
	_log(TRADESKILLS__TRACE, "...Current skill: %d , Trivial: %d , Success chance: %f percent", user_skill , spec->trivial , chance);
	_log(TRADESKILLS__TRACE, "...Bonusstat: %d , INT: %d , WIS: %d , DEX: %d , STR: %d", bonusstat , GetINT() , GetWIS() , GetDEX() , GetSTR());
	
	float res = MakeRandomFloat(0, 99);
	if ((tradeskill==75) || GetGM() || (chance > res)){
		success_modifier = 1;
		
		if(over_trivial < 0)
			CheckIncreaseTradeskill(bonusstat, stat_modifier, skillup_modifier, success_modifier, tradeskill);
		
		Message_StringID(4,TRADESKILL_SUCCEED);

		_log(TRADESKILLS__TRACE, "Tradeskill success");

		itr = spec->onsuccess.begin();
		while(itr != spec->onsuccess.end()) {
			//should we check this crap?
			SummonItem(itr->first, itr->second);
			itr++;
		}
		return(true);
	} else {
		success_modifier = 2; // Halves the chance
		
		if(over_trivial < 0)
			CheckIncreaseTradeskill(bonusstat, stat_modifier, skillup_modifier, success_modifier, tradeskill);
		
		Message_StringID(4,TRADESKILL_FAILED);

		_log(TRADESKILLS__TRACE, "Tradeskill failed");
		
		itr = spec->onfail.begin();
		while(itr != spec->onfail.end()) {
			//should we check these arguments?
			SummonItem(itr->first, itr->second);
			itr++;
		}
	}
	return(false);
}

void Client::CheckIncreaseTradeskill(sint16 bonusstat, sint16 stat_modifier, float skillup_modifier, uint16 success_modifier, SkillType tradeskill)
{
	uint16 current_raw_skill = GetRawSkill(tradeskill);
	int maxskill = MaxSkill(tradeskill);

	if(current_raw_skill >= 252)
		return;	//try not to break things...
	if(maxskill == 254 || current_raw_skill >= maxskill)
		return;	//not allowed to go higher.
	
	float chance_stage2 = 0;

	//A successfull combine doubles the stage1 chance for an skillup
	//Some tradeskill are harder than others. See above for more.
	float chance_stage1 = (bonusstat - stat_modifier) / (skillup_modifier * success_modifier);

	//In stage2 the only thing that matters is your current unmodified skill.
	//If you want to customize here you probbably need to implement your own
	//formula instead of tweaking the below one.
	if (chance_stage1 > MakeRandomFloat(0, 99)) {
		if (current_raw_skill < 15) {
			// Allways succeed
			chance_stage2 = 100;
		} else if (current_raw_skill < 175) {
			//From skill 16 to 174 your chance of success falls linearly from 92% to 13%.
			chance_stage2 = (200 - current_raw_skill) / 2;
		} else {
			//At skill 175, your chance of success falls linearly from 12.5% to 2.5% at skill 300.
			chance_stage2 = 12.5 - (.08 * (current_raw_skill - 175));
		}
	}
	   
	if (chance_stage2 > MakeRandomFloat(0, 99)) {
		//Only if stage1 and stage2 succeeded you get a skillup.
		SetSkill(tradeskill, current_raw_skill + 1);
	}

	_log(TRADESKILLS__TRACE, "...skillup_modifier: %f , success_modifier: %d , stat modifier: %d", skillup_modifier , success_modifier , stat_modifier);
	_log(TRADESKILLS__TRACE, "...Stage1 chance was: %f percent",  chance_stage1);
	_log(TRADESKILLS__TRACE, "...Stage2 chance was: %f percent. 0 percent means stage1 failed",  chance_stage2);
}


bool ZoneDatabase::GetTradeRecipe(const ItemInst* container, uint8 c_type, uint8 tradeskill, 
	DBTradeskillRecipe_Struct *spec)
{
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	char buf2[2048];
	
	uint32 sum = 0;
	uint32 count = 0;
	uint32 qcount = 0;
	uint32 qlen = 0;
	
	//use the world item type as type if we have a world item
	//otherwise use the item's ID... this make the assumption that
	//no tradeskill containers will have an item ID which is
	//below the highest ID of objects, which is currently 0x30
	uint32 type = c_type;
	
	//dunno why I have to cast this up to call GetItem
	const Item_Struct *istruct = ((const ItemInst *) container)->GetItem();
	if(c_type == 0 && istruct) {
		type = istruct->ID;
	}
	
	buf2[0] = '\0';
	
	//Could prolly watch for stacks in this loop and handle them properly...
	//just increment sum and count accordingly
	bool first = true;
	uint8 i;
	char *pos = buf2;
	for (i=0; i<10; i++) {
		const ItemInst* inst = container->GetItem(i);
		if (inst) {
			const Item_Struct* item = GetItem(inst->GetItem()->ID);
			if (item) {
				if(first) {
					pos += snprintf(pos, 19, "%d", item->ID);
					first = false;
				} else {
					pos += snprintf(pos, 19, ",%d", item->ID);
				}
				sum += item->ID;
				count++;
			}
		}
	}
	*pos = '\0';
	
	if(count < 1) {
		return(false);	//no items == no recipe
	}
	
	qlen = MakeAnyLenString(&query, "SELECT tre.recipe_id "
	" FROM tradeskill_recipe_entries AS tre"
	" WHERE ( tre.item_id IN(%s) AND tre.componentcount>0 )"
	"  OR ( tre.item_id=%u AND tre.iscontainer=1 )"
	" GROUP BY tre.recipe_id HAVING sum(tre.componentcount) = %u"
	"  AND sum(tre.item_id * tre.componentcount) = %u", buf2, type, count, sum);
	
	if (!RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept search, query: %s", query);
		safe_delete_array(query);
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept search, error: %s", errbuf);
		return(false);
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount > 1) {
		//multiple recipes, partial match... do an extra query to get it exact.
		//this happens when combining components for a smaller recipe
		//which is completely contained within another recipe
		
		first = true;
		pos = buf2;
		for (i = 0; i < qcount; i++) {
			row = mysql_fetch_row(result);
			uint32 recipeid = (uint32) atoi(row[0]);
			if(first) {
				pos += snprintf(pos, 19, "%u", recipeid);
				first = false;
			} else {
				pos += snprintf(pos, 19, ",%u", recipeid);
			}
			//length sanity check on buf2
			if(pos > (buf2 + 2020))
				break;
		}
		
		qlen = MakeAnyLenString(&query, "SELECT tre.recipe_id"
		" FROM tradeskill_recipe_entries AS tre"
		" WHERE tre.recipe_id IN (%s)"
		" GROUP BY tre.recipe_id HAVING sum(tre.componentcount) = %u"
		"  AND sum(tre.item_id * tre.componentcount) = %u", buf2, count, sum);
		
		if (!RunQuery(query, qlen, errbuf, &result)) {
			LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept, re-query: %s", query);
			safe_delete_array(query);
			LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept, error: %s", errbuf);
			return(false);
		}
		safe_delete_array(query);
	
		qcount = mysql_num_rows(result);
	}
	if (qcount != 1) {
		if(qcount > 1) {
			LogFile->write(EQEMuLog::Error, "Combine error: Recipe is not unique!");
		}
		//else, just not found i guess..
		return(false);
	}
	
	row = mysql_fetch_row(result);
	uint32 recipe_id = (uint32)atoi(row[0]);
	mysql_free_result(result);
	
	return(GetTradeRecipe(recipe_id, c_type, tradeskill, spec));
}
	


bool ZoneDatabase::GetTradeRecipe(uint32 recipe_id, uint8 c_type, uint8 tradeskill, 
	DBTradeskillRecipe_Struct *spec)
{	
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
    char *query = 0;
	
	uint32 qcount = 0;
	uint32 qlen;
	
	qlen = MakeAnyLenString(&query, "SELECT tr.skillneeded, tr.trivial, tr.nofail, tr.replace_container"
	" FROM tradeskill_recipe AS tr"
	" WHERE tr.id = %lu AND tr.tradeskill = %u", recipe_id, tradeskill);
		
	if (!RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept, query: %s", query);
		safe_delete_array(query);
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept, error: %s", errbuf);
		return(false);
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount != 1) {
		//just not found i guess..
		return(false);
	}
	
	row = mysql_fetch_row(result);
	spec->skill_needed		= (sint16)atoi(row[0]);
	spec->trivial			= (uint16)atoi(row[1]);
	spec->nofail			= atoi(row[2]) ? true : false;
	spec->replace_container	= atoi(row[3]) ? true : false;
	mysql_free_result(result);
	
	//Pull the on-success items...
	qlen = MakeAnyLenString(&query, "SELECT item_id,successcount FROM tradeskill_recipe_entries"
	 " WHERE successcount>0 AND componentcount=0 AND recipe_id=%u", recipe_id);
	 
	if (!RunQuery(query, qlen, errbuf, &result)) {
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept success query '%s': %s", query, errbuf);
		safe_delete_array(query);
		return(false);
	}
	safe_delete_array(query);
	
	qcount = mysql_num_rows(result);
	if(qcount < 1) {
		LogFile->write(EQEMuLog::Error, "Error in GetTradeRecept success: no success items returned");
		return(false);
	}
	uint8 r;
	spec->onsuccess.clear();
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);
		/*if(r == 0) {
			*product1_id	= (uint32)atoi(row[0]);
			*productcount	= (uint32)atoi(row[1]);
		} else if(r == 1) {
			*product2_id	= (uint32)atoi(row[0]);
		} else {
			LogFile->write(EQEMuLog::Warning, "Warning: recipe returned more than 2 products, not yet supported.");
		}*/
		uint32 item = (uint32)atoi(row[0]);
		uint8 num = (uint8) atoi(row[1]);
		spec->onsuccess.push_back(pair<uint32,uint8>::pair(item, num));
	}
	mysql_free_result(result);
	
	
	//Pull the on-fail items...
	qlen = MakeAnyLenString(&query, "SELECT item_id,failcount FROM tradeskill_recipe_entries"
	 " WHERE failcount>0 AND componentcount=0 AND recipe_id=%u", recipe_id);

	spec->onfail.clear();
	if (RunQuery(query, qlen, errbuf, &result)) {
		
		qcount = mysql_num_rows(result);
		uint8 r;
		for(r = 0; r < qcount; r++) {
			row = mysql_fetch_row(result);
			/*if(r == 0) {
				*failproduct_id	= (uint32)atoi(row[0]);
			} else {
				LogFile->write(EQEMuLog::Warning, "Warning: recipe returned more than 1 fail product, not yet supported.");
			}*/
			uint32 item = (uint32)atoi(row[0]);
			uint8 num = (uint8) atoi(row[1]);
			spec->onfail.push_back(pair<uint32,uint8>::pair(item, num));
		}
		mysql_free_result(result);
	}
	safe_delete_array(query);
	
	return(true);
}


