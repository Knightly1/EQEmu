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
#include "../common/moremath.h"
#include <iostream.h>
#include <stdlib.h>
#include <math.h>
#ifdef WIN32
#define snprintf	_snprintf
#endif

#include "NpcAI.h"
#include "entity.h"
#include "npc.h"
#include "client.h"
#include "faction.h"
#include "spdat.h"

extern EntityList entity_list;
#ifndef NEW_LoadSPDat
	extern SPDat_Spell_Struct spells[SPDAT_RECORDS];
#endif
extern Database database;
extern Zone *zone;

//////////////////////////////////////////////////
//
//  GetAILevel
//      return: The database value of AILevel.
//  tries to read the value once and sets to zero if no success
sint8 NPC::GetAILevel(bool iForceReRead)
{
    static sint8 iAILevel = -1;

    if (iAILevel >= 0 && !iForceReRead)
        return iAILevel;
    else
    {
        char tmp[10];
        if (database.GetVariable("AILevel", tmp, 9))
        {
            iAILevel = int8(atoi(tmp));
            //g_LogFile.write("AI LEVEL set to %d\n",iAILevel);
        }
        else
            iAILevel = 0;
    }
    return iAILevel;
}

/* return true as long as you want to NPC to be active,
return false if you want the NPC to be depopped forever */
bool NPC::ProcessAI()
{
	// Limit the AI to executing every 50ms to keep processor usage down
	if (!(npcai_timer->Check() || attack_timer->Check(false)))
		return true;
//	if (GetID() == 0)
//		cout << "EQDEBUG: " << GetID() << ": " << Timer::GetCurrentTime() << endl;
    //float remaining_dist;

    switch( m_Status )
    {
    case S_SPAWNING:
		adverrorinfo = 4;
    	SaveSpawnSpot();
        SetAttackTimer();
        if (GetOwnerID())
            m_Status = S_PETFOLLOWMASTER;
        else
        {
            m_Status = S_BUFFINGSELF;
        }
        return true;
    case S_RANDOMWAITFORBUFFING:
		adverrorinfo = 5;
        if (IsEngaged())
        {
            m_RememberStatus = S_BUFFINGSELF;
            m_Status = S_FIGHTING;
            return true;
        }
        if (randombuff_timer->Check())
        {
            randombuff_timer->Disable();
            delete randombuff_timer;
            m_Status = S_BUFFINGSELF;
        }
        return true;
    case S_BUFFINGSELF:
		adverrorinfo = 6;
        if (IsEngaged())
        {
            if (m_RememberStatus == S_UNKNOWN)
                m_RememberStatus = S_BUFFINGSELF;
            m_Status = S_FIGHTING;
            return true;
        }
   	    if (this->CheckSelfBuffs() == false)
		{
            m_Status = S_BUFFINGOTHERS;
		}
        return true;
    case S_BUFFINGOTHERS:
		adverrorinfo = 7;
        if (IsEngaged())
        {
            if (m_RememberStatus == S_UNKNOWN)
                m_RememberStatus = S_BUFFINGOTHERS;
            m_Status = S_FIGHTING;
            return true;
        }
        if (entity_list.CheckOthersBuffs(this) == false) {
            if (roamer)
                m_Status = S_ROAMING;
            else {
				if (heading != GetSpawnHeading()) {
					heading = GetSpawnHeading();
					pLastChange = Timer::GetCurrentTime();
				}
                m_Status = S_CHECKINGAREA;
            }
 		}
        return true;
    case S_CHECKINGAREA:
		adverrorinfo = 8;
        if (IsEngaged())
        {
            m_RememberStatus = S_CHECKINGAREA;
            m_Status = S_FIGHTING;
            return true;
        }
		adverrorinfo = 822;
        if (scanarea_timer->Check())
        {
			adverrorinfo = 824;
            if (entity_list.CheckCloseMobs(this))
            {
				adverrorinfo = 833;
                m_RememberStatus = S_CHECKINGAREA;
                m_Status = S_FIGHTING;
                return true;
            }
			adverrorinfo = 844;
            entity_list.CheckSupportCloseMobs(this);
	    }
        m_RememberStatus = S_CHECKINGAREA;
        m_Status = S_BUFFINGSELF;
        return true;
    case S_ROAMING:
		adverrorinfo = 9;
        if (IsEngaged())
        {
            m_RememberStatus = S_ROAMING;
            m_Status = S_FIGHTING;
            return true;
        }
		if (movement_timer->Check())
		{
			if (wp_x[wp_a[3]] == GetX() && wp_y[wp_a[3]] == GetY())
			{
				CalculateNewWaypoint();
                if (appearance != 0)
                {
				    appearance = 0;
				    pLastChange = Timer::GetCurrentTime();
                    if(HasMoved())
					HasMoved(false);
					else
					HasMoved(true);
                }
            	x_pos += 1; //temp fix
			}
			if (!walking_timer->Enabled())
			{
				CalculateNewPosition(wp_x[wp_a[3]], wp_y[wp_a[3]], wp_z[wp_a[3]], GetWalkspeed());
			}
		}
        m_Status = S_CHECKWHILEPATROLLING;
        m_RememberStatus = S_ROAMING;
        return true;
    case S_CHECKWHILEPATROLLING:
		adverrorinfo = 10;
        if (IsEngaged())
        {
            m_Status = S_FIGHTING;
            return true;
        }
        if (scanarea_timer->Check() &&(!zone->AggroLimitReached()))
        {
#ifdef GUILDWARS
	    	if(entity_list.CheckCloseMobs(this))
            {
                m_Status = S_FIGHTING;
		    	zone->AddAggroMob();
            }
#endif
            entity_list.CheckSupportCloseMobs(this);
	    }
        m_Status = S_BUFFINGSELF;
        return true;
    case S_RESTING:
		adverrorinfo = 11;
        if (GetHPRatio() < 100)
        {
            if (appearance != 0)
            {
                HasMoved(true);
                appearance = 0;
                pLastChange = Timer::GetCurrentTime();
            }
            if (GetCasterClass() != 'N')
            {
                // Find Spell (Heal);
                int16 spell_id = FindSpell(GetClass(), GetLevel(), SE_CurrentHP, SPELLTYPE_SELF, 0, GetMana());
                if (spell_id>0)
                {
                    if (appearance != 0)
                    {
                        HasMoved(true);
					    appearance = 0;
					    pLastChange = Timer::GetCurrentTime();
                    }
                    CastSpell(spell_id, GetID(), 10, spells[spell_id].cast_time);
                }
            }
        }
		if (!walking_timer->Enabled())
		{
			if (!roamer)
			{
			   m_Status = S_RETURNHOMETOSPAWN;
			}
			else
			{
				m_Status = S_ROAMING;
			}
		}
		if(IsEngaged())
        {
            target = GetHateTop();
            m_Status = S_FIGHTING;
		  	zone->AddAggroMob();
		}
        return true;
    case S_FIGHTING_RANGED:
        return true;
    case S_FIGHTING_CAST:
    {
        if (!IsEngaged())
        {
            SetTarget(NULL);
            m_Status = S_WINNINGBATTLE;
            return true;
        }
        if (float(GetMana()) / float(GetMaxMana()) < 0.50f)
        {
            m_Status = S_FIGHTING_MELEE;
            return true;
        }

        int8 newtype[6] = { SE_ResistMagic, SE_ResistFire, SE_MovementSpeed, SE_ArmorClass, SE_AttackSpeed, SE_CurrentHP };
        int16 spell = 0;
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( newtype[i] == SE_CurrentHP || !target->FindType(newtype[i], true, 0))
				spell = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_OFFENSIVE, Dist(target), GetMana());
            if (spell)
                break;
        }
        if (spell)
        {
            CastSpell(spell, target->GetID(), 0, spells[spell].cast_time);
            if (appearance)
            {
                appearance = 0;
                HasMoved(true);
                pLastChange = Timer::GetCurrentTime();
            }
            m_Status = S_FIGHTING_FINISHCAST;
        }
        else
            m_Status = S_FIGHTING_MELEE;
        return true;
    }
    case S_FIGHTING_FINISHCAST:
    {
        if (casting_spell_id == 0)
        {
            m_Status = S_FIGHTING;
        }
        return true;
    }
    case S_FIGHTING_MELEE:
    {
        if (!IsEngaged() || target == this || target == 0)
        {
            SetTarget(0);
            m_Status = S_WINNINGBATTLE;
            return true;
        }
        if (!CombatRange(target))
        {
			m_Status = S_PURSUING;
            return true;
        }
		if(!attack_timer->Enabled())
		{
			m_Status = S_CHECKINGAREA;
			return true;
		}
        adverrorinfo = 127;
    	appearance = 0;
        pLastChange = Timer::GetCurrentTime();
        heading = CalculateHeadingToTarget(target->GetX(), target->GetY());
        HasMoved(true);
		adverrorinfo = 128;
        if (casting_spell_id == 0 && GetHP() > 0 && target != 0 && attack_timer->Check())
        {
            adverrorinfo = 129;
    	    Attack(target, 13);
            bool successda = false;
            if (CanThisClassDoubleAttack())
		    {
			    float DoubleAttackProbability = (250 + GetLevel()) / 500.0f; // 62.4 max
			    float random = (float)rand()/RAND_MAX;
                if (DoubleAttackProbability > random)
                {
            	    successda = Attack(target, 14);
                }
		    }
            // lets see if we can do a triple attack with the main hand
            if (successda && SpecAttacks[SPECATK_TRIPLE])
            {
			    float TripleAttackProbability = GetLevel() / 500.0f; // 12.5% max
			    float random = (float)rand()/RAND_MAX;
                if (TripleAttackProbability > random)
                {
                    LogFile->write(EQEMuLog::Normal, "TripleAttack %s", GetName());
            	    successda = Attack(target, 14);
                }
            }
            else
                successda = false;

            // now lets check the quad attack
            if (successda && SpecAttacks[SPECATK_QUAD])
            {
			    float QuadAttackProbability = GetLevel() / 700.0f; // 9% max
			    float random = (float)rand()/RAND_MAX;
                if (QuadAttackProbability > random)
                {
                    LogFile->write(EQEMuLog::Normal, "QuadAttack %s", GetName());
            	    successda = Attack(target, 14);
                }
            }
                    
            if (SpecAttacks[SPECATK_FLURRY])
                Flurry();
            if (SpecAttacks[SPECATK_RAMPAGE])
                Rampage();
        }
        m_Status = S_FIGHTING;
        return true;
    }
    case S_FIGHTING:
		adverrorinfo = 12;
        // fight code here
        if (scanarea_timer->Check())
        {
			adverrorinfo = 121;
            entity_list.AddHateToCloseMobs(this);
        }
		adverrorinfo = 122;
        this->hate_list.CheckFrenzyHate();
		adverrorinfo = 123;
        if (!IsEngaged())
        {
			adverrorinfo = 124;
            // hm monster won?
            SetTarget(NULL);
            m_Status = S_WINNINGBATTLE;
            return true;
        }
        else
        {
		SetTarget(hate_list.GetTop());

        if(IsRooted())
        {
			  if (!CombatRange(target))
			  {
				  Mob* closehate = hate_list.GetClosest(this->CastToMob());
				  if(closehate != 0)
					  SetTarget(closehate);
					else // Im not sure if its possible for it to be 0, but just in case
					SetTarget(hate_list.GetTop());
			  }
		  }

			adverrorinfo = 125;
            if (GetHPRatio() < 15)
                StartEnrage();
            else
            {
                if (GetAILevel() > 5 && GetCasterClass() != 'N')
                {
                    if ((float)GetMana() / (float)GetMaxMana() > 0.50f)
                    {
                        m_Status = S_FIGHTING_CAST;
                        return true;
                    }
                }
            }
			adverrorinfo = 126;
            if (!CombatRange(target))
            {
                m_Status = S_PURSUING;
            }
            else
            {
                m_Status = S_FIGHTING_MELEE;
            }
        }
        return true;
    case S_PURSUING:
		adverrorinfo = 13;
        if (!target)
        {
            m_Status = S_WINNINGBATTLE;
        }
        else
        {
            // See if we can summon the mob to us
            HateSummon();
            // do we need to move?
            if (movement_timer->Check())
            {
                CalculateNewPosition(target->GetX(), target->GetY(), target->GetZ(), GetRunspeed());
            }
            m_Status = S_FIGHTING;
        }
        return true;
    case S_RETURNHOMETOSPAWN:
		adverrorinfo = 14;
        if (IsEngaged())
        {
            m_Status = S_PURSUING;
            target = GetHateTop();
        }
        else if (movement_timer->Check())
        {
            if (!CalculateNewPosition(GetSpawnX(), GetSpawnY(), GetSpawnZ(), GetWalkspeed()))
            {
                LogFile->write(EQEMuLog::Normal, "REACHED DESTINATION");
                m_Status = S_CHECKINGAREA;
                appearance = 0;
                heading = GetSpawnHeading();
                HasMoved(true);
                pLastChange = Timer::GetCurrentTime();
            }
        }
        return true;
    case S_PATROLLING:
		adverrorinfo = 15;
        return true;
    case S_CHANGEROAMPATH:
		adverrorinfo = 16;
        return true;
    case S_LOSINGBATTLE:
		adverrorinfo = 17;
        // try to flee, or gate
        return true;
    case S_WINNINGBATTLE:
		adverrorinfo = 18;
        LogFile->write(EQEMuLog::Normal, "WINNINGBATTLE");
        if (GetOwner() && m_RememberStatus == S_PETGUARD)
            m_Status = S_PETRETURNTOGUARD;
        else if (GetOwner())
            m_Status = m_RememberStatus;
        else
            m_Status = S_RESTING;
        return true;
    case S_PETFOLLOWMASTER:
		adverrorinfo = 19;
    {
        if (GetHateTop())
        {
            m_RememberStatus = S_PETFOLLOWMASTER;
            m_Status = S_PURSUING;
            target = GetHateTop();
            return true;
        }

        if (!movement_timer->Check())
            return true;

        Mob* owner = GetOwner();
        // if owner isnt there anymore, we gonna depop
        if (!owner)
            return true;

        if (!CalculateNewPosition(owner->GetX()-5, owner->GetY()-5, owner->GetZ(), GetRunspeed()))
        {
            if (appearance != 0)
            {
                FaceTarget(owner);
                appearance = 0;
                pLastChange = Timer::GetCurrentTime();
                HasMoved(true);
            }
        }
        return true;
    }
    case S_PETASSISTMASTER:
		adverrorinfo = 20;
        target = GetHateTop();
        m_Status = S_PURSUING;
        return true;
    case S_PETGUARD:
		adverrorinfo = 21;
        // TODO: face closest NPC
        LogFile->write(EQEMuLog::Normal, "Guarding: %f:%f:%f", GetX(),GetY(),GetZ());
        if (appearance != 0)
        {
            appearance = 0;
            pLastChange = Timer::GetCurrentTime();
            HasMoved(true);
        }
        if (GetHateTop())
        {
            m_RememberStatus = S_PETGUARD;
            m_Status = S_PURSUING;
            target = GetHateTop();
        }
        return true;
    case S_PETSTARTGUARD:
		adverrorinfo = 22;
        LogFile->write(EQEMuLog::Normal, "Start Guarding: %f:%f:%f", GetGuardX(),GetGuardY(),GetGuardZ());
        switch(m_RememberStatus)
        {
        case S_PETFOLLOWMASTER:
        case S_PETRETURNTOGUARD:
        case S_WINNINGBATTLE:
            if (appearance)
            {
                appearance = 0;
                pLastChange = Timer::GetCurrentTime();
                HasMoved(true);
            }
            m_Status = S_PETGUARD;
            break;
        case S_PURSUING:
        case S_FIGHTING:
            m_Status = m_RememberStatus;
            m_RememberStatus = S_PETGUARD;
            break;
		default:
			break;
        }
        return true;
    case S_PETRETURNTOGUARD:
		adverrorinfo = 23;
        // now move until we are at guard spot
        LogFile->write(EQEMuLog::Normal, "Return to Guarding: %f:%f:%f", GetGuardX(),GetGuardY(),GetGuardZ());
        if (GetHateTop())
        {
            m_Status = S_PURSUING;
            target = GetHateTop();
        }
        else
        {
            if (!CalculateNewPosition(GetGuardX(), GetGuardY(), GetGuardZ(), GetWalkspeed()))
            {
                m_Status = S_PETGUARD;
                if (appearance)
                {
                    HasMoved(true);
                    appearance = 0;
		    		pLastChange = Timer::GetCurrentTime();
                }
            }
        }
        return true;
    case S_PETSITDOWN:
		adverrorinfo = 24;
        // hm change appearance to sit, then increase regen and mana
        WhipeHateList();
        m_Status = S_PETGUARD;
        return true;
    default:
        cerr << "AI Finite State Machine: unknown state = " << m_Status << " - last status was = " << m_RememberStatus << endl;
        break;
    }
    return false;
}


