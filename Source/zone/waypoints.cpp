/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2004  EQEMu Development Team (http://eqemu.org)

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
#ifdef _EQDEBUG
#include <iostream>
using namespace std;
#endif
//#include <iomanip>
#include <stdlib.h>
#include <math.h>
#include "npc.h"
#include "masterentity.h"
#include "NpcAI.h"
#include "map.h"
#include "../common/moremath.h"
#include "parser.h"
#include "StringIDs.h"
#ifdef GUILDWARS
#include "../GuildWars/GuildWars.h"
extern GuildWars guildwars;
#endif

void Mob::AI_SetRoambox(float iDist, float iRoamDist, int32 iDelay) {
	AI_SetRoambox(iDist, GetX()+iRoamDist, GetX()-iRoamDist, GetY()+iRoamDist, GetY()-iRoamDist, iDelay);
}

void Mob::AI_SetRoambox(float iDist, float iMaxX, float iMinX, float iMaxY, float iMinY, int32 iDelay) {
	roambox_distance = iDist;
	roambox_max_x = iMaxX;
	roambox_min_x = iMinX;
	roambox_max_y = iMaxY;
	roambox_min_y = iMinY;
	roambox_movingto_x = roambox_max_x + 1; // this will trigger a recalc
	roambox_delay = iDelay;
}


// support for new wandering quest commands

void Mob::StopWandering()
{	// stops a mob from wandering, takes him off grid and sends him back to spawn point
	roamer=false;
	this->CastToNPC()->SetGrid(0);
	SendPosition();
	return;
}

void Mob::ResumeWandering()
{	// causes wandering to continue - overrides waypoint pause timer and PauseWandering()
	if (this->CastToNPC()->GetGrid() != 0)
	{
		if (this->CastToNPC()->GetGrid() < 0)
		{	// we were paused by a quest
			AIwalking_timer->Disable();
			this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid());
			if (cur_wp==-1)
			{	// got here by a MoveTo()
				cur_wp=save_wp;
				UpdateWaypoint(cur_wp);	// have him head to last destination from here
			}
		}
		else if (AIwalking_timer->Enabled())
		{	// we are at a waypoint paused normally
			AIwalking_timer->Disable();	// disable timer to end pause now
		}
		else
		{
			LogFile->write(EQEMuLog::Error, "NPC not paused - can't resume wandering: %lu", GetNPCTypeID());
			return;
		}
		if (cur_wp_x == GetX() && cur_wp_y == GetY()) 
		{	// are we we at a waypoint? if so, trigger event and start to next
			char temp[100]; 
			parse->Event(EVENT_WAYPOINT,this->GetNPCTypeID(), itoa(cur_wp,temp,10), this->CastToMob(), 0); 
			CalculateNewWaypoint(); 
	        SetAppearance(0, false); 
		}	// if not currently at a waypoint, we continue on to the one we were headed to before the stop
	}
	else
	{
		LogFile->write(EQEMuLog::Error, "NPC not on grid - can't resume wandering: %lu", GetNPCTypeID());
	}
	return;
}

void Mob::PauseWandering(int pausetime)
{	// causes wandering to stop but is resumable
	// 0 pausetime means pause until resumed
	// otherwise automatically resume when time is up
	if (this->CastToNPC()->GetGrid() != 0)
	{
		SendPosition();
		if (pausetime<1)
		{	// negative grid number stops him dead in his tracks until ResumeWandering()
			this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid());
		}
		else
		{	// specified waiting time, he'll resume after that
			AIwalking_timer->Start(pausetime*1000); // set the timer
		}
	}
	else
	{
		LogFile->write(EQEMuLog::Error, "NPC not on grid - can't pause wandering: %lu", GetNPCTypeID());
	}
	return;
}

