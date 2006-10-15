// cgrid.cpp: implementation of the cgrid class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cgrid.h"

#include "../common/types.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cgrid::cgrid()
{
	db = false;
	this->db_id = 0;
	this->waypoints = NULL;
}

cgrid::cgrid( cgrid* grid )
{
	this->db_id = grid->db_id;
	this->db = grid->db;
	if ( grid->waypoints == NULL ) {
		this->waypoints = NULL;
	} else {
		this->waypoints = new waypoint_list( grid->waypoints );
	}
}

cgrid::~cgrid()
{
	delete waypoints;
}