void NPC::CalculateNewWaypoint()
{
	int8 max_wp = wp_a[0];
	int8 wandertype = wp_a[1];
	int8 pausetype = wp_a[2];
	int8 cur_wp = wp_a[3];
	int8 ranmax = cur_wp;
	int8 ranmax2 = max_wp - cur_wp;
	bool reached_end = false;
	bool reached_beginning = false;

	//Determine if we're at the last/first waypoint
	if (cur_wp == max_wp)
		reached_end = true;
	if (cur_wp == 0)
		reached_beginning = true;

	//Declare which waypoint to go to
	switch (wandertype)
	{
	case 0: //Circular
		if (reached_end)
			wp_a[3] = 0;
		else
			wp_a[3] = cur_wp + 1;
		break;
	case 1: //Random 5
		if (ranmax > 5)
			ranmax = 5;
		if (ranmax2 > 5)
			ranmax2 = 5;
		wp_a[3] = cur_wp + rand()%(ranmax+1) - rand()%(ranmax2+1);
		break;
	case 2: //Random
			wp_a[3] = (rand()%max_wp) + (rand()%2);
		break;
	case 3: //Patrol
		if (reached_end)
			wp_a[5] = 1;
		else if (reached_beginning)
			wp_a[5] = 0;
		if (wp_a[5] == 1)
			wp_a[3] = cur_wp - 1;
		else
			wp_a[3] = cur_wp + 1;
		break;
	}
	//Declare time to wait on current WP
	if (wp_s[cur_wp] == 0)
	{
		walking_timer->Start(100);
	}
	else
	{
		
		switch (pausetype)
		{
		case 0: //Random Half
			walking_timer->Start((wp_s[cur_wp] - rand()%wp_s[cur_wp]/2)*1000);
			break;
		case 1: //Full
			walking_timer->Start(wp_s[cur_wp]*1000);
			break;
		case 2: //Random Full
			walking_timer->Start((rand()%wp_s[cur_wp])*1000);
			break;
		}
	}
}

