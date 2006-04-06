// EQBuilderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "EQBuilderDlg.h"
#include <afxtempl.h>
#include <math.h>
#include  <io.h>
#include "globals.h"
#include "../common/types.h"
#include "../common/classes.h"
#include "ZoneViewer.h"
#include "BigZoneDlg.h"
#include "../common/TextMapFile.h"
#include "../zone/map.h"
#include "LoadingMap.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define COLINEAR_DOT_PRODUCT_THRESHOLD 0.97		//~14 degrees
#define MAX_PATH_LENGTH 600.0f



void CEQBuilderDlg::Reprocess() {
	ProcessData();
}

void CEQBuilderDlg::ProcessData() {

	npcid = npcstartid;
	currentlog->compilepos = 50;
	pProgress->SetPos( 50 );

	//process the logs and consolicate the entities in our lists
	//combine all the mob entires, set npc ids, and handle merchants
	getNPCData();
	
	//split the spawn data into roaming and non-roaming mobs
	//creating spawns for the roaming mobs, into gridSpawns
	//and making a list of non-roaming mobs for clustering, into fixedMobs
	splitSpawnData();
	
	//cluster fixed mobs into spawn points, into fixedSpawns
	clusterFixedMobs();
	
	//look through each spawn point in the spawn list.
	//find the longest path of any mob in the spawn point, and use it for the point
	getGridData(gridSpawns);
	
	//clean up both of the spawn lists, also setting their IDs
	int spawnid = 0;
	if ( sqloptions->m_useopt == 0 ) {
		spawnid = zoneid * sqloptions->m_zoneid;
	} else if ( sqloptions->m_useopt == 1 ) {
		spawnid = sqloptions->m_spawnid;
	}
	fixedSpawns = CleanSpawnList(fixedSpawns, spawnid);
	gridSpawns = CleanSpawnList(gridSpawns, spawnid);

	//we now have our seperate fixed and moving spawn points
	
	//display the spawns
	afficherSpawns();
	SetDataDisplay(lSpawnTabs->GetCurSel());
	
	// scores
	CString doors;
	doors.Format( "%d", nbdoors() );

	CString teleports;
	teleports.Format( "%d", nbteleports() );

	CString mobs;
	mobs.Format( "%d", nbmobs() );

	CString npcs;
	npcs.Format( "%d", nbnpcs() );

	CString spawns;
	spawns.Format( "%df,%dg", nbfspawns(), nbgspawns() );

	CString grids;
	grids.Format( "%d", nbgrids() );

	int msets, mcount;
	CountMerchants(msets, mcount);
	CString merchants;
	merchants.Format( "%d/%d", msets, mcount );

	sDoorStatic->SetWindowText( doors );
	sTeleportStatic->SetWindowText( teleports );
	sMobStatic->SetWindowText( mobs );
	sNPCStatic->SetWindowText( npcs );
	sSpawnStatic->SetWindowText( spawns );
	sGridStatic->SetWindowText( grids );
	sMerchantStatic->SetWindowText( merchants );
	
	//zone view
	vZoneView.ClearSpawnLists();
	vZoneView.AddSpawnList(gridSpawns);
	vZoneView.AddSpawnList(fixedSpawns);
	vZoneView.RedrawWindow();

	//big zone view.
	m_bigZone->SetLists(gridSpawns, fixedSpawns, listNPCs);

	// boutons
	bWriteButton->EnableWindow();

}

void CEQBuilderDlg::ReloadMap() {
	LoadingMap stat;
	
	stat.ShowWindow(true);
	stat.RedrawWindow();

	if(text_map != NULL)
		delete text_map;
	text_map = new TextMapReader();
	if(!text_map->ReadFile(sqloptions->m_EQMaps, zonename)) {
		delete text_map;
		text_map = NULL;
	}
	//set in either case
	vZoneView.SetMapReader(text_map);
	vZoneView.RedrawWindow();
	m_bigZone->SetMapReader(text_map);

	zone_map = Map::LoadMapfile(zonename, sqloptions->m_EQEmuMaps);
}

spawn_list *CEQBuilderDlg::CleanSpawnList(spawn_list *from, int &spawnid) {
	spawn_list *to = new spawn_list();

	//just because our linked list wants to delete itself, we have to copy everything here...

	int minprobability = 50;

	if ( bMinprobCheck->GetCheck() ) {
		if ( filtres.minprob != "" ) {
			minprobability = atoi(filtres.minprob);
			if ( ( minprobability < 0 ) || ( minprobability > 100 ) ) {
				minprobability = 50;
			}
		}
	}

	int maxpos = from->getsize();

	for ( int i=0;i<maxpos;i++ ) {
		cspawn *ospawn = from->get(i);

		//make the NPCs in this group unique, adjusting probabilities as needed
		cleanSpawn( ospawn );
		
		// check minimum probability
		if ( ospawn->probability < minprobability ) {
			continue;
		}

		//clone the spawn for the gay linked list stuff
		cspawn* spawn = new cspawn( ospawn );

		// give it the next spawn id
		spawn->id = spawnid;
		spawnid++;
		
		//add it to the result list
		to->add(spawn);
	}

	delete from;

	return(to);
}

