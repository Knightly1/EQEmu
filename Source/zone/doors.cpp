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
#include <iostream>
#include <stdlib.h>
using namespace std;

#include "masterentity.h"
#include "StringIDs.h"
#include "../common/database.h"
#include "../common/packet_functions.h"
#include "../common/packet_dump.h"

#include <string.h>

extern Database database;
extern EntityList entity_list;

Doors::Doors(const Door* door)
:    close_timer(5000)
{
    db_id = door->db_id;
    door_id = door->door_id;
    strncpy(zone_name,door->zone_name,16);
    strncpy(door_name,door->door_name,16);
    pos_x = door->pos_x;
    pos_y = door->pos_y;
    pos_z = door->pos_z;
    heading = door->heading;
    incline = door->incline;
    opentype = door->opentype;
    guildid = door->guildid;
    lockpick = door->lockpick;
    keyitem = door->keyitem;
    trigger_door = door->trigger_door;
    trigger_type = door->trigger_type;
	triggered=false;
    liftheight = door->liftheight;
    invert_state = door->invert_state;
		SetOpenState(false);

    close_timer.Disable();
    
    strncpy(dest_zone,door->dest_zone,16);
    dest_x = door->dest_x;
    dest_y = door->dest_y;
    dest_z = door->dest_z;
    dest_heading = door->dest_heading;

}

Doors::~Doors()
{
}

bool Doors::Process()
{
    if(close_timer.Enabled() && close_timer.Check() && IsDoorOpen())
    {
		triggered=false;
        close_timer.Disable();
        SetOpenState(false);
    }
	return true;
}

void Doors::HandleClick(Client* sender)
{
 #if EQDEBUG>=5  
        LogFile->write(EQEMuLog::Debug, "Doors:HandleClick(%s)", sender->GetName());
        DumpDoor();
#endif
	if(GetTriggerType() == 255) { // this object isnt triggered
		return;
	}
#if 1
    APPLAYER* outapp = new APPLAYER(OP_MoveDoor, sizeof(MoveDoor_Struct));
	MoveDoor_Struct* md=(MoveDoor_Struct*)outapp->pBuffer;
	//DumpPacket(app);
	md->doorid = door_id;
	/////////////////////////////////////////////////////////////////
	//used_pawn: Locked doors! Rogue friendly too =) 
	//TODO: add check for other lockpick items 
	//////////////////////////////////////////////////////////////////

	int keyneeded=GetKeyItem(), playerkey=sender->GetItemIDAt(SLOT_CURSOR);

	if( (keyneeded==0 && GetLockpick() == 0)
	   || (IsDoorOpen() && opentype == 58))
	{	//door not locked
		if( !IsDoorOpen() || opentype == 58 )
		{
			md->action = 0x02; 
		} 
		else
		{ 
			md->action = 0x03; 
		} 
	} 
	else
	{ // a key is required or the door is locked but can be picked or both
		sender->Message(4,"This is locked...");		// debug spam - should probably go
	    if (sender->GetGM())		// GM can always open locks - should probably be changed to require a key
		{
			sender->Message_StringID(4,DOORS_GM);
			if( !IsDoorOpen() || opentype == 58 )
			{ 
				md->action = 0x02; 
			} 
			else
			{ 
				md->action = 0x03; 
			} 
		}
		else if (playerkey)
		{	// they have something they are trying to open it with
			if (keyneeded && keyneeded == playerkey)
			{	// key required and client is using the right key 
				sender->Message(4,"You got it open!");		// more debug spam
				if( !IsDoorOpen() || opentype == 58 )
				{ 
					md->action = 0x02; 
				} 
				else
				{ 
					md->action = 0x03; 
				} 
			}
			else
			{	// door is locked, either no key works for it or player doesn't have the key - try to pick it
				if(sender->MaxSkill(35, sender->GetClass(), sender->GetLevel())>0 && GetLockpick()!=0)
				{	// client has the lock pick skill and this lock can be picked
					float modskill=0.0f; 
					const ItemInst* inst = sender->GetInv().GetItem(SLOT_CURSOR);
					if (inst && inst->IsType(ItemTypeCommon)
						&& inst->GetItem()->Common.ItemUse == ItemUseLockPick)
					{	// we can try to pick the lock with these lock picking tools
						modskill=sender->GetSkill(PICK_LOCK);
						
						//WR: Check the 2nd arg to this, was 25...
						sender->CheckIncreaseSkill(PICK_LOCK, 1);
#if EQDEBUG>=5
						LogFile->write(EQEMuLog::Debug,"Client has lockpicks: skill=%f", modskill);
#endif
						if(GetLockpick() <= modskill)
						{ // lockpick is the minimum skill needed to open the lock
							
							if(!IsDoorOpen())
							{ 
								md->action = 0x02; 
							} 
							else
							{ 
								md->action = 0x03; 
							}
							sender->Message_StringID(4,DOORS_SUCCESSFUL_PICK);
						} 
						else
						{	// failed to pick the lock
							sender->Message_StringID(4,DOORS_INSUFFICIENT_SKILL);
							return;
						} 
					} 
					else
					{	// those aren't lock picks the client is using
						sender->Message_StringID(4,DOORS_NO_PICK);
						return;
					} 
				}
				else
				{	// they can't pick this lock - skill not available or lock not pickable
					sender->Message_StringID(4,DOORS_CANT_PICK);
					return;
				}
			}
		}
		else
		{	// locked door and nothing to open it with
			sender->Message_StringID(4,DOORS_LOCKED);
			return;
		}
	}
	

	entity_list.QueueClients(sender, outapp, false);
	safe_delete(outapp);

	if(GetTriggerDoorID() != 0)
	{
		Doors* triggerdoor = entity_list.FindDoor(GetTriggerDoorID());
		if(triggerdoor && !triggerdoor->triggered)
		{
			printf("Door %d triggering door %d\n", GetDoorID(), triggerdoor->GetDoorID());
			triggered=true;
			triggerdoor->HandleClick(sender);
		}
		else
		{
			triggered=false;
		}
	}

    if(!IsDoorOpen() || opentype == 58) {
        close_timer.Start();
				SetOpenState(true);
    }
    else {
        close_timer.Disable();
				SetOpenState(false);
    }
#endif	// 1

    if (opentype == 58 && strncmp(dest_zone,"NONE",sizeof("NONE")) != 0 ){ // Teleport door!
        if ( strncmp(dest_zone,zone_name,sizeof(zone_name)) == 0){
            sender->GMMove(dest_x,dest_y,dest_z);
        }
        else {
           	sender->MovePC(dest_zone, dest_x, dest_y, dest_z);
        }
    }
}

