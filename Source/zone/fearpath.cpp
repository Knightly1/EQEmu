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
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "../common/files.h"
#include "zone_profile.h"
#include "fearpath.h"
#include "map.h"
#include "zone.h"
#ifdef WIN32
#define snprintf	_snprintf
#endif


/*

Fear guide points. these are not used directly by the fear pathing
code. They are used by `apathing` to act as hint points to 
provide more valid points for it to process on

CREATE TABLE fear_hints (
	id INT AUTO_INCREMENT PRIMARY KEY,
	zone VARCHAR(16) NOT NULL,
	x FLOAT NOT NULL,
	y FLOAT NOT NULL,
	z FLOAT NOT NULL,
	UNIQUE KEY(zone,x,y,z)
);

*/


extern Zone* zone;

//#define OPTIMIZE_FEAR_QT_LOOKUPS

//#define DEBUG_SEEK 1
#define DEBUG_NEXT
//#define DEBUG_BEST_Z 1

//quick functions to clean up vertex code.
#define Vmin3(o, a, b, c) ((a.o<b.o)? (a.o<c.o?a.o:c.o) : (b.o<c.o?b.o:c.o))
#define Vmax3(o, a, b, c) ((a.o>b.o)? (a.o>c.o?a.o:c.o) : (b.o>c.o?b.o:c.o))

FearPathManager* FearPathManager::LoadPathFile(const char* in_zonename) {
	FILE *fp = NULL;
	char zBuf[64];
	char cWork[256];
	FearPathManager* ret = 0;
	
	//have to convert to lower because the short names im getting
	//are not all lower anymore, copy since strlwr edits the str.
	strncpy(zBuf, in_zonename, 64);
	zBuf[63] = '\0';
	
	snprintf(cWork, 250, MAP_DIR "/%s.path", strlwr(zBuf));
	
	if ((fp = fopen( cWork, "rb" ))) {
		ret = new FearPathManager();
		if(ret->loadPaths(fp)) {
			printf("Path File %s loaded.\n", cWork);
		} else {
			printf("Path File %s loading failed.\n", cWork);
		}
		fclose(fp);
	}
	else {
		printf("Path File %s not found.\n", cWork);
	}
	return ret;
}

FearPathManager::FearPathManager() {
	_minz = 999999e111;
	_minx = 999999e111;
	_miny = 999999e111;
	_maxz = -999999e111;
	_maxx = -999999e111;
	_maxy = -999999e111;
	
	m_Nodes = 0;
	m_Links = 0;
	nodes = NULL;
	links = NULL;
	m_QTNodes = 0;
	m_NodeLists = 0;
	QTNodes = NULL;
	nodelists = NULL;
}

bool FearPathManager::loadPaths(FILE *fp) {
#ifndef INVERSEXY
#warning Path files do not work without inverted XY
	return(false);
#endif

	PathFile_Header head;
	if(fread(&head, sizeof(head), 1, fp) != 1) {
		//map read error.
		return(false);
	}
	if(head.version != PATHFILE_VERSION) {
		//invalid version... if there really are multiple versions,
		//a conversion routine could be possible.
		printf("Invalid path file version 0x%lx, we want 0x%lx\n", head.version, PATHFILE_VERSION);
		return(false);
	}
	
	printf("Path header: %lu nodes, %lu links, %u QT nodes, %lu nodelists\n", head.node_count, head.link_count, head.qtnode_count, head.nodelist_count);
	
	m_Nodes = head.node_count;
	m_Links = head.link_count;
	m_QTNodes = head.qtnode_count;
	m_NodeLists = head.nodelist_count;
	
	nodes = new PathNode_Struct[m_Nodes];
	links = new PathLink_Struct[m_Links];
	QTNodes = new PathTree_Struct[m_QTNodes];
	nodelists = new FearNodeRef[m_NodeLists];
	
	unsigned long count;
	if((count=fread(nodes, sizeof(PathNode_Struct), m_Nodes , fp)) != m_Nodes) {
		printf("Unable to read %lu nodes from path file, got %lu.\n", m_Nodes, count);
		return(false);
	}
	if((count=fread(links, sizeof(PathLink_Struct), m_Links , fp)) != m_Links) {
		printf("Unable to read %lu nodes from path file, got %lu.\n", m_Links, count);
		return(false);
	}
	
	if((count=fread(QTNodes, sizeof(PathTree_Struct), m_QTNodes, fp)) != m_QTNodes) {
		printf("Unable to read %lu qt nodes from path file.\n", m_Nodes);
		return(false);
	}
	if((count=fread(nodelists, sizeof(FearNodeRef), m_NodeLists, fp)) != m_NodeLists) {
		printf("Unable to read %lu node lists from path file. Got %lu.\n", m_NodeLists, count);
		return(false);
	}
	
	return(true);
}

