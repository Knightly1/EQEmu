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
	if(!IsNPC())
		return;
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
			parse->Event(EVENT_WAYPOINT,this->GetNPCTypeID(), itoa(cur_wp,temp,10), CastToNPC(), NULL); 
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
	
	_ZP(Mob_CalculateNewPosition2);
	
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
	
	_ZP(Mob_CalculateNewPosition);
	
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

int8 Database::GetGridType2(int16 grid, int16 zoneid) {
	char *query = 0;
	char errbuff[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int type2 = 0;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT type2 from grid where id = %i and zoneid = %i",grid,zoneid),errbuff,&result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			type2 = atoi( row[0] );
		}
		mysql_free_result(result);
	} else {
		LogFile->write(EQEMuLog::Error, "Error in GetGridType2 query '%s': %s", query, errbuff);
		safe_delete_array(query);
	}

	return(type2);
}

bool Database::GetWaypoints(int16 grid,int16 zoneid, int16 num, wplist* wp) {
	_CP(Database_GetWaypoints);
	char *query = 0;
	char errbuff[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT x, y, z, pause from grid_entries where gridid = %i and number = %i and zoneid = %i",grid,num,zoneid),errbuff,&result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			if ( wp ) {
				wp->x = atof( row[0] );
				wp->y = atof( row[1] );
				wp->z = atof( row[2] );
				wp->pause = atoi( row[3] );
			}
			mysql_free_result(result);
			return true;
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error in GetWaypoints query '%s': %s", query, errbuff);
		safe_delete_array(query);
	}
	return false;
}

void Database::AssignGrid(Client *client, float x, float y, int32 grid)
{
	char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	MYSQL_RES *result;
	MYSQL_ROW row;
	int matches = 0, fuzzy = 0, spawn2id = 0;
	int32 affected_rows;
	float dbx = 0, dby = 0;

	// solar: looks like most of the stuff in spawn2 is straight integers
	// so let's try that first
	if(!RunQuery(
		query,
		MakeAnyLenString(
			&query,
			"SELECT id,x,y FROM spawn2 WHERE zone='%s' AND x=%i AND y=%i",
			zone->GetShortName(), (int)x, (int)y
		),
		errbuf,
		&result
	)) {
			LogFile->write(EQEMuLog::Error, "Error querying spawn2 '%s': '%s'", query, errbuf);
			return;
	}
	safe_delete_array(query);

// how much it's allowed to be off by
#define _GASSIGN_TOLERANCE	1.0
	if(!(matches = mysql_num_rows(result)))	// try a fuzzy match if that didn't find it
	{
		mysql_free_result(result);
		if(!RunQuery(
			query,
			MakeAnyLenString(
				&query,
				"SELECT id,x,y FROM spawn2 WHERE zone='%s' AND "
				"ABS( ABS(x) - ABS(%f) ) < %f AND "
				"ABS( ABS(y) - ABS(%f) ) < %f",
				zone->GetShortName(), x, _GASSIGN_TOLERANCE, y, _GASSIGN_TOLERANCE
			),
			errbuf,
			&result
		)) {
			LogFile->write(EQEMuLog::Error, "Error querying fuzzy spawn2 '%s': '%s'", query, errbuf);
			return;
		}
		safe_delete_array(query);
		fuzzy = 1;
		if(!(matches = mysql_num_rows(result)))
			mysql_free_result(result);
	}
	if(matches)
	{
		if(matches > 1)
		{
			client->Message(0, "ERROR: Unable to assign grid - multiple spawn2 rows match");
			mysql_free_result(result);
		}
		else
		{
			row = mysql_fetch_row(result);
			spawn2id = atoi(row[0]);
			dbx = atof(row[1]);
			dby = atof(row[2]);
			if(!RunQuery(
				query,
				MakeAnyLenString(
					&query,
					"UPDATE spawn2 SET pathgrid = %d WHERE id = %d", grid, spawn2id
				),
				errbuf,
				&result,
				&affected_rows
			)) {
				LogFile->write(EQEMuLog::Error, "Error updating spawn2 '%s': '%s'", query, errbuf);
				return;
			}
			if(affected_rows == 1)
			{
				if(client) client->LogSQL(query);
				if(fuzzy)
				{
					float difference;
					difference = sqrt(pow(fabs(x-dbx),2) + pow(fabs(y-dby),2));
					client->Message(0, 
						"Grid assign: spawn2 id = %d updated - fuzzy match: deviation %f",
						spawn2id, difference
					);
				}
				else
				{
					client->Message(0, "Grid assign: spawn2 id = %d updated - exact match", spawn2id);
				}
			}
			else
			{
				client->Message(0, "ERROR: found spawn2 id %d but the update query failed", spawn2id);
			}
		}
	}
	else
	{
		client->Message(0, "ERROR: Unable to assign grid - can't find it in spawn2");
	}
}