/*void CEQBuilderDlg::CreateSpawns() {
	
	if ( fixedSpawns != NULL ) {
		delete fixedSpawns;
		fixedSpawns = NULL;
	}
	if ( gridSpawns != NULL ) {
		delete gridSpawns;
		gridSpawns = NULL;
	}

	fixedSpawns = new spawn_list();
	gridSpawns = new spawn_list();

	int minprobability = 50;

	if ( bMinprobCheck->GetCheck() ) {
		if ( filtres.minprob != "" ) {
			minprobability = atoi(filtres.minprob);
			if ( ( minprobability < 0 ) || ( minprobability > 100 ) ) {
				minprobability = 50;
			}
		}
	}

	int spawnid;

	if ( sqloptions->m_useopt == 0 ) {
		spawnid = zoneid * sqloptions->m_zoneid;
	} else if ( sqloptions->m_useopt == 1 ) {
		spawnid = sqloptions->m_spawnid;
	}

	int maxpos = listSpawns->getsize();

	for ( int i=0;i<maxpos;i++ ) {

		cspawn* spawn = new cspawn( listSpawns->get(i) );

		// probability
		if ( spawn->probability < minprobability ) {
			delete [] spawn;
			continue;
		}

		// on clean les occurences de npc
		cleanSpawn( spawn );

		// spawn id
		spawn->id = spawnid;
		spawnid++;

		// grid spawns
		if ( spawn->roaming == true ) {

			gridSpawns->add( spawn );			

		// fixed spawns
		} else {

			fixedSpawns->add( spawn );

		}

		// progression
		currentlog->compilepos = 80 + ( i * 10 ) / maxpos;
		pProgress->SetPos( currentlog->compilepos );

	}

}*/

/*bool CEQBuilderDlg::isInGrid( cspawn* spawn, int &gridid, int &wpindex ) {

	for ( int i=0; i<listGrids->GetSize(); i++ ) {

		cgrid grid = listGrids->GetAt(i);

		for ( int j=0; j<grid.nbwaypoints; j++ ) {
			cwaypoint wp = grid.waypoints->GetAt(j);
			
			if ( isSameLoc( spawn.loc, wp.loc ) == locSamePoint ) {
				gridid = grid.id;
				wpindex = j;
				return true;
			}

		}

	}

	return false;

}*/

void CEQBuilderDlg::cleanSpawn( cspawn* spawn ) {
	
	int r,k;
	for(r = 0; r < spawn->mobs->getsize(); r++) {
		cmob* mob1 = spawn->mobs->get(r);
		for(k = r+1; k < spawn->mobs->getsize();) {
			cmob* mob2 = spawn->mobs->get(k);
			if ( isSameNPC( mob1->npc, mob2->npc ) ) {
				//bonus probabilities for two logs with the same mob in the same spot
				if(spawn->probability < 100) {
					spawn->probability += filtres.affirm;
					if(spawn->probability > 100)
						spawn->probability = 100;
				}
				
				//see the mob we are replacing has better 'detail' fields
				if(mob1->npc->helmtexture == 0 && mob1->npc->face == -1) {
					if(mob2->npc->helmtexture != 0 || mob2->npc->face != -1) {
						//mob2 might have better info, try to grab it.
						mob1->npc->helmtexture = mob2->npc->helmtexture;
						mob1->npc->face = mob2->npc->face;
						mob1->npc->luclin_hairstyle = mob2->npc->luclin_hairstyle;
						mob1->npc->luclin_haircolor = mob2->npc->luclin_haircolor;
						mob1->npc->luclin_eyecolor = mob2->npc->luclin_eyecolor;
						mob1->npc->luclin_eyecolor2 = mob2->npc->luclin_eyecolor2;
						mob1->npc->luclin_beard = mob2->npc->luclin_beard;
						mob1->npc->luclin_beardcolor = mob2->npc->luclin_beardcolor;
					}
				}
				
				//copy over merchant info if they have it and we do not
				//this is prolly not the right place to do this, but hey
				//nothing else is done right in this POS.
				if(mob1->npc->merchant == NULL && mob2->npc->merchant != NULL)
					mob1->npc->merchant = new cmerchant(mob2->npc->merchant);
				
				spawn->mobs->remove( mob2 );
			} else {
				k++;
			}
		}
	}

/*
	int i = 1;
	while ( i < spawn->mobs->getsize() ) {
		cmob* mob1 = spawn->mobs->get(i);
		for ( int j=0; j<i; j++ ) {
			cmob* mob2 = spawn->mobs->get(j);
			if ( isSameNPC( mob1->npc, mob2->npc ) ) {
					spawn->mobs->remove( mob1 );
					mob1 = NULL;
					break;
			}
		}
		if ( mob1 != NULL ) {
			i++;
		}
	}*/

}