float NPC::CalculateDistanceToNextWaypoint()
{
    return CalculateDistance(wp_x[wp_a[3]], wp_y[wp_a[3]], wp_z[wp_a[3]]);
}

float NPC::CalculateDistance(float x, float y, float z)
{
    return (float)sqrt((float)pow(x_pos-x,2)+pow(y_pos-x,2)+pow(z_pos-z,2));
}


int8 NPC::CalculateHeadingToNextWaypoint()
{
    return CalculateHeadingToTarget(wp_x[wp_a[3]], wp_y[wp_a[3]]);
}

sint8 NPC::CalculateHeadingToTarget(float in_x, float in_y)
{
	float angle;

	if (in_x-x_pos > 0)
		angle = - 90 + atan((double)(in_y-y_pos) / (double)(in_x-x_pos)) * 180 / M_PI;
	else if (in_x-x_pos < 0)
		angle = + 90 + atan((double)(in_y-y_pos) / (double)(in_x-x_pos)) * 180 / M_PI;
	else // Added?
	{
		if (in_y-y_pos > 0)
			angle = 0;
		else
			angle = 180;
	}
	if (angle < 0)
		angle += 360;
	if (angle > 360)
		angle -= 360;
	return (sint8) (256*(360-angle)/360.0f);
}

bool NPC::CalculateNewPosition(float x, float y, float z, float speed)
{
    float nx = this->x_pos;
    float ny = this->y_pos;
    float nz = this->z_pos;
    float vx, vy, vz;
    float vb;

    // if NPC is rooted
    if (speed == 0.0)
    {
        heading = CalculateHeadingToTarget(x, y);
        appearance = 0;
        pLastChange = Timer::GetCurrentTime();
        return true;
    }
    appearance = (int8)speed*11;
    speed *= 3.6;

    // --------------------------------------------------------------------------
    // 1: get Vector AB (Vab = B-A)
    // --------------------------------------------------------------------------
    vx = x - nx;
    vy = y - ny;
    vz = z - nz;

    if (vx == 0 && vy == 0 && vz == 0)
        return false;

    HasMoved(true);
    // --------------------------------------------------------------------------
    // 2: get unit vector
    // --------------------------------------------------------------------------
    vb = speed / sqrt (vx*vx + vy*vy + vz*vz);
	heading = CalculateHeadingToTarget(x, y);

    if (vb >= 1.0)
    {
        x_pos = x;
        y_pos = y;
        z_pos = z;
    }
    else
    {
        // --------------------------------------------------------------------------
        // 3: destination = start plus movementvector (unitvektor*speed)
        // --------------------------------------------------------------------------
        x_pos = x_pos + vx*vb;
        y_pos = y_pos + vy*vb;
        z_pos = z_pos + vz*vb;
    }

    // now get new heading
    pLastChange = Timer::GetCurrentTime();
    return true;
}