/******************
* ModifyGrid - Either adds an empty grid, or removes a grid and all its waypoints, for a particular zone.
*	remove:		TRUE if we are deleting the specified grid, FALSE if we are adding it
*	id:		The ID# of the grid to add or delete
*	type,type2:	The type and type2 values for the grid being created (ignored if grid is being deleted)
*	zoneid:		The ID number of the zone the grid is being created/deleted in
*/

void Database::ModifyGrid(Client *c, bool remove, int16 id, int8 type, int8 type2, int16 zoneid) { 
	char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
	if (!remove)
	{
		if(!RunQuery(query, MakeAnyLenString(&query,"INSERT INTO grid(id,zoneid,type,type2) VALUES(%i,%i,%i,%i)",id,zoneid,type,type2), errbuf)) {
			LogFile->write(EQEMuLog::Error, "Error creating grid entry '%s': '%s'", query, errbuf);
		} else {
			if(c) c->LogSQL(query);
		}
		safe_delete_array(query);
	}
	else
	{
		if(!RunQuery(query, MakeAnyLenString(&query,"DELETE FROM grid where id=%i",id), errbuf)) {
			LogFile->write(EQEMuLog::Error, "Error deleting grid '%s': '%s'", query, errbuf);
		} else {
			if(c) c->LogSQL(query);
		}
		safe_delete_array(query);
		query = 0;
		if(!RunQuery(query, MakeAnyLenString(&query,"DELETE FROM grid_entries WHERE zoneid=%i AND gridid=%i",zoneid,id), errbuf)) {
			LogFile->write(EQEMuLog::Error, "Error deleting grid entries '%s': '%s'", query, errbuf);
		} else {
			if(c) c->LogSQL(query);
		}
		safe_delete_array(query);
	}
} /*** END Database::ModifyGrid() ***/

/**************************************
* AddWP - Adds a new waypoint to a specific grid for a specific zone.
*/

void Database::AddWP(Client *c, int32 gridid, int16 wpnum, float xpos, float ypos, float zpos, int32 pause, int16 zoneid)
{   
	char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];

	if(!RunQuery(query,MakeAnyLenString(&query,"INSERT INTO grid_entries (gridid,zoneid,`number`,x,y,z,pause) values (%i,%i,%i,%f,%f,%f,%i)",gridid,zoneid,wpnum,xpos,ypos,zpos,pause), errbuf)) {
		LogFile->write(EQEMuLog::Error, "Error adding waypoint '%s': '%s'", query, errbuf);
	} else {
		if(c) c->LogSQL(query);
	}
	safe_delete_array(query);
} /*** END Database::AddWP() ***/


/**********
* ModifyWP() has been obsoleted.  The #wp command either uses AddWP() or DeleteWaypoint()
***********/

/******************
* DeleteWaypoint - Removes a specific waypoint from the grid
*	grid_id:	The ID number of the grid whose wp is being deleted
*	wp_num:		The number of the waypoint being deleted
*	zoneid:		The ID number of the zone that contains the waypoint being deleted
*/

void Database::DeleteWaypoint(Client *c, int16 grid_num, int32 wp_num, int16 zoneid)
{
	char *query=0;
	char errbuf[MYSQL_ERRMSG_SIZE];

	if(!RunQuery(query, MakeAnyLenString(&query,"DELETE FROM grid_entries where gridid=%i and zoneid=%i and `number`=%i",grid_num,zoneid,wp_num), errbuf)) {
			LogFile->write(EQEMuLog::Error, "Error deleting waypoint '%s': '%s'", query, errbuf);
	} else {
		if(c) c->LogSQL(query);
	}
	safe_delete_array(query);
} /*** END Database::DeleteWaypoint() ***/


/******************
* AddWPForSpawn - Used by the #wpadd command - for a given spawn, this will add a new waypoint to whatever grid that spawn is assigned to.
* If there is currently no grid assigned to the spawn, a new grid will be created using the next available Grid ID number for the zone
* the spawn is in.
* Returns 0 if the function didn't have to create a new grid.  If the function had to create a new grid for the spawn, then the ID of
* the created grid is returned.
*/