bool CEQBuilderDlg::isMovingAtInit( cmob* mob ) {

	return ( ( mob->speed->x != 0 ) || ( mob->speed->y != 0 ) || ( mob->speed->z != 0 ) );

}

void CEQBuilderDlg::getGridData(spawn_list *spawns) {
	
	if ( listGrids != NULL ) {
		delete [] listGrids;
		listGrids = NULL;
	}

	listGrids = new grid_list();

	int gridid;


	if ( sqloptions->m_useopt == 0 ) {
		gridid = zoneid * sqloptions->m_zoneid / 2;
	} else if ( sqloptions->m_useopt == 1 ) {
		gridid = sqloptions->m_gridid;
	}

	int maxpos = spawns->getsize();

	for ( int i=0; i<maxpos; i++ ) {
		
		cspawn* spawn = spawns->get(i);

		if ( spawn->roaming == true ) {

			cgrid* grid = NULL;

			//find the longest grid for this spawn's set of mobs
			for ( int j = 0; j<spawn->mobs->getsize(); j++ ) {
				
				cmob* mob = spawn->mobs->get(j);
				
				if ( mob->waypoints == NULL )
					continue;
					
				cgrid* mobgrid = getGrid( mob );
				if(mobgrid == NULL)
					continue;

				if ( grid == NULL ) {
					grid = mobgrid;
				} else {
					if ( grid->waypoints->getsize() < mobgrid->waypoints->getsize() ) {
						delete [] grid;
						grid = mobgrid;
					} else {
						delete [] mobgrid;
					}
				}

			}


			if ( grid == NULL ) {
				continue;
			}

			grid->id = gridid;
			gridid++;

			spawn->grid = new cgrid( grid );

			cloc *loc = new cloc();
			loc->x = grid->waypoints->get(0)->loc->x;
			loc->y = grid->waypoints->get(0)->loc->y;
			loc->z = grid->waypoints->get(0)->loc->z;
			loc->heading = grid->waypoints->get(0)->loc->heading;

			if(spawn->locs != NULL)
				delete spawn->locs;
			spawn->locs = new loc_list();
			spawn->locs->add(loc);
			spawn->center = *loc;

			listGrids->add( grid );

		}

		//progression
		currentlog->compilepos = 70 + ( i * 10 ) / maxpos;
		pProgress->SetPos( currentlog->compilepos );

	}


}

class g_point { public:
	g_point(const cloc *l) { x = l->x; y = l->y; z = l->z; }
	bool equals(const cloc *r) const {
		if((x - r->x) > 0.5) return(false);
		if((y - r->y) > 0.5) return(false);
		if((z - r->z) > 0.5) return(false);
		return(true);
	}
	float x, y, z; };
bool operator<(const g_point &l, const g_point &r) {
	return(	l.x < r.x?true : (
			l.x > r.x?false : (
			l.y < r.y?true : (
			l.y > r.y?false : (
			l.z < r.z ) ) ) ) );
}