////////////////////////////////////////////////////
//
//  AddHateToCloseMobs
//      i: sender - the mobs "asking" for assistance
//      return: true if any mobs changed their behavior
//
bool EntityList::AddHateToCloseMobs(NPC* sender)
{
    // thoughts on aggro:
    // - each mob has aggro radius, which can be lowered by spells
    // - mobs that cant see each other dont aggro (exception outdoors)
    // - in dungeons green mobs dont add
    // - mobs will aggro even if max ally, if you attack one of their mates
    // - if attacker is already on hate list dont increase aggro

    // if no sender, or sender doesnt even have aggro return
	if (sender == 0 || sender->GetHateTop() == 0)
		return false;

    // levels 0 and 1 are handled by the default attack code
    if (NPC::GetAILevel() <= 2)
        return false;

    int count = 0;

	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();

    int32 attackerlevel = 200;
    // now assume the offender is the one with the most hate on the sender.
    // would be cool to check all the hatelist - but how?
    // TODO: make hatelist iterator
    Mob* attacker = sender->GetHateTop();
    if (attacker && attacker->IsClient())
    {
        // seems to be ok. attacker exists and is a client
        attackerlevel = attacker->GetLevel();
    }
    else if (attacker)
    {
        // ok, attacker is a charmed npc or pet
        attackerlevel = attacker->GetLevel();
    }

    if (!CanAddHateForMob(attacker->CastToMob()))
        return false;
    
    // check if frenzy applies
    bool bFrenzy = false;
    if (NPC::GetAILevel() >= 4 && attacker->GetHPRatio() <= 20)
        bFrenzy = true;

	while(iterator.MoreElements())
	{
		if (iterator.GetData() != sender && iterator.GetData() != 0)
		{
            if (iterator.GetData()->IsNPC())
            {
                NPC * currentnpc = iterator.GetData()->CastToNPC();

                if (attacker == currentnpc)
                {
                    iterator.Advance();
                    continue;
                }

                sint32 current_aggroradius = currentnpc->GetAggroRadius();
                // the mobs in database arent set so get some defaultvalue
                // defaultvalue is set in NPCType, but just to make sure
                if (current_aggroradius <= 0 || current_aggroradius >= 999)
                    current_aggroradius = DEFAULT_AGGRORADIUS;
                // check distance -> aggroradius
                float distance = currentnpc->DistNoZ( sender );

                // if its out of aggrorange dont do anything at all
                if (distance > current_aggroradius || abs((int)(sender->GetZ()-currentnpc->GetZ())) > 30)
                {
                    iterator.Advance();
                    continue;
                }

                // now check level difference - can we get indoor/outdoor status??
                int32 currentmoblevel = currentnpc->GetLevel();

                // Check if attacker is already on hatelist
                if (!bFrenzy && currentnpc->hate_list.IsOnHateList(attacker))
                {
                    iterator.Advance();
                    continue;
                }

                // use some obscure function to determine con
                int32 con = GetLevelCon(attackerlevel, currentmoblevel);
                // if the mob cons green it wont help its friend
#ifdef GUILDWARS
               if ((!bFrenzy && (con == CON_GREEN || con == CON_LIGHTBLUE)) && !(sender->GetGuildOwner() != 0 && currentnpc->GetGuildOwner() != 0))
#else
               if ((!bFrenzy && (con == CON_GREEN || con == CON_LIGHTBLUE)))
#endif
                {
                    iterator.Advance();
                    continue;
                }

                // TODO: how can we check if the mobs can see each other
                //  face-2-face - aggroradius is lower if back turned to sender

                // ok now we need to check factions
                // aggro is only spread between mobs on the same faction
                // if the mob would aggro on the Client is another story
                // this is only for spreading the hate
                // (aggro by distance is hopefully checked elsewhere)
                LinkedList<struct NPCFaction*> *currentmob_faclist = &currentnpc->faction_list;
                LinkedListIterator<struct NPCFaction*> fac_iteratorcur(*currentmob_faclist);

                LinkedList<struct NPCFaction*> *sender_faclist = &sender->faction_list;
                LinkedListIterator<struct NPCFaction*> fac_iteratorsend(*sender_faclist);

                fac_iteratorcur.Reset();

                bool bSameFaction = false;
				while(fac_iteratorcur.MoreElements())
                {
                    struct NPCFaction *pfac_send = NULL;
                    struct NPCFaction *pfac_cur = NULL;

                    pfac_cur = fac_iteratorcur.GetData();

                    fac_iteratorsend.Reset();
					while(fac_iteratorsend.MoreElements())
                    {
                        pfac_send = fac_iteratorsend.GetData();

                        // now we can check the factions - we should get the faction-modifier
                        // and only if both ids are equal and modifier is negative we have a winner
#ifdef GUILDWARS
                       if (((pfac_send->factionID == pfac_cur->factionID) && pfac_send->value_mod <= 0) || (sender->GetGuildOwner() == currentnpc->GetGuildOwner()))

#else
                       if (((pfac_send->factionID == pfac_cur->factionID) && pfac_send->value_mod <= 0))

#endif
                          bSameFaction = true;
                        fac_iteratorsend.Advance();
                    } // while sender fac_list
                    fac_iteratorcur.Advance();
                } // while currentmob fac_list

                if (bSameFaction)
                {
                    // mob already fighting - dont add him
                    // except if frenzy
                    if (!currentnpc->IsEngaged() || bFrenzy)
                    {
                        currentnpc->AddToHateList(attacker, 1, 0, bFrenzy);
                        count++;
                    }
                }
            } // if ->IsNPC
            else
            {
                // if its not a npc we cant do anything
                iterator.Advance();
                continue;
            }
        } // if != sender && != NULL
        iterator.Advance();
    } // while
    if (count > 0)
        return true;
    return false;
}

