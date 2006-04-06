// czone.cpp: implementation of the czone class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "czone.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

czone::czone()
{
	this->id = 0;
	this->long_name = "";
	this->short_name = "";
}

czone::~czone()
{
	
}
