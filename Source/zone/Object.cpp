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

#include "../common/debug.h"
#include <iostream>
#include <stdlib.h>

#include "masterentity.h"
#include "../common/database.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"
#include "StringIDs.h"
using namespace std;

const unsigned int MIN_LEVEL_ALCHEMY = 25;
const char DEFAULT_OBJECT_NAME[] = "IT63_ACTORDEF";
const char DEFAULT_OBJECT_NAME_SUFFIX[] = "_ACTORDEF";

extern Database database;
extern Zone* zone;
extern EntityList entity_list;

// Loading object from database
Object::Object(uint32 id, uint32 type, uint32 icon, const Object_Struct& object, const ItemInst* inst)
{
	// Invoke base class
	Entity::Entity();
	
	// Initialize members
	m_id = id;
	m_type = type;
	m_icon = icon;
	m_inuse = false;
	m_inst = NULL;
	m_ground_spawn=false;
	decay_timer = NULL;
	// Copy object data
	memcpy(&m_data, &object, sizeof(Object_Struct));
	if (inst) {
		m_inst = inst->Clone();
		decay_timer = new Timer(1800000);
		decay_timer->Start();
	}
	
	// Set drop_id to zero - it will be set when added to zone with SetID()
	m_data.drop_id = 0;
}
Object::Object(const ItemInst* inst, char* name,float max_x,float min_x,float max_y,float min_y,float z,float heading,int32 respawntimer){
	
	m_max_x=max_x;
	m_max_y=max_y;
	m_min_x=min_x;
	m_min_y=min_y;
	Entity::Entity();
	m_id	= 0;
	m_inst	= (inst) ? inst->Clone() : NULL;
	m_type	= OT_DROPPEDITEM;
	m_icon	= 0;
	m_inuse	= false;
	m_ground_spawn = true;
	decay_timer = NULL;
	// Set as much struct data as we can
	memset(&m_data, 0, sizeof(Object_Struct));
	m_data.heading = heading;
	m_data.y = ((rand()%(int)max_y)-(rand()%(int)min_y));
	m_data.x = ((rand()%(int)max_x)-(rand()%(int)min_x));
	//printf("Spawning object %s at %f,%f,%f\n",name,m_data.x,m_data.y,m_data.z);
	m_data.z = z;
	m_data.zone_id = zone->GetZoneID();
	respawn=new Timer(respawntimer);
	respawn->Disable();
	// Hardcoded portion for unknown members
	m_data.unknown020[1] = 0x00001194;
	m_data.unknown060[0] = 0x0000000D;
	m_data.unknown060[1] = 0x0000001E;
	m_data.unknown060[2] = 0x000032ED;
	m_data.unknown060[4] = 0xFFFFFFFF;
	m_data.unknown084[0] = 0xFFFFFFFF;
	strcpy(m_data.object_name, name);
}
// Loading object from client dropping item on ground
Object::Object(Client* client, const ItemInst* inst)
{
	// Invoke base class
	Entity::Entity();
	
	// Initialize members
	m_id	= 0;
	m_inst	= (inst) ? inst->Clone() : NULL;
	m_type	= OT_DROPPEDITEM;
	m_icon	= 0;
	m_inuse	= false;
	m_ground_spawn = false;
	// Set as much struct data as we can
	memset(&m_data, 0, sizeof(Object_Struct));
	m_data.heading = client->GetHeading();
	m_data.y = client->GetX();
	m_data.x = client->GetY();
	m_data.z = client->GetZ();
	m_data.zone_id = zone->GetZoneID();

	decay_timer = new Timer(1800000);
	decay_timer->Start();

	// Hardcoded portion for unknown members
	m_data.unknown020[1] = 0x00001194;
	m_data.unknown060[0] = 0x0000000D;
	m_data.unknown060[1] = 0x0000001E;
	m_data.unknown060[2] = 0x000032ED;
	m_data.unknown060[4] = 0xFFFFFFFF;
	m_data.unknown084[0] = 0xFFFFFFFF;
	
	// Set object name
	if (inst) {
		const Item_Struct* item = inst->GetItem();
		if (item && item->IDFile) {
			if (strlen(item->IDFile) == 0) {
				strcpy(m_data.object_name, DEFAULT_OBJECT_NAME);
			}
			else {
				// Object name is idfile + _ACTORDEF
				uint32 len_idfile = strlen(inst->GetItem()->IDFile);
				uint32 len_copy = sizeof(m_data.object_name) - len_idfile - 1;
				if (len_copy > sizeof(DEFAULT_OBJECT_NAME_SUFFIX)) {
					len_copy = sizeof(DEFAULT_OBJECT_NAME_SUFFIX);
				}
				
				memcpy(&m_data.object_name[0], inst->GetItem()->IDFile, len_idfile);
				memcpy(&m_data.object_name[len_idfile], DEFAULT_OBJECT_NAME_SUFFIX, len_copy);
			}
		}
		else {
			strcpy(m_data.object_name, DEFAULT_OBJECT_NAME);
		}
	}
}