bool EntityList::CheckOthersBuffs(NPC* sender)
{
	if (sender == 0)
		return false;
    if (NPC::GetAILevel() <= 3)
        return false;
    if (sender->casting_spell_id != 0)
        return true;

    sint32 sender_aggroradius = sender->GetAggroRadius();
    if (sender_aggroradius <= 0 || sender_aggroradius >= 999)
        sender_aggroradius = DEFAULT_AGGRORADIUS;
    sint8 sender_level = sender->GetLevel();

    LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();
    while(iterator.MoreElements())
	{
        if (iterator.GetData() != sender && iterator.GetData() != 0)
		{
            if (iterator.GetData()->IsNPC())
            {
                NPC * currentnpc = iterator.GetData()->CastToNPC();

                if (currentnpc == sender)
                {
                    iterator.Advance();
                    continue;
                }

                // filter out classes which cant cast, and filter those out
                // that are not of appropriate level
                switch(sender->GetClass())
                {
                case CLERIC:
                case CLERICGM:
                case DRUID:
                case DRUIDGM:
                case SHAMAN:
                case SHAMANGM:
                    // any level can help
                    break;
                case ENCHANTER:
                case ENCHANTERGM:
                    // enchanter dont get anything good before that level
                    if (sender_level < 16)
                    {
                        iterator.Advance();
                        continue;
                    }
                    break;
                case MAGICIAN:
                case MAGICIANGM:
                    // only level 8 and higher since they get damageshield in 8
                    if (sender_level < 8)
                    {
                        iterator.Advance();
                        continue;
                    }
                    break;
                case PALADIN:
                case PALADINGM:
                case RANGER:
                case RANGERGM:
                case BEASTLORD:
                case BEASTLORDGM:
                    // only level 9 and higher
                    if (sender_level < 9)
                    {
                        iterator.Advance();
                        continue;
                    }
                    break;
                default:
                    iterator.Advance();
                    continue;
                }
                // check distance -> aggroradius
                float distance = currentnpc->DistNoZ( sender );

                // if its out of aggrorange dont do anything at all
                // and warrior need to be much closer to shield their comrade
                if (distance > sender_aggroradius || abs((int)(sender->GetZ()-currentnpc->GetZ())) > 200)
                {
                    iterator.Advance();
                    continue;
                }

                // check faction
                LinkedList<struct NPCFaction*> *currentmob_faclist = &currentnpc->faction_list;
                LinkedListIterator<struct NPCFaction*> fac_iteratorcur(*currentmob_faclist);

                LinkedList<struct NPCFaction*> *sender_faclist = &sender->faction_list;
                LinkedListIterator<struct NPCFaction*> fac_iteratorsend(*sender_faclist);

                fac_iteratorcur.Reset();

                bool bSameFaction = false;
	            while(fac_iteratorcur.MoreElements())
                {
                    struct NPCFaction *pfac_send = NULL;
                    struct NPCFaction *pfac_cur = NULL;

                    pfac_cur = fac_iteratorcur.GetData();

                    fac_iteratorsend.Reset();
	                while(fac_iteratorsend.MoreElements())
                    {
                        pfac_send = fac_iteratorsend.GetData();

                        // now we can check the factions - we should get the faction-modifier
                        // and only if both ids are equal and modifier is negative we have a winner
                        if ((pfac_send->factionID == pfac_cur->factionID) && pfac_send->value_mod <= 0)
                            bSameFaction = true;
                        fac_iteratorsend.Advance();
                    } // while sender fac_list
                    fac_iteratorcur.Advance();
                } // while currentmob fac_list

                // go to next NPC in entitylist if faction doesnt fit
                if (!bSameFaction)
                {
                    iterator.Advance();
                    continue;
                }
#ifdef _EQDEBUG
//cout << "Distance between << " << currentnpc->GetName() << " and " << sender->GetName() << " is: " << distance << " --> Aggroradius=" << current_aggroradius << endl;
//cout << "Now helping..." << endl;
#endif

                int16 spell_id = 0;

                //g_LogFile.write("---- COB: %s[%d] is searching for a spell ----", sender->GetName(), sender->GetLevel());

                switch(sender->GetClass())
                    {
                    case CLERIC:
                    case CLERICGM:
                        if (!currentnpc->FindType(SE_TotalHP))
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_TotalHP, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_ResistMagic) && sender_level >= 19 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_ResistMagic, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_ArmorClass) && sender_level >= 5 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_ArmorClass, SPELLTYPE_OTHER, distance, sender->GetMana());
                        break;
                    case PALADIN:
                    case PALADINGM:
                        if (!currentnpc->FindType(SE_TotalHP) && sender_level >= 15)
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_TotalHP, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_ResistMagic) && sender_level >= 30 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_ResistMagic, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_ArmorClass))
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_ArmorClass, SPELLTYPE_OTHER, distance, sender->GetMana());
                        break;
                    case DRUID:
                    case DRUIDGM:
                        if (!currentnpc->FindType(SE_TotalHP))
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_TotalHP, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_DamageShield) && sender_level >= 9 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_DamageShield, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_MovementSpeed) && sender_level >= 14 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_MovementSpeed, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_ResistMagic) && sender_level >= 19 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_ResistMagic, SPELLTYPE_OTHER, distance, sender->GetMana());
                        break;
                    case RANGER:
                    case RANGERGM:
                        if (!currentnpc->FindType(SE_TotalHP))
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_TotalHP, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_DamageShield) && sender_level >= 30 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_DamageShield, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_MovementSpeed) && sender_level >= 30 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_MovementSpeed, SPELLTYPE_OTHER, distance, sender->GetMana());
                        break;
                    case SHAMAN:
                    case SHAMANGM:
                        if (!currentnpc->FindType(SE_TotalHP))
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_TotalHP, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_AttackSpeed) && sender_level >= 29 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_AttackSpeed, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_MovementSpeed) && sender_level >= 9 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_MovementSpeed, SPELLTYPE_OTHER, distance, sender->GetMana());
                        break;
                    case BEASTLORD:
                    case BEASTLORDGM:
                        if (!currentnpc->FindType(SE_TotalHP))
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_TotalHP, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_MovementSpeed) && sender_level >= 30 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_MovementSpeed, SPELLTYPE_OTHER, distance, sender->GetMana());
                        break;
                    case MAGICIAN:
                    case MAGICIANGM:
                        if (!currentnpc->FindType(SE_DamageShield))
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_DamageShield, SPELLTYPE_OTHER, distance, sender->GetMana());
                        break;
                    case ENCHANTER:
                    case ENCHANTERGM:
                        if (!currentnpc->FindType(SE_DamageShield) && sender_level >= 29)
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_DamageShield, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_AttackSpeed) && sender_level >= 16 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_AttackSpeed, SPELLTYPE_OTHER, distance, sender->GetMana());
                        else if (!currentnpc->FindType(SE_ResistMagic) && sender_level >= 20 )
    	                    spell_id = sender->FindSpell(sender->GetClass(), sender_level, SE_ResistMagic, SPELLTYPE_OTHER, distance, sender->GetMana());
                        break;
                    } // switch

                // if we did find a spell begin to cast it
                if (spell_id != 0)
                {
                    LogFile->write(EQEMuLog::Normal, "%s found a spell: now casting %s on %s", sender->GetName(), spells[spell_id].name, currentnpc->GetName());
                    if (sender->CastToNPC()->appearance)
                    {
					    sender->CastToNPC()->appearance = 0;
					    sender->CastToNPC()->pLastChange = Timer::GetCurrentTime();
                        sender->HasMoved(true);
                    }
                    sender->CastSpell(spell_id, currentnpc->GetID(), 1, spells[spell_id].cast_time);
                    return true;
                }
            } // if NPC
        } // if GetData
        iterator.Advance();
    } // while
    return false;
}