cgrid* CEQBuilderDlg::getGrid( cmob* mob ) {
	int size = mob->waypoints->getsize();
	if(size < 2)
			return(NULL);
	

	cgrid* grid = new cgrid();
	grid->waypoints = new waypoint_list();
	

	double max_init_dist = 80*80;
	cloc *path_first = mob->waypoints->get(0)->loc;
	//if the mob's initial position was close to their first waypoint, use it on the path
	//this is attempting to correct a major problem
	if(mob->loc->dist2xy(path_first) < max_init_dist) {
		cwaypoint* wp_init = new cwaypoint();
		wp_init->id = mob->id;
		wp_init->loc = new cloc( mob->loc );
		wp_init->pause = true;
		wp_init->valid = true;

		grid->waypoints->add( wp_init );
	}

	cwaypoint* wp_init = new cwaypoint();
	wp_init->id = mob->id;
	wp_init->loc = new cloc( path_first );
	wp_init->pause = true;
	wp_init->valid = true;

	/*
		We are counting points to try to solve this retarded issue
		where live seems to send us an invalid point in a mob's movement
		seemingly between each valid spawn point. sometimes multiple times...
		but sometimes not at all... its a strange one...
	*/
	map<g_point, uint16> point_counts;
	//count all our points...
	g_point pc(wp_init->loc);
	if(point_counts.find(pc) == point_counts.end())
		point_counts[pc] = 1;
	else
		point_counts[pc]++;
	grid->waypoints->add( wp_init );

	int i;
	for ( i=1; i < size; i++ ) {
		cwaypoint* wp2 = mob->waypoints->get(i);
		g_point p(wp2->loc);
		if(point_counts.find(p) == point_counts.end())
			point_counts[p] = 1;
		else
			point_counts[p]++;
	}
	map<g_point, uint16>::iterator curp, endp;
	curp = point_counts.begin();;
	endp = point_counts.end();
	uint16 bcount = 0;
	bool discard_most_frequent = false;
	g_point most_frequent(wp_init->loc);
	for(; curp != endp; curp++) {
		if(curp->second > bcount) {
			bcount = curp->second;
			most_frequent = curp->first;
		}
	}
	if(bcount > 3 && bcount > size/3) {
		discard_most_frequent = true;
	}


	cloc* lastlocation = wp_init->loc;
	cloc* lastlocation2 = NULL;
	bool lastpause = true;
	bool valid;
	bool colinear;

	VERTEX start, end, hit;
	float len;
	for ( i=1; i < size; i++ ) {
		cwaypoint* wp2 = mob->waypoints->get(i);

		if(discard_most_frequent && most_frequent.equals(wp2->loc))
			continue;
		
valid = true;
colinear = false;
/*		//watch for long paths...
		end.x = lastlocation->x - wp2->loc->x;
		end.y = lastlocation->y - wp2->loc->y;
		float len2 = end.x*end.x + end.y*end.y;
		if(len2 > MAX_PATH_LENGTH*MAX_PATH_LENGTH) {
			valid = false;
		}
		
		//test for colinearity
		if(lastlocation2 != NULL) {

//temp disabled for testing..
//				colinear = arePointsColinear(wp2->loc, lastlocation, lastlocation2);
				colinear = false;
		} else
			colinear = false;
		
		if(zone_map != NULL) {
			start.x = lastlocation->x;
			start.y = lastlocation->y;
			start.z = lastlocation->z+3;
			end.x = wp2->loc->x;
			end.y = wp2->loc->y;
			end.z = wp2->loc->z+3;
			
			//len calculated above!
			if(len > MAX_PATH_LENGTH)
				valid = false;
			else
				valid = !zone_map->LineIntersectsZone(start, end, 0.1f, &hit);
		} else {
			if(len > MAX_PATH_LENGTH)
				valid = false;
			else
				valid = true;
		}
*/
		
		cwaypoint* wp1 = new cwaypoint( wp2 );
		wp2->valid = wp1->valid = valid;
		wp2->colinear = wp1->colinear = colinear;
		grid->waypoints->add(wp1);
		lastpause = wp1->pause;

		lastlocation2 = lastlocation;
		lastlocation = wp1->loc;
	}

	if(grid->waypoints->getsize() < 2) {
		delete grid;
		return(NULL);
	}

	return grid;

}

bool CEQBuilderDlg::arePointsColinear(const cloc *pt1, const cloc *pt2, const cloc *pt3) {
	VERTEX start, end;
	float len, dot;
	
	//last walking vector
	start.x = pt2->x - pt1->x;
	start.y = pt2->y - pt1->y;
	start.z = 0;
	//current walking vector
	end.x = pt2->x - pt3->x;
	end.y = pt2->y - pt3->y;
	end.z = 0;
	//normalize both vectors
	len = sqrt(end.x*end.x + end.y*end.y);
	end.x /= len;
	end.y /= len;
	len = sqrt(start.x*start.x + start.y*start.y);
	start.x /= len;	//this len is used below!
	start.y /= len;

	//dot product of the two vectors:
	dot = start.x*end.x + start.y*end.y;

	return(dot > COLINEAR_DOT_PRODUCT_THRESHOLD);
}

