
//this is the path finding portion of the program.

#include "../common/types.h"
#include "../zone/map.h"
#include "../common/rdtsc.h"
#include "quadtree.h"
#include "apathing.h"
#include "boostcrap.h"
#include <stdio.h>
#include <mysql.h>
#include <stdlib.h>
#include <string.h>


void find_path_info(Map *map, PathGraph *big, vector< vector<PathEdge*> > &path_finding) {
	//make sure our path finding vector is big enough, and all NULL
	{
		int size = big->nodes.size();
		vector<PathEdge*> tmp(size, (PathEdge*)NULL);
		path_finding.resize(0);
		path_finding.resize(size, tmp);
	}
	
	//take our minimal spanning tree and try to link every single
	//point in it like spawns are linked. The idea is to create
	//a large set of alternative paths and cycles.
	//get our list of nodes.
	vector<PathNode *> all_points = big->nodes;
	big->nodes.clear();
	//make our current edge list
	map< pair<PathNode *, PathNode *>, bool > edgelist;
	list<PathEdge*>::iterator cur,end,tmp;
	PathEdge *e;
	cur = big->edges.begin();
	end = big->edges.end();
	PathNode *from;
	PathNode *to;
	for(; cur != end; cur++) {
		e = *cur;
		from = e->from;
		to = e->to;
		//hack to make order on the ID not matter
		if(int32(from) < int32(to)) {
			PathNode *tmp = from;
			from = to;
			to = tmp;
		}
		pair<PathNode *, PathNode *> id(from, to);
		edgelist[id] = true;
	}
	//link up the points, making sure not to add edges allready in the MST
	link_spawns(map, big, db_spawns, MAX_LINK_SPAWN_DIST, &edgelist);
	
	//now we have our graph all connected up.
	//run all pairs shortest path so we have some information to
	//use for our metric in A*
	
	
	//TODO: build our boost graph with length weights
	
	vector< vector<int> > D;
	vector<int> counts;
	vector<int> disjoint_counts;
	vector<int> first_node;
	
	//color the graph and get us the info we need to do our job
	color_disjoint_graphs(big, vg, map, NULL, D, counts, disjoint_counts, first_node);
	
	//figure out what edges link to each node.
	std::map<PathNode*, vector<PathEdge*> > node_edges;
	find_node_edges(big, node_edges);
	
	/*
	we have our node edges, our all pairs shortest path
	our graphs, and our MST edge list.
	now we can run A*
	
	- our metric should use the all pairs shortest path to
	  determine the value for h.
	- We want to favor edges from the MST over newly created edges,
	  so we will give such edges a lower weight, maybe by 20%?
	
	
	
	*/
	
}
