void Mob::MoveTo(float mtx, float mty, float mtz)
{	// makes mob walk to specified location
	if (this->CastToNPC()->GetGrid() != 0)
	{	// he is on a grid
		if (this->CastToNPC()->GetGrid() < 0)
		{	// currently stopped by a quest command
			this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid());	// get him moving again
		}
		AIwalking_timer->Disable();	// disable timer in case he is paused at a wp
		if (cur_wp>=0)
		{	// we've not already done a MoveTo()
			save_wp=cur_wp;	// save the current waypoint
			cur_wp=-1;		// flag this move as quest controlled
		}
	}
	else
	{	// not on a grid
		roamer=true;
		save_wp=0;
		cur_wp=-2;		// flag as quest controlled w/no grid
	}
	cur_wp_x = mtx;
	cur_wp_y = mty;
	cur_wp_z = mtz;
	cur_wp_pause = 0;
}



void Mob::UpdateWaypoint(int wp_index)
{
	MyListItem <wplist> * Ptr = Waypoints.First;
	while (Ptr) {
		if ( Ptr->Data->index == wp_index) {
				cur_wp_x = Ptr->Data->x;
				cur_wp_y = Ptr->Data->y;
				cur_wp_z = Ptr->Data->z;
				
#ifdef FIX_PATHING_WHEN_MOVING
			    //fix up pathing Z
			    if(zone->map != NULL) {
			    	VERTEX dest;
			    	dest.x = cur_wp_x;
			    	dest.y = cur_wp_y;
			    	dest.z = cur_wp_z;
			    	NodeRef n = zone->map->SeekNode( zone->map->GetRoot(), dest.x, dest.y);
			    	if(n != NODE_NONE) {
			    		float newz = zone->map->FindBestZ(n, dest, NULL, NULL);
			    		if(newz < cur_wp_z && newz > -2000) {
							cur_wp_z = newz;
			    		}
			    	}
			    }
#endif
				cur_wp_pause = Ptr->Data->pause;
				break;
		}
		Ptr = Ptr->Next;
	}
	return;
}

void Mob::CalculateNewWaypoint()
{
//	int8 max_wp = wp_a[0];
//	int8 wandertype = wp_a[1];
//	int8 pausetype = wp_a[2];
//	int8 cur_wp = wp_a[3];

	int16 ranmax = cur_wp;
	int16 ranmax2 = max_wp - cur_wp;
	int old_wp = cur_wp;

	bool reached_end = false;
	bool reached_beginning = false;

// handle quest mob wandering control
	if (cur_wp <0)
	{	// under quest control, so no new wp - just stop
// printf("cur_wp<0\n");
		if (cur_wp==-1)
		{	// mob is on a grid			
			this->CastToNPC()->SetGrid( 0 - this->CastToNPC()->GetGrid());
		}
		else
		{	// mob is not on a grid
			cur_wp=0;
		}
		return;
	}

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
			cur_wp = 0;
		else
			cur_wp = cur_wp + 1;
		break;
	case 1: //Random 5
		if (ranmax > 5)
			ranmax = 5;
		if (ranmax2 > 5)
			ranmax2 = 5;
		cur_wp = cur_wp + rand()%(ranmax+1) - rand()%(ranmax2+1);
		break;
	case 2: //Random
			cur_wp = (rand()%max_wp) + (rand()%2);
		break;
	case 3: //Patrol
		if (reached_end)
			patrol = 1;
		else if (reached_beginning)
			patrol = 0;
		if (patrol == 1)
			cur_wp = cur_wp - 1;
		else
			cur_wp = cur_wp + 1;
		break;
// MYRA - Added wander type 4 (single run)
		case 4:  // single run 
			cur_wp = cur_wp + 1; 
		break;
// end Myra
	}
	tar_ndx=52;		// force new packet to be sent extra 2 times

	// Check to see if we need to update the waypoint. - Wes
	if (cur_wp != old_wp)
		UpdateWaypoint(cur_wp);

}

void Mob::SetWaypointPause() 
{ 
   //Declare time to wait on current WP 
    
   if (cur_wp_pause == 0) { 
      AIwalking_timer->Start(100); 
   } 
   else 
   { 
       
      switch (pausetype) 
      { 
      case 0: //Random Half 
         AIwalking_timer->Start((cur_wp_pause - rand()%cur_wp_pause/2)*1000); 
         break; 
      case 1: //Full 
         AIwalking_timer->Start(cur_wp_pause*1000); 
         break; 
      case 2: //Random Full 
         AIwalking_timer->Start((rand()%cur_wp_pause)*1000); 
         break; 
      } 
   } 
} 




