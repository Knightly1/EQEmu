

#include "stdafx.h"
#include "EQBuilder.h"
#include "EQBuilderDlg.h"
#include <afxtempl.h>
#include <math.h>
#include "globals.h"
#include "../common/types.h"
#include "../common/classes.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif


#define MIN_COMMON_WAYPOINTS_MERGE 7
#define MIN_UNIQUE_POINTS_NOMERGE 3



/*
the goal of this method is to try to combine fixed spawns with
roaming spawns if the roaming spawn passes through the spawn
point of the fixed spawn, and there is a common spawn between the
roaming spawn

this is going to be very slow for now.
*/
void CEQBuilderDlg::JoinFixedMobs() {
	if ( !bFixedMergeErrCheck->GetCheck() )
		return;

	int i,j,k,l;
	int maxpos = fixedSpawns->getsize() + gridSpawns->getsize();
	int curpos = 0;

	float err2 = filtres.fixedmerge_error*filtres.fixedmerge_error;

	int fs = fixedSpawns->getsize();
	int rs = gridSpawns->getsize();
	bool foundgrid, foundmob;
	for ( i = 0; i < fs; i++ ) {
		cspawn* spawn = fixedSpawns->get(i);
		
		foundgrid = false;
		for ( j = 0; j < rs; j++ ) {
			cspawn* grid = gridSpawns->get(j);

			if(   (spawn->is_special && !grid->is_special)
				||(grid->is_special && !spawn->is_special))
				continue;	//special and non-special spawns may never merge

			if(grid->grid == NULL)
				continue;	//WTF... why is it here???
			
			int ms = spawn->mobs->getsize();
			foundmob = false;
			for(k = 0; k < ms; k++) {
				const cmob *mob = spawn->mobs->get(k);
				if(grid->ContainsNPC(mob->npc, false)) {
					break;	//found it
				}
			}
			if(k >= ms)
				continue;	//NPC not found, skip this spawn group

			//we found a grid which has a mob in common with this fixed spawn
			//see if their grid crosses our spawn point
//should we force all spawn points to match, or just one, or not consider multi-point spawns, or split them???
			ms = spawn->locs->getsize();
			int mw = grid->grid->waypoints->getsize();
			for(k = 0; k < ms; k++) {
				const cspawnpoint *loc = spawn->locs->get(k);
				for(l = 0; l < mw; l++) {
					const cwaypoint *pt = grid->grid->waypoints->get(l);
					if(isSameLoc(loc, pt->loc, err2))
						break;
				}
				if(l < mw)
					break;
			}
			if(k >= ms && mw > 1) {
				//none of this spawn's points match this grid waypoints
				//now see if the points fall close to any of the line segments
				for(k = 0; k < ms; k++) {
					const cspawnpoint *loc = spawn->locs->get(k);

					//move along the path, remembering the last point we were at
					//and seeing if the spawn point is close to the line segment
					//between the last path point and the current path point
					const cloc *lpt = grid->grid->waypoints->get(0)->loc;
					for(l = 1; l < mw; l++) {
						const cwaypoint *pt = grid->grid->waypoints->get(l);
						if(pointCloseToLine(loc, lpt, pt->loc, err2))
							break;
						lpt = pt->loc;
					}
					if(l < mw)
						break;
				}
			}
			if(k >= ms)
				continue;	//its not near waypoints or line segments, no match
			
			//found a roamer which matches this fixed spawn
			//move the fixed mob list into this roamer's mob list
			//making sure that each mob is still unique
			grid->takeMobs(spawn);
			break;
		}
		
		if(j < rs) {
			fixedSpawns->remove(spawn);
			i--;	//we need to do this index again
			fs--;
			delete spawn;
		}
	}

}