void Doors::NPCOpen(NPC* sender)
{
	if(GetTriggerType() == 255 || GetTriggerDoorID() > 0 || GetLockpick() != 0 || GetKeyItem() != 0 || opentype == 59 || opentype == 58) { // this object isnt triggered or door is locked - NPCs should not open locked doors!
		return;
	}
    APPLAYER* outapp = new APPLAYER(OP_MoveDoor, sizeof(MoveDoor_Struct));
	MoveDoor_Struct* md=(MoveDoor_Struct*)outapp->pBuffer;
	md->doorid = door_id;
	md->action = 0x02;
	entity_list.QueueCloseClients(sender,outapp,false,200);
	safe_delete(outapp);

    if(!isopen) {
        close_timer.Start();
        isopen=true;
    }
    else {
        close_timer.Disable();
        isopen=false;
    }
}

void Doors::DumpDoor(){
    LogFile->write(EQEMuLog::Debug,
        "db_id:%i door_id:%i zone_name:%s door_name:%s pos_x:%f pos_y:%f pos_z:%f heading:%f",
        db_id, door_id, zone_name, door_name, pos_x, pos_y, pos_z, heading);
    LogFile->write(EQEMuLog::Debug,
        "opentype:%i guildid:%i lockpick:%i keyitem:%i trigger_door:%i trigger_type:%i liftheight:%i open:%s",
        opentype, guildid, lockpick, keyitem, trigger_door, trigger_type, liftheight, (isopen) ? "open":"closed");
    LogFile->write(EQEMuLog::Debug,
        "dest_zone:%s dest_x:%f dest_y:%f dest_z:%f dest_heading:%f",
        dest_zone, dest_x, dest_y, dest_z, dest_heading);
}

