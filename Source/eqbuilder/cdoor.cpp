// cdoor.cpp: implementation of the cdoor class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cdoor.h"

#include "../common/types.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cdoor::cdoor()
{
	this->id = 0;
	this->zone = "";
	this->type = 0;
	this->model = "";
	this->teleportid = 0;
	this->loc = NULL;
}

cdoor::~cdoor()
{
	delete loc;
}