void CEQBuilderDlg::cleanMobMovement(cmob *mob) {
	if(!mob->moved && !mob->roamed)
		return;
	if(mob->waypoints == NULL)
		return;

	cwaypoint *lastwp = NULL;
	const cloc *lastlocation = mob->loc;
	const cloc *lastlocation2 = NULL;
	bool allready_moved = false;
	bool cut_path = false;
	VERTEX start, end, hit;
	
	waypoint_list *current_path = new waypoint_list();
	vector<waypoint_list *> paths;
	
	int maxpos = mob->waypoints->getsize();
	for ( int i=0; i<maxpos; i++ ) {
		cwaypoint *c = mob->waypoints->get(i);
		cut_path = false;
		
		if(isSameCoord(lastlocation, c->loc)) {
			//allow pure-heading changes if they are really walking around
			if(!allready_moved || lastlocation->heading == c->loc->heading) {
				//skip this waypoint
				continue;
			}
		}
		
		//test for colinearity
		if(lastlocation2 != NULL) {
			if(false/*arePointsColinear(lastlocation2, lastlocation, c->loc)*/) {
				//last three points are colinear, we want to drop the middle point
				//but this point is allready in our list and hence cannot be removed
				//so instead we make it the last point...
				lastwp->loc->x = c->loc->x;
				lastwp->loc->y = c->loc->y;
				lastwp->loc->z = c->loc->z;
				lastwp->loc->heading = c->loc->heading;
				lastwp->id = c->id;
				lastwp->pause = c->pause;
				lastwp->db = c->db;
				lastwp->valid = c->valid;
				lastwp->colinear = c->colinear;
				
				//now drop the current point, since we just essentially added it.
				//leave last pointers as they were, they are still valid...
				continue;
			}
		}
		
		//once we get here, we need to break the path if we find a break in it
		
		
		if(zone_map != NULL) {
			start.x = lastlocation->x;
			start.y = lastlocation->y;
			start.z = lastlocation->z+3;
			end.x = c->loc->x;
			end.y = c->loc->y;
			end.z = c->loc->z+3;
			
			cut_path = zone_map->LineIntersectsZone(start, end, 0.1f, &hit);
		}

		//watch for long paths...
		end.x = lastlocation->x - c->loc->x;
		end.y = lastlocation->y - c->loc->y;
		float len2 = end.x*end.x + end.y*end.y;
		if(len2 > MAX_PATH_LENGTH*MAX_PATH_LENGTH) {
			cut_path = true;
		}
		
		//we need to add 'does this segment allready exist in our path' checking..
		
		if(cut_path) {
			//we need to cut the paths here...
			//add this as the last point in the now ended path
//			current_path->add(new cwaypoint(c));
			//if the path isnt empty, add it to the potentials
			if(current_path->getsize() > 1)
				paths.push_back(current_path);
			else
				delete current_path;
			
			
			//now make a new path...
			current_path = new waypoint_list();
			//and create our first point if it is not our original starting location...
			if(!isSameCoord(mob->loc, c->loc)) {
				lastwp = new cwaypoint(c);
				current_path->add(lastwp);
			} else {
				lastwp = NULL;
			}
			
			//reset our trailing pointers
			lastlocation2 = NULL;
			lastlocation = c->loc;
			allready_moved = false;
		} else {
			//new location is not the same as the old one
			allready_moved = true;
			lastwp = new cwaypoint(c);
			current_path->add(lastwp);
			
			lastlocation2 = lastlocation;
			lastlocation = c->loc;
		}
	}

	
	//if the path isnt empty, add it to the potentials
	if(current_path->getsize() > 1)
		paths.push_back(current_path);
	else
		delete current_path;
	
	if(paths.size() > 0)
		allready_moved = true;
	
	if(!allready_moved) {
		mob->moved = false;
		mob->roamed = false;
		delete mob->waypoints;
		mob->waypoints = NULL;
		return;
	}
	
	waypoint_list *best_path = NULL;
	vector<waypoint_list *>::iterator curwp, endwp;
	if(paths.size() == 1) {
		//single path... use it..
		best_path = paths[0];
	} else {
		//find the longest path of the segments
		int best_length = 0, length;
		curwp = paths.begin();
		endwp = paths.end();
		for(; curwp != endwp; curwp++) {
			waypoint_list *wp = *curwp;
			length = wp->getsize();
			if(length > best_length) {
				best_length = length;
				best_path = wp;
			}
		}
	}
	
	if(best_path != NULL) {
		mob->moved = true;
		mob->roamed = true;
		delete mob->waypoints;
		mob->waypoints = best_path;
	}
	
	//free all our extra lists
	curwp = paths.begin();
	endwp = paths.end();
	for(; curwp != endwp; curwp++) {
		if(*curwp != best_path)
			delete *curwp;
	}
}

LocDifference CEQBuilderDlg::isSameLoc( const cloc* loc1, const cloc* loc2 ) {
	
	float err2 = filtres.coord_error*filtres.coord_error;
	float camp2 = filtres.camp_range*filtres.camp_range;
	
	float dx = loc1->x - loc2->x;
	float dy = loc1->y - loc2->y;
	float dz = loc1->z - loc2->z;
	if(dz < 0)
		dz = -dz;
	
	if(dz > filtres.deltaz)
		return(locUnrelated);
	
	float d2 = dx*dx + dy*dy;
	
	if(d2 <= err2)
		return(locSamePoint);
	if(d2 <= camp2)
		return(locCampRange);
	return(locUnrelated);
}

