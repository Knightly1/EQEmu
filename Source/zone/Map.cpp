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
#include "map.h"
#ifdef WIN32
#define snprintf	_snprintf
#endif

//Do we believe the normals from the map file?
//you want this enabled if it dosent break things.
//#define TRUST_MAPFILE_NORMALS

//#define OPTIMIZE_QT_LOOKUPS

//#define DEBUG_SEEK 1

/*
 of note:
 it is possible to get a null node in a valid region if it
 does not have any triangles in it.
 this will dictate bahaviour on getting a null node
 TODO: listen to the above
 */

//quick functions to clean up vertex code.
#define Vmin3(o, a, b, c) ((a.o<b.o)? (a.o<c.o?a.o:c.o) : (b.o<c.o?b.o:c.o))
#define Vmax3(o, a, b, c) ((a.o>b.o)? (a.o>c.o?a.o:c.o) : (b.o>c.o?b.o:c.o))


Map* Map::LoadMapfile(const char* in_zonename) {
	FILE *fp;
	char cWork[256];
	Map* ret = 0;
	
	snprintf(cWork, 250, MAP_DIR "/%s.map", in_zonename);
	
	if ((fp = fopen( cWork, "rb" ))) {
		ret = new Map();
		if(ret != NULL) {
			ret->loadMap(fp);
			printf("Map %s loaded.\n", cWork);
		} else {
			printf("Map %s loading failed.\n", cWork);
		}
		fclose(fp);
	}
	else {
		printf("Map %s not found.\n", cWork);
	}
	return ret;
}

Map::Map() {
#ifdef MAP_COUNT_THINGS
	_branches = 0;
	_finals = 0;
	_minx = 999999;
	_miny = 999999;
	_minz = 999999;
	_maxx = -999999;
	_maxy = -999999;
	_maxz = -999999;
#endif
	m_Faces = 0;
	m_Nodes = 0;
	m_FaceLists = 0;
	mFinalFaces = NULL;
	mNodes = NULL;
	mFaceLists = NULL;
}

