#ifndef APATHING_H
#define APATHING_H

#define DB_HOST "10.1.1.1"
#define DB_LOGIN "eqserver"
#define DB_PASSWORD "pw4eqserver"
#define DB_NAME "peq"

//for now, all of these were arbitrarily chosen 

//load up doors as spawn points, since they are also valid locations
#define INCLUDE_DOORS

//this is the furthest a mob can be from a fear point and still expect 
//to find the node.
#define FEAR_MAXIMUM_DISTANCE 250

#define ENDPOINT_CONNECT_MAX_DISTANCE 25	//begin and end point must be this close
											//for them to get connected when loading path
#define MIN_FIX_Z 20.0f		//minimum drop before correcting waypoint

#define ALMOST_COLINEAR_COS 0.99f //cosine of an angle to consider colinear... .99== 8 degrees

//if we miss the map on one Z-checking try, move the point by these
#define X_JITTER 3
#define Y_JITTER 3

//if two nodes are this close together, consider them the same
#define CLOSE_ENOUGH 2.5

//this causes us to check all of a node's edges for LOS from the new
//node when we are considering combining into that node
//this is not working as well as one might hope, leads to many invalids
//#define COMBINE_CHECK_ALL_LOS

//this is bigger than close enough, since we check LOS when combining these
//so that we prevent little juntions
#define CLOSE_ENOUGH_COMBINE 30

//a second link on a spawn must be this far away from the first.
#define SPAWN_MIN_SECOND_DIST 100

//uncomment to split a pathin with two waypoints which cannot see eachother into two
//#define SPLIT_INVALID_PATHS

//enabled linking of path endpoints as if they were spawn points.
#define LINK_PATH_ENDPOINTS

//the maximum distance of the closest point to a spawn inorder to link it
#define MAX_LINK_SPAWN_DIST 400

//enables linking a spawn point to two nodes instead of just one.
#define SPAWN_LINK_TWICE
#define SPAWN_LINK_THRICE //link up to 3 times.. requires twice enabled
//a second link point must be further than this from the first for closest merge
#define MERGE_MIN_SECOND_DIST 30

//if an edge is longer than this, it will get cut
#define SPLIT_LINE_LENGTH 300
#define SPLIT_LINE_INTERVAL 200

//If a final path is longer than this, force check on it
//#define LONG_PATH_CHECK_LOS 400

//an edge must cross this many lines before being considered for cross reduction
#define CROSS_REDUCE_COUNT 5
//an edge must be longer than this before we think it can cross anything
#define CROSS_MIN_LENGTH 1
//The intersect points of two edges must be within this range of Z to count
#define CROSS_MAX_Z_DIFF 20
//the max distance between two nodes to consider them the same
#define CLOSE_ENOUGH_CROSS 120

//divide the image scale by this number in each direction
#define IMAGE_SCALE 4

//enable drawing of a color-by-reachability graph when coloring a graph
//#define DRAW_ALL_COLORS 1

//enable drawing of the original non-reduced combined graph.
//#define DRAW_PRETREE_GRAPH 1

#include "gpoint.h"
#include <vector>
#include <list>
using namespace std;

class PathNode : public GPoint {
public:
	PathNode() {
		color = 0;
//		color2 = 0;
		final_id = -1;
		longest_path = 0;
		valid = true;
	}
	PathNode(const GPoint &them) : GPoint(them) {
		color = 0;
//		color2 = 0;
		final_id = -1;
		longest_path = 0;
		valid = true;
	}
	
	int node_id;
	int final_id;
// inherited:
//	float x;
//	float y;
//	float z;
//	VertDesc vert;
	
	int color;
	int longest_path;
	
	bool valid;
	
	float Dist2(const GPoint *o) const {
		float tmp;
		float sum;
		tmp = x - o->x;
		sum = tmp*tmp;
		tmp = y - o->y;
		sum += tmp*tmp;
		tmp = z - o->z;
		sum += tmp*tmp;
		return(sum);
	}
};

class PathEdge {
public:
	PathEdge( PathNode *_from, PathNode *_to) {
		from = _from;
		to = _to;
		normal_reach = -1;
		reverse_reach = -1;
	}
	PathNode *from;
	PathNode *to;
	
	int normal_reach;
	int reverse_reach;
//	EdgeDesc edge_id;
};

class PathGraph {
public:
	~PathGraph() {
		{
			list<PathNode *>::iterator cur,end;
			cur = nodes.begin();
			end = nodes.end();
			for(; cur != end; cur++) {
				delete *cur;
			}
		}
		{
			list<PathEdge *>::iterator cur,end;
			cur = edges.begin();
			end = edges.end();
			for(; cur != end; cur++) {
				delete *cur;
			}
		}
	}
	
	void add_edge(PathNode *b, PathNode *e) {
		edges.push_back(new PathEdge(b, e));
	}
	
	void add_edges(list<PathEdge *> &o) {
		list<PathEdge *>::iterator cur,end;
		cur = o.begin();
		end = o.end();
		for(; cur != end; cur++) {
			edges.push_back(*cur);
		}
	}
	
	list<PathNode *> nodes;
	list<PathEdge *> edges;
	
	//used for graph color accounts
	int curcolor;
	int ccount;
};


class ColorRecord {
public:
	int color;
	float height;
};




extern int load_split_paths;
extern int z_fixed_count;
extern int z_not_fixed_count;
extern int z_no_map_count;
extern float z_fixed_diffs;
extern float z_not_fixed_diffs;
extern int wp_reduce_count;
extern int trivial_merge_count;
extern int closest_merge_count;
extern int closest_merge2_count;
extern int link_spawn_count;
extern int link_spawn_invalid;
extern int link_spawn2_count;
extern int link_spawn3_count;
extern int link_spawn_nocount;
extern int combine_broke_los;
extern int combined_grid_points;
extern int removed_edges_los;
extern int removed_long_edges_los;
extern int broke_paths;
extern int cross_edge_count;
extern int cross_add_count;
extern int los_cache_misses;
extern int los_cache_hits;

#endif