void CEQBuilderDlg::JoinPathingMobs() {
	if(!bPathingErrCheck->GetCheck())
		return;
	
	float err2 = filtres.path_error*filtres.path_error;

	int rs = gridSpawns->getsize();
	int r,k,l,e;
	bool foundmob;
	for(r = 0; r < rs; r++) {
		cspawn *outer = gridSpawns->get(r);
		
		if(outer->grid == NULL || outer->grid->waypoints == NULL)
			continue;
		
		for(e = r+1; e < rs; e++) {
			cspawn *inner = gridSpawns->get(e);
			
			if(e <= r)
				continue;

			if(inner == outer || inner->grid == outer->grid)
				continue;

			if(inner->grid == NULL || inner->grid->waypoints == NULL)
				continue;
			
			if(   (inner->is_special && !outer->is_special)
				||(outer->is_special && !inner->is_special))
				continue;	//special and non-special spawns may never merge

			//see if these spawns have a mob in common
			int ms = inner->mobs->getsize();
			foundmob = false;
			for(k = 0; k < ms; k++) {
				const cmob *mob = inner->mobs->get(k);
				if(outer->ContainsNPC(mob->npc, false)) {
					break;	//found it
				}
			}
			if(k >= ms)
				continue;	//no matches, skip this spawn group
			
			//ok, these two grids are canidates for merging...
			//see how many points they have in common
			int matches = 0;
			int mo, mi;
			mo = outer->grid->waypoints->getsize();
			mi = inner->grid->waypoints->getsize();
			for(k = 0; k < mo; k++) {
				const cwaypoint *owp = outer->grid->waypoints->get(k);
				const cloc *ll = NULL;
				for(l = 0; l < mi; l++) {
					const cwaypoint *iwp = inner->grid->waypoints->get(l);
					if(isSameLoc(owp->loc, iwp->loc, err2)) {
						matches++;
						break;
					} else if(l > 0) {
						if(pointCloseToLine(owp->loc, ll, iwp->loc, err2)) {
							matches++;
							break;
						}
					}
					ll = iwp->loc;
				}
			}
			
			//look for whole path matches
			if((mo-matches) <= MIN_UNIQUE_POINTS_NOMERGE) {
				//essentially all of the outer spawn matches the inner spawn
				//basically just throw out the few points of outer which dont match
				if(mi < mo) {
					//outer is longer than inner, rare case
					//use outer as the grid
				} else {
					//inner is loner than outer
					cgrid *g = outer->grid;
					outer->grid = inner->grid;
					inner->grid = g;
				}
				//in either case, merge inner's mobs into outer, free inner
				outer->takeMobs(inner);
				gridSpawns->remove(inner);	//take it from the list
				e--;	//adjust our nice loop counters
				rs--;
				delete inner;
				continue;
			} else if((mi-matches) <= MIN_UNIQUE_POINTS_NOMERGE) {
				//essentially all of the inner spawn matches the outer spawn
				//just discard whatever part of outer dosent match
				//assumes outer is longer than inner because we didnt match most of outer
				//merge inner's mobs into outer, free inner
				outer->takeMobs(inner);
				gridSpawns->remove(inner);	//take it from the list
				e--;	//adjust our nice loop counters
				rs--;
				delete inner;
				continue;
			}

			if(matches < MIN_COMMON_WAYPOINTS_MERGE) {
				//we did not meet the minimum threshold for merging
				//we still want to merge small paths if over half the points matched
				if(((matches*2)/mo) >= 1) {
					//but over half of outer matched... merge em
				} else if(((matches*2)/mi) >= 1) {
					//but over half of inner matched... merge em
				} else {
					continue;
				}
			}

			//if we get here, we want to merge these two paths
			//right now I am too lazy to find a really good way to do this

			cgrid *from;
			cgrid *into;
			int mf, mt;
			if(mi > mo) {
				//inner is longer than outer, swap grids
				from = outer->grid;
				into = inner->grid;
				//we need to swap out the grids for memory management too
				inner->grid = from;
				outer->grid = into;
			} else {
				from = inner->grid;
				into = outer->grid;
			}
			//the final grid will be in outer->grid
			mf = from->waypoints->getsize();
			mt = into->waypoints->getsize();
			
			//ok, the overall idea here is to find the segment of overlap between the two paths
			//basically the first line segment where they were the same, and the last
			//ignoring small glitches in the middle
			vector<uint16> left_matches(mf), right_matches(mf);
			bool found;
			const cloc *ll;
			for(k = 0; k < mf; k++) {
				const cwaypoint *fwp = from->waypoints->get(k);

				found = false;
				ll = NULL;
				for(l = 0; l < mt; l++) {
					const cwaypoint *twp = into->waypoints->get(l);
					if(isSameLoc(fwp->loc, twp->loc, err2)) {
						left_matches[k] = l;
						found = true;
						break;
					} else if(l > 0) {
						if(pointCloseToLine(fwp->loc, ll, twp->loc, err2)) {
							//decide which waypoint it is closest to, and record that one
							if(fwp->loc->dist2xy(ll) < fwp->loc->dist2xy(twp->loc)) {
								left_matches[k] = l-1;
							} else {
								left_matches[k] = l;
							}
							found = true;
							break;
						}
					}
					ll = twp->loc;
				}
				if(!found)
					left_matches[k] = mt;

				found = false;
				ll = NULL;
				for(l = mt; l > 0;) {
					l--;
					const cwaypoint *twp = into->waypoints->get(l);
					if(isSameLoc(fwp->loc, twp->loc, err2)) {
						right_matches[k] = l;
						found = true;
						break;
					} else if(ll != NULL) {
						if(pointCloseToLine(fwp->loc, ll, twp->loc, err2)) {
							//decide which waypoint it is closest to, and record that one
							if(fwp->loc->dist2xy(ll) < fwp->loc->dist2xy(twp->loc)) {
								right_matches[k] = l-1;
							} else {
								right_matches[k] = l;
							}
							found = true;
							break;
						}
					}
					ll = twp->loc;
				}
				if(!found)
					right_matches[k] = 0;
			}
			
			int best_l, best_r, best_len, len;
			best_len = 0;
			best_l = mf;
			best_r = mf;
			for(k = 0; k < mf; k++) {
				//first consider left matches
				if(k > left_matches[k]) {
					//this left path has potential to extend the grid
					len = mf - left_matches[k] + k;

					//see if there is a right path which will grow it, prolly shouldent happen (since into is longer than from)
					int br, br_len;
					br_len = 0;
					br = mf;
					for(l = k+1; l < mf; l++) {
						if((mf-l) > (mt-right_matches[l])) {
							//this is potentially longer than whats in 'into' after `right_matches[l]`
							if((mf-l) > br_len) {
								//its longer than the longest one we have seen...
								br = l;
								br_len = mf-l;
							}
						}
					}
					len += br_len;

					if(len > best_len) {
						best_len = len;
						best_l = k;
						best_r = br;
					}
				}
				//now consider right
				if((mf-k) > (mt-right_matches[k])) {
					//this is potentially longer than whats in 'into' after `right_matches[l]`
					if((mf-k) > best_len) {
						//its longer than the longest one we have seen...
						best_r = k;
						best_l = mf;
						best_len = mf-k;
					}
				}

			}

			if(best_l == mf && best_r == mf) {
				//no path improvement... just merge the mobs and be done with it
			} else if((best_r-best_l) <= MIN_UNIQUE_POINTS_NOMERGE) {
				//almost the entire path was consumed, why was this not caught above??
				//ignore the from path, and just merge the mobs
			} else if(best_l == mf) {
				//we have only a right path segment to append
				waypoint_list *result = new waypoint_list();

				//copy in all the points in 'into' left of the join point
				int mm = right_matches[best_r];
				for(k = 0; k < mm; k++) {
					result->add(new cwaypoint(into->waypoints->get(k)));
				}
				//now copy in all points in 'from' right of the join point
				for(k = best_r; k < mf; k++) {
					result->add(new cwaypoint(from->waypoints->get(k)));
				}

				//now swap this out for into's waypoints
				delete into->waypoints;
				into->waypoints = result;
			} else if(best_r == mf) {
				//we have only a left path segment to prepend
				waypoint_list *result = new waypoint_list();

				//now copy in all points in 'from' left of the join point
				for(k = 0; k < best_l; k++) {
					result->add(new cwaypoint(from->waypoints->get(k)));
				}
				//copy in all the points in 'into' right of the join point
				int mm = left_matches[best_l];
				for(k = mm; k < mt; k++) {
					result->add(new cwaypoint(into->waypoints->get(k)));
				}

				//now swap this out for into's waypoints
				delete into->waypoints;
				into->waypoints = result;
			} else {
				//we have both a left and right path segment to attach... strange
				waypoint_list *result = new waypoint_list();

				//now copy in all points in 'from' left of the join point
				for(k = 0; k < best_l; k++) {
					result->add(new cwaypoint(from->waypoints->get(k)));
				}
				//copy in all the points in 'into' between the two join points
				k = left_matches[best_l];
				int mm = right_matches[best_r];
				for(; k < mm; k++) {
					result->add(new cwaypoint(into->waypoints->get(k)));
				}
				//now copy in all points in 'from' right of the join point
				for(k = best_r; k < mf; k++) {
					result->add(new cwaypoint(from->waypoints->get(k)));
				}

				//now swap this out for into's waypoints
				delete into->waypoints;
				into->waypoints = result;
			}

			
			
			//we are done now, kill off inner
			outer->takeMobs(inner);
			gridSpawns->remove(inner);	//take it from the list
			e--;	//adjust our nice loop counters
			rs--;
			delete inner;
		}
	}
}