FearPathManager::~FearPathManager() {
	safe_delete_array(nodes);
	safe_delete_array(links);
	safe_delete_array(QTNodes);
	safe_delete_array(nodelists);
}


FearNodeRef FearPathManager::SeekNode( FearNodeRef node_r, float x, float y ) {
	if(node_r == FEAR_NODE_NONE || node_r >= m_QTNodes) {
		return(FEAR_NODE_NONE);
	}
	PFPNODE _node = &QTNodes[node_r];
#ifdef DEBUG_SEEK
printf("Seeking node for %u:(%.2f, %.2f) with root 0x%x.\n", node_r, x, y, _node);

printf("	Current Box: (%.2f -> %.2f, %.2f -> %.2f)\n", _node->minx, _node->maxx, _node->miny, _node->maxy);
#endif
	if( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy ) {
		if( _node->flags & fearNodeFinal ) {
#ifdef DEBUG_SEEK
printf("Seeking node for %u:(%.2f, %.2f) with root 0x%x.\n", node_r, x, y, _node);
printf("	Final Node: (%.2f -> %.2f, %.2f -> %.2f)\n", _node->minx, _node->maxx, _node->miny, _node->maxy);
fflush(stdout);
printf("	Final node found with %d fear nodes.\n", _node->nodelist.count);
/*printf("	Faces:\n");
unsigned long *cfl = mFaceLists + _node->faces.offset;
unsigned long m;
for(m = 0; m < _node->faces.count; m++) {
	FACE *c = &mFinalFaces[ *cfl ];
	printf("	%lu (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				*cfl, c->a.x, c->a.y, c->a.z,
				c->b.x, c->b.y, c->b.z, 
				c->c.x, c->c.y, c->c.z);
	cfl++;
}*/
#endif
			return node_r;
		}
#ifdef DEBUG_SEEK
printf("	Kids: %u, %u, %u, %u\n", _node->nodes[0], _node->nodes[1], _node->nodes[2], _node->nodes[3]);
		
printf("	Contained In Box: (%.2f -> %.2f, %.2f -> %.2f)\n", _node->minx, _node->maxx, _node->miny, _node->maxy);
		
/*printf("	Node found has children.\n");
if(_node->node1 != NULL) {
	printf("\tNode: (%.2f -> %.2f, %.2f -> %.2f)\n", 
		_node->node1->minx, _node->node1->maxx, _node->node1->miny, _node->node1->maxy);
}
if(_node->node2 != NULL) {
	printf("\tNode: (%.2f -> %.2f, %.2f -> %.2f)\n", 
		_node->node2->minx, _node->node2->maxx, _node->node2->miny, _node->node2->maxy);
}
if(_node->node3 != NULL) {
	printf("\tNode: (%.2f -> %.2f, %.2f -> %.2f)\n", 
		_node->node3->minx, _node->node3->maxx, _node->node3->miny, _node->node3->maxy);
}
if(_node->node4 != NULL) {
	printf("\tNode: (%.2f -> %.2f, %.2f -> %.2f)\n", 
		_node->node4->minx, _node->node4->maxx, _node->node4->miny, _node->node4->maxy);
}*/
#endif
		//NOTE: could precalc these and store them in node headers

		FearNodeRef tmp = FEAR_NODE_NONE;
#ifdef OPTIMIZE_FEAR_QT_LOOKUPS
		float midx = _node->minx + (_node->maxx - _node->minx) * 0.5;
		float midy = _node->miny + (_node->maxy - _node->miny) * 0.5;
		//follow ordering rules from map.h...
		if(x < midx) {
			if(y < midy) { //quad 3
				if(_node->nodes[2] != FEAR_NODE_NONE)
					tmp = SeekNode( _node->nodes[2], x, y );
			} else {	//quad 2
				if(_node->nodes[2] != FEAR_NODE_NONE)
					tmp = SeekNode( _node->nodes[1], x, y );
			}
		} else {
			if(y < midy) {  //quad 4
				if(_node->nodes[2] != FEAR_NODE_NONE)
					tmp = SeekNode( _node->nodes[3], x, y );
			} else {	//quad 1
				if(_node->nodes[2] != FEAR_NODE_NONE)
					tmp = SeekNode( _node->nodes[0], x, y );
			}
		}
		if( tmp != FEAR_NODE_NONE ) return tmp;
#else
		tmp = SeekNode( _node->nodes[0], x, y );
		if( tmp != FEAR_NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[1], x, y );
		if( tmp != FEAR_NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[2], x, y );
		if( tmp != FEAR_NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[3], x, y );
		if( tmp != FEAR_NODE_NONE ) return tmp;
#endif

	}
#ifdef DEBUG_SEEK
printf(" No node found.\n");
#endif
	return(FEAR_NODE_NONE);
}

