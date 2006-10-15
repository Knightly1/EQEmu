// cwaypoint.cpp: implementation of the cwaypoint class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cwaypoint.h"

#include "../common/types.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cwaypoint::cwaypoint()
{
	db = false;
	this->loc = new cloc();
	this->pause = false;
	this->valid = true;
	this->colinear = false;
}

cwaypoint::cwaypoint( const cwaypoint* wp )
{
	this->db = wp->db;
	this->loc = new cloc( wp->loc );
	this->pause = wp->pause;
	this->valid = wp->valid;
	this->colinear = wp->colinear;
}

cwaypoint::~cwaypoint()
{
	delete loc;
}

