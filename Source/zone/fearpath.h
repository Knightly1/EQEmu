#ifndef FEARPATH_H
#define FEARPATH_H

#include "../common/types.h"
#include <stdio.h>


//this defines the version of path files that we read
#define PATHFILE_VERSION 0x02000000

//grow this type if you wanna support more than 64k fear nodes
//this will affect path files, so change their version. (aka dont change this)
typedef uint16 FearNodeRef;		//this is a quad tree node
typedef uint16 FearPointRef;	//this is really a path node ref
typedef uint32 FearLinkRef;

//special values for PathNodeRefs
#define FEAR_LINK_NONE 0xFFFFFFFF
#define FEAR_NODE_NONE 65534
#define FEAR_ROOT_NODE 0

/*

File format:
	PathFile_Header		head;
	PathNode_Struct		nodes[head.node_count];
	PathLink_Struct		links[head.link_count*2];
	PathTree_Struct		quadtree[head.qtnode_count];
	//we could prolly shrink this node list into uint16
	FearPointRef		nodelist[head.nodelist_count];

//things to add later, maybe:
	//this matrix is symetric, only need to store half of it:
	FearPointRef			reachability[head.node_count*head.node_count/2];
the problem here is size: even 4000 nodes is 15Mb of file, and its not sparse
*/

#pragma pack(1)

struct PathFile_Header {
	uint32 version;
	uint32 node_count;
	uint32 link_count;
	uint16 qtnode_count;
	uint32 nodelist_count;
};

struct PathNode_Struct {
	float		x;
	float		y;
	float		z;
	FearLinkRef	link_offset;		//offset into the links array for our data
	uint8		link_count;			//how many links we have
	FearNodeRef	distance;			//distance from the tree's root		
};

struct PathLink_Struct {
	FearPointRef 	dest_node;		//what node is reachable from this link
	FearNodeRef		reach;			//# of nodes reachable after traversing this link
};


enum {  //node flags
	fearNodeFinal = 0x01
	//7 more bits if theres something to use them for...
};

typedef struct _PathTree_Struct {
	//bounding box of this node
	//there is no reason that these could not be unsigned
	//shorts other than we have to compare them to floats
	//all over the place, so they stay floats for now.
	float minx;
	float miny;
	float maxx;
	float maxy;
	
	unsigned char flags;
	//un-unioned because all nodes store a list
//	union {
		FearNodeRef nodes[4];	//index 0 means NULL, not root
		struct {
			unsigned long count;
			unsigned long offset;
		} nodelist;
//	};
} PathTree_Struct, FPNODE, *PFPNODE;

#pragma pack()



class FearPathManager;

class MobFearState {
	friend class FearPathManager;
public:
	MobFearState() {
		goal_node = FEAR_NODE_NONE;
		last_link = FEAR_LINK_NONE;
		state = stuckSomewhere;
	}
	
	float x;
	float y;
	float z;
	
	inline bool IsValidState() { return(state != stuckSomewhere && goal_node != FEAR_NODE_NONE); }
	
protected:
	//stuff that only the fear manager needs to know about
	FearPointRef goal_node;
	FearLinkRef  last_link;
	
	enum {
		stuckSomewhere,
		runToTop,		//trying to run to the top of the tree
		runToBottom		//we have reached a peak, descend to a leaf
	} state;
};



class FearPathManager {
public:
	static FearPathManager* LoadPathFile(const char* in_zonename);
	FearPathManager();
	~FearPathManager();
	
	bool loadPaths(FILE *fp);
	
	//the result is always final, except special NODE_NONE
	FearNodeRef SeekNode( FearNodeRef _node, float x, float y );
	FearNodeRef SeekNodeGuarantee( FearNodeRef _node, float x, float y );
	
	bool FindNearestPath(MobFearState *state, float x, float y, float z, bool check_los = true);
	bool NextPathNode(MobFearState *state);
	
	inline FearNodeRef		GetRoot( ) { return FEAR_ROOT_NODE; }
	
	inline float GetMinX() const { return(_minx); }
	inline float GetMaxX() const { return(_maxx); }
	inline float GetMinY() const { return(_miny); }
	inline float GetMaxY() const { return(_maxy); }
	inline float GetMinZ() const { return(_minz); }
	inline float GetMaxZ() const { return(_maxz); }
	
	inline uint32 CountNodes() { return(m_Nodes); }
	inline PathNode_Struct *GetNode(FearPointRef r) { return(r<m_Nodes?nodes+r:NULL); }
	
private:
	//path data
	uint32 m_Nodes;
	uint32 m_Links;
	PathNode_Struct *nodes;
	PathLink_Struct *links;
//	PathNodeRef *reachability;
	
	//quadtree data
	uint32 m_QTNodes;
	uint32 m_NodeLists;
	PathTree_Struct *QTNodes;
	FearPointRef *nodelists;
	
	//quick access to abstract away how we access these things
	inline PathTree_Struct *GetQTNode( FearNodeRef r ) { return( QTNodes + r ); }
	inline FearPointRef *GetNodeList(PathTree_Struct *qt) {
		return(qt->nodelist.offset>=m_NodeLists?NULL : nodelists + qt->nodelist.offset); }
	inline PathLink_Struct *GetLinks(FearLinkRef r) { return(r<m_Links?links+r:NULL); }
	
	
	float _minz, _maxz;
	float _minx, _miny, _maxx, _maxy;
	
	static inline float Dist2(float x1, float y1, float z1, float x2, float y2, float z2) {
		float tmp;
		float sum;
		tmp = x1 - x2;
		sum = tmp*tmp;
		tmp = y1 - y2;
		sum += tmp*tmp;
		tmp = z1 - z2;
		sum += tmp*tmp;
		return(sum);
	}
};


#endif