bool CEQBuilderDlg::isSameCoord( const cloc* loc1, const cloc* loc2 ) {
	float dx = loc1->x - loc2->x;
	float dy = loc1->y - loc2->y;
	float d2 = dx*dx + dy*dy;
	return(d2 < 0.1*0.1);
}

int CEQBuilderDlg::getGridIndex( cgrid* grid1 ) {

	for ( int i=0; i<listGrids->getsize(); i++ ) {
		cgrid* grid2 = listGrids->get(i);
		for ( int j=0; j<grid2->waypoints->getsize(); j++ ) {
			cwaypoint* wp2 = grid2->waypoints->get(j);
			for ( int k=0; k<grid1->waypoints->getsize(); k++ ) {
				cwaypoint* wp1 = grid1->waypoints->get(k);
				if ( isSameLoc( wp1->loc, wp2->loc ) == locSamePoint ) {
					return i;
				}
			}
		}
	}

	return -1;

}

int CEQBuilderDlg::getNPCid( cnpc* npc ) {

	for ( int i=0; i<listNPCs->getsize(); i++ ) {
		if ( isSameNPC( npc, listNPCs->get(i) ) ) {
			return listNPCs->get(i)->id;
		}
	}

	return 0;

}

void CEQBuilderDlg::AddNewSpawn(spawn_list *list, cmob *mob, logType log_type, bool calc_prob) {
	cspawn* spawn = new cspawn();
	spawn->locs->add(new cloc( mob->loc ));
	spawn->locs->getcenter(&spawn->center);
	spawn->center = cloc(mob->loc);
	spawn->mobs = new mob_list();
	spawn->mobs->add( new cmob( mob ) );
	spawn->nbmobs = 1;
	spawn->truespawn = false;
	spawn->falsespawn = false;
	if ( ( mob->roamed == true ) && ( mob->killed == false ) ) {
		spawn->roaming = true;
	} else {
		spawn->roaming = false;
	}
	if ( filtres.movementok == false ) {
		if ( spawn->roaming == true ) {
			return;
		}
	}
	
	if(calc_prob) {
		spawn->probability = getSpawnProbability( spawn, log_type );
	} else {
		spawn->probability = 100;
	}
	list->add( spawn );
}

void CEQBuilderDlg::splitSpawnData() {

	if ( gridSpawns != NULL ) {
		delete gridSpawns;
		gridSpawns = NULL;
	}
	if ( fixedMobs != NULL ) {
		delete fixedMobs;
		fixedMobs = NULL;
	}

	gridSpawns = new spawn_list();
	fixedMobs = new mob_list();

	int maxpos = nblogs;

	for ( int numlog=0; numlog<nblogs; numlog++ ) {

		const clog* log = compiledlogs->get(numlog);
		
		if ( !(log->compiled) ) {
			continue;
		}
		
		int initcount = log->mobinit->getsize();
		for ( int i=0; i < initcount; i++ ) {

			cmob* mob = log->mobinit->get(i);

			mob->npc->id = getNPCid( mob->npc );
			
			cleanMobMovement(mob);

/*			if ( log->type == logRaid ) {
				//this logic seems retarded....
				if ( mob->npc->level <= 65 ) {
					continue;
				}
				if ( ( mob->moved == true ) || ( mob->roamed == true ) ) {
					continue;
				}
			}
*/
			
			//save fixed spawns for the clustering algorithm
			if(!mob->roamed) {
				//add them to the fixed list and be done with them.
				fixedMobs->add(new cmob(mob));
				continue;
			}
			//only roamers get here...
			
			if ( isMovingAtInit( mob ) == true ) {
				if ( log->type != logPathing ) {
					continue;
				}
			}

			LocDifference diff;
			cspawn *cs = getSpawnPoint( gridSpawns, mob, diff );
			if ( cs == NULL ) {
				
				AddNewSpawn(gridSpawns, mob, log->type, true);
				
			} else {
				//only join two roaming spawns if they are real close
				if(diff == locSamePoint)
					updateSpawnPoint( cs, mob, false, log->type );
				else
					AddNewSpawn(gridSpawns, mob, log->type, true);

			}

		}

		int addcount = log->mobadd->getsize();
		for ( int j=0; j < addcount; j++ ) {

			cmob* mob = log->mobadd->get(j);

			cleanMobMovement(mob);

			mob->npc->id = getNPCid( mob->npc );
			
			//save fixed spawns for the clustering algorithm
			if(!mob->roamed) {
				//add them to the fixed list and be done with them.
				fixedMobs->add(new cmob(mob));
				continue;
			}
			//only roamers get here...
			
			
			LocDifference diff;
			cspawn *cs = getSpawnPoint( gridSpawns, mob, diff );
			if ( cs == NULL ) {
				
				AddNewSpawn(gridSpawns, mob, log->type, false);

			} else {

				//only join two roaming spawns if they are real close
				if(diff == locSamePoint)
					updateSpawnPoint( cs, mob, true, log->type );
				else
					AddNewSpawn(gridSpawns, mob, log->type, false);

			}

		}

		// progression
		currentlog->compilepos = 60 + ( numlog * 10 ) / maxpos;
		pProgress->SetPos( currentlog->compilepos );

	}

}