//this looks for our QT node, if it cannot find our, it
//will find one close to us
FearNodeRef FearPathManager::SeekNodeGuarantee( FearNodeRef node_r, float x, float y ) {
	if(node_r == FEAR_NODE_NONE || node_r >= m_QTNodes) {
		return(FEAR_NODE_NONE);
	}
	
	PFPNODE _node = &QTNodes[node_r];
	if( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy ) {
		if( _node->flags & fearNodeFinal ) {
			return node_r;
		}
		//NOTE: could precalc these and store them in node headers

		FearNodeRef tmp = FEAR_NODE_NONE;
#ifdef OPTIMIZE_FEAR_QT_LOOKUPS
		float midx = _node->minx + (_node->maxx - _node->minx) * 0.5;
		float midy = _node->miny + (_node->maxy - _node->miny) * 0.5;
		//follow ordering rules from map.h...
		if(x < midx) {
			if(y < midy) { //quad 3
				if(_node->nodes[2] != FEAR_NODE_NONE)
					tmp = SeekNode( _node->nodes[2], x, y );
			} else {	//quad 2
				if(_node->nodes[2] != FEAR_NODE_NONE)
					tmp = SeekNode( _node->nodes[1], x, y );
			}
		} else {
			if(y < midy) {  //quad 4
				if(_node->nodes[2] != FEAR_NODE_NONE)
					tmp = SeekNode( _node->nodes[3], x, y );
			} else {	//quad 1
				if(_node->nodes[2] != FEAR_NODE_NONE)
					tmp = SeekNode( _node->nodes[0], x, y );
			}
		}
		if( tmp != FEAR_NODE_NONE ) return tmp;
#else
		tmp = SeekNode( _node->nodes[0], x, y );
		if( tmp != FEAR_NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[1], x, y );
		if( tmp != FEAR_NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[2], x, y );
		if( tmp != FEAR_NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[3], x, y );
		if( tmp != FEAR_NODE_NONE ) return tmp;
#endif
		
		//if we get here we didnt find the node in our children,
		//so use ourself as the node.
		return(node_r);
	}
#ifdef DEBUG_SEEK
printf(" No node found.\n");
#endif
	return(FEAR_NODE_NONE);
}

