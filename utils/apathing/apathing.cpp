/*

Fear Pathing generation utility.
(c) 2005 Father Nitwit



Settings table:

CREATE TABLE fear_settings (
	zone VARCHAR(16) NOT NULL PRIMARY KEY,
	
	#general settings:
	use_doors TINYINT NOT NULL DEFAULT 1,
	min_fix_z FLOAT NOT NULL DEFAULT 20,
	max_fear_distance FLOAT NOT NULL DEFAULT 250,
	image_scale TINYINT NOT NULL DEFAULT 4,
	
	#path related
	check_initial_los TINYINT NOT NULL DEFAULT 0,
	split_invalid_paths TINYINT NOT NULL DEFAULT 0,
	link_path_endpoints TINYINT NOT NULL DEFAULT 1,
	end_distance FLOAT NOT NULL DEFAULT 25,
	split_long_min FLOAT NOT NULL DEFAULT 300,
	split_long_step FLOAT NOT NULL DEFAULT 200,
	
	#node combining settings:
	same_dist FLOAT NOT NULL DEFAULT 2.5,
	node_combine_dist FLOAT NOT NULL DEFAULT 30,
	grid_combine_dist FLOAT NOT NULL DEFAULT 30,
	close_all_los TINYINT NOT NULL DEFAULT 0,
	
	#line-crossing reduction settings:
	cross_count INT NOT NULL DEFAULT 5,
	cross_min_length FLOAT NOT NULL DEFAULT 1,
	cross_max_z_diff FLOAT NOT NULL DEFAULT 20,
	cross_combine_dist FLOAT NOT NULL DEFAULT 120,
	
	#linking:
	second_link_dist FLOAT NOT NULL DEFAULT 100,
	link_max_dist FLOAT NOT NULL DEFAULT 400,
	link_count TINYINT NOT NULL DEFAULT 1
	
);

*/




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
#include <gd.h>




//parameters:
bool INCLUDE_DOORS = true;
float FEAR_MAXIMUM_DISTANCE = 250;
float ENDPOINT_CONNECT_MAX_DISTANCE = 25;
float MIN_FIX_Z = 20.0f;
float CLOSE_ENOUGH = 2.5;
bool COMBINE_CHECK_ALL_LOS = false;
float CLOSE_ENOUGH_COMBINE = 30;
float SPAWN_MIN_SECOND_DIST = 100;
bool SPLIT_INVALID_PATHS = false;
bool LINK_PATH_ENDPOINTS = true;
float MAX_LINK_SPAWN_DIST = 400;
bool SPAWN_LINK_TWICE = true;
bool SPAWN_LINK_THRICE = true;
float MERGE_MIN_SECOND_DIST = 30;
float SPLIT_LINE_LENGTH = 300;
float SPLIT_LINE_INTERVAL = 200;
float LONG_PATH_CHECK_LOS = 0;   //0=disable
int CROSS_REDUCE_COUNT = 5;
float CROSS_MIN_LENGTH = 1;
float CROSS_MAX_Z_DIFF = 20;
float CLOSE_ENOUGH_CROSS = 120;
int IMAGE_SCALE = 4;