/*float Mob::CalculateDistanceToNextWaypoint() {
    return CalculateDistance(cur_wp_x, cur_wp_y, cur_wp_z);
}*/

float Mob::CalculateDistance(float x, float y, float z) {
    return (float)sqrt( ((x_pos-x)*(x_pos-x)) + ((y_pos-y)*(y_pos-y)) + ((z_pos-z)*(z_pos-z)) );
}


int8 Mob::CalculateHeadingToNextWaypoint() {
    return CalculateHeadingToTarget(cur_wp_x, cur_wp_y);
}

sint8 Mob::CalculateHeadingToTarget(float in_x, float in_y) {
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

bool Mob::CalculateNewPosition2(float x, float y, float z, float speed, bool checkZ) {
	if(GetID()==0)
		return true;
	
	if ((x_pos-x == 0) && (y_pos-y == 0)) {//spawn is at target coords
		if(z_pos-z != 0) {
			z_pos = z;
			return true;
		}
		return false;
	}

	if(tar_ndx<20 && tarx==x && tary==y){
		x_pos = x_pos + tar_vx*tar_vector;
		y_pos = y_pos + tar_vy*tar_vector;
		z_pos = z_pos + tar_vz*tar_vector;
		
#ifdef FIX_PATHING_WHEN_MOVING
	    //fix up pathing Z
	    if(checkZ && zone->map != NULL) {
	    	VERTEX dest;
	    	dest.x = x_pos;
	    	dest.y = y_pos;
	    	dest.z = z_pos;
	    	NodeRef n = zone->map->SeekNode( zone->map->GetRoot(), x_pos, y_pos);
	    	if(n != NODE_NONE) {
	    		float newz = zone->map->FindBestZ(n, dest, NULL, NULL);
	    		if(newz < z_pos && newz > -2000) {
					z_pos = newz;
	    		}
	    	}
	    }
#endif
		tar_ndx++;
		return true;
	}
	else{
		if (tar_ndx>50)
		{
			tar_ndx--;
		}
		else
		{
			tar_ndx=0;
		}
		tarx=x;
		tary=y;
		tarz=z;
	}

	float nx = this->x_pos;
    float ny = this->y_pos;
    float nz = this->z_pos;
//	float nh = this->heading;
	
	tar_vx = x - nx;
	tar_vy = y - ny;
	tar_vz = z - nz;

	pRunAnimSpeed = (sint8)(speed*NPC_RUNANIM_RATIO);
	speed *= 46;
	// --------------------------------------------------------------------------
	// 2: get unit vector
	// --------------------------------------------------------------------------
	float mag = sqrt (tar_vx*tar_vx + tar_vy*tar_vy + tar_vz*tar_vz);
	tar_vector = speed / mag;

// mob move fix
	int numsteps = (int) ( mag * 20 / speed) + 1;


// mob move fix

	if (numsteps<20)
	{
		if (numsteps>1)
		{
			tar_vector=1.0f				;
			tar_vx = tar_vx/numsteps;
			tar_vy = tar_vy/numsteps;
			tar_vz = tar_vz/numsteps;
			x_pos = x_pos + tar_vx;
			y_pos = y_pos + tar_vy;
			z_pos = z_pos + tar_vz;
			tar_ndx=22-numsteps;
			heading = CalculateHeadingToTarget(x, y);
		}
	    else
		{
			x_pos = x;
			y_pos = y;
			z_pos = z;
		}
	}

	else {
		tar_vector/=20;
		x_pos = x_pos + tar_vx*tar_vector;
		y_pos = y_pos + tar_vy*tar_vector;
		z_pos = z_pos + tar_vz*tar_vector;
		heading = CalculateHeadingToTarget(x, y);
	}
	
#ifdef FIX_PATHING_WHEN_MOVING
    //fix up pathing Z
    if(checkZ && zone->map != NULL) {
    	VERTEX dest;
    	dest.x = x_pos;
    	dest.y = y_pos;
    	dest.z = z_pos;
    	NodeRef n = zone->map->SeekNode( zone->map->GetRoot(), x_pos, y_pos);
    	if(n != NODE_NONE) {
    		float newz = zone->map->FindBestZ(n, dest, NULL, NULL);
    		if(newz < z_pos && newz > -2000) {
				z_pos = newz;
    		}
    	}
    }
#endif
	
	SetMoving(true);
	moved=true;
	
	delta_x=x_pos-nx;
	delta_y=y_pos-ny;
	delta_z=z_pos-nz;
	delta_heading=0;
	
	SendPosUpdate();
	SetAppearance(0, false);
    pLastChange = Timer::GetCurrentTime();
    return true;
}

bool Mob::CalculateNewPosition(float x, float y, float z, float speed, bool checkZ) {
	if(GetID()==0)
		return true;
	
    float nx = x_pos;
    float ny = y_pos;
    float nz = z_pos;
//	float nh = heading;
	
    // if NPC is rooted
    if (speed == 0.0) {
        SetHeading(CalculateHeadingToTarget(x, y));
		if(moved){
			SendPosition();
			SetMoving(false);
			moved=false;
		}
		SetRunAnimSpeed(0);
        return true;
    }

	float old_test_vector=test_vector;
	tar_vx = x - nx;
	tar_vy = y - ny;
	tar_vz = z - nz;

	if (tar_vx == 0 && tar_vy == 0)
		return false;
	pRunAnimSpeed = (int8)(speed*NPC_RUNANIM_RATIO);
	speed *= NPC_SPEED_MULTIPLIER;
	// --------------------------------------------------------------------------
	// 2: get unit vector
	// --------------------------------------------------------------------------
	test_vector=sqrt (x*x + y*y + z*z);
	tar_vector = speed / sqrt (tar_vx*tar_vx + tar_vy*tar_vy + tar_vz*tar_vz);
	heading = CalculateHeadingToTarget(x, y);

	if (tar_vector >= 1.0) {
		x_pos = x;
		y_pos = y;
		z_pos = z;
	}
	else {
		x_pos = x_pos + tar_vx*tar_vector;
		y_pos = y_pos + tar_vy*tar_vector;
		z_pos = z_pos + tar_vz*tar_vector;
	}
	
#ifdef FIX_PATHING_WHEN_MOVING
    //fix up pathing Z
    if(checkZ && zone->map != NULL) {
    	VERTEX dest;
    	dest.x = x_pos;
    	dest.y = y_pos;
    	dest.z = z_pos;
    	NodeRef n = zone->map->SeekNode( zone->map->GetRoot(), dest.x, dest.y);
    	if(n != NODE_NONE) {
    		float newz = zone->map->FindBestZ(n, dest, NULL, NULL);
    		if(newz < z_pos && newz > -2000) {
				z_pos = newz;
    		}
    	}
    }
#endif
	
	//OP_MobUpdate
	if((old_test_vector!=test_vector) || tar_ndx>20){ //send update
		tar_ndx=0;
		this->SetMoving(true);
		moved=true;
		delta_x=(x_pos-nx);
		delta_y=(y_pos-ny);
		delta_z=(z_pos-nz);
		delta_heading=0;//(heading-nh)*8;
		SendPosUpdate();
	}
	tar_ndx++;
	
    // now get new heading
	SetAppearance(0, false); // make sure they're standing
    pLastChange = Timer::GetCurrentTime();
    return true;
}

void Mob::AssignWaypoints(int16 grid)
{ char errbuf[MYSQL_ERRMSG_SIZE];
  char *query = 0;
  MYSQL_RES *result;
  MYSQL_ROW row;

  bool	GridErr = false,
	WPErr = false;		// Will be set true if any errors encountered while querying the waypoints


	Waypoints.ClearListAndData();
#ifdef _EQDEBUG
	cout<<"Assigning waypoints for grid "<<grid<<" to "<<name<<"...\n";
#endif

	// Retrieve the wander and pause types for this grid
	if(database.RunQuery(query,MakeAnyLenString(&query,"SELECT `type`,`type2` FROM `grid` WHERE `id`=%i AND `zoneid`=%i",grid,zone->GetZoneID()),errbuf, &result))
	{   if((row = mysql_fetch_row(result)))
	    {
	    	if(row[0] != 0)
	    		wandertype = atoi(row[0]);
			else
				wandertype = 0;
			if(row[1] != 0)
				pausetype = atoi(row[1]);
			else
				pausetype = 0;
	    }
	    else	// No grid record found in this zone for the given ID
		GridErr = true;
	    mysql_free_result(result);
	}
	else	// DB query error!
	{
		GridErr = true;
		LogFile->write(EQEMuLog::Error, "MySQL Error while trying to assign grid %u to mob %s: %s", grid, name, errbuf);
	}
	safe_delete_array(query);

	if(!GridErr)
	{   this->CastToNPC()->SetGrid(grid);	// Assign grid number
	    adverrorinfo = 7561;

	    // Retrieve all waypoints for this grid
	    if(database.RunQuery(query,MakeAnyLenString(&query,"SELECT `x`,`y`,`z`,`pause` FROM grid_entries WHERE `gridid`=%i AND `zoneid`=%i ORDER BY `number`",grid,zone->GetZoneID()),errbuf,&result))
	    {
	    	roamer = true;
			max_wp = -1;	// Initialize it; will increment it for each waypoint successfully added to the list
			adverrorinfo = 7564;

			while((row = mysql_fetch_row(result)))
			{   
			    if(row[0] != 0 && row[1] != 0 && row[2] != 0 && row[3] != 0)
			    {
			    	wplist* newwp = new wplist;
					newwp->index = ++max_wp;
					newwp->x = atof(row[0]);
					newwp->y = atof(row[1]);
					newwp->z = atof(row[2]);
					newwp->pause = atoi(row[3]);
					Waypoints.AddItem(newwp);
			    }
			}
			mysql_free_result(result);
	    }
	    else	// DB query error!
	    {
	    	WPErr = true;
			LogFile->write(EQEMuLog::Error, "MySQL Error while trying to assign waypoints from grid %u to mob %s: %s", grid, name, errbuf);
	    }
	    safe_delete_array(query);
	} // end if (!GridErr)


#ifdef _EQDEBUG
	cout<<" done."<<endl;
#endif
	if(!GridErr && !WPErr)
	{   UpdateWaypoint(0);
	    SetWaypointPause();
	    SendTo(cur_wp_x, cur_wp_y, cur_wp_z);
	    if (wandertype == 1 || wandertype == 2)
		CalculateNewWaypoint();
	}
}

void Mob::SendTo(float new_x, float new_y, float new_z) {
	
//	float angle;
//	float dx = new_x-x_pos;
//	float dy = new_y-y_pos;
	// 0.09 is a perfect magic number for a human pnj's
//	AIwalking_timer->Start((int32) ( sqrt( dx*dx + dy*dy ) * 0.09f ) * 1000 );
	
/*	if (new_x-x_pos > 0)
		angle = - 90 + atan((double)(new_y-y_pos) / (double)(new_x-x_pos)) * 180 / M_PI;
	else {
		if (new_x-x_pos < 0)	
			angle = + 90 + atan((double)(new_y-y_pos) / (double)(new_x-x_pos)) * 180 / M_PI;
		else { // Added?
			if (new_y-y_pos > 0)
				angle = 0;
			else
				angle = 180;
		}
	}
	if (angle < 0)
		angle += 360;
	if (angle > 360	)
		angle -= 360;
	
	heading	= 256*(360-angle)/360.0f;
	SetRunAnimSpeed(5);*/
	//	SendPosUpdate();
	x_pos = new_x;
	y_pos = new_y;
	z_pos = new_z + 0.1;
	
    //fix up pathing Z, this shouldent be needed IF our waypoints 
    //are corrected instead
#ifdef FIX_SENDTO_Z
    if(zone->map != NULL) {
    	VERTEX dest;
    	dest.x = x_pos;
    	dest.y = y_pos;
    	dest.z = z_pos;
    	NodeRef n = zone->map->SeekNode( zone->map->GetRoot(), dest.x, dest.y);
    	if(n != NODE_NONE) {
    		float newz = zone->map->FindBestZ(n, dest, NULL, NULL);
    		if(newz < z_pos && newz > -2000) {
				z_pos = newz + 0.1;
    		}
    	}
    }
#endif
}

void Mob::SendToFixZ(float new_x, float new_y, float new_z) {
	x_pos = new_x;
	y_pos = new_y;
	z_pos = new_z + 0.1;
	
    //fix up pathing Z, this shouldent be needed IF our waypoints 
    //are corrected instead
    if(zone->map != NULL) {
    	VERTEX dest;
    	dest.x = x_pos;
    	dest.y = y_pos;
    	dest.z = z_pos;
    	NodeRef n = zone->map->SeekNode( zone->map->GetRoot(), dest.x, dest.y);
    	if(n != NODE_NONE) {
    		float newz = zone->map->FindBestZ(n, dest, NULL, NULL);
    		if(newz < z_pos && newz > -2000) {
				z_pos = newz + 0.1;
    		}
    	}
    }
}


#define FEAR_PATHING_DEBUG
void Mob::SetFeared(Mob *caster, int32 duration) {
	if(zone->map == NULL) {
		fear_state = fearStateStuck;
		return;	//just stand there
	}
	
	//our goal is to run along this vector...
	fear_vector.x = GetX() - caster->GetX();
	fear_vector.y = GetY() - caster->GetY();
	fear_vector.z = 0;	//I dont see any reason to use Z
	float mag = sqrt(fear_vector.x*fear_vector.x + fear_vector.y*fear_vector.y);
	fear_vector.x /= mag;
	fear_vector.y /= mag;
	
	//now see if we can just run without hitting anything...
	VERTEX start, end, hit;
	start.x = GetX();
	start.y = GetY();
	start.z = GetZ() + 5.0;	//raise up a little over small bumps
	
	//distance moved per movement tic.
	float distance = NPC_SPEED_MULTIPLIER * GetRunspeed();
	//times number of movement tics in the spell.
	distance *= float(duration) / float(AImovement_duration);
	
	end.x = start.x + fear_vector.x * distance;
	end.y = start.y + fear_vector.y * distance;
	end.z = start.z;
	
	if(!zone->map->LineIntersectsZone(start, end, 0.5, &hit, NULL)) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing Start: can run entire vector from (%.2f, %.2f, %.2f) to (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), end.x, end.y, end.z);
#endif
		//no hit, we can run this whole vector.
		cur_wp_x = end.x;
		cur_wp_y = end.y;
		cur_wp_z = GetZ();
		fear_state = fearStateRunningForever;
	} else {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing Start: From (%.2f, %.2f, %.2f), hit at (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), hit.x, hit.y, hit.z);