/*

A quadtree is not the ideal structure for finding the closest
node, because if you are close to a boundary, you will not search
nodes which are just over the boundary, even if they are much closer.

Also, the sparse nature of the grid points might lead to several gaps
in the quadtree which contain no nodes

to help fix this, I modified the quadtree to store a node list
at each level, this allows us to get a node list even if our current
node dosent exist/is empty.

I also dont just store nodes WITHIN this QT node, but also nodes
within a defined range, so if you stay under that range, you will always
find the true closest node. (FEAR_MAXIMUM_DISTANCE)

This also means that if we cannot find our QT node, that there are no
nodes within FEAR_MAXIMUM_DISTANCE from the mob, so you might wanna 
just call it quits.

*/

bool FearPathManager::FindNearestPath(MobFearState *state, float x, float y, float z, bool check_los) {
	if(zone->map == NULL)
		return(false);
	
	FearNodeRef qtnodeR;
	qtnodeR = SeekNodeGuarantee(GetRoot(), x, y);
	if(qtnodeR == FEAR_NODE_NONE) {
		//we have no node from garuntee... this is bad
		return(false);
	}
	
	//get our real node
	PathTree_Struct *qtnode = GetQTNode(qtnodeR);
	
	//grab our node list, which is qtnode->nodelist.count long
	FearPointRef *nlist = GetNodeList(qtnode);
	
#ifdef DEBUG_SEEK
printf("QT Node starts at %d and has %d points.\n", qtnode->nodelist.offset, qtnode->nodelist.count);
#endif
	PathNode_Struct *curn, *bestn = NULL;
	FearPointRef bestref = FEAR_NODE_NONE;
	float cur_dist, best_dist2;
	VERTEX p1, p2, liz_res;
	int32 r;
	
	p1.x = x;
	p1.y = y;
	p1.z = z + 6.0;
	
	//loop through each node in the list, and find the closest we can see
	best_dist2 = 9999999e111;
	for(r = 0; r < qtnode->nodelist.count; r++, nlist++) {
		curn = GetNode(*nlist);
		if(curn == NULL)
			continue;
#ifdef DEBUG_SEEK
printf("\nTrying node %d at (%.3f,%.3f,%.3f) ", *nlist, curn->x, curn->y, curn->z);
#endif
		
		//make sure its the closest
		cur_dist = Dist2(x, y, z, curn->x, curn->y, curn->z);
		if(cur_dist >= best_dist2)
			continue;
#ifdef DEBUG_SEEK
printf("Maybe..(%.3f<%.3f)", cur_dist, best_dist2);
#endif
		
		//make sure we can see it...
		p2.x = curn->x;
		p2.y = curn->y;
		p2.z = curn->z + 6.0;
		if(check_los && zone->map->LineIntersectsZone(p1, p2, 0.5, &liz_res))
			continue;
#ifdef DEBUG_SEEK
printf("New Best");
#endif
		
		best_dist2 = cur_dist;
		bestn = curn;
		bestref = *nlist;
	}
#ifdef DEBUG_SEEK
printf("\nBest node: 0x%x\n", bestn);
#endif
	
	if(bestn == NULL)
		return(false);	//unable to see any of our closest nodes..
	
	//start with running to bottom since it will stay
	//localized longer, once they get to top, they really only
	//run along one path from top to bottom.
	state->state = MobFearState::runToBottom;
	state->goal_node = bestref;
	state->last_link = FEAR_LINK_NONE;
	
	state->x = bestn->x;
	state->y = bestn->y;
	state->z = bestn->z;
	
#ifdef DEBUG_SEEK
printf("Found fear node: (%.3f,%.3f,%.3f)\n", bestn->x, bestn->y, bestn->z);
#endif
	
	return(true);
}