bool EntityList::CheckCloseMobs(NPC *pMob)
{
	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();

    sint32 aggroradius = pMob->GetAggroRadius();
    bool bFrenzy = false;
    pMob->adverrorinfo = 825;
    while (iterator.MoreElements())
    {
		pMob->adverrorinfo = 826;//This allows the guards to attack newbie mobs within their aggrorange.
		if (iterator.GetData() && iterator.GetData()->IsNPC() && (pMob->race==112 || pMob->race==106 || pMob->race==44) && pMob->class_<16){
			NPC *p;
			pMob->adverrorinfo = 827;
			p = iterator.GetData()->CastToNPC();
			float distance = pMob->DistNoZ( p );
			if (distance > aggroradius || abs((int)(p->GetZ()-pMob->GetZ())) > 30)
			{
				iterator.Advance();
				continue;
			}
			if ((p->race==34 || p->race==54 || p->race==109 || p->race==60 || p->race==60 || p->race==22 || p->race==259 || p->race==37) && p->ownerid==0){
				pMob->adverrorinfo = 8291;
				pMob->AddToHateList(p);
				p->flag[3]=3; //Flagging npc so that the corpse vanishes when its dead.
				return true;
			}
			else{
				iterator.Advance();
				continue;
			}
		}
        if (iterator.GetData()->IsNPC())
        {
            NPC *p;
			pMob->adverrorinfo = 827;
			p = iterator.GetData()->CastToNPC();
            if (p == pMob)
            {
                iterator.Advance();
                continue;
            }
			float distance = pMob->DistNoZ( p );
            // if its out of aggrorange dont do anything at all
            if (distance > aggroradius || abs((int)(p->GetZ()-pMob->GetZ())) > 30)
            {
                iterator.Advance();
                continue;
            }
			pMob->adverrorinfo = 828;
			if (pMob->hate_list.IsOnHateList(p) || (pMob->GetFactionID() == p->GetFactionID()) || (pMob->GetGuildOwner() == p->GetGuildOwner()))
            {
                iterator.Advance();
                continue;
            }
			pMob->adverrorinfo = 829;

                LinkedList<struct NPCFaction*> *currentmob_faclist = &p->faction_list;
                LinkedListIterator<struct NPCFaction*> fac_iteratorcur(*currentmob_faclist);

                LinkedList<struct NPCFaction*> *sender_faclist = &pMob->faction_list;
                LinkedListIterator<struct NPCFaction*> fac_iteratorsend(*sender_faclist);

                fac_iteratorcur.Reset();

                bool bEnemyFaction = false;
#ifdef GUILDWARS
				if(p->GetGuildOwner() != 0 && pMob->GetGuildOwner() != 0 && p->GetGuildOwner() != pMob->GetGuildOwner())
					bEnemyFaction = true;
#endif
	            while(fac_iteratorcur.MoreElements() && !bEnemyFaction)
                {
                    struct NPCFaction *pfac_send = NULL;
                    struct NPCFaction *pfac_cur = NULL;

                    pfac_cur = fac_iteratorcur.GetData();

                    fac_iteratorsend.Reset();
	                while(fac_iteratorsend.MoreElements())
                    {
                        pfac_send = fac_iteratorsend.GetData();

                        // now we can check the factions - we should get the faction-modifier
                        // and only if both ids are equal and modifier is negative we have a winner
                        if ((pfac_send->factionID == pfac_cur->factionID) && pfac_send->value_mod >= 0)
                            bEnemyFaction = true;
                        fac_iteratorsend.Advance();
                    } // while sender fac_list
                    fac_iteratorcur.Advance();
                } // while currentmob fac_list

			if (p->IsAttackAllowed(pMob) && bEnemyFaction)
            {
                if (!CanAddHateForMob(p))
                {
                    iterator.Advance();
                    continue;
                }
				pMob->adverrorinfo = 8291;
                pMob->AddToHateList(p);
                return true;
            }
			iterator.Advance();
			continue;
        }
        if (iterator.GetData() && iterator.GetData()->IsClient())
        {
            Client *p;

            p = iterator.GetData()->CastToClient();
			if(!p->Connected())
			{
				iterator.Advance();
				continue;
			}
			pMob->adverrorinfo = 830;
            if (p->GetGM())
            {
                iterator.Advance();
                continue;
            }

            // check distance -> aggroradius
            float distance = pMob->DistNoZ( p );
            // if its out of aggrorange dont do anything at all
            if (distance > aggroradius || abs((int)(p->GetZ()-pMob->GetZ())) > 30)
            {
                iterator.Advance();
                continue;
            }

            // Check if client is already on hatelist
            if (pMob->hate_list.IsOnHateList(p))
            {
                iterator.Advance();
                continue;
            }

			pMob->adverrorinfo = 831;
            if (NPC::GetAILevel() >= 4 && p->GetHPRatio() <= 20)
                bFrenzy = true;
            else
                bFrenzy = false;
			pMob->adverrorinfo = 832;
            int32 con = GetLevelCon(p->GetLevel(), pMob->GetLevel());
            // if the mob cons green it wont help its friend
#ifdef GUILDWARS
            if (!bFrenzy && (con == CON_GREEN || con == CON_LIGHTBLUE) && !(p->GuildDBID() != 0 && pMob->GetGuildOwner() != 0))
#else
            if (!bFrenzy && (con == CON_GREEN || con == CON_LIGHTBLUE))
#endif
            {
                iterator.Advance();
                continue;
            }
            FACTION_VALUE val = p->GetFactionLevel(p->CharacterID(),pMob->GetNPCTypeID(), p->GetRace(), p->GetClass(), p->GetDeity(), pMob->GetFactionID(), pMob);
            if (val == FACTION_THREATENLY || val == FACTION_SCOWLS)
            {
                if (!CanAddHateForMob(p))
                {
                    iterator.Advance();
                    continue;
                }
                pMob->AddToHateList(p);
                return true;
            }
        }
        iterator.Advance();
    }
    return false;
}