#endif
		//use the hit point - a little + a little Z as the first waypoint.
		cur_wp_x = hit.x - fear_vector.x * 2;
		cur_wp_y = hit.y - fear_vector.y * 2;
		cur_wp_z = GetZ();
		fear_state = fearStateRunning;
	}
}

void Mob::CalculateFearPosition() {
	if(zone->map == NULL) {
		return;	//just stand there
	}
	/*
		The idea...
		
		try to run along fear vector.
		If we can see along it, run
		otherwise, try to walk up a hill along the same vector
		then try to move along a wall along largest component of FV
		if cant move, change stae to stuck.
		
		once we know a place to run, use the waypoint code to do it
		then if combat ends, we will reach the waypoint and
		
		
	*/
	if (cur_wp_x != GetX() && cur_wp_y != GetY()) {
		// not at waypoint yet, so keep moving
		CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true); 
		return;
	}
	if(fear_state == fearStateRunningToStick) {
		fear_state = fearStateStuck;
		return;
	}
		
	
	//figure out a new waypoint to run at...
	
	//first try our original fear vector again...
	VERTEX start, end, hit, normalhit;
	start.x = GetX();
	start.y = GetY();
	start.z = GetZ() + 6.0;	//raise up a little over small bumps
	
	end.x = start.x + fear_vector.x * 10;
	end.y = start.y + fear_vector.y * 10;
	end.z = start.z;
	
	if(!zone->map->LineIntersectsZone(start, end, 0.5, &normalhit, NULL)) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) normal run to (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), end.x, end.y, end.z);
