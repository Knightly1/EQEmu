// cloc.cpp: implementation of the cloc class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cloc.h"
#include "math.h"
#include "globals.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cloc::cloc()
{

}

cloc::cloc( const cloc* loc )
{
	this->x = loc->x;
	this->y = loc->y;
	this->z = loc->z;
	this->heading = loc->heading;

}

cloc::cloc( double x, double y, double z )
{
	this->x = x;
	this->y = y;
	this->z = z;
}

cloc::cloc( double x, double y, double z, double heading )
{
	this->x = x;
	this->y = y;
	this->z = z;
	this->heading = heading;
}

cspawnpoint::cspawnpoint( const cspawnpoint* loc )
{
	this->x = loc->x;
	this->y = loc->y;
	this->z = loc->z;
	this->heading = loc->heading;
	this->db_id = loc->db_id;
}

cspawnpoint::cspawnpoint( const cloc* loc )
{
	this->x = loc->x;
	this->y = loc->y;
	this->z = loc->z;
	this->heading = loc->heading;
	this->db_id = 0;
}