bool EntityList::CheckSupportCloseMobs(NPC* sender)
{
    // healer will try to heal or buff mobs (mostly heal)
    // even if they con green. they wont add to hatelist though
    // nor give faction when killed by others
    // even if mobs are in battle they will try to heal others
    // need to be quite close though. at least aggro range to be noticed
    // ?? should this function be used for wandering mobs getting buffed
    //    by the sender?

	if (sender == 0)
		return false;

    // no action needed below level 4
    // levels 0 and 1 are handled by the default attack code
    // level 2 is handled in the AddMobToHateList function
    if (NPC::GetAILevel() <= 3)
        return false;

	LinkedListIterator<Entity*> iterator(list);
	iterator.Reset();

    while(iterator.MoreElements())
	{
		// see if the entry is valid and not the sender
        if (iterator.GetData() != sender && iterator.GetData() != 0)
		{
            // only npcs are relevant
            if (iterator.GetData()->IsNPC())
            {
                NPC * currentnpc = iterator.GetData()->CastToNPC();
                sint32 current_aggroradius = currentnpc->GetAggroRadius();
                sint32 current_level = currentnpc->GetLevel();
                // the mobs in database arent set so get some defaultvalue
                // defaultvalue is set in NPCType, but just to make sure
                if (current_aggroradius <= 0 || current_aggroradius >= 999)
                    current_aggroradius = DEFAULT_AGGRORADIUS;

                // if already casting take no action
                if (currentnpc->casting_spell_id != 0)
                {
                    iterator.Advance();
                    continue;
                }

                // filter out classes which cant cast, and filter those out
                // that are not of appropriate level
                switch(currentnpc->GetClass())
                {
                case CLERIC:
                case CLERICGM:
                case DRUID:
                case DRUIDGM:
                case SHAMAN:
                case SHAMANGM:
                    // any level can help
                    break;
                case ENCHANTER:
                case ENCHANTERGM:
                    // enchanter dont get anything good before that level
                    if (current_level < 16)
                    {
                        iterator.Advance();
                        continue;
                    }
                    break;
                case MAGICIAN:
                case MAGICIANGM:
                    // only level 8 and higher since they get damageshield in 8
                    if (current_level < 8)
                    {
                        iterator.Advance();
                        continue;
                    }
                    break;
                case PALADIN:
                case PALADINGM:
                case RANGER:
                case RANGERGM:
                case BEASTLORD:
                case BEASTLORDGM:
                    // only level 9 and higher
                    if (current_level < 9)
                    {
                        iterator.Advance();
                        continue;
                    }
                    break;
                case NECROMANCER:
                case NECROMANCERGM:
                    // necromancers get their "heal" in level 20
                    // EQLive dont remove HP when healing mobs close by, but we do
                    // so dont let the necro heal himself to death
                    if (current_level < 20 || currentnpc->GetHPRatio() < 75)
                    {
                        iterator.Advance();
                        continue;
                    }
                    break;
                case WARRIOR:
                case WARRIORGM:
                    // shield is a level 35 discipline
                    if (current_level < 35)
                    {
                        iterator.Advance();
                        continue;
                    }
                    break;
                // only these classes will be able to do something here
                // bards would be possible, but real eq doesnt do it either
                default:
                    iterator.Advance();
                    continue;
                }
                // check distance -> aggroradius
                float distance = currentnpc->DistNoZ( sender );

                // if its out of aggrorange dont do anything at all
                // and warrior need to be much closer to shield their comrade
                if (distance > current_aggroradius || abs((int)(sender->GetZ()-currentnpc->GetZ())) > 200 ||
                    (currentnpc->GetClass() == WARRIOR && distance > MAX_SHIELDRADIUS))
                {
                    iterator.Advance();
                    continue;
                }

                // check faction
                LinkedList<struct NPCFaction*> *currentmob_faclist = &currentnpc->faction_list;
                LinkedListIterator<struct NPCFaction*> fac_iteratorcur(*currentmob_faclist);

                LinkedList<struct NPCFaction*> *sender_faclist = &sender->faction_list;
                LinkedListIterator<struct NPCFaction*> fac_iteratorsend(*sender_faclist);

                fac_iteratorcur.Reset();

                bool bSameFaction = false;
	            while(fac_iteratorcur.MoreElements())
                {
                    struct NPCFaction *pfac_send = NULL;
                    struct NPCFaction *pfac_cur = NULL;

                    pfac_cur = fac_iteratorcur.GetData();

                    fac_iteratorsend.Reset();
	                while(fac_iteratorsend.MoreElements())
                    {
                        pfac_send = fac_iteratorsend.GetData();

                        // now we can check the factions - we should get the faction-modifier
                        // and only if both ids are equal and modifier is negative we have a winner
                        if ((pfac_send->factionID == pfac_cur->factionID) && pfac_send->value_mod <= 0)
                            bSameFaction = true;
                        fac_iteratorsend.Advance();
                    } // while sender fac_list
                    fac_iteratorcur.Advance();
                } // while currentmob fac_list

                // if Race equals and AILEVEL is high enough --> help
#ifdef GUILDWARS
                if ((NPC::GetAILevel() >= 6 && sender->GetRace() == currentnpc->GetRace()) || (sender->GetGuildOwner() == currentnpc->GetGuildOwner()))
#else
                if (NPC::GetAILevel() >= 6 && sender->GetRace() == currentnpc->GetRace())
#endif
                {
                        bSameFaction = true;
                }
                // go to next NPC in entitylist if faction doesnt fit
                if (!bSameFaction)
                {
                    iterator.Advance();
                    continue;
                }
#ifdef _EQDEBUG
//cout << "Distance between << " << currentnpc->GetName() << " and " << sender->GetName() << " is: " << distance << " --> Aggroradius=" << current_aggroradius << endl;
//cout << "Now helping..." << endl;
#endif

                int16 spell_id = 0;

                //g_LogFile.write("---- CSCM: %s[%d] is searching for a spell ----", currentnpc->GetName(), currentnpc->GetLevel());

                if (NPC::GetAILevel() < 5 || sender->GetHPRatio() < 50 )
                {
                    if ( sender->GetHPRatio() < 50 )
                    {
                        //g_LogFile.write("Healing");
                        if (current_level > 39 && sender->GetLevel() > 39 && currentnpc->GetClass() == CLERIC)
                            spell_id = 13; // Complete Healing
                        else
                            spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_CurrentHP, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                    }
                }
                else
                {
                    switch(currentnpc->GetClass())
                    {
                    case WARRIOR:
                    case WARRIORGM:
                        // TODO: shield code here
                        break;
                    case CLERIC:
                    case CLERICGM:
                        if (!sender->FindType(SE_TotalHP))
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_TotalHP, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_ResistMagic) && current_level >= 19 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_ResistMagic, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_ArmorClass) && current_level >= 5 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_ArmorClass, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        break;
                    case PALADIN:
                    case PALADINGM:
                        if (!sender->FindType(SE_TotalHP) && current_level >= 15)
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_TotalHP, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_ResistMagic) && current_level >= 30)
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_ResistMagic, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_ArmorClass))
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_ArmorClass, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        break;
                    case DRUID:
                    case DRUIDGM:
                        if (!sender->FindType(SE_TotalHP))
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_TotalHP, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_DamageShield) && current_level >= 9 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_DamageShield, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_MovementSpeed) && current_level >= 14 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_MovementSpeed, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_ResistMagic) && current_level >= 19 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_ResistMagic, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        break;
                    case RANGER:
                    case RANGERGM:
                        if (!sender->FindType(SE_TotalHP))
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_TotalHP, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_DamageShield) && current_level >= 30 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_DamageShield, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_MovementSpeed) && current_level >= 30 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_MovementSpeed, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        break;
                    case SHAMAN:
                    case SHAMANGM:
                        if (!sender->FindType(SE_TotalHP))
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_TotalHP, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_AttackSpeed) && current_level >= 29 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_AttackSpeed, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_MovementSpeed) && current_level >= 9 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_MovementSpeed, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        break;
                    case BEASTLORD:
                    case BEASTLORDGM:
                        if (!sender->FindType(SE_TotalHP))
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_TotalHP, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_MovementSpeed) && current_level >= 30 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_MovementSpeed, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        break;
                    case MAGICIAN:
                    case MAGICIANGM:
                        if (!sender->FindType(SE_DamageShield))
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_DamageShield, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        break;
                    case ENCHANTER:
                    case ENCHANTERGM:
                        if (!sender->FindType(SE_DamageShield) && current_level >= 29)
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_DamageShield, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_AttackSpeed) && current_level >= 16 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_AttackSpeed, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        else if (!sender->FindType(SE_ResistMagic) && current_level >= 20 )
    	                    spell_id = currentnpc->FindSpell(currentnpc->GetClass(), current_level, SE_ResistMagic, SPELLTYPE_OTHER, distance, currentnpc->GetMana());
                        break;
                    } // switch
                } // else

                // if we did find a spell begin to cast it
                if (spell_id != 0)
                {
                    LogFile->write(EQEMuLog::Normal, "%s found a spell: now casting %s on %s", currentnpc->GetName(), spells[spell_id].name, sender->GetName());
                    if (currentnpc->appearance)
                    {
					    currentnpc->appearance = 0;
					    currentnpc->pLastChange = Timer::GetCurrentTime();
                        currentnpc->HasMoved(true);
                    }
                    currentnpc->CastSpell(spell_id, sender->GetID(), 1, spells[spell_id].cast_time);
                    return true;
                }
            } // if NPC
        } // if GetData