bool FearPathManager::NextPathNode(MobFearState *state) {
#ifdef DEBUG_NEXT
	printf("Finding next node from node %d, via link %d. (%.3f,%.3f,%.3f)\n", state->goal_node, state->last_link, state->x, state->y, state->z);
#endif
	if(!state->IsValidState()) {
		return(false);
	} else if(state->state == MobFearState::runToBottom) {
		//assume we have reached our goal node, find our next goal
		
		//the goal of running to bottom is to increase our distance
		//from the root node, while taking the path which has the
		//greatest reach
#ifdef DEBUG_NEXT
	printf("Starting 'To Bottom' search\n");
#endif
		
		PathNode_Struct *gnode = GetNode(state->goal_node);
		if(gnode == NULL)
			return(false);
		
		PathLink_Struct *links = GetLinks(gnode->link_offset);
		int r;
		
		PathNode_Struct *look_node;
		FearNodeRef longest = 0;
		FearPointRef long_node = FEAR_NODE_NONE;
		FearLinkRef long_link = FEAR_LINK_NONE;
		for(r = 0; r < gnode->link_count; r++, links++) {
			if(state->last_link == (gnode->link_offset + r))
				continue;		//never traverse the same link we just came from.
			//make sure its further away than where we are
			look_node = GetNode(links->dest_node);
			if(look_node == NULL)
				continue;		//this shouldent happen.
#ifdef DEBUG_NEXT
			printf("  Checking... d=%d, old_d=%d, best=%d, cur=%d.\n", look_node->distance, gnode->distance, longest, links->reach);
#endif
			//if this node is closer than our current node, skip it
			if(look_node->distance <= gnode->distance)
				continue;	//only looking for further nodes.
			
			if(links->reach > longest) {
				longest = links->reach;
				long_node = links->dest_node;
				long_link = gnode->link_offset + r;
			} else if(links->reach == longest) {
				//try to provide some variance at junction nodes
				//same length longest, give it a 50% chance
				if(MakeRandomInt(0, 99) >= 50)
					continue;
				//it passed, crown this as the new longest
				longest = links->reach;
				long_node = links->dest_node;
				long_link = gnode->link_offset + r;
			} else if(links->reach > (longest - 5)) {
				//try to provide some variance at junction nodes
				//close to the longest, give it a chance based on closeness
				if(MakeRandomInt(0, 99) >= 10*(longest-links->reach))
					continue;
				//it passed, crown this as the new longest
				longest = links->reach;
				long_node = links->dest_node;
				long_link = gnode->link_offset + r;
				
			}
		}
		
		//this means we skipped all links cause they were closer to root
		if(long_node == FEAR_NODE_NONE) {
			//we reached the top, start heading back down...
			state->state = MobFearState::runToTop;
			//call ourselves again to get a real node...
			//I dont THINK infinite recursion is posible... lets hope not (:
			//a node should never be top or bottom unless its alont
			if(m_Links < 2)
				return(false);	//watch for empty grids
			return(NextPathNode(state));
		}
		
		PathNode_Struct *bestn = NULL;
		bestn = GetNode(long_node);
		if(bestn == NULL)
			return(false);
		
		//not a criteria for state change anymore
/*		if(state->last_link != FEAR_LINK_NONE) {
			//Get the link
			PathLink_Struct *last_link = GetLinks(state->last_link);
#ifdef DEBUG_NEXT
	printf("  Last link reach was %d, new link is %d\n", last_link->reach, longest);
#endif
		}*/
		
		
		//only go to the next node if we did not change state.
		//if we changed, let them pick next time we get called.
		if(state->state == MobFearState::runToBottom) {
			state->last_link = long_link;
			state->goal_node = long_node;
		
			state->x = bestn->x;
			state->y = bestn->y;
			state->z = bestn->z;

#ifdef DEBUG_NEXT
		printf("To Bottom picked %d, via link %d. (%.3f,%.3f,%.3f)\n", state->goal_node, state->last_link, state->x, state->y, state->z);
#endif
		}
#ifdef DEBUG_NEXT
else {
			printf("To Top changed state.\n");
}
#endif
		
		return(true);
	} else if(state->state == MobFearState::runToTop) {
		//assume we have reached our goal node, find our next goal
		
		//our goal is to minimize our distance. The root of the tree
		//can reach a minimum number of nodes down each branch
		//beacuse it divides them equally on each branch.
		
#ifdef DEBUG_NEXT
	printf("Starting 'To Top' search\n");
#endif		
		PathNode_Struct *gnode = GetNode(state->goal_node);
		if(gnode == NULL)
			return(false);
		
		PathLink_Struct *links = GetLinks(gnode->link_offset);
		int r;
		
		PathNode_Struct *look_node;
		FearNodeRef shortest = 0xFFFF;
		FearPointRef short_node = FEAR_NODE_NONE;
		FearLinkRef short_link = FEAR_LINK_NONE;
		for(r = 0; r < gnode->link_count; r++, links++) {
			if(state->last_link == (gnode->link_offset + r))
				continue;		//never traverse the same link we just came from.
			//make sure its further away than where we are
			look_node = GetNode(links->dest_node);
			if(look_node == NULL)
				continue;	//this shouldent happen...
#ifdef DEBUG_NEXT
			printf("  Checking...best=%d, cur=%d.\n", shortest, look_node->distance);
#endif
			
			if(look_node->distance < shortest) {
				shortest = look_node->distance;
				short_node = links->dest_node;
				short_link = gnode->link_offset + r;
			}
			//do we want to add variance for going up? seems like we might
			//end up screwing up our ascent
			/* else if(links->distance == shortest) {
				//try to provide some variance at junction nodes
				//same length shortest, give it a 50% chance
				if(MakeRandomInt(0, 99) >= 50)
					continue;
				//it passed, crown this as the new shortest
				shortest = links->distance;
				short_node = links->dest_node;
				short_link = gnode->link_offset + r;
			}*/
		}
		if(short_node == FEAR_NODE_NONE)
			return(false);
		
		PathNode_Struct *bestn = NULL;
		bestn = GetNode(short_node);
		if(bestn == NULL)
			return(false);
		
/*		if(state->last_link != FEAR_LINK_NONE) {
			//Get the link
			PathLink_Struct *last_link = GetLinks(state->last_link);
#ifdef DEBUG_NEXT
	printf("  Last link shortest was %d, new link is %d\n", last_link->distance, shortest);
#endif
			if(shortest > last_link->distance) {
				//we reached the top, start heading back down...
				state->state = MobFearState::runToBottom;
			}
		}*/
#ifdef DEBUG_NEXT
	printf("  Last link shortest was %d, new link is %d\n", gnode->distance, shortest);
#endif
		if(shortest > gnode->distance) {
			//we reached the top, start heading back down...
			//call ourselves again to get a real node...
			state->state = MobFearState::runToBottom;
			if(m_Links < 2)
				return(false);	//watch for empty grids
			return(NextPathNode(state));
		}
		
		//we reached the top, start heading back down...
		if(shortest == 1) {
			//call ourselves again to get a real node...
			state->state = MobFearState::runToBottom;
			if(m_Links < 2)
				return(false);	//watch for empty grids
			return(NextPathNode(state));
		}
		
		//only go to the next node if we did not change state.
		//if we changed, let them pick next time we get called.
		if(state->state == MobFearState::runToTop) {
		
			state->last_link = short_link;
			state->goal_node = short_node;
		
			state->x = bestn->x;
			state->y = bestn->y;
			state->z = bestn->z;

#ifdef DEBUG_NEXT
			printf("To Top picked %d, via link %d. (%.3f,%.3f,%.3f)\n", state->goal_node, state->last_link, state->x, state->y, state->z);
#endif
		}
#ifdef DEBUG_NEXT
else {
			printf("To Top changed state.\n");
}
#endif
		
		return(true);
	}
	
	return(false);
}

