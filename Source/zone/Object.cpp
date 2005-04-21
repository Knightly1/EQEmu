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

const char DEFAULT_OBJECT_NAME[] = "IT63_ACTORDEF";
const char DEFAULT_OBJECT_NAME_SUFFIX[] = "_ACTORDEF";

extern Database database;
extern Zone* zone;
extern EntityList entity_list;

// Loading object from database
Object::Object(uint32 id, uint32 type, uint32 icon, const Object_Struct& object, const ItemInst* inst)
 : respawn_timer(0), decay_timer(1800000)
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
	// Copy object data
	memcpy(&m_data, &object, sizeof(Object_Struct));
	if (inst) {
		m_inst = inst->Clone();
		decay_timer.Start();
	} else {
		decay_timer.Disable();
	}
	respawn_timer.Disable();
	
	// Set drop_id to zero - it will be set when added to zone with SetID()
	m_data.drop_id = 0;
}
Object::Object(const ItemInst* inst, char* name,float max_x,float min_x,float max_y,float min_y,float z,float heading,int32 respawntimer)
 : respawn_timer(respawntimer), decay_timer(1800000)
{
	
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
	decay_timer.Disable();
	// Set as much struct data as we can
	memset(&m_data, 0, sizeof(Object_Struct));
	m_data.heading = heading;
	m_data.y = ((rand()%(int)max_y)-(rand()%(int)min_y));
	m_data.x = ((rand()%(int)max_x)-(rand()%(int)min_x));
	//printf("Spawning object %s at %f,%f,%f\n",name,m_data.x,m_data.y,m_data.z);
	m_data.z = z;
	m_data.zone_id = zone->GetZoneID();
	respawn_timer.Disable();
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
 : respawn_timer(0), decay_timer(1800000)
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
	
	decay_timer.Start();
	respawn_timer.Disable();

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
	
	if (m_inst && m_inst->IsType(ItemClassContainer)) {
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

void Object::Close() {
	m_inuse = false;
}

// Remove item from container
void Object::DeleteItem(uint8 index)
{
	if (m_inst && m_inst->IsType(ItemClassContainer)) {
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
	
	if (m_inst && m_inst->IsType(ItemClassContainer)) {
		ItemContainerInst* bag = (ItemContainerInst*)m_inst;
		inst = bag->PopItem(index);
		
		// This is _highly_ inefficient, but for now it will work: Save entire object to database
		Save();
	}
	
	return inst;
}

void Object::CreateSpawnPacket(EQApplicationPacket* app)
{
	app->SetOpcode(OP_GroundSpawn);
	app->pBuffer = new uchar[sizeof(Object_Struct)];
	app->size = sizeof(Object_Struct);
	memcpy(app->pBuffer, &m_data, sizeof(Object_Struct));
}

void Object::CreateDeSpawnPacket(EQApplicationPacket* app)
{
	app->SetOpcode(OP_ClickObject);
	app->pBuffer = new uchar[sizeof(ClickObject_Struct)];
	app->size = sizeof(ClickObject_Struct);
	ClickObject_Struct* co = (ClickObject_Struct*) app->pBuffer;
	co->drop_id = m_data.drop_id;
	co->player_id = 0;
}
bool Object::Process(){
	if(m_type == OT_DROPPEDITEM && decay_timer.Enabled() && decay_timer.Check()) {
		// Send click to all clients (removes entity on client)
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ClickObject, sizeof(ClickObject_Struct));
		ClickObject_Struct* click_object = (ClickObject_Struct*)outapp->pBuffer;
		click_object->drop_id = GetID();
		entity_list.QueueClients(NULL, outapp, false);
		safe_delete(outapp);

		// Remove object
		database.DeleteObject(m_id);
		return false;
	}
	
	if(m_ground_spawn && respawn_timer.Check()){
		respawn_timer.Disable();
		m_data.y = ((rand()%(int)m_max_y)-(rand()%(int)m_min_y));
		m_data.x = ((rand()%(int)m_max_x)-(rand()%(int)m_min_x));
		//printf("Spawning object %s at %f,%f,%f\n",m_data.object_name,m_data.x,m_data.y,m_data.z);
		EQApplicationPacket app;
		CreateSpawnPacket(&app);
		entity_list.QueueCloseClients(0,&app,true);
	}
	return true;
}
bool Object::HandleClick(Client* sender, const ClickObject_Struct* click_object)
{
	if(m_ground_spawn){//This is a Cool Groundspawn
			respawn_timer.Start();
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
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ClickObject, sizeof(ClickObject_Struct));
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
		EQApplicationPacket* outapp = new EQApplicationPacket(OP_ClickObjectAck, sizeof(ClickObjectAck_Struct));
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

		if (m_inst && m_inst->IsType(ItemClassContainer)) {

			//Clear out no-drop and no-rent items first
			//TODO: should/could only do this if a different player opens it
			ItemContainerInst* container = (ItemContainerInst*)m_inst;
			container->ClearByFlags(byFlagSet, byFlagSet);
			
			EQApplicationPacket* outapp=new EQApplicationPacket(OP_ClientReady,0);
			sender->QueuePacket(outapp);
			safe_delete(outapp);
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