int main(int argc, char *argv[]) {
	
	srand(2038833498);
	
/*	const char *zone = 
		//"qeynos";
		"qeynos2";
		//"northkarana";
		*/
	if(argc != 2) {
		printf("Usage: %s [zone_short_name]\n", argv[0]);
		return(1);
	}
	const char *zone = argv[1];
	char buf[256];
	
	MYSQL m;
	
	list<PathGraph *> db_paths;
	list<PathNode *> db_spawns;

	mysql_init(&m);
	
	if(!mysql_real_connect(&m, DB_HOST, DB_LOGIN, DB_PASSWORD, DB_NAME, 0, NULL, 0)) {
		printf("Unable to connect: %s.\n", mysql_error(&m));
		return(1);
	}
	
	/*
		Data loading phase
	*/
	
	//load up our map file
	Map *map = Map::LoadMapfile(zone);
	if(map == NULL) {
		printf("Unable to load map file.");
		return(1);
	}
	
	//try to load the EQ map file to make our pictures prettier
	PathGraph eqmap;
	if(load_eq_map(zone, &eqmap)) {
		printf("Loaded EQ Client map: %d edges.\n", eqmap.edges.size());
	} else {
		printf("Unable to load EQ Client map, continuing without it.\n");
	}
	
	//load our crap from the DB...
	if(!load_paths_from_db(&m, map, zone, db_paths, db_spawns))
		return(1);
	
	if(db_paths.size() == 0)
		db_paths.push_back(new PathGraph());
	
	if(!load_spawns_from_db(&m, zone, db_spawns))
		return(1);

	if(INCLUDE_DOORS) {
		if(!load_doors_from_db(&m, zone, db_spawns))
			return(1);
	}
	
	if(!load_hints_from_db(&m, zone, db_spawns))
		return(1);
	
	//try to load settings, dont care if it fails
	if(load_settings_from_db(&m, zone))
		printf("Loaded zone settings from the database.\n");
	else
		printf("Unable to load settings from database. Using defaults.\n");
	
	
	printf("Load: got %d paths and %d spawn points from the database.\n", db_paths.size(), db_spawns.size());
	printf("Load: had to split up %d invalid paths.\n", load_split_paths);
	
	/*
		The make-the-db-suck-less phase
	*/
	
	//try to lower waypoints way in the sky:
	repair_high_waypoints(map, db_paths, db_spawns);
	printf("Fix Z: %d missed map, %d were not broken, %d were fixed.\n", z_no_map_count, z_not_fixed_count, z_fixed_count);
	printf("Fix Z: broken avg diff=%.3f, not broken avg diff=%.3f\n", z_fixed_diffs/z_fixed_count, z_not_fixed_diffs/z_not_fixed_count);
	
	//run a waypoint reduction algorithm in 3space:
	reduce_waypoints(db_paths);
	printf("WP Reduce: removed %d redundant waypoints.\n", wp_reduce_count);
	
	/*
		Graph connection and merging phase
	*/
	//make trivial connections of nodes at about the same spot on diff grids
	combine_trivial_grids(map, db_paths);
	printf("Trivial Merge: %d grids merged.\n", trivial_merge_count);
	
	//now do the 'closest with LOS' connection method
	combine_closest_grids(map, db_paths);
	printf("Closest Merge: %d grids linked, %d grids double-linked.\n", closest_merge_count, closest_merge2_count);
	PathGraph *big = db_paths.front();
	
	//now add in the spawn points, and link to closest with LOS
	link_spawns(map, big, db_spawns, MAX_LINK_SPAWN_DIST, NULL);
	printf("Link Spawns: %d linked once, %d linked twice, %d not linked, %d invalid.\n", link_spawn_count-link_spawn2_count, link_spawn2_count, link_spawn_nocount, link_spawn_invalid);
	
	//combining close points might be causing small LOS obstacles... 
	//so we want to run this before we combine them.
	//this seems to do more harm than good right now
//	check_edge_los(map, big);
//	printf("Bad Edges: removed %d no-LOS edges.\n", removed_edges_los);
	
#ifdef LONG_PATH_CHECK_LOS
	//check long paths LOS, we seem to have a problem with random long links
	//disable this if we check all edges above...
	check_long_edge_los(map, big);
	printf("Bad Edges: removed %d long no-LOS edges.\n", removed_long_edges_los);
#endif
	
	//clean up points close enough to eachother to be the same point.
	combine_grid_points(map, big, CLOSE_ENOUGH_COMBINE);
	printf("Point Combine: combined %d very close nodes (%d missed strict LOS).\n", combined_grid_points, combine_broke_los);
	printf("Point Combine: so far, %d LOS cache hits, %d LOS cache misses.\n", los_cache_hits, los_cache_misses);
	
	list<PathEdge *> all_edges = big->edges;
	printf("Big Graph: %d original nodes, %d original edges.\n", big->nodes.size(), big->edges.size());
	
#ifdef DRAW_PRETREE_GRAPH
	//draw out our graph before trimming
	sprintf(buf, "paths-%s-pretree.png", zone);
	draw_paths(map, big->edges, eqmap.edges, buf);
#endif
	
	
	/*
		Graph algorithm application (boost)
		run it on each disjoint graph
	*/
	
	vector<PathGraph *> disjoints;
	vector<int> start_nodes;
	
	//find all the disjoint graphs
	{
		//build the boost graph
		MyGraph boost_graph(big->nodes.size());
		property_map<MyGraph, edge_weight_t>::type weightlist;
		std::map<PathEdge *, EdgeDesc> edgemap;
		build_boost_graph(boost_graph, weightlist, edgemap, big, false);
		
		//find the grid which has most of the edges
		sprintf(buf, "paths-%s-colors.png", zone);
		find_disjoint_grids(map, boost_graph, big, buf, start_nodes, disjoints);
	}
	printf("\nSplit: There are %d valid disjoint graphs.\n", disjoints.size());
	
	//for each disjoint graph....
	int djnum = 0;
	PathGraph *real_big = big, *tmpg; int start_node;
	vector<PathGraph *>::iterator cur,end;
	vector<int>::iterator curs,ends;
	cur = disjoints.begin(); curs = start_nodes.begin();
	end = disjoints.end(); ends = start_nodes.end();
	for(; cur != end; cur++,curs++) {
		big = *cur;
		start_node = *curs;
		
		printf("Disjoint %d: has %d edges and %d nodes.\n", djnum, big->edges.size(), big->nodes.size());
		
		//reset our stats...
		combine_broke_los = 0;
		combined_grid_points = 0;
		removed_edges_los = 0;
		removed_long_edges_los = 0;
		broke_paths = 0;
		cross_edge_count = 0;
		cross_add_count = 0;
		
		{
			//build the boost graph
			MyGraph boost_graph(big->nodes.size());
			property_map<MyGraph, edge_weight_t>::type weightlist;
			std::map<PathEdge *, EdgeDesc> edgemap;
			build_boost_graph(boost_graph, weightlist, edgemap, big);
			
			//calculate the MST
			run_min_spanning_tree(boost_graph, weightlist, edgemap, big, start_node);
			printf("Ran Min Spanning Tree: ended with %d edges\n", big->edges.size());
		}

		/*
			Now we have our minimal spanning tree, try to refine it.
			
			the goal of this crap is to fix newbie fields and open zones
		*/
		std::map<PathEdge*, vector<GPoint> > cross_list;
		PathGraph *cross_big = new PathGraph();
		PathGraph *cross_excess = new PathGraph();
		
		//count the number of times each edge crosses another edge, and record
		//the intersection points. Also seperate crossers from non-crossers
		count_crossing_lines(big->edges, cross_big, cross_excess, cross_list);
		printf("Cross Count: %d edges cross more than the specified number of other edges.\n", cross_edge_count);
		
		if(cross_edge_count > 2) {
			//Make waypoints at all points of intersection
			cut_crossed_grids(cross_big, cross_list);
			printf("Cross Cut: Created %d new nodes cutting intersections\n", cross_add_count);
			
			//combine close points with a somewhat big radius...
			combine_grid_points(map, cross_big, CLOSE_ENOUGH_CROSS);
			printf("Cross Combine: combined %d nodes.  (%d missed strict LOS)\n", combined_grid_points, combine_broke_los);
			printf("Cross Combine: so far, %d LOS cache hits, %d LOS cache misses.\n", los_cache_hits, los_cache_misses);
			
			//build our boost graph, so we can do reachability
			MyGraph cross_graph(cross_big->nodes.size());
			property_map<MyGraph, edge_weight_t>::type cross_weightlist;
			std::map<PathEdge *, EdgeDesc> cross_edgemap;
			build_boost_graph(cross_graph, cross_weightlist, cross_edgemap, cross_big, false);
			
			//isolate each disjoint graph and try to reduce it, gathering 
			//all non-cross points and edges while we are at it.
			sprintf(buf, "paths-%s-crosses.png", zone);
			consolidate_cross_graphs(map, cross_big, cross_excess, cross_graph, buf);
			
			//rebuild the big graph by merging cross_big and cross_excess
			//might be as simple as append the two arrays and run a combine on it.
			//leaks 'big'
			big = cross_excess;
			cross_excess->add_edges(cross_big->edges);
			rebuild_node_list(big->edges, big->nodes);
			
			//This is used to re-link the cross grids with the non-cross stuff
			combine_grid_points(map, big, CLOSE_ENOUGH_COMBINE);
			printf("Cross Merge: combined %d close nodes. (%d missed strict LOS)\n", combined_grid_points, combine_broke_los);
			printf("Cross Merge: so far, %d LOS cache hits, %d LOS cache misses.\n", los_cache_hits, los_cache_misses);
			
			//build yet another boost graph so we can run MST
			MyGraph cross_graph_final(big->nodes.size());
			property_map<MyGraph, edge_weight_t>::type cross_weightlist_final;
			std::map<PathEdge *, EdgeDesc> cross_edgemap_final;
			build_boost_graph(cross_graph_final, cross_weightlist_final, cross_edgemap_final, big);
			
			//run our MST to reduce the new cross-reduced graph
			run_min_spanning_tree(cross_graph_final, cross_weightlist_final, cross_edgemap_final, big, 0);
			printf("Ran Min Spanning Tree 2: ended with %d edges\n", big->edges.size());
		}	//end if there were some cross edges
		
		/*
			Final refinement phase, the graph has been reduced to our final
			form, this phase is for adding anything back in we might want
		*/

		//now that we reduced all our co-linear points, add a bunch back in for
		//long paths, so we have more points over space. Idea is that this way
		//we control how many colinear points there are, it isnt random
		//this counteracts any attempts to use line-crossing as a criteria for reduction
		//for some stupid reason, this just destroys the graph
	//	break_long_lines(db_paths);
	//	printf("WP Increase: created %d waypoints on long paths.\n", broke_paths);
		

		/*
		Things we might want to do:
	 - Try to create some cycles. Specifically large cycles.
	   - do this after reachability calculations
	   - use allready calculated reacahbility and distances, just adjust them as cycles added
	   - these will add more realism to the pathing
	   - might be implemented like this:
		 - run all pairs shortest path on the MST (is this a byproduct?)
		 - for each node N, for each other node K
		   - if path(N, K) is at least MIN_CYCLE_JOIN_PATH (to prevent making small cycles)
		   - and dist(N,K) is less than MAX_CYCLE_JOIN_DIST (do not want to invent long paths)
		   - and there is LOS from N to K
			 - connect N and K
			 - have to re-run all pairs again... that sucks
	   - another twist on the implementation would be to only look at paths
		 which were discarded by the MST. Or maybe run this first, then the other.
		 */
		 
		/*
			The final tree has been built, remove anything we dont need from
			it, and gather some information.
		*/
		
		//build a boost graph out of our minimal spanning tree.
		MyGraph boost_mst(big->nodes.size());
		property_map<MyGraph, edge_weight_t>::type weightlist_mst;
		std::map<PathEdge *, EdgeDesc> edgemap_mst;
		build_boost_graph(boost_mst, weightlist_mst, edgemap_mst, big, false);
		
		//determine the path lengths to all nodes from all others
		//including the longest path reachable by each node.
		//this also cleans up big by removing anything unreachable
	//	vector< vector<int> > AllPairs;
		sprintf(buf, "paths-%s-mstcolors%d.png", zone, djnum++);
	//	calc_path_lengths(map, boost_mst, big, AllPairs, buf);
		calc_path_lengths(map, boost_mst, big, edgemap_mst, buf);
		printf("Calculated the longest paths from each node.\n");
		printf("\n");
	}
	
	printf("Combining all disjoint graphs...\n");
	//now combine all our disjoint graphs into one big one...
	big = real_big;
	cur = disjoints.begin();
	end = disjoints.end();
	big->nodes.clear();
	big->edges.clear();
	for(; cur != end; cur++) {
		tmpg = *cur;
		list<PathEdge *>::iterator cure,ende;
		cure = tmpg->edges.begin();
		ende = tmpg->edges.end();
		for(; cure != ende; cure++) {
			big->edges.push_back(*cure);
		}
	}
	rebuild_node_list(big->edges, big->nodes, NULL);
	
	//now we have our final node and edge set.
	//build a graph of all final nodes to find pathing info
//	MyGraph final(big->nodes.size());
//	property_map<MyGraph, edge_weight_t>::type weightlist_final;
//	std::map<PathEdge *, EdgeDesc> edgemap_final;
//	build_boost_graph(final, weightlist_final, edgemap_final, big, true);
	
	vector< vector<PathEdge*> > path_finding;
	find_path_info(map, big, path_finding);
	printf("Calculated pathing information...\n");
	
	//write out a nice image of our MST
	sprintf(buf, "paths-%s-mstree.png", zone);
	draw_paths2(map, all_edges, big->edges, eqmap.edges, db_spawns, buf);
	
	//write out a map for inside the EQ client, not as pretty as the PNG
	sprintf(buf, "eqmaps_out/%s_2.txt", zone);
	write_eq_map(big->edges, buf);
	
	/*
		Build structures, and write out the path file.
	*/
	RDTSC_Timer t1, t2;
	QTNode *root;
	root = build_quadtree(map, big);
	if(root == NULL) {
		printf("Failed to build quadtree, quitting.\n");
		return(1);
	}
	
	t2.start();
	sprintf(buf, "%s.path", zone);
	if(!write_path_file(root, big, buf, path_finding)) {
		printf("Unable to write path file.\n");
		return(1);
	}
	
	printf("Everything completed successfully. %s.path has been generated.\n", zone);
	
/*

** now we have a minimal connected graph such that any mob can get anywhere
   by only a single path.



** now we have our final pathing grid. determine some useful info for each node.

look for dead ends/node preference:
 - at each node, sum up the length of all edges reachable by using that link
   - for each node as N
   -- for each edge leaving N as E
   ---- reset marks on all edges
   ---- mark all edges leaving N
   ---- perform a depth first search of all reachable nodes
   ------ Only traverse unmarked edges. Mark each edge as it is traversed.
   ---- unmark all edges leaving N, except E
   ---- sum the length of all marked edges
   ---- store this reachable length as a weight for this edge of this node.
   -- for each edge leaving N as E
   ---- determine highest edge weight in this node
   -- for each edge leaving N as E
   ---- count number of edges within RANDOM_WALK % of the highest node.
   -- sort the edge list for this node to have all random nodes first
   -- store the random walkable counter for this node
 


** Finally we have all the calculation

things we need to store:
nodes
edge lists
quadtree containing nodes

 - We do not really need to store edges which are not on the random walk
   list since the pathing code will never consider using them.

Node {
 x, y, z
 edge list pointer
 edge list length
 random walkable counter
}

edge list entry {
 ending node pointer
 reachable edge weight? (not used by pathing, might have other purpose)
}

quadtree node {
 minx, maxx, miny, maxy
 union {
  node pointers: q1, q2, q3, q4
  node list offset and length
 }
}




how to do fear pathing...
when a mob is feared, it uses the quadtree to find the node closest to it
that it can see to. The LOS requirement will make this more difficult, but feasible.

Once it has found its first node, the mob will set this as its waypoint and go there.

Once the mob reaches a node, it will look at the node's random counter (RC)
 if RC is 1, take the first edge and use its terminal as next waypoint
 if RC is >1, randomly choose an edge from 0 to RC-1, and walk that edge.
*/
	
	
	
	
	mysql_close(&m);
}