Object::~Object()
{
	safe_delete(m_inst);
	safe_delete(decay_timer);
}

void Object::SetID(int16 set_id)
{
	// Invoke base class
	Entity::SetID(set_id);
	
	// Store new id as drop_id
	m_data.drop_id = (uint32)this->GetID();
}

// Reset state of object back to zero
void Object::ResetState()
{
	safe_delete(m_inst);
	
	m_id	= 0;
	m_type	= 0;
	m_icon	= 0;
	memset(&m_data, 0, sizeof(Object_Struct));
}

bool Object::Save()
{
	if (m_id) {
		// Update existing
		database.UpdateObject(m_id, m_type, m_icon, m_data, m_inst);
	}
	else {
		// Doesn't yet exist, add now
		m_id = database.AddObject(m_type, m_icon, m_data, m_inst);
	}
	
	return true;
}

// Remove object from database
void Object::Delete(bool reset_state)
{
	if (m_id != 0) {
		database.DeleteObject(m_id);
	}
	
	if (reset_state) {
		ResetState();
	}
}

// Add item to object (only logical for world tradeskill containers
void Object::PutItem(uint8 index, const ItemInst* inst)
{
	if (index > 9) {
		LogFile->write(EQEMuLog::Error, "Object::PutItem: Invalid index specified (%i)", index);
		return;
	}
	
	if (m_inst && m_inst->IsType(ItemTypeContainer)) {
		ItemContainerInst* bag = (ItemContainerInst*)m_inst;
		if (inst) {
			bag->PutItem(index, *inst);
		}
		else {
			bag->DeleteItem(index);
		}
		database.SaveWorldContainer(zone->GetZoneID(),m_id,bag);
		// This is _highly_ inefficient, but for now it will work: Save entire object to database
		//Save();
	}
}

// Remove item from container
void Object::DeleteItem(uint8 index)
{
	if (m_inst && m_inst->IsType(ItemTypeContainer)) {
		ItemContainerInst* bag = (ItemContainerInst*)m_inst;
		bag->DeleteItem(index);
		
		// This is _highly_ inefficient, but for now it will work: Save entire object to database
		Save();
	}
}

// Pop item out of container
ItemInst* Object::PopItem(uint8 index)
{
	ItemInst* inst = NULL;
	
	if (m_inst && m_inst->IsType(ItemTypeContainer)) {
		ItemContainerInst* bag = (ItemContainerInst*)m_inst;
		inst = bag->PopItem(index);
		
		// This is _highly_ inefficient, but for now it will work: Save entire object to database
		Save();
	}
	
	return inst;
}

