// cmerchant.cpp: implementation of the cmerchant class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "cmerchant.h"

#include "../common/types.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

cmerchant::cmerchant()
{
	db = false;
	this->id = 0;
	owner = NULL;
	rate = 1;
}

cmerchant::cmerchant( cmerchant* merchant )
{
	this->db = merchant->db;
	this->id = merchant->id;
	this->rate = merchant->rate;
	this->owner = merchant->owner;
	
	items = merchant->items;
}

cmerchant::~cmerchant()
{
}