bool load_paths_from_db(MYSQL *m, Map *map, const char *zone, list<PathGraph*> &db_paths, list<PathNode*> &end_points) {
	char query[512];
	
	sprintf(query, 
		"SELECT x,y,z,gridid FROM grid_entries,zone "
		"WHERE zone.zoneidnumber=zoneid AND short_name='%s' "
		"ORDER BY gridid,number", zone);
	if(mysql_query(m, query) != 0) {
		printf("Unable to query: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_RES *res = mysql_store_result(m);
	if(res == NULL) {
		printf("Unable to store res: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_ROW row;
	
	PathNode *cur = NULL, *last = NULL, *first = NULL;
	PathGraph *g = NULL;
	int cur_g = -1,last_g = -1;
	
//	int lid = 0;
	
	while((row = mysql_fetch_row(res))) {
		last = cur;
		cur = new PathNode;
//		cur->load_id = lid++;
		cur->x = atof(row[0]);
		cur->y = atof(row[1]);
		cur->z = atof(row[2]);
		cur_g = atoi(row[3]);
		if(cur_g != last_g) {
			if(g != NULL) {
				//if we have a first and last node for this path
				//and they are not the same node, try to connect them.
				if(first != NULL && last != NULL && first != last && last->Dist2(first) < ENDPOINT_CONNECT_MAX_DISTANCE*ENDPOINT_CONNECT_MAX_DISTANCE) {
					if(CheckLOS(map, last, first))
						g->add_edge(last, first);
				}
#ifdef LINK_PATH_ENDPOINTS
				if(first != last && last != NULL)
					end_points.push_back(last);
#endif
				db_paths.push_back(g);
			}
			g = new PathGraph();
			first = cur;
			last_g = cur_g;
			last = NULL;
		}
		
		g->nodes.push_back(cur);
		
#ifdef LINK_PATH_ENDPOINTS
		//this is a begining point
		if(last == NULL)
			end_points.push_back(cur);
#endif
		
		if(last != NULL) {
#ifdef SPLIT_INVALID_PATHS
			if(CheckLOS(map, last, cur)) {
				g->edges.push_back(new PathEdge(last, cur));
			} else {
				//no LOS, split the path into two
				load_split_paths++;
				last_g = -1;	//tell this thing to start over
			}
#else
			g->edges.push_back(new PathEdge(last, cur));
#endif
		}
	}
	
	//handle the last active path
	if(g != NULL) {
		if(first != NULL && cur->Dist2(first) < ENDPOINT_CONNECT_MAX_DISTANCE*ENDPOINT_CONNECT_MAX_DISTANCE) {
			if(CheckLOS(map, cur, first))
				g->add_edge(cur, first);
		}
		db_paths.push_back(g);
	}
	
	mysql_free_result(res);
	return(true);
}


bool load_spawns_from_db(MYSQL *m, const char *zone, list<PathNode*> &db_spawns) {
	char query[512];
	
	sprintf(query, 
		"SELECT x,y,z FROM spawn2 "
		"WHERE  zone='%s'", zone);
	if(mysql_query(m, query) != 0) {
		printf("Unable to query: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_RES *res = mysql_store_result(m);
	if(res == NULL) {
		printf("Unable to store res: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_ROW row;
	
	PathNode *cur = NULL;
	
	while((row = mysql_fetch_row(res))) {
		cur = new PathNode;
		cur->x = atof(row[0]);
		cur->y = atof(row[1]);
		cur->z = atof(row[2]);
		db_spawns.push_back(cur);
	}
	
	mysql_free_result(res);
	
	return(true);
}


bool load_doors_from_db(MYSQL *m, const char *zone, list<PathNode*> &db_spawns) {
	char query[512];
	
	sprintf(query, 
		"SELECT pos_x,pos_y,pos_z FROM doors "
		"WHERE  zone='%s'", zone);
	if(mysql_query(m, query) != 0) {
		printf("Unable to query: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_RES *res = mysql_store_result(m);
	if(res == NULL) {
		printf("Unable to store res: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_ROW row;
	
	PathNode *cur = NULL;
	
	while((row = mysql_fetch_row(res))) {
		cur = new PathNode;
		cur->x = atof(row[0]);
		cur->y = atof(row[1]);
		cur->z = atof(row[2]);
		//TODO: it would be nice if we could get to the middle of these
		//doors, not the edge of them which I assume these points are
		db_spawns.push_back(cur);
	}
	
	mysql_free_result(res);
	
	return(true);
}


bool load_hints_from_db(MYSQL *m, const char *zone, list<PathNode*> &db_spawns) {
	char query[512];
	
	sprintf(query, 
		"SELECT x,y,z,forced,disjoint FROM fear_hints "
		"WHERE  zone='%s'", zone);
	if(mysql_query(m, query) != 0) {
		printf("Unable to query: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_RES *res = mysql_store_result(m);
	if(res == NULL) {
		printf("Unable to store res: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_ROW row;
	
	PathNode *cur = NULL;
	
	while((row = mysql_fetch_row(res))) {
		cur = new PathNode;
		cur->x = atof(row[0]);
		cur->y = atof(row[1]);
		cur->z = atof(row[2]);
		cur->forced = atoi(row[3])?true:false;
		cur->disjoint = atoi(row[4])?true:false;
		db_spawns.push_back(cur);
	}
	
	mysql_free_result(res);
	
	return(true);
}


bool load_settings_from_db(MYSQL *m, const char *zone) {
	char query[512];
	
	sprintf(query, 
		"SELECT use_doors, min_fix_z, max_fear_distance, image_scale, split_invalid_paths,"
		" link_path_endpoints, end_distance, split_long_min, split_long_step, same_dist, node_combine_dist,"
		" grid_combine_dist, close_all_los, cross_count, cross_min_length, cross_max_z_diff, cross_combine_dist,"
		" second_link_dist, link_max_dist, link_count"
		" FROM fear_settings"
		" WHERE zone='%s'", zone);
	if(mysql_query(m, query) != 0) {
//		printf("Unable to query: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_RES *res = mysql_store_result(m);
	if(res == NULL) {
//		printf("Unable to store res: %s\n", mysql_error(m));
		return(false);
	}
	
	MYSQL_ROW row;
		
	int r = 0;
	if((row = mysql_fetch_row(res))) {
		INCLUDE_DOORS = atoi(row[r++])?true:false;
		MIN_FIX_Z = atof(row[r++]);
		FEAR_MAXIMUM_DISTANCE = atof(row[r++]);
		IMAGE_SCALE = atoi(row[r++]);
		SPLIT_INVALID_PATHS = atoi(row[r++])?true:false;
		LINK_PATH_ENDPOINTS = atoi(row[r++])?true:false;
		ENDPOINT_CONNECT_MAX_DISTANCE = atof(row[r++]);
		SPLIT_LINE_LENGTH = atof(row[r++]);
		SPLIT_LINE_INTERVAL = atof(row[r++]);
		CLOSE_ENOUGH = atof(row[r++]);
		CLOSE_ENOUGH_COMBINE = atof(row[r++]);
		MERGE_MIN_SECOND_DIST = atof(row[r++]);
		COMBINE_CHECK_ALL_LOS = atoi(row[r++])?true:false;
		CROSS_REDUCE_COUNT = atoi(row[r++]);
		CROSS_MIN_LENGTH = atof(row[r++]);
		CROSS_MAX_Z_DIFF = atof(row[r++]);
		CLOSE_ENOUGH_CROSS = atof(row[r++]);
		
		SPAWN_MIN_SECOND_DIST = atof(row[r++]);
		MAX_LINK_SPAWN_DIST = atof(row[r++]);
		int sc = atoi(row[r++]);
		SPAWN_LINK_TWICE = sc >= 2?true:false;
		SPAWN_LINK_THRICE = sc >= 3?true:false;
		mysql_free_result(res);
		return(true);
	}
	
	mysql_free_result(res);
	
	return(false);
}

/*
	We assume this overwrites the edge array in big
	and does not free the old edges
*/
void build_boost_graph(MyGraph &vg, property_map<MyGraph, edge_weight_t>::type &weightmap, map<PathEdge *, EdgeDesc> &em, PathGraph *big, bool set_weights) {
	
	list<PathEdge*>::iterator cur,end;
	list<PathNode*>::iterator curn,endn;
	PathNode *n;
	PathEdge *e;
	
	//first we need to number our nodes...
	curn = big->nodes.begin();
	endn = big->nodes.end();
	int r = 0;
	for(; curn != endn; curn++) {
		n = *curn;
		n->node_id = r;
		r++;
	}
	
	typedef std::pair < int, int > E;	//our edge type
	
	weightmap = get(edge_weight, vg); 
	
	//now add all our edges to the graph
	cur = big->edges.begin();
	end = big->edges.end();
	for(r = 0; cur != end; cur++, r++) {
		e = *cur;
		if(e->from == e->to)
			continue;
		
//		printf("A %d/%d (%.3f,%.3f,%.3f) -> (%.3f,%.3f,%.3f) d=%.3f\n", r, big->edges.size(), e->from->x, e->from->y, e->from->z, e->to->x, e->to->y, e->to->z, e->from->Dist2(e->to));
		EdgeDesc ed;
		bool inserted;
		tie(ed, inserted) = add_edge(e->from->node_id, e->to->node_id, vg);
		if(set_weights)
			weightmap[ed] = int(e->from->Dist2(e->to));
		else
			weightmap[ed] = 1;
		em[e] = ed;
//		e->edge_id = ed;
	}
	//now we should have a nice happy undirected graph...
}
	
void run_min_spanning_tree(MyGraph &vg, property_map<MyGraph, edge_weight_t>::type &weightmap, map<PathEdge *, EdgeDesc> &em, PathGraph *big, int start_node) {
	int noedge = 0;
	list<PathEdge *> out_paths;
	
	vector < EdgeDesc > spanning_tree;
	
	kruskal_minimum_spanning_tree(vg, back_inserter(spanning_tree));
	
	vector < EdgeDesc >::iterator cur,end;
	list<PathEdge*>::iterator cure,ende;
	cur = spanning_tree.begin();
	end = spanning_tree.end();
	for(; cur != end; cur++) {
		//find the edge
		cure = big->edges.begin();
		ende = big->edges.end();
		for(; cure != ende; cure++) {
			if(em[*cure] == *cur) {
				out_paths.push_back(new PathEdge((*cure)->from, (*cure)->to));
				break;
			}
		}
	}
	noedge = big->edges.size() - out_paths.size() - 1;
	printf("Ran Min Spanning Tree: %d paths were eliminated.\n", noedge);
	
	
	/*
	//make our results list.
	vector < VertDesc > p(num_vertices(vg));
	
	//finally run the stupid algorithm.
	prim_minimum_spanning_tree(vg, &p[0], root_vertex(start_node));
	
	
	for (std::size_t i = 0; i != p.size(); ++i) {
		if (p[i] != i) {
			out_paths.push_back(new PathEdge(big->nodes[p[i]], big->nodes[i]));
		} else {
			//no edge here...
			noedge++;
		}
	}
	printf("Ran Min Spanning Tree: %d nodes were disconnected.\n", noedge);
	
	
	*/
	
	//now swap out our edge list to the MST.
	big->edges = out_paths;
}

void find_disjoint_grids(Map *map, MyGraph &vg, PathGraph *big, const char *fname, vector<int> &start_nodes, vector<PathGraph *> &disjoints) {
	
	vector< vector<int> > D;
	vector<int> counts;
	vector<int> disjoint_counts;
	vector<int> first_node;
	
	//color the graph and get us the info we need to do out job
	color_disjoint_graphs(big, vg, map, fname, D, counts, disjoint_counts, first_node);
	
	
	//find the biggest grid, that one gets to be included no matter what
	int best_graph = 0;
	int best_count = counts[0];
	unsigned int r;
	for(r = 1; r < counts.size(); r++) {
		if(best_count < counts[r]) {
			best_count = counts[r];
			best_graph = r;
		}
	}
	disjoint_counts[best_graph] = 1;
	
	
	//break up the graphs based on color if we want them.
	vector<int>::iterator ccur,djcur,fcur,cend;
	list<PathEdge*>::iterator cur,end;
	PathEdge *e;
	PathGraph *pg;
	ccur = counts.begin();
	djcur = disjoint_counts.begin();
	fcur = first_node.begin();
	cend = counts.end();
	int color = 0;
	for(; ccur != cend; ccur++, djcur++, fcur++, color++) {
		int count = *ccur;
		int dj = *djcur;
//		int fn = *fcur;
		
		if(dj < 1)
			continue;   //skip disjoint sets not marked for use.
		
		if(count < MIN_DISJOINT_NODES)
			continue;   //make sure we have a reasonable node count
		
		pg = new PathGraph();
		
		cur = big->edges.begin();
		end = big->edges.end();
		for(; cur != end; cur++) {
			e = *cur;
			if(e->from->color != e->to->color) {
				printf("Color Mismatch %d-%d: #%d(%.3f,%.3f,%.3f) -> #%d(%.3f,%.3f,%.3f)\n", e->from->color, e->to->color, e->from->node_id, e->from->x, e->from->y, e->from->z, e->to->node_id, e->to->x, e->to->y, e->to->z);
				//... what to do...
			}
			//just use the color of the from node
			if(e->from->color == color) {
				pg->edges.push_back(e);
			}
		}
		
		//get our list of nodes based on our edge list.
		rebuild_node_list(pg->edges, pg->nodes, NULL);
		
		disjoints.push_back(pg);
		start_nodes.push_back(1);   //each graph only contains its own nodes, so any node will work.
	}
	
	
	
	/*
	int best_graph = 0;
	int best_count = counts[0];
	unsigned int r;
	for(r = 1; r < counts.size(); r++) {
		if(best_count < counts[r]) {
			best_count = counts[r];
			best_graph = r;
		}
//		printf("Graph %d has %d edges\n", r, counts[r]);
	}
	
	start_node = first_node[best_graph];
	printf("Best sub-graph: chose #%d of %d, has %d nodes, and contains node %d\n", best_graph, counts.size()-1, best_count, start_node);
	//todo: eliminate other graphs...
	*/
	
/*	list<PathEdge*>::iterator cure,ende;
	PathEdge *e;
	cure = big->edges.begin();
	ende = big->edges.end();
	for(; cure != ende; cure++) {
		e = *cure;
		if(e->from->color != e->to->color) {
			printf("Color Mismatch %d-%d: #%d(%.3f,%.3f,%.3f) -> #%d(%.3f,%.3f,%.3f)\n", e->from->color, e->to->color, e->from->node_id, e->from->x, e->from->y, e->from->z, e->to->node_id, e->to->x, e->to->y, e->to->z);
		} else if(e->from->color != best_graph) {
			printf("Color %d: (%.3f,%.3f,%.3f) -> (%.3f,%.3f,%.3f)\n", e->from->color, e->from->x, e->from->y, e->from->z, e->to->x, e->to->y, e->to->z);
		}
	}
*/
}



static const int INT_LIMIT = (std::numeric_limits < int >::max)();

void color_disjoint_graphs(
	PathGraph *big,
	MyGraph &vg, 
	Map *map,
	const char *fname,

	vector< vector<int> > &D,		//output
	vector<int> &counts,			//output
	vector<int> &disjoint_counts,   //output
	vector<int> &first_node			//output
) {
	
	
	int count = big->nodes.size();
//	int r;
	
//	vector< vector<int> > D(count, vector<int>(count, INT_LIMIT));
//	vector<int> counts(1, 0);
//	vector<int> first_node(1, 0);
	
	//make sure everything is inited right...
	D.resize(0);
	D.resize(count, vector<int>(count, INT_LIMIT));
	counts.resize(1);
	counts[0] = 0;
	disjoint_counts.resize(1);
	disjoint_counts[0] = 0;
	first_node.resize(1);
	first_node[0] = 0;
	
	//make up a weight map with all 1s, done while building now
/*	property_map < MyGraph, edge_weight_t >::type w = get(edge_weight, vg);
	graph_traits < MyGraph >::edge_iterator e, e_end;
	for (boost::tie(e, e_end) = edges(vg); e != e_end; ++e)
		w[*e] = 1;*/
	
	
	johnson_all_pairs_shortest_paths(vg, D);
	
	list<PathNode*>::iterator cur,end,cur2;
	PathNode *n,*f;
	
	int cur_color = 1;
	int cc,djc;
	
	//clear node colors
	cur = big->nodes.begin();
	end = big->nodes.end();
	for(; cur != end; cur++) {
		n = *cur;
		n->color = 0;
	}
	
	
	//color the graph based on reachability, basically labeling subgraphs
	//this finds the number of nodes reachable from each node
	//which is used to pick the best disconnected graph in the tree.
	//its a series of wrong bullshit that forces us to do this, but it works
	cur = big->nodes.begin();
	end = big->nodes.end();
	for(; cur != end; cur++) {
		n = *cur;
		if(n->color != 0)
			continue;	//allready visited

//printf("New Color at: (%.3f,%.3f,%.3f)\n", n->x, n->y, n->z);

		cc = 1;
		djc = 0;
		
		if(n->disjoint)
			djc++;
		
		n->color = cur_color;
		cur_color++;
		
		cur2 = cur;
		for(; cur2 != end; cur2++) {
			f = *cur2;
			if(f->color == 0 && D[n->node_id][f->node_id] != INT_LIMIT) {
				cc++;
				f->color = n->color;
			}
			if(f->disjoint)
				djc++;
		}
		counts.push_back(cc);
		disjoint_counts.push_back(djc);
		first_node.push_back(n->node_id);
	}
	
#ifdef DRAW_ALL_COLORS	
	printf("Drawing with %d seperate sub-graphs from %d nodes and %d edges\n", cur_color-1, big->nodes.size(), big->edges.size());

	if(fname != NULL) {
		FILE *pngout;
		pngout = fopen(fname, "wb");
		if(pngout == NULL) {
			printf("Unable to open %s\n", fname);
			return;
		}
		
		gdImagePtr im;
		int minx = int(map->GetMinX());
		int maxx = int(map->GetMaxX());
		int miny = int(map->GetMinY());
		int maxy = int(map->GetMaxY());
		
		im = gdImageCreate((maxx - minx)/IMAGE_SCALE, (maxy - miny)/IMAGE_SCALE);
	//	im = gdImageCreate(maxx - minx, maxy - miny);
		
		//allocate this first, to make it the BG color.
		/*int black =*/ gdImageColorAllocate(im, 0, 0, 0);
		
	//	int grey = gdImageColorAllocate(im, 100, 100, 100);
		
		int *clist = new int[cur_color];
		int r;
		for(r = 0; r < cur_color; r++) {
			clist[r] = gdImageColorAllocate(im, rand()%255, rand()%255, rand()%255);
		}
		
		{
			list<PathEdge*>::iterator cur,end;
			PathEdge *e;
			
			cur = big->edges.begin();
			end = big->edges.end();
			for(; cur != end; cur++) {
				e = *cur;
				int x1 = int(e->from->x) - minx;
				int y1 = int(e->from->y) - miny;
				int x2 = int(e->to->x) - minx;
				int y2 = int(e->to->y) - miny;
				x1 /= IMAGE_SCALE;
				y1 /= IMAGE_SCALE;
				x2 /= IMAGE_SCALE;
				y2 /= IMAGE_SCALE;
				gdImageLine(im, x1, y1, x2, y2, clist[e->from->color]);
			}
		}
		delete[] clist;
		
		gdImagePng(im, pngout);
		gdImageDestroy(im);
		
		fclose(pngout);
		
		printf("Wrote image: %s\n", fname);
	}
#endif 	//DRAW_ALL_COLORS
}

void calc_path_lengths(Map *map, MyGraph &vg, PathGraph *big, map<PathEdge *, EdgeDesc> &em, const char *fname) {

	vector<int> counts;
	vector<int> disjoint_counts;
	vector<int> first_node;
	vector< vector<int> > D;
	list<PathNode*>::iterator cur,end;
	PathNode *n;
	
	/*
		Node distances:
		1. find the root node.
			 - find the longest path from each node to any other node
			 - the node with the shortest of these longest paths is the root
		2. record each node's distance from that root node
	*/
	
	//color the graph and get us the info we need to do out job
	color_disjoint_graphs(big, vg, map, fname, D, counts, disjoint_counts, first_node);
	
	
	vector<int>::iterator curp,endp;
	
	int shortest_node = 0;
	int shortest = 0xFFFFFF;
	int the_longest = 0;
	int longest_node = 0;
	
	//stores the longest path from each node
	vector<int> longest_dists(D.size(), 0);
	
	//find the node with the longest path of all, so we know what
	//tree we are trying to find the root of
	cur = big->nodes.begin();
	end = big->nodes.end();
	for(; cur != end; cur++) {
		n = *cur;
		vector<int> &cv = D[n->node_id];
		curp = cv.begin();
		endp = cv.end();
		int longest = 0;
		int discount = 0;
		for(; curp != endp; curp++) {
			if(*curp == INT_LIMIT) {
				discount++;
				continue;
			}
			if(*curp > longest)
				longest = *curp;
		}
		
		longest_dists[n->node_id] = longest;
		
		if(longest > the_longest) {
			the_longest = longest;
			longest_node = n->node_id;
		}
	}
	
	//find the node with the shortest, longest path
	//the idea is to locate the 'root' of the tree.
	cur = big->nodes.begin();
	end = big->nodes.end();
	for(; cur != end; cur++) {
		n = *cur;
		vector<int> &cv = D[n->node_id];
		if(cv[longest_node] == INT_LIMIT)
			continue;	//this node cannot reach the root
		
		int longest = longest_dists[n->node_id];
		//n->longest_path = longest;
//printf("Node %d's longest path is %d\n", n->node_id, longest);
		
		if(longest < shortest) {
			shortest = longest;
			shortest_node = n->node_id;
		}
	}
	
	//now we have our root, set each node to their distance from the root
	vector<int> &root_dists = D[shortest_node];
//printf("The tree's root is %d\n", shortest_node);
	cur = big->nodes.begin();
	end = big->nodes.end();
	for(; cur != end; cur++) {
		n = *cur;
		if(n->node_id == shortest_node)
			n->longest_path = 0;
		else
			n->longest_path = root_dists[n->node_id];
//printf("Node %d's longest path is %d\n", n->node_id, root_dists[n->node_id]);
	}
	
	
	
	
	
	/*
		to find reachability:
		loop through each edge e
			Remove that edge from the graph
			color_disjoint_graphs
			fc = count number of nodes with the same color as 'e->from'
			tc = count number of nodes with the same color as 'e->to'
			e->normal_reach = tc;
			e->reverse_reach = fc;
	*/
	property_map<MyGraph, edge_weight_t>::type weightmap;
	weightmap = get(edge_weight, vg);
	
	int tc,fc;
	int from_color, to_color;
	
	printf("Finding lengths (%d dots)", big->edges.size()/10);
	int pos = 0;
	
	PathEdge *e;
	list<PathEdge*>::iterator cur4,end4;
	cur4 = big->edges.begin();
	end4 = big->edges.end();
	for(; cur4 != end4; cur4++, pos++) {
		if(pos % 10 == 0) {
			printf(".");
			fflush(stdout);
		}
		e = *cur4;
		
		//remove this edge from the boost graph..
		remove_edge(em[e], vg);
		
		//color the graph and get us the info we need to do our job
		color_disjoint_graphs(big, vg, map, NULL, D, counts, disjoint_counts, first_node);
		
		//add the edge back in
		EdgeDesc ed;
		bool inserted;
		tie(ed, inserted) = add_edge(e->from->node_id, e->to->node_id, vg);
		em[e] = ed;
		weightmap[em[e]] = 1;	//cause its a new edge
		
		//make sure this stupid thing worked.
		if(e->from->color == e->to->color) {
			printf("Cycle detected in MST... WTF?\n");
			e->normal_reach = -1;
			e->reverse_reach = -1;
			continue;
		}
		from_color = e->from->color;
		to_color = e->to->color;
		
		//count our crap.
		tc = 0;
		fc = 0;
		cur = big->nodes.begin();
		end = big->nodes.end();
		for(; cur != end; cur++) {
			n = *cur;
			if(n->color == to_color)
				tc++;
			else if(n->color == from_color)
				fc++;
		}
		
		//put it on our edge
		e->normal_reach = tc;
		e->reverse_reach = fc;
	}
	
	printf("\n");
	
	choose_biggest_graph(big, counts, first_node);
}

/*
void calc_path_lengths(Map *map, MyGraph &vg, PathGraph *big, vector< vector<int> > &D, const char *fname) {

//	vector< vector<int> > D;
	vector<int> counts;
	vector<int> first_node;
	
	
	choose_biggest_graph(big, counts, first_node);
}
*/


//assumes that the graph is freshly colored disjoint, and counts/first_node are results from it
void choose_biggest_graph(PathGraph *big, vector<int> &counts, vector<int> &first_node) {
	int best_graph = 0;
	int best_count = counts[0];
	unsigned int r;
	for(r = 1; r < counts.size(); r++) {
		if(best_count < counts[r]) {
			best_count = counts[r];
			best_graph = r;
		}
//		printf("Graph %d has %d edges\n", r, counts[r]);
	}
	
	printf("Best Tree: Detected %d graphs in the MST, selected #%d\n", counts.size(), best_graph);
	
	list<PathNode*> new_nodes;
	list<PathEdge*> new_edges;
	
	//rebuild the node list to include only nodes which are in
	//the best graph, also only keeping the edges which connect them.
	std::map<PathNode *, int> havenodelist;
	list<PathEdge*>::iterator cur4,end4;
	cur4 = big->edges.begin();
	end4 = big->edges.end();
	for(; cur4 != end4; cur4++) {
		PathEdge *e = *cur4;
		
		//remove the edge if it is not the main color
		//this also causes the removal of the nodes if they
		//are not used in any other edges.
		if(e->from->color != best_graph) {
			continue;
		}
		if(e->to->color != best_graph) {
			continue;
		}
		
		new_edges.push_back(e);
		
		if(havenodelist.count(e->from) != 1) {
			e->from->final_id = new_nodes.size();
			new_nodes.push_back(e->from);
			havenodelist[e->from] = 1;
		}
		if(havenodelist.count(e->to) != 1) {
			e->to->final_id = new_nodes.size();
			new_nodes.push_back(e->to);
			havenodelist[e->to] = 1;
		}
	}
	
	printf("Best Tree: Removed %d nodes and %d edges which were disconnected.\n", 
		big->nodes.size() - new_nodes.size(), big->edges.size() - new_edges.size());
	big->nodes = new_nodes;
	big->edges = new_edges;
}

void consolidate_cross_graphs(Map *map, PathGraph *big, PathGraph *excess, MyGraph &cross_graph, const char *fname) {
	
	vector< vector<int> > D;
	vector<int> counts;
	vector<int> disjoint_counts;
	vector<int> first_node;
	
	//color the graph and get us the info we need to do our job
	color_disjoint_graphs(big, cross_graph, map, fname, D, counts, disjoint_counts, first_node);
}

void find_path_info(Map *map, MyGraph &vg, vector< vector<PathEdge*> > &path_finding, PathGraph *big) {
	//make sure our path finding vector is big enough.
	{
		int size = big->nodes.size();
		vector<PathEdge*> tmp(size, (PathEdge*)NULL);
		path_finding.resize(0);
		path_finding.resize(size, tmp);
	}
	
	vector< vector<int> > D;
	vector<int> counts;
	vector<int> disjoint_counts;
	vector<int> first_node;
	
	//color the graph and get us the info we need to do our job
	color_disjoint_graphs(big, vg, map, NULL, D, counts, disjoint_counts, first_node);
	
	//figure out what edges link to each node.
	std::map<PathNode*, vector<PathEdge*> > node_edges;
	find_node_edges(big, node_edges);
	
	
	//for each node, find best edge to reach each other node.
	list<PathNode*>::iterator cur,end;
	PathNode *n;
	vector<int>::iterator curp,endp;
	vector<PathEdge *>::iterator cure,ende;
	int r;
	cur = big->nodes.begin();
	end = big->nodes.end();
	for(; cur != end; cur++) {
		n = *cur;
		//get all our vector refs that we need since this is kinda expensive
		vector<int> &cv = D[n->node_id];	//distance array
		vector<PathEdge *> &pf = path_finding[n->node_id];	//result
		vector<PathEdge *> &el = node_edges[n];	//our edges
		curp = cv.begin();
		endp = cv.end();
		for(r = 0; curp != endp; curp++, r++) {
			if(n->node_id == r) {
				//this is the end of the path, we cannot take an edge to reach ourself
				pf[r] = NULL;
				continue;
			}
			int my_dist = *curp;
			if(my_dist == INT_MAX) {
				//this node is unreachable.
				pf[r] = NULL;
				continue;
			}
			
			//else, node r reachable from us, find best edge.
			cure = el.begin();
			ende = el.end();
//			int shortest;
			PathEdge *ce;
			PathNode *cn;
			//for each edge
			for(; cure != ende; cure++) {
				ce = *cure;
				//find the node other than ourself on the edge
				if(ce->from == n)
					cn = ce->to;
				else
					cn = ce->from;
				//see how far away this node is
				int cdist = D[cn->node_id][r];
				if(cdist < my_dist) {
					//found one which is closer... due to min span tree, there
					//should only be one path possible so just go with it.
					pf[r] = ce;
					break;
				}
			}
			//assume that this node got assigned.. next
if(pf[r] == NULL) {
printf("Node id %d was not able to find a good path to node %d\n", n->node_id, r);
}
		}
	}
}