#ifdef ENABLE_FEAR_PATHING

#define FEAR_PATHING_DEBUG

//we need to start acting scared...
void Mob::SetFeared(Mob *caster, int32 duration) {
	//special args to stop fear
	if(caster == NULL && duration == 0) {
		fear_state = fearStateNotFeared;
		safe_delete(fear_path_state);
		return;
	}
	
	//fear dosent work without at least maps
	if(zone->map == NULL) {
		fear_state = fearStateStuck;
		return;	//just stand there
	}
	
	//if we are allready feared, and we are on a fear grid..
	//then just stay happy on the grid...
	if(fear_path_state != NULL) {
		if(fear_state != fearStateGrid) {
			LogFile->write(EQEMuLog::Debug, "Umm... %s has a fear path state, but is not in a grid state. Wtf?", GetName());
			fear_state = fearStateGrid;
		}
		return;
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
		return;	//were done, nothing difficult needed.
	}
	
	//OK, so if we just run, we are going to hit something...
	//now we have to think a little more.
	
	//first, try to find a fear node that we can see.
	if(zone->fear != NULL) {
		fear_path_state = new MobFearState();
		if(zone->fear->FindNearestPath(fear_path_state, GetX(), GetY(), GetZ())) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing Start: found path, moving from (%.2f, %.2f, %.2f) to path node  (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), fear_path_state->x, fear_path_state->y, fear_path_state->z);
#endif
			//we found a fear node... were on our way..
			cur_wp_x = fear_path_state->x;
			cur_wp_y = fear_path_state->y;
			cur_wp_z = fear_path_state->z;
			fear_state = fearStateGrid;
			return;
		}
		
		//we have failed to find a path, so we dont need this..
		safe_delete(fear_path_state);
	}
	
	//if we cannot just run, and we cannot see any paths, then
	//we will give one last ditch effort to find a legit path. We 
	//will run as far as we can away from the player, and hope we 
	//can see a path from there if not, we will start breaking rules
	
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing Start: Hope run from (%.2f, %.2f, %.2f), hit at (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), hit.x, hit.y, hit.z);
#endif
	//use the hit point - a little + a little Z as the first waypoint.
	cur_wp_x = hit.x - fear_vector.x * 2;
	cur_wp_y = hit.y - fear_vector.y * 2;
	cur_wp_z = GetZ();
	fear_state = fearStateRunning;
}

