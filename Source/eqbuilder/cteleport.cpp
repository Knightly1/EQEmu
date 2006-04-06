// cteleport.cpp: implementation of the cteleport class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cteleport.h"

#include "../common/types.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cteleport::cteleport()
{
	this->id = 0;
	this->loc = NULL;
	this->zone = "";
}

cteleport::~cteleport()
{
	delete loc;
}
