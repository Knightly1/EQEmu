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
#include "../common/database.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"
#include "StringIDs.h"

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
	ItemContainerInst* container = NULL;
	ItemInst* inst = NULL;
	uint8 tradeskill = 0xE8;
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
			if (item && inst->IsType(ItemTypeContainer)) {
				tradeskill = item->Container.PackType;
			}
		}
	}
	
	if (!inst || !inst->IsType(ItemTypeContainer)) {
		user->Message(13, "Error: Server does not recognize specified tradeskill container");
		return;
	}
	
	container = (ItemContainerInst*)inst;
	
	// Convert container type to tradeskill type
	switch (tradeskill)
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
		tradeskill = 75;// Making our own type here
		break;
	case 46: //Fishing Still needs completion
		tradeskill = FISHING;
		break;
	default:
		user->Message(13, "This tradeskill has not been implemented yet, if you get this message send a "
			"petition and let them know what tradeskill you were trying to use. and give them the following code: 0x%02X", tradeskill);
	}
	
	if (tradeskill == 0) {
		return;
	}
	
	DBTradeskillRecipe_Struct spec;
	if (!database.GetTradeRecipe(container, passtype, tradeskill, &spec)) {
		user->Message_StringID(4,TRADESKILL_NOCOMBINE);
		APPLAYER* outapp = new APPLAYER(OP_TradeSkillCombine, 0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		return;
	}
	
	//do the check and send results...
	user->TradeskillExecute(&spec, tradeskill);
	
	// Send acknowledgement packets to client
	APPLAYER* outapp = new APPLAYER(OP_TradeSkillCombine, 0);
	user->QueuePacket(outapp);
	safe_delete(outapp);
	if(worldcontainer){
		container->Clear();
		outapp = new APPLAYER(OP_ClearObject,0);
		user->QueuePacket(outapp);
		safe_delete(outapp);
		database.DeleteWorldContainer(worldo->m_id, zone->GetZoneID());
	} else{
		for (uint8 i=0; i<10; i++){
			const ItemInst* inst = container->GetItem(i);
			if (inst)
				user->DeleteItemInInventory(Inventory::CalcSlotId(in_combine->container_slot,i),0,true);
		}
		container->Clear();
	}
}