bool Map::loadMap(FILE *fp) {
#ifndef INVERSEXY
#warning Map files do not work without inverted XY
	return(false);
#endif

	mapHeader head;
	if(fread(&head, sizeof(head), 1, fp) != 1) {
		//map read error.
		return(false);
	}
	if(head.version != MAP_VERSION) {
		//invalid version... if there really are multiple versions,
		//a conversion routine could be possible.
		printf("Invalid map version 0x%x\n", head.version);
		return(false);
	}
	
	printf("Map header: %lu faces, %u nodes, %lu facelists\n", head.face_count, head.node_count, head.facelist_count);
	
	m_Faces = head.face_count;
	m_Nodes = head.node_count;
	m_FaceLists = head.facelist_count;
	
	/*	fread(&m_Vertex, 4, 1, fp);
	fread(&m_Faces , 4, 1, fp);*/
//	mFinalVertex = new VERTEX[m_Vertex];
	mFinalFaces = new FACE	[m_Faces];
	mNodes = new NODE[m_Nodes];
	mFaceLists = new unsigned long[m_FaceLists];
	
//	fread(mFinalVertex, m_Vertex, sizeof(VERTEX), fp);
	unsigned long count;
	if((count=fread(mFinalFaces, sizeof(FACE), m_Faces , fp)) != m_Faces) {
		printf("Unable to read %lu faces from map file, got %lu.\n", m_Faces, count);
		return(false);
	}
	if(fread(mNodes, sizeof(NODE), m_Nodes, fp) != m_Nodes) {
		printf("Unable to read %lu faces from map file.\n", m_Nodes);
		return(false);
	}
	if(fread(mFaceLists, sizeof(unsigned long), m_FaceLists, fp) != m_FaceLists) {
		printf("Unable to read %lu faces from map file.\n", m_FaceLists);
		return(false);
	}
	
	
/*	mRoot = new NODE();
	RecLoadNode(mRoot, fp );*/

#ifdef MAP_COUNT_THINGS
	unsigned long i;
	float v;
	for(i = 0; i < m_Faces; i++) {
		v = Vmax3(x, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
		if(v > _maxx)
			_maxx = v;
		v = Vmin3(x, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
		if(v < _minx)
			_minx = v;
		v = Vmax3(y, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
		if(v > _maxy)
			_maxy = v;
		v = Vmin3(y, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
		if(v < _miny)
			_miny = v;
		v = Vmax3(z, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
		if(v > _maxz)
			_maxz = v;
		v = Vmin3(z, mFinalFaces[i].a, mFinalFaces[i].b, mFinalFaces[i].c);
		if(v < _minz)
			_minz = v;
	}
	printf("Loaded map: %lu vertices, %lu faces, %lu branch nodes, %lu final nodes\n", m_Faces*3, m_Faces, _branches, _finals);
	printf("Map BB: (%.2f -> %.2f, %.2f -> %.2f, %.2f -> %.2f)\n", _minx, _maxx, _miny, _maxy, _minz, _maxz);
#endif
	return(true);
}

Map::~Map() {
//	safe_delete_array(mFinalVertex);
	safe_delete_array(mFinalFaces);
	safe_delete_array(mNodes);
	safe_delete_array(mFaceLists);
//	RecFreeNode( mRoot );
}


NodeRef Map::SeekNode( NodeRef node_r, float x, float y ) {
	if(node_r == NODE_NONE || node_r >= m_Nodes) {
		return(NODE_NONE);
	}
	PNODE _node = &mNodes[node_r];
#ifdef DEBUG_SEEK
printf("Seeking node for %u:(%.2f, %.2f) with root 0x%x.\n", node_r, x, y, _node);

printf("	Current Box: (%.2f -> %.2f, %.2f -> %.2f)\n", _node->minx, _node->maxx, _node->miny, _node->maxy);
#endif
	if( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy ) {
		if( _node->flags & nodeFinal ) {
#ifdef DEBUG_SEEK
printf("Seeking node for %u:(%.2f, %.2f) with root 0x%x.\n", node_r, x, y, _node);
printf("	Final Node: (%.2f -> %.2f, %.2f -> %.2f)\n", _node->minx, _node->maxx, _node->miny, _node->maxy);
fflush(stdout);
printf("	Final node found with %d faces.\n", _node->faces.count);
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

		NodeRef tmp = NULL;
#ifdef OPTIMIZE_QT_LOOKUPS
		float midx = _node->minx + (_node->maxx - _node->minx) * 0.5;
		float midy = _node->miny + (_node->maxy - _node->miny) * 0.5;
		//follow ordering rules from map.h...
		if(x < midx) {
			if(y < midy) { //quad 3
				if(_node->nodes[2] != NODE_NONE)
					tmp = SeekNode( _node->nodes[2], x, y );
			} else {	//quad 2
				if(_node->nodes[2] != NODE_NONE)
					tmp = SeekNode( _node->nodes[1], x, y );
			}
		} else {
			if(y < midy) {  //quad 4
				if(_node->nodes[2] != NODE_NONE)
					tmp = SeekNode( _node->nodes[3], x, y );
			} else {	//quad 1
				if(_node->nodes[2] != NODE_NONE)
					tmp = SeekNode( _node->nodes[0], x, y );
			}
		}
		if( tmp != NODE_NONE ) return tmp;
#else
		tmp = SeekNode( _node->nodes[0], x, y );
		if( tmp != NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[1], x, y );
		if( tmp != NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[2], x, y );
		if( tmp != NODE_NONE ) return tmp;
		tmp = SeekNode( _node->nodes[3], x, y );
		if( tmp != NODE_NONE ) return tmp;
#endif

	}
#ifdef DEBUG_SEEK
printf(" No node found.\n");
#endif
	return(NODE_NONE);
}

// maybe precalc edges.
int* Map::SeekFace(  NodeRef node_r, float x, float y ) {
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(NULL);
	}
	PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(NULL);   //not a final node... could find the proper node...
	}
	
	
//printf("Seeking face for (%.2f, %.2f) with root 0x%x.\n", x, y, _node);
	float	dx,dy;
	float	nx,ny;
	int		*face = mCandFaces;
	unsigned long i;
	for( i=0;i<_node->faces.count;i++ ) {
		const FACE &cf = mFinalFaces[ mFaceLists[_node->faces.offset + i] ];
		const VERTEX &v1 = cf.a;
		const VERTEX &v2 = cf.b;
		const VERTEX &v3 = cf.c;
		
		dx = v2.x - v1.x; dy = v2.y - v1.y;
		nx =    x - v1.x; ny =    y - v1.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		dx = v3.x - v2.x; dy = v3.y - v2.y;
		nx =    x - v2.x; ny =    y - v2.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		dx = v1.x - v3.x; dy = v1.y - v3.y;
		nx =    x - v3.x; ny =    y - v3.y;
		if( dx*ny - dy*nx >0.0f ) continue;
		
		*face++ = mFaceLists[_node->faces.offset + i];
	}
	*face = -1;
	return mCandFaces;
}

// can be op?
float Map::GetFaceHeight( int _idx, float x, float y ) {
	PFACE	pface = &mFinalFaces[ _idx ];
	return ( pface->nd - x * pface->nx - y * pface->ny ) / pface->nz;
}

//FatherNitwit's LOS code...
//Algorithm stolen from internet
//p1=start of segment
//p2=end of segment

bool Map::LineIntersectsZone(VERTEX start, VERTEX end, float step_mag, VERTEX *result, FACE **on) {
	VERTEX step;
	VERTEX cur = start;
	
	float diff;
	diff = (end.x - start.x);
	if (diff < 1 && diff > -1)
		end.x = start.x;
	diff = (end.y - start.y);
	if (diff < 1 && diff > -1)
		end.y = start.y;
	diff = (end.z - start.z);
	if (diff < 1 && diff > -1)
		end.z = start.z;
	
	step.x = end.x - start.x;
	step.y = end.y - start.y;
	step.z = end.z - start.z;
	//since step size is kinda arbitrary, this sqrt could be
	//approximated somehow to save CPU time
	float factor = step_mag / sqrt(step.x*step.x + step.y*step.y + step.z*step.z);
	step.x *= factor;
	step.y *= factor;
	step.z *= factor;
	
	NodeRef cnode, lnode;
	lnode = NULL;
	//while we are not past end
	//always do this once, even if start == end.
	do {
		//look at current location
		cnode = SeekNode(GetRoot(), cur.x, cur.y);
		if(cnode != NODE_NONE && cnode != lnode) {
			if(LineIntersectsNode(cnode, start, end, result, on))
				return(true);
			lnode = cnode;
		}
		
		//move 1 step
		cur.x += step.x;
		cur.y += step.y;
		cur.z += step.z;
		
		//watch for end conditions
		if ( (cur.x > end.x && end.x > start.x) || (cur.x < end.x && end.x < start.x) ) {
			cur.x = end.x;
		}
		if ( (cur.y > end.y && end.y > start.y) || (cur.y < end.y && end.y < start.y) ) {
			cur.y = end.y;
		}
		if ( (cur.z > end.z && end.z > start.z) || (cur.z < end.z && end.z < start.z) ) {
			cur.z = end.z;
		}
	} while(cur.x != end.x || cur.y != end.y || cur.z != end.z);
	
	//walked entire line and didnt run into anything...
	return(false);
}

bool Map::LocWithinNode( NodeRef node_r, float x, float y ) {
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(NULL);
	}
	PNODE _node = &mNodes[node_r];
	//this function exists so nobody outside of MAP needs to know
	//how the NODE sturcture works
	return( x>= _node->minx && x<= _node->maxx && y>= _node->miny && y<= _node->maxy );
}

bool Map::LineIntersectsNode( NodeRef node_r, VERTEX p1, VERTEX p2, VERTEX *result, FACE **on) {
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(true);   //can see through empty nodes, just allow LOS on error...
	}
	PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(true);   //not a final node... not sure best action
	}
	
	unsigned long i;
	
	PFACE cur;
	unsigned long *cfl = mFaceLists + _node->faces.offset;
	
	for(i = 0; i < _node->faces.count; i++) {
		cur = &mFinalFaces[ *cfl ];
		if(LineIntersectsFace(cur,p1, p2, result)) {
			*on = cur;
			return(true);
		}
		cfl++;
	}

//printf("Checked %ld faces and found none in the way.\n", i);

	return(false);
}


float Map::FindBestZ( NodeRef node_r, VERTEX p1, VERTEX *result, FACE **on) {
	if( node_r == NODE_NONE || node_r >= m_Nodes) {
		return(NULL);
	}
	PNODE _node = &mNodes[node_r];
	if(!(_node->flags & nodeFinal)) {
		return(-999999);   //not a final node... could find the proper node...
	}
	
	p1.z -= 1;
	
	VERTEX p2(p1);
	p2.z = -999999;
	
	float best_z = -999999;

	unsigned long i;

	VERTEX r_tmp;
	if(result == NULL)
		result = &r_tmp;

	PFACE cur;
	unsigned long *cfl = mFaceLists + _node->faces.offset;

printf("Start finding best Z...\n");
	for(i = 0; i < _node->faces.count; i++) {
		cur = &mFinalFaces[ *cfl ];
//printf("Intersecting with face %lu\n", *cfl);
		if(LineIntersectsFace(cur, p1, p2, result)) {
printf("  %lu (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f) (%.2f, %.2f, %.2f)\n",
				*cfl, cur->a.x, cur->a.y, cur->a.z,
				cur->b.x, cur->b.y, cur->b.z, 
				cur->c.x, cur->c.y, cur->c.z);
printf("Found a z: %.2f\n", result->z);
			if (result->z > best_z) {
				*on = cur;
				best_z = result->z;
			}
		}
		cfl++;
	}
fflush(stdout);
printf("Best Z found: %.2f\n", best_z);

	return best_z;
}


bool Map::LineIntersectsFace( PFACE cface, VERTEX p1, VERTEX p2, VERTEX *result) {
	if( cface == NULL ) {
		return(false);  //cant intersect a face we dont have... i guess
	}

#define ABS(x) ((x)<0?-(x):(x))
#define EPS 0.002f	//acceptable error
	
	const VERTEX &pa = cface->a;
	const VERTEX &pb = cface->b;
	const VERTEX &pc = cface->c;
	
	//quick bounding box checks
	float tbb;
	
	tbb = Vmin3(x, pa, pb, pc);
	if(p1.x < tbb && p2.x < tbb)
		return(false);
	tbb = Vmin3(y, pa, pb, pc);
	if(p1.y < tbb && p2.y < tbb)
		return(false);
	tbb = Vmin3(z, pa, pb, pc);
	if(p1.z < tbb && p2.z < tbb)
		return(false);
	
	tbb = Vmax3(x, pa, pb, pc);
	if(p1.x > tbb && p2.x > tbb)
		return(false);
	tbb = Vmax3(y, pa, pb, pc);
	if(p1.y > tbb && p2.y > tbb)
		return(false);
	tbb = Vmax3(z, pa, pb, pc);
	if(p1.z > tbb && p2.z > tbb)
		return(false);
	
	//begin attempt 2
#define ABS(x) ((x)<0?-(x):(x))
//#define RTOD 57.2957795 	//radians to degrees constant.

	float d;
	float a1,a2,a3;
	float total,denom,mu;
	VERTEX n,pa1,pa2,pa3, intersect;
	
//	FACE *thisface = &mFinalFaces[ _node->pfaces[ i ] ];
	
	VERTEX *p = &intersect;
	if(result != NULL)
		p = result;

	// Calculate the parameters for the plane
	//recalculate from points
#ifndef TRUST_MAPFILE_NORMALS
	n.x = (pb.y - pa.y)*(pc.z - pa.z) - (pb.z - pa.z)*(pc.y - pa.y);
	n.y = (pb.z - pa.z)*(pc.x - pa.x) - (pb.x - pa.x)*(pc.z - pa.z);
	n.z = (pb.x - pa.x)*(pc.y - pa.y) - (pb.y - pa.y)*(pc.x - pa.x);
	Normalize(&n);
	d = - n.x * pa.x - n.y * pa.y - n.z * pa.z;
#else
	//use precaled data from .map file
	n.x = thisface->nx;
	n.y = thisface->ny;
	n.z = thisface->nz;
	d = thisface->nd;
#endif
	
	//try inverting the normals...
	n.x = -n.x;
	n.y = -n.y;
	n.z = -n.z;
	d = - n.x * pa.x - n.y * pa.y - n.z * pa.z;	//recalc
	
	
	// Calculate the position on the line that intersects the plane
	denom = n.x * (p2.x - p1.x) + n.y * (p2.y - p1.y) + n.z * (p2.z - p1.z);
	if (ABS(denom) < EPS)         // Line and plane don't intersect
	  return(false);
	mu = - (d + n.x * p1.x + n.y * p1.y + n.z * p1.z) / denom;
	p->x = p1.x + mu * (p2.x - p1.x);
	p->y = p1.y + mu * (p2.y - p1.y);
	p->z = p1.z + mu * (p2.z - p1.z);
	if (mu < 0 || mu > 1)   // Intersection not along line segment
	  return(false);

	// Determine whether or not the intersection point is bounded by pa,pb,pc
	pa1.x = pa.x - p->x;
	pa1.y = pa.y - p->y;
	pa1.z = pa.z - p->z;
	Normalize(&pa1);
	pa2.x = pb.x - p->x;
	pa2.y = pb.y - p->y;
	pa2.z = pb.z - p->z;
	Normalize(&pa2);
	pa3.x = pc.x - p->x;
	pa3.y = pc.y - p->y;
	pa3.z = pc.z - p->z;
	Normalize(&pa3);
	a1 = pa1.x*pa2.x + pa1.y*pa2.y + pa1.z*pa2.z;
	a2 = pa2.x*pa3.x + pa2.y*pa3.y + pa2.z*pa3.z;
	a3 = pa3.x*pa1.x + pa3.y*pa1.y + pa3.z*pa1.z;
	
//	total = (acos(a1) + acos(a2) + acos(a3));
//	if (ABS(total - 2*M_PI) > EPS)
	total = (acos(a1) + acos(a2) + acos(a3)) * 57.2957795;
	if (ABS(total - 360) > EPS)
	  return(false);

	return(true);
}

void Map::Normalize(VERTEX *p) {
	float len = sqrt(p->x*p->x + p->y*p->y + p->z*p->z);
	p->x /= len;
	p->y /= len;
	p->z /= len;
}