bool CEQBuilderDlg::isSameLoc( const cloc* loc1, const cloc* loc2, const float err2 ) const {
	
	float dz = loc1->z - loc2->z;
	if(dz < 0)
		dz = -dz;
	
	if(dz > filtres.deltaz)
		return(false);
	
	float dx = loc1->x - loc2->x;
	float dy = loc1->y - loc2->y;
	float d2 = dx*dx + dy*dy;
	
	return(d2 <= err2);
}


bool CEQBuilderDlg::pointCloseToLine( const cloc *Point, const cloc *LineStart, const cloc *LineEnd, float threshold2) const {
    float LineMag;
    float U;
    cloc Intersection;
	
	LineMag = LineStart->dist2xy(LineEnd);

	if(LineMag <= threshold2)
		return(false);		//the line is short enough that the point should be able to join with an end point, dont need more math

    LineMag = sqrtf(LineMag);
 
    U = ( ( ( Point->x - LineStart->x ) * ( LineEnd->x - LineStart->x ) ) +
        ( ( Point->y - LineStart->y ) * ( LineEnd->y - LineStart->y ) ) +
        ( ( Point->z - LineStart->z ) * ( LineEnd->z - LineStart->z ) ) ) /
        ( LineMag * LineMag );
 
    if( U < 0.0f || U > 1.0f )
        return(false);   // closest point does not fall within the line segment, if we were to be close enough, it would be to an end point
 
    Intersection.z = LineStart->z + U * ( LineEnd->z - LineStart->z );

	//make sure the Z coords are within allowed variance
	float dz = Point->z - Intersection.z;
	if(dz < 0)
		dz = -dz;
	if(dz > filtres.deltaz)
		return(false);

    Intersection.x = LineStart->x + U * ( LineEnd->x - LineStart->x );
    Intersection.y = LineStart->y + U * ( LineEnd->y - LineStart->y );
	
	return(Intersection.dist2xy(Point) <= threshold2);
}