void Object::HandleAutoCombine(Client* user, const RecipeAutoCombine_Struct* rac) {
	
	//get our packet ready, gotta send one no matter what...
	APPLAYER* outapp = new APPLAYER(OP_RecipeAutoCombine, sizeof(RecipeAutoCombine_Struct));
	RecipeAutoCombine_Struct *outp = (RecipeAutoCombine_Struct *)outapp->pBuffer;
	outp->object_type = rac->object_type;
	outp->some_id = rac->some_id;
	outp->unknown1 = rac->unknown1;
	outp->recipe_id = rac->recipe_id;
	outp->reply_code = 0xFFFFFFF5;	//default fail.
	
	
	uint32 tskill = Object::TypeToSkill(rac->object_type);
	if(tskill == 0) {
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
	
	user->TradeskillExecute(&spec, tskill);
	
}

uint32 Object::TypeToSkill(uint32 type) {
	uint32 tradeskill = 0;
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
		case OT_FIERDALF:
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
		case OT_POTTERYWHEEL: {
			tradeskill = POTTERY;
			break;
		}
		case OT_OVEN:
		case OT_KILN: {
			tradeskill = BAKING;
			break;
		}
		case OT_TACKLEBOX: {
			tradeskill = FISHING;
			break;
		}
		case OT_KEYMAKER: { //unknown for now...
			tradeskill = 0;
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
	//I could prolly get away with allocating a single APPLAYER, and
	//just re-using it, but this is safe, and im not sure.
	for(r = 0; r < qcount; r++) {
		row = mysql_fetch_row(result);
		uint32 recipe = (uint32)atoi(row[0]);
		const char *name = row[1];
		uint32 trivial = (uint32) atoi(row[2]);
		uint32 comp_count = (uint32) atoi(row[3]);
		
		APPLAYER* outapp = new APPLAYER(OP_RecipeReply, sizeof(RecipeReply_Struct));
		RecipeReply_Struct *reply = (RecipeReply_Struct *) outapp->pBuffer;
		
		reply->object_type = objtype;
		reply->some_id = someid;
		reply->component_count = comp_count;
		reply->recipe_id = recipe;
		reply->trivial = trivial;
		strncpy(reply->recipe_name, name, 63);
		
		QueuePacket(outapp);
		//DumpPacket(outapp);
		safe_delete(outapp);
	}
	mysql_free_result(result);
}

void Client::SendTradeskillDetails(unsigned long recipe_id) {

//from server in response to a 4 byte OP_RecipeDetails, just the item id
/*struct RecipeDetails_Struct {
	unsigned long recipe_id;	//backwards byte order from the Reply
	//dynamic part...
	// there are as many as 10 0xFFFFFFFF here in a row..
	// there are 10 - component count of them...
	
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
	char *buf = new char[775];	//dynamic so we can just give it to APPLAYER
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
	
	APPLAYER* outapp = new APPLAYER(OP_RecipeDetails);
	outapp->size = total;
	outapp->pBuffer = (uchar*) buf;
	QueuePacket(outapp);
	DumpPacket(outapp);
	safe_delete(outapp);
}

void Client::TradeskillExecute(DBTradeskillRecipe_Struct *spec, uint16 tradeskill) {
	if(spec == NULL || tradeskill == 0)
		return;
	
	sint16 user_skill = (sint16) GetSkill(tradeskill);
	float chance = 0;
	
	// statbonus 20%/10% with 200 + 0.05% / 0.025% per point above 200
	float wisebonus =  (m_pp.WIS > 200) ? 20 + ((m_pp.WIS - 200) * 0.05) : m_pp.WIS * 0.1;
	float intbonus =  (m_pp.INT > 200) ? 10 + ((m_pp.INT - 200) * 0.025) : m_pp.INT * 0.05;
	
	vector< pair<uint32,uint8> >::iterator itr;
	
	//Reworked this because it seemed to use spec->skill_needed as spec->trivial...
	if(spec->nofail) {
		chance = 100;	//cannot fail.
	} else if(((sint16)user_skill - (sint16)spec->skill_needed) < 0) {
		chance = 0;
		//impossible... is there a message for this???
	} else if (((sint16)user_skill - (sint16)spec->trivial) >= 0) {
		chance = 80+wisebonus-10; // 80% basechance + max 20% stats
		Message_StringID(4,TRADESKILL_TRIVIAL);
	} else {
		if ((spec->trivial - user_skill) < 20) {
			// 40 base chance success + max 40% skill + 20% max stats
			chance = 40 + wisebonus + 40 - ((spec->trivial - user_skill)*2);
		}
		else {
			// 0 base chance success + max 30% skill + 10% max stats
			chance = 0 + (wisebonus/2) + 30 - (((spec->trivial - user_skill) * (spec->trivial - user_skill))*0.01875);
		}
		
//Is there a reason we dont use CheckIncreaseSkill()?
		// skillincrease?
		if ((55-(user_skill*0.236))+intbonus > (float)rand()/RAND_MAX*100) {
			SetSkill(tradeskill, GetRawSkill(tradeskill) + 1);
			//Message(4, "You have become better at (skillid=%i)", tradeskill);
		}
	}
	
	float res = ((float)rand()/RAND_MAX*100);
	if ((tradeskill==75) || GetGM() || (chance > res)){
		Message_StringID(4,TRADESKILL_SUCCEED);
		
		itr = spec->onsuccess.begin();
		while(itr != spec->onsuccess.end()) {
			//should we check this crap?
			SummonItem(itr->first, itr->second);
			itr++;
		}
	} else {
		Message_StringID(4,TRADESKILL_FAILED);
		
		itr = spec->onfail.begin();
		while(itr != spec->onfail.end()) {
			//should we check these arguments?
			SummonItem(itr->first, itr->second);
			itr++;
		}
	}
}



