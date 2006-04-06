// clog.cpp: implementation of the clog class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "clog.h"
#include "../common/types.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

clog::clog()
{
	this->name = "";
	this->compiled = false;
	this->nbmobinit = 0;
	this->mobinit = NULL;
	this->nbmobadd = 0;
	this->mobadd = NULL;
	this->type = logPathing;
	this->compilepos = 0;
	this->is_build_file = false;
	is_db_file = false;
}

clog::~clog()
{
	delete mobinit;
	delete mobadd;
}