void Object::CreateSpawnPacket(APPLAYER* app)
{
	app->opcode = OP_CreateObject;
	app->pBuffer = new uchar[sizeof(Object_Struct)];
	app->size = sizeof(Object_Struct);
	memcpy(app->pBuffer, &m_data, sizeof(Object_Struct));
}

void Object::CreateDeSpawnPacket(APPLAYER* app)
{
	app->opcode = OP_ClickObject;
	app->pBuffer = new uchar[sizeof(ClickObject_Struct)];
	app->size = sizeof(ClickObject_Struct);
	ClickObject_Struct* co = (ClickObject_Struct*) app->pBuffer;
	co->drop_id = m_data.drop_id;
	co->player_id = 0;
}
bool Object::Process(){
	if(decay_timer && decay_timer->Check())
		return false;
	if(m_ground_spawn && respawn->Check()){
		respawn->Disable();
		m_data.y = ((rand()%(int)m_max_y)-(rand()%(int)m_min_y));
		m_data.x = ((rand()%(int)m_max_x)-(rand()%(int)m_min_x));
		//printf("Spawning object %s at %f,%f,%f\n",m_data.object_name,m_data.x,m_data.y,m_data.z);
		APPLAYER app;
		CreateSpawnPacket(&app);
		entity_list.QueueCloseClients(0,&app,true);
	}
	return true;
}
bool Object::HandleClick(Client* sender, const ClickObject_Struct* click_object)
{
	if(m_ground_spawn){//This is a Cool Groundspawn
			respawn->Start();
	}
	if (m_type == OT_DROPPEDITEM) {
		if (m_inst && sender) {
			// Transfer item to client
			sender->PutItemInInventory(SLOT_CURSOR, *m_inst, false);
			sender->SendItemPacket(SLOT_CURSOR, m_inst, ItemPacketTrade);
			if(!m_ground_spawn)
				safe_delete(m_inst);
			
			// No longer using a tradeskill object
			sender->SetTradeskillObject(NULL);
		}
		
		// Send click to all clients (removes entity on client)
		APPLAYER* outapp = new APPLAYER(OP_ClickObject, sizeof(ClickObject_Struct));
		memcpy(outapp->pBuffer, click_object, sizeof(ClickObject_Struct));
		entity_list.QueueClients(NULL, outapp, false);
		safe_delete(outapp);
		
		// Remove object
		database.DeleteObject(m_id);
		if(!m_ground_spawn)
		entity_list.RemoveEntity(this->GetID());
	}
	else {
		// Tradeskill item
		APPLAYER* outapp = new APPLAYER(OP_ClickObjectAck, sizeof(ClickObjectAck_Struct));
		ClickObjectAck_Struct* coa = (ClickObjectAck_Struct*)outapp->pBuffer;
		
			// Starting to use this object
			sender->SetTradeskillObject(this);
			coa->open		= 0x01;
			m_inuse			= true;
			coa->type		= m_type;
			coa->unknown16	= 0x0a;	
		
		coa->drop_id	= click_object->drop_id;
		coa->player_id	= click_object->player_id;
		coa->icon		= m_icon;
		
		sender->QueuePacket(outapp);
		safe_delete(outapp);
		// Send items inside of container

			if (m_inst && m_inst->IsType(ItemTypeContainer)) {
				APPLAYER* outapp=new APPLAYER(OP_ClientReady,0);
				sender->QueuePacket(outapp);
				safe_delete(outapp);
				ItemContainerInst* container = (ItemContainerInst*)m_inst;
				for (uint8 i=0; i<10; i++) {
					const ItemInst* inst = container->GetItem(i);
					if (inst) {
						//sender->GetInv().PutItem(i+4000,inst);
						sender->SendItemPacket(i, inst, ItemPacketWorldContainer);
					}
				}
			}
	}
	
	return true;
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
//this is supposed to be PackType, but for whatever reason, 
//the real value is in Slots
//If I reoder ItemContainer_Struct, it pisses everything else off
				tradeskill = item->Container.Slots;
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