/*

INSERT INTO variables 'AILevel', '1';

0: No Action from NPCs, beside fighting back when attacked

1. Level: NPCs will use special abilities
      (rampage, enrage, ...)

2. Level: same as 1, Check Aggroradius, Join if of sufficient Level and not
otherwise engaged
      todo: check if guard and if faction of victim is not the same help
player

3. Level: same as 2, Check Frenzyradius, Join even if engaged
      even greens will join and help finish the offender

4. Level: same as 3, but caster of not sufficient level (or aggrorange or
faction?) will start helping wounded comrades
      ( /shield for warriors?)

5. Level: same as 4, but caster will start buffing comrades when caster is
not engaged in combat itself

need mana formula to work correctly with NPCs
      ((Intelligence (or Wisdom) / 5) + 2) * LevelofCaster
need priority list of spells
      (HP-Buff (Cleric / Other), Damage Shield, ResistMagic, AttackSpeed
for Buffs, Depending on Level approriate Heals at 20%. Over 39 use CH at
30%)
need priority of targets
      (Closest, most wounded)
need to determine the value of extended aggrorange for caster combat
support
      (70 as default for aggrorange, and 1.5 * aggro for extended)
*/


        iterator.Advance();
    } // while
    return false;
}

bool Mob::CheckSelfBuffs()
{

    // neotokyo: fix for GM classes
    int8 npc_class = GetClass();

    if (npc_class > 16)
        npc_class -= 16;

    // check if we are casting
    if (this->casting_spell_id != 0)
        return true;

    // check if we are caster class
    if (this->GetCasterClass() == 'N')
        return false;

    if (NPC::GetAILevel() <= 3)
        return false;

    int16 spell_id = 0;

    // pet caster cast pet first thing
    if (this->GetPetID() == 0 && ((npc_class == DRUID && GetLevel() >= 55) || npc_class != DRUID))
    {
        switch(npc_class)
        {
        case NECROMANCER:
        case SHADOWKNIGHT:
            spell_id = FindSpell(GetClass(), GetLevel(), SE_NecPet, SPELLTYPE_SELF, 0, GetMana());
            break;
        case SHAMAN:
        case ENCHANTER:
        case MAGICIAN:
        case DRUID:
            spell_id = FindSpell(GetClass(), GetLevel(), SE_SummonPet, SPELLTYPE_SELF, 0, GetMana());
            break;
        case BEASTLORD:
            spell_id = FindSpell(GetClass(), GetLevel(), SE_SummonBSTPet, SPELLTYPE_SELF, 0, GetMana());
            break;
        }
    }

    if (spell_id && this->GetPetID() == 0)
    {
        LogFile->write(EQEMuLog::Normal, "%s found a pet spell: now casting %s", GetName(), spells[spell_id].name);
        if (appearance)
        {
		    appearance = 0;
		    pLastChange = Timer::GetCurrentTime();
            CastToNPC()->HasMoved(true);
        }
        CastSpell(spell_id, GetID(), 1, spells[spell_id].cast_time);
        return true;
    }
    else
        spell_id = 0;

    // next cast misc spells
    switch(npc_class)
    {
    case NECROMANCER:
    {
		int8 newtype[3] = { SE_ArmorClass,  SE_DamageShield, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case MAGICIAN:
    {
		int8 newtype[3] = { SE_ArmorClass,  SE_DamageShield, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case WIZARD:
    {
		int8 newtype[4] = { SE_AbsorbMagicAtt, SE_ArmorClass,  SE_DamageShield, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case ENCHANTER:
    {
		int8 newtype[4] = { SE_DamageShield, SE_AttackSpeed, SE_Rune, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case DRUID:
    {
		int8 newtype[4] = { SE_DamageShield, SE_MovementSpeed, SE_ResistMagic, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case CLERIC:
    {
		int8 newtype[3] = { SE_ArmorClass, SE_ResistMagic, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case SHAMAN:
    {
		int8 newtype[4] = { SE_ArmorClass, SE_AttackSpeed, SE_MovementSpeed, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case RANGER:
    {
		int8 newtype[3] = { SE_DamageShield, SE_MovementSpeed, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case PALADIN:
    {
		int8 newtype[3] = { SE_ArmorClass, SE_ResistMagic, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case SHADOWKNIGHT:
    {
		int8 newtype[3] = { SE_ArmorClass,  SE_DamageShield, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    case BEASTLORD:
    {
		int8 newtype[3] = { SE_AttackSpeed, SE_MovementSpeed, SE_TotalHP };
		for (int32 i = 0; i < sizeof(newtype)/sizeof(newtype[0]); i++)
        {
			if ( !FindType(newtype[i]))
				spell_id = FindSpell(GetClass(), GetLevel(), newtype[i], SPELLTYPE_SELF, 0, GetMana());
        }
        break;
    }
    } // switch

    if (spell_id)
    {
        LogFile->write(EQEMuLog::Normal, "%s found a spell: now casting %s", GetName(), spells[spell_id].name);
        if (appearance)
        {
		    appearance = 0;
		    pLastChange = Timer::GetCurrentTime();
            CastToNPC()->HasMoved(true);
        }
        CastSpell(spell_id, GetID(), 1, spells[spell_id].cast_time);
        return true;
    }
    return false;
}

int32 GetLevelCon(int8 PlayerLevel, int8 NPCLevel)
{
    sint8 diff = NPCLevel - PlayerLevel;
	int32 conlevel=0;

    if (diff == 0)
        return CON_WHITE;
    else if (diff >= 1 && diff <= 2)
        return CON_YELLOW;
    else if (diff >= 3)
        return CON_RED;

    if (PlayerLevel <= 6)    // i didnt notice light blue mobs before level 6
    {
        if (diff <= -4)
            conlevel = CON_GREEN;
        else
            conlevel = CON_BLUE;
    }
    else if (PlayerLevel <= 9)
	{
        if (diff <= -5)
            conlevel = CON_GREEN;
        else if (diff <= -4)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
    else if (PlayerLevel <= 13)
	{
        if (diff <= -6)
            conlevel = CON_GREEN;
        else if (diff <= -5)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (PlayerLevel <= 18)
	{
        if (diff <= -7)
            conlevel = CON_GREEN;
        else if (diff <= -6)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (PlayerLevel <= 24)
	{
        if (diff <= -8)
            conlevel = CON_GREEN;
        else if (diff <= -7)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (PlayerLevel <= 35)
	{
        if (diff <= -9)
            conlevel = CON_GREEN;
        else if (diff <= -8)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (PlayerLevel <= 44)
	{
        if (diff <= -12)
            conlevel = CON_GREEN;
        else if (diff <= -11)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (PlayerLevel <= 50)
	{
        if (diff <= -14)
            conlevel = CON_GREEN;
        else if (diff <= -12)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (PlayerLevel <= 60)
	{
        if (diff <= -16)
            conlevel = CON_GREEN;
        else if (diff <= -14)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
	}
	else if (PlayerLevel >= 61)
    {
        if (diff <= -17)
            conlevel = CON_GREEN;
        else if (diff <= -15)
            conlevel = CON_LIGHTBLUE;
        else
            conlevel = CON_BLUE;
    }
	return conlevel;

};