//this function is 'add another mob to this spawn point cause its close enough'
void CEQBuilderDlg::updateSpawnPoint( cspawn *spawn, cmob* mob, bool addspawn, logType typelog ) {
	
	//just verify this again
	if(spawn->is_camp && mob->roamed)
		return;	//cannot add a roamer to a camp

	//if the new mob roamed (and was not killed), and the spawn point is not a roamer
	//and we are collecting pathing from this log, then make this spawn
	//point a roamer
	if ( ( mob->roamed == true ) && ( spawn->roaming == false ) && ( mob->killed == false ) ) {
		if ( typelog == logPathing ) {
			spawn->roaming = true;
		}
	}

	if ( filtres.movementok == false ) {
		if ( spawn->roaming == true ) {
			return;
		}
	}

	// mob list
	//only add it if this mob ID is not allready in this spawn point.
	//if(spawn->mobs->getmobbyid(mob->id) == NULL) {
		spawn->mobs->add( new cmob( mob ) );
		spawn->nbmobs++;
	//}

	//update the center
	spawn->locs->getcenter(&spawn->center);
	

	// probability
	if ( addspawn ) {
		spawn->truespawn = true;
		spawn->falsespawn = false;
		spawn->probability = 100;
	} else {
		spawn->truespawn = false;
		spawn->falsespawn = false;
		spawn->probability = getSpawnProbability( spawn, typelog ) ;
	}
}

cspawn *CEQBuilderDlg::getSpawnPoint( spawn_list *list, cmob* mob, LocDifference &diff ) {

	int maxpos = list->getsize();
	for ( int i=0; i<maxpos; i++ ) {
		
		cspawn *c = list->get(i);
		
		//cannot add a roaming mob to a camp spawn point
		if(c->is_camp && mob->roamed)
			continue;
		
		diff = isSameLoc( &c->center, mob->loc );
		//if either mob or spawn point is a roamer, we cannot be in camp range
		if(mob->roamed || c->roaming) {
			if(diff == locSamePoint)
				return c;
			//else, not a match... either camp or unrelated
		} else {
			//neither entity is moving, we can build a camp if we want.
			//let the update routine handle the camp logic.
			if(diff == locSamePoint || diff == locCampRange)
				return(c);
		}
	}

	return(NULL);
}

int CEQBuilderDlg::getSpawnProbability( cspawn* spawn, logType typelog ) {

	int prob = 50;

	if ( spawn->truespawn ) {
		return 100;
	}

	if ( spawn->falsespawn ) {
		return 0;
	}

	int nb_total = spawn->nbmobs;
	int nb_moving = 0;
	int nb_dead = 0;
	int dead_multiplier = 1;

	for ( int i = 0; i<nb_total; i++ ) {
		cmob* mob = spawn->mobs->get( i );
		if ( mob->killed == true ) {
			nb_dead ++;
		}
		if ( mob->moved == true ) {
			nb_moving ++;
		}
		if ( mob->npc->level >= 70 ) {
			return 100;
		}
	}

	// moving
	int moving_factor = 5;
	if ( bMovedCheck->GetCheck() ) {
		if ( filtres.moved != "" ) {
			moving_factor = atoi(filtres.moved);
		}
		if ( ( moving_factor < 0 ) || ( moving_factor > 20 ) ) {
			moving_factor = 10;
		}
		if ( typelog == logPathing ) {
			moving_factor = 0;
		}
		prob += moving_factor * ( nb_total - 2 * nb_moving );
		if ( nb_moving == 0 ) {
			prob += moving_factor;
		}
		if ( nb_moving == nb_total ) {
			if ( typelog != logPathing ) {
				prob = 0;
			}
		}
	}

	// dead
	int dead_factor = 5;
	if ( bDeadCheck->GetCheck() ) {
		if ( filtres.dead != "" ) {
			dead_factor = atoi(filtres.dead);
		} 
		if ( ( dead_factor < 0 ) || ( dead_factor > 20 ) ) {
			dead_factor = 5;
		}
		prob += dead_factor * ( nb_total - 2 * nb_dead );
	}
	if ( nb_dead == 0 ) {
		prob += dead_factor;
	}
	if ( nb_dead == nb_total ) {
		prob = 0;
	}

	// occurence
	int occurence_factor = 5;
	if ( bOccurCheck->GetCheck() ) {
		if ( filtres.occur != "" ) {
			occurence_factor = atoi(filtres.occur);
		}
		if ( ( occurence_factor < 0 ) || ( occurence_factor > 20 ) ) {
			occurence_factor = 5;
		}
	}

	if ( nb_total >= 4 ) {
		prob += (nb_total-4) * occurence_factor;
	}

	if ( prob > 100 ) {
		prob = 100;
	} else if ( prob < 0 ) {
		prob = 0;
	}

	return prob;
}