void Mob::CalculateFearPosition() {
	if(zone->map == NULL || fear_state == fearStateStuck) {
		return;	//just stand there
	}
	
	//This is the entire movement section, right here:
	if (cur_wp_x != GetX() && cur_wp_y != GetY()) {
		// not at waypoint yet, so keep moving
		CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true); 
		return;
	}	
	
	
	
	//we have reached our waypoint, now what?
	//figure out a new waypoint to run at...
	
	if(fear_state == fearStateRunningForever) {
		//we were supposed to run forever, but we did not...
		//should re-fear ourself or something...
		fear_state = fearStateStuck;
		return;
	}
	
	//first see if we are on a path. if so our life is easy
	if(fear_state == fearStateGrid && fear_path_state) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: on path, moving from (%.2f, %.2f, %.2f) to path node  (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), fear_path_state->x, fear_path_state->y, fear_path_state->z);
#endif
		//assume that we have zone->fear since we got to this state.
		if(!zone->fear->NextPathNode(fear_path_state)) {
			//this is bad, we were on a path and now its giving us
			//an error... we dont have a good way to deal with this
			fear_state = fearStateStuck;
			return;
		}
		//we found a fear node... were on our way..
		cur_wp_x = fear_path_state->x;
		cur_wp_y = fear_path_state->y;
		cur_wp_z = fear_path_state->z;
		
		CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true);
		return;
	}
	
	//the only valid state left is fearStateRunning, where we try to
	//find a grid once we reach our waypoint, which we have..
	if(fear_state != fearStateRunning) {
		//wtf... unknown state
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: Reached our fear waypoint, bu we are in an unknown state... stopping.");
		fear_state = fearStateStuck;
		return;
	}
	
	//we wanted to try to find a waypoint now, so lets try..
	if(zone->fear != NULL) {
		fear_path_state = new MobFearState();
		
		if(zone->fear->FindNearestPath(fear_path_state, GetX(), GetY(), GetZ())) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: ran to find path, moving from (%.2f, %.2f, %.2f) to path node  (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), fear_path_state->x, fear_path_state->y, fear_path_state->z);
