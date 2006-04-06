

#include "stdafx.h"
#include "EQBuilder.h"
#include "EQBuilderDlg.h"
#include <afxtempl.h>
#include <math.h>
#include  <io.h>
#include "globals.h"
#include "../common/types.h"
#include "../common/classes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define LARGE_DISTANCE 9e10f

float CEQBuilderDlg::MobDistance(cmob *left, cmob *right) {
	float distance = 0;
	
	float dx = left->loc->x - right->loc->x;
	float dy = left->loc->y - right->loc->y;
	float dz = left->loc->z - right->loc->z;
	if(dz < 0)
		dz = -dz;
	
	if(dz > filtres.deltaz)
		return(LARGE_DISTANCE);
	
	float d2 = dx*dx + dy*dy;

	//ignore camp radius here, used in algorithm

	distance = sqrt(d2);
	

	//modifiers
	if(left->npc->type == right->npc->type)
		distance *= 0.95f;		//5% bonus for same body type
	if(left->npc->race == right->npc->race)
		distance *= 0.90f;		//10% bonus for same race
	if(left->npc->texture == right->npc->texture && left->npc->texture != 0 && left->npc->texture != 255 && left->npc->texture != -1)
		distance *= 0.90f;		//10% bonus for same non-0/255 texture
	if(left->npc->size == right->npc->size)
		distance *= 0.95f;		//5% bonus for same size
	
	//level difference modifiers
	int dl = left->npc->level-right->npc->level;
	if(dl < 0)
		dl = -dl;
	if(dl < 3)
		distance *= 0.90f;		//10% bonus for <3 level difference
	else if(dl > 10)
		distance *= 10;			//1000% penalty for >10 level difference
	else if(dl > 5)
		distance *= 1.10f;		//10% penalty for >5 level difference

	return(distance);
}

float CEQBuilderDlg::SpawnDistance(cmob *left, cspawn *right) {
	float dsum = 0;
	int max;
	float count = max = right->mobs->getsize();
	int r;
	for(r = 0; r < max; r++) {
		cmob *mob = right->mobs->get(r);
		dsum += MobDistance(left, mob);
	}
	dsum /= count;
	return(dsum);
}

bool CEQBuilderDlg::WillMergeSpawns(cspawn *left, cspawn *right) {
	float distance;
	
	float dx = left->center.x - right->center.x;
	float dy = left->center.y - right->center.y;
	
	float d2 = dx*dx + dy*dy;
	distance = sqrt(d2);

	//modifiers
	//TODO: add some of these...
	//based on all kinds of things, penalize 
	// - reduction in racial entropy by merging
	// - reduction in mob level variance

	return(distance < filtres.camp_range);
}

//input = fixedMobs
//output = fixedSpawns
void CEQBuilderDlg::clusterFixedMobs() {
	delete fixedSpawns;
	fixedSpawns = new spawn_list();

	int mobcount = fixedMobs->getsize();
	int i,r;
	
	/*
	int rowoff;

	float *distances = new float[mobcount*mobcount];	//1/2 of this is wasted
	cspawn *used = new cspawn[mobcount];

	memset(used, 0, sizeof(cspawn)*mobcount);
	
	for (i=0; i < mobcount; i++ ) {
		rowoff = i*mobcount;
		cmob *mobi = fixedMobs->get(i);
		for(r = 0; r < i; r++) {
			//mirror cells before diagonal
			distances[rowoff + r] = distances[r*mobcount + i];
		}
		for(r = i; r < mobcount; r++) {
			cmob *mobr = fixedMobs->get(r);
			distances[rowoff + r] = MobDistance(mobi, mobr);
		}
	}*/

	
	float err2 = filtres.coord_error*filtres.coord_error;
	
	spawn_list core_spawns;

	//first time through just collapses spawns within coord_error range
	for( i = 0; i < mobcount; i++ ) {
		cmob *mob = fixedMobs->get(i);

		int spawncount = core_spawns.getsize();

		cspawn *match = NULL;
		float dist = LARGE_DISTANCE+1;
		float curdist;
		for(r = 0; r < spawncount; r++) {
			cspawn *spawn = core_spawns.get(r);
			curdist = SpawnDistance(mob, spawn);
			if(curdist < err2) {
				match = spawn;
				break;
			}
		}

		if(match == NULL)
			AddNewSpawn(&core_spawns, mob, logPathing, true);
		else
			updateSpawnPoint(match, mob, false, logPathing);
	}
	
	
	int corecount = core_spawns.getsize();
	for(i = 0; i < corecount; i++) {
		cspawn *core = core_spawns.get(i);

		int spawncount = fixedSpawns->getsize();
		cspawn *merge_with = NULL;
		for(r = 0; r < spawncount; r++) {
			cspawn *spawn = fixedSpawns->get(r);
			if(WillMergeSpawns(core, spawn)) {
				merge_with = spawn;
				break;
			}
		}
		if(merge_with == NULL) {
			//nobody to merge with, just clone ourself and add to fixedSpawns
			fixedSpawns->add(new cspawn(core));
		} else {
			//merge ourself into the older spawn thats allready in fixedSpawns

			//add a new spawn point for the group at the center of the older group
			//this assumes the core group is really only one spawn point (within coord error)
			merge_with->locs->add(new cloc(core->center));

			//now copy all the mobs into the older group
			int k;
			int max = core->mobs->getsize();
			for(k = 0; k < max; k++) {
				merge_with->mobs->add(new cmob(core->mobs->get(k)));
			}

			//redo center
			merge_with->locs->getcenter(&merge_with->center);

			//redo probability
			merge_with->truespawn = false;		//camps cannot be true spawns
			merge_with->falsespawn = false;		//camps are not false either
			merge_with->probability = getSpawnProbability( merge_with, logPathing ) ;
		}
	}

}