int32 Database::AddWPForSpawn(Client *c, int32 spawn2id, float xpos, float ypos, float zpos, int32 pause, int type1, int type2, int16 zoneid) {
	char	*query = 0;
    int32	grid_num,	// The grid number the spawn is assigned to (if spawn has no grid, will be the grid number we end up creating)
		next_wp_num;	// The waypoint number we should be assigning to the new waypoint
    bool	CreatedNewGrid;	// Did we create a new grid in this function?
    MYSQL_RES	*result;
    MYSQL_ROW	row;
	char errbuf[MYSQL_ERRMSG_SIZE];
	
	// See what grid number our spawn is assigned
	if(RunQuery(query, MakeAnyLenString(&query,"SELECT pathgrid FROM spawn2 WHERE id=%i",spawn2id),errbuf,&result))
	{
	   safe_delete_array(query);
	    if(mysql_num_rows(result) > 0)
	    {
			row = mysql_fetch_row(result);
			grid_num = atoi(row[0]);
	    }
	    else	// This spawn ID was not found in the `spawn2` table
		return 0;

	    mysql_free_result(result);
	}
	else {	// Query error
		LogFile->write(EQEMuLog::Error, "Error setting pathgrid '%s': '%s'", query, errbuf);
		return 0;
	}

	if (grid_num == 0)	// Our spawn doesn't have a grid assigned to it -- we need to create a new grid and assign it to the spawn
	{
	    CreatedNewGrid = true;
	    if((grid_num = GetFreeGrid(zoneid)) == 0)	// There are no grids for the current zone -- create Grid #1
		grid_num = 1;

	    if(!RunQuery(query, MakeAnyLenString(&query,"insert into grid set id='%i',zoneid= %i, type='%i', type2='%i'",grid_num,zoneid,type1,type2), errbuf)) {
			LogFile->write(EQEMuLog::Error, "Error adding grid '%s': '%s'", query, errbuf);
	    } else {
			if(c) c->LogSQL(query);
		}
	    safe_delete_array(query);

	    query = 0;
	    if(!RunQuery(query, MakeAnyLenString(&query,"update spawn2 set pathgrid='%i' where id='%i'",grid_num,spawn2id), errbuf)) {
			LogFile->write(EQEMuLog::Error, "Error updating spawn2 pathing '%s': '%s'", query, errbuf);
	    } else {
			if(c) c->LogSQL(query);
		}
		safe_delete_array(query);
	}
	else	// NPC had a grid assigned to it
		CreatedNewGrid = false;
	
	
	// Find out what the next waypoint is for this grid
	query = 0;
	if(RunQuery(query, MakeAnyLenString(&query,"SELECT max(`number`) FROM grid_entries WHERE zoneid='%i' AND gridid='%i'",zoneid,grid_num),errbuf,&result))
	{
	    safe_delete_array(query);
	    row = mysql_fetch_row(result);
	    if(row[0] != 0)
		next_wp_num = atoi(row[0]) + 1;
	    else	// No waypoints in this grid yet
		next_wp_num = 1;

	    mysql_free_result(result);
	}
	else {	// Query error
		LogFile->write(EQEMuLog::Error, "Error getting next waypoint id '%s': '%s'", query, errbuf);
		return 0;
	}

	query = 0;
	if(!RunQuery(query, MakeAnyLenString(&query,"INSERT INTO grid_entries(gridid,zoneid,`number`,x,y,z,pause) VALUES (%i,%i,%i,%f,%f,%f,%i)",grid_num,zoneid,next_wp_num,xpos,ypos,zpos,pause), errbuf)) {
		LogFile->write(EQEMuLog::Error, "Error adding grid entry '%s': '%s'", query, errbuf);
	} else {
		if(c) c->LogSQL(query);
	}
	safe_delete_array(query);
	
	if(CreatedNewGrid)
		return grid_num;
	
	return 0;
} /*** END Database::AddWPForSpawn() ***/


int16 Database::GetFreeGrid(int16 zoneid) {
    char *query = 0;
	char errbuf[MYSQL_ERRMSG_SIZE];
    MYSQL_RES *result;
    MYSQL_ROW row;
	if (RunQuery(query, MakeAnyLenString(&query,"SELECT max(id) from grid where zoneid = %i",zoneid),errbuf,&result)) {
		safe_delete_array(query);
		if (mysql_num_rows(result) == 1) {
			row = mysql_fetch_row(result);
			int16 tmp=0;
			if (row[0]) 
				tmp = atoi(row[0]);
			mysql_free_result(result);
			tmp++;
			return tmp;
		}
		mysql_free_result(result);
	}
	else {
		LogFile->write(EQEMuLog::Error, "Error in GetFreeGrid query '%s': %s", query, errbuf);
		safe_delete_array(query);
	}
	return 0;
}