#endif
			//we found a fear node... were on our way..
			cur_wp_x = fear_path_state->x;
			cur_wp_y = fear_path_state->y;
			cur_wp_z = fear_path_state->z;
			fear_state = fearStateGrid;
			CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true);
			return;
		}
	
		//if we get here... all valid methods have failed
		
		//ok, now we start making shit up

		//for now, we will limit our bullshitting to ignoring LOS
		//when finding a pathing node, we SHOULD always get something..
		if(zone->fear->FindNearestPath(fear_path_state, GetX(), GetY(), GetZ(), false)) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: Bullshit Path from (%.2f, %.2f, %.2f) to path node  (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), fear_path_state->x, fear_path_state->y, fear_path_state->z);
#endif
			//we found a fear node... were on our way..
			cur_wp_x = fear_path_state->x;
			cur_wp_y = fear_path_state->y;
			cur_wp_z = fear_path_state->z;
			fear_state = fearStateGrid;
			CalculateNewPosition2(cur_wp_x, cur_wp_y, cur_wp_z, GetRunspeed(), true);
			return;
		}
		
		//we have failed to find a path once again, so we dont need this..
		safe_delete(fear_path_state);
	}
	
	//if we get HERE... then NOTHING worked... just stick
	fear_state = fearStateStuck;

	//end of function, everything else is #ifdef'd out
//}

//I dont wanna get rid of this right now because it was a lot of hard
//work to write... but it dosent work reliably, so oh well..
#ifdef OLD_FEAR_PATHING	
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
	
	//first try our original fear vector again...
	VERTEX start, end, hit, normalhit;
	start.x = GetX() - fear_vector.x * 0.4;
	start.y = GetY() - fear_vector.y * 0.4;
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
	
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) normal hit at (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), normalhit.x, normalhit.y, normalhit.z);
#endif
	
	//if we get here, we cannot run along our normal vector...
	//try up hill first
	
	/*
	while this uphill stuff works great in outdoor zones,
	it totally breaks dungeons...
	
	float speed = GetRunspeed();
	end.x = start.x + fear_vector.x * speed;
	end.y = start.y + fear_vector.y * speed;
	end.z = start.z + speed + speed;
	
	if(!zone->map->LineIntersectsZone(start, end, 0.5, &hit, NULL)) {
#ifdef FEAR_PATHING_DEBUG
		LogFile->write(EQEMuLog::Debug, "Fear Pathing: From (%.2f, %.2f, %.2f) up hill run to (%.2f, %.2f, %.2f)",
			GetX(), GetY(), GetZ(), end.x, end.y, end.z);
#endif
		//we can run along this vector without hitting anything...
		cur_wp_x = end.x - 0.4 * fear_vector.x;
		cur_wp_y = end.y - 0.4 * fear_vector.y;
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
	*/
	
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
		cur_wp_x = end.x - 0.4 * fear_vector.x;
		cur_wp_y = end.y - 0.4 * fear_vector.y;
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
		cur_wp_x = end.x - 0.4 * fear_vector.x;
		cur_wp_y = end.y - 0.4 * fear_vector.y;
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
#endif	//OLD_FEAR_PATHING
}
#endif