#endif
		//we can run along this vector without hitting anything...
		cur_wp_x = end.x;
		cur_wp_y = end.y;
		cur_wp_z = end.z - 6.0;
		CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true); 
		return;
	}
	//see if we can make ANY useful progress along that vector
	
	//first, adjust normalhit to back up a little bit
	//so we dont run through the wall
	normalhit.x -= 0.4 * fear_vector.x;
	normalhit.y -= 0.4 * fear_vector.y;
	
	float xd = normalhit.x - start.x;
	if(xd < 0)
		xd = 0 - xd;
	float yd = normalhit.y - start.y;
	if(yd < 0)
		yd = 0 - yd;
	
	//this 2 is arbitrary
	if((xd+yd) > 2.0) {
#ifdef FEAR_PATHING_DEBUG
	LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) small run to (%.2f, %.2f, %.2f)",
		GetX(), GetY(), GetZ(), cur_wp_x, cur_wp_y, cur_wp_z);
#endif
		cur_wp_x = normalhit.x;
		cur_wp_y = normalhit.y;
		cur_wp_z = GetZ();
		CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true); 
		return;
	}
	
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) normal hit at (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), normalhit.x, normalhit.y, normalhit.z);
#endif
	
	//if we get here, we cannot run along our normal vector...
	//try up hill first
	
	
	end.x = start.x + fear_vector.x * 2;
	end.y = start.y + fear_vector.y * 2;
	end.z = start.z + 4;
	
	if(!zone->map->LineIntersectsZone(start, end, 0.5, &hit, NULL)) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) up hill run to (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), end.x, end.y, end.z);
