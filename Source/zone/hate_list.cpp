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
#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include "masterentity.h"
#include "../common/linked_list.h"
#include "hate_list.h"

class tHateEntry
{
public:
    Mob *ent;
    sint32 damage, hate;
    bool bFrenzy;
};

HateList::HateList()
{
}

HateList::~HateList()
{
}

// neotokyo: added for frenzy support
// checks if target still is in frenzy mode
void HateList::CheckFrenzyHate()
{
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        if (iterator.GetData()->ent->GetHPRatio() >= 20)
            iterator.GetData()->bFrenzy = false;
        iterator.Advance();
    }
}

void HateList::Wipe()
{
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
        iterator.RemoveCurrent();
}

bool HateList::IsOnHateList(Mob *mob)    
{    
	if (Find(mob))    
		return true;    
	return false;    
} 

tHateEntry *HateList::Find(Mob *ent)
{
	_ZP(HateList_Find);
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        if (iterator.GetData()->ent == ent)
            return iterator.GetData();
        iterator.Advance();
    }
	return NULL;
}

void HateList::Set(Mob* other, int32 in_hate, int32 in_dam)
{
    tHateEntry *p = Find(other);
    if (p)
    {
		if (in_dam > 0)
        	p->damage = in_dam;
		if (in_hate > 0)
    		p->hate = in_hate;
    }
}

Mob* HateList::GetDamageTop(Mob* hater)
{
	_ZP(HateList_GetDamageTop);
	Mob* current = NULL;
	Group* grp = NULL;
	int32 dmg_amt = 0;

    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        grp = entity_list.GetGroupByMob(iterator.GetData()->ent);

		if (iterator.GetData()->ent != NULL && grp != NULL)
        {
			if (grp->GetTotalGroupDamage(hater) >= dmg_amt)
            {
				current = iterator.GetData()->ent;
				dmg_amt = grp->GetTotalGroupDamage(hater);
            }
        }
        else if (iterator.GetData()->ent != NULL && (int32)iterator.GetData()->damage >= dmg_amt)
        {
			current = iterator.GetData()->ent;
			dmg_amt = iterator.GetData()->damage;
        }
        iterator.Advance();
    }
	return current;
}

Mob* HateList::GetClosest(Mob *hater) {
	_ZP(HateList_GetClosest);
	Mob* close = NULL;
	float closedist = 99999.9f;
	float thisdist;
	
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements()) {
		thisdist = iterator.GetData()->ent->DistNoRootNoZ(*hater);
		if(iterator.GetData()->ent != NULL && thisdist <= closedist) {
			closedist = thisdist;
			close = iterator.GetData()->ent;
		}
        iterator.Advance();
	}
	
	if (close == 0 && hater->IsNPC())
		close = hater->CastToNPC()->GetHateTop();
	
	return close;
}


// neotokyo: a few comments added, rearranged code for readability
void HateList::Add(Mob *ent, sint32 in_hate, sint32 in_dam, bool bFrenzy, bool iAddIfNotExist)
{
	if(!ent)
        return;

    tHateEntry *p = Find(ent);
    if (p)
    {
		p->damage+=in_dam;
		p->hate+=in_hate;
        p->bFrenzy = bFrenzy;
    }
	else if (iAddIfNotExist) {
        p = new tHateEntry;
        p->ent = ent;
        p->damage = in_dam;
        p->hate = in_hate;
        p->bFrenzy = bFrenzy;
        list.Append(p);
    }
}

bool HateList::RemoveEnt(Mob *ent)
{
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        if (iterator.GetData()->ent == ent)
        {
            iterator.RemoveCurrent();
        }
        else
            iterator.Advance();
    }
    return false;
}

void HateList::DoFactionHits(sint32 nfl_id) {
	if (nfl_id <= 0)
		return;
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        Client *p;

        if (iterator.GetData()->ent && iterator.GetData()->ent->IsClient())
            p = iterator.GetData()->ent->CastToClient();
        else
            p = NULL;

        if (p)
			p->SetFactionLevel(p->CharacterID(), nfl_id, p->GetClass(), p->GetRace(), p->GetDeity());
        iterator.Advance();
    }
}

Mob *HateList::GetTop()
{
	_ZP(HateList_GetTop);
	Mob* top = NULL;
	sint32 hate = -1;
	
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
    	tHateEntry *cur = iterator.GetData();
		if(cur->ent != NULL && ((cur->hate > hate) || cur->bFrenzy ))
		{
            top = cur->ent;
            hate = cur->hate;
		}
        iterator.Advance();
	}
    return top;
}

Mob *HateList::GetRandom()
{
    int count = 0;
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
        iterator.Advance();
        count++;
    }
    int random = rand()%count;
    iterator.Reset();
    for (int i = 0; i < random-1; i++)
        iterator.Advance();
    return iterator.GetData()->ent;
}

sint32 HateList::GetEntHate(Mob *ent, bool damage)
{
	tHateEntry *p;

    p = Find(ent);
	
	if ( p && damage)
        return p->damage;
	else if (p)
		return p->hate;
	else
		return 0;
}

//looking for any mob with hate > -1
bool HateList::IsEmpty() {
	_ZP(HateList_IsEmpty);
	
    LinkedListIterator<tHateEntry*> iterator(list);
    iterator.Reset();
	while(iterator.MoreElements())
    {
    	tHateEntry *cur = iterator.GetData();
		if(cur->ent != NULL
			&& (
				(cur->hate > -1) 
				|| cur->bFrenzy )
			)
		{
			return(false);
		}
        iterator.Advance();
	}
    return(true);
}