void CEQBuilderDlg::processNPCData(cmob *mob, IntArray* usednpcids) {
	cnpc *existing_npc = isNPCDejaSauve( mob->npc );
	if ( existing_npc == NULL ) {

		if ( sqloptions->m_usedb == TRUE ) {
			int id = db->getnpcid( mob->npc );
			if ( id == 0 ) {
				id = db->getfreenpcid( npcid, usednpcids );
			}
			mob->npc->id = id;
		} else {
			mob->npc->id = npcid;
			npcid++;
		}
		
		if(mob->npc->merchant != NULL) {
			listShops->add(new cmerchant( mob->npc->merchant ));
		}
		listNPCs->add( new cnpc( mob->npc ) );
	} else {
		if(mob->npc->merchant != NULL) {
			if(existing_npc->merchant == NULL) {
				//the old mob didnt have a merchant set, but the new one does.
				//move the merchant set over to the old mob.
				existing_npc->merchant = mob->npc->merchant;
				existing_npc->merchant->owner = existing_npc;
				mob->npc->merchant = NULL;
				listShops->add(new cmerchant( existing_npc->merchant ));
			} else {
				//TODO: could merge merchant lists or do something else intelligent...
			}
		}
	}
}

void CEQBuilderDlg::getNPCData() {

	if(listNPCs != NULL) {
		delete listNPCs;
	}
	if(listShops != NULL) {
		delete listShops;
	}

	listNPCs = new npc_list();
	listShops = new merchant_list();

	if ( sqloptions->m_useopt == 0 ) {
		npcstartid = zoneid * sqloptions->m_zoneid;
	} else if ( sqloptions->m_useopt == 1 ) {
		npcstartid = sqloptions->m_npcid;
	}

	int maxpos = nblogs;
	
	IntArray* usednpcids = new IntArray();

	for ( int numlog=0; numlog<nblogs; numlog++ ) {

		clog* log = compiledlogs->get(numlog);
		
		if ( !(log->compiled) ) {
			continue;
		}

		for ( int i=0; i<log->nbmobinit; i++ ) {

			cmob* mob = log->mobinit->get(i);
			processNPCData(mob, usednpcids);

		}

		for ( int j=0; j<log->nbmobadd; j++ ) {

			cmob* mob = log->mobadd->get(j);
			processNPCData(mob, usednpcids);
		}

		// progression
		currentlog->compilepos = 50 + ( numlog * 10 ) / maxpos;
		pProgress->SetPos( currentlog->compilepos );

	}

	delete [] usednpcids;

}

bool CEQBuilderDlg::IsValidSpawn( cmob* mob ) {

	if ( mob->owned == true ) return false;
	if ( mob->hp < 100 ) return false;
	if ( mob->type != 1 ) return false;
	if ( mob->npc->race == 127 ) return false;

	// nom
	CString mobnom = mob->npc->nom;
	if ( mobnom.Find( "_Mount", 0 ) >= 0 ) return false;
//	if ( mobnom.Left( 1 ) == "_" ) return false;
	if ( mobnom.Left( 7 ) == "Eye_of_" ) return false;

	return true;
}

bool CEQBuilderDlg::isSameNPC( cnpc* npc1, cnpc* npc2 ) {
	return(npc1->IsSameAs(npc2));
}

cnpc *CEQBuilderDlg::isNPCDejaSauve( cnpc* npc ) {

	cnpc *cc = NULL;
	for ( int i=0; i<nbnpcs(); i++ ) {
		cc = listNPCs->get(i);
		if ( isSameNPC( npc, cc ) ) {
			return(cc);
		}
	}

	return(NULL);
}