#endif
		//we can run along this vector without hitting anything...
		cur_wp_x = end.x;
		cur_wp_y = end.y;
		cur_wp_z = end.z;
		
		//try and fix up the Z coord if possible
		//not sure if this is worth it, since it prolly isnt up much
		
		NodeRef c = zone->map->SeekNode(zone->map->GetRoot(), end.x, end.y);
		if(c != NODE_NONE) {
			cur_wp_z = zone->map->FindBestZ(c, end, &hit, NULL);
			if(cur_wp_z < start.z)
				cur_wp_z = end.z;	//revert on error
		}
		
		
		CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true); 
		return;
	}
	
	
	//cant run along our vector at all....
	//one last ditch effort... try to move to the side a little
	//along the minor component of the fear vector.
	//try it in one direction first...
	if(fear_vector.x < fear_vector.y) {
		end.x = start.x + fear_vector.x * 3;
		end.y = start.y;
	} else {
		end.x = start.x;
		end.y = start.y + fear_vector.y * 3;
	}
	end.z = start.z + 3;	//a little lift as always
	
	if(!zone->map->LineIntersectsZone(start, end, 0.5, &hit, NULL)) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) strafe 1 to (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), end.x, end.y, end.z);
#endif
		//we can run along this vector without hitting anything...
		cur_wp_x = end.x;
		cur_wp_y = end.y;
		cur_wp_z = end.z - 3;
		CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true); 
		return;
	}
	
	//now the other...
	if(fear_vector.x < fear_vector.y) {
		end.x = start.x + fear_vector.x * 3;
		end.y = start.y;
	} else {
		end.x = start.x;
		end.y = start.y + fear_vector.y * 3;
	}
	end.z = start.z + 3;	//a little lift as always
	
	if(!zone->map->LineIntersectsZone(start, end, 0.5, &hit, NULL)) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) strafe 2 to (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), end.x, end.y, end.z);
#endif
		//we can run along this vector without hitting anything...
		cur_wp_x = end.x;
		cur_wp_y = end.y;
		cur_wp_z = end.z - 3;
		CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true); 
		return;
	}
	
	//if we get here... we have wasted enough CPU cycles
	//just call it quits on fear pathing...
	
	//send them to normalhit and then stop
	cur_wp_x = normalhit.x;
	cur_wp_y = normalhit.y;
	cur_wp_z = GetZ();
	fear_state = fearStateRunningToStick;
#ifdef FEAR_PATHING_DEBUG
	LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) final move to (%.2f, %.2f, %.2f)",
		GetX(), GetY(), GetZ(), normalhit.x, normalhit.y, normalhit.z);
#endif
}





