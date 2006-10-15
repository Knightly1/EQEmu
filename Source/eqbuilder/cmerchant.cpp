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
	this->db_id = 0;
	owner = NULL;
	rate = 1;
}

cmerchant::cmerchant( cmerchant* merchant )
{
	this->db = merchant->db;
	this->db_id = merchant->db_id;
	this->rate = merchant->rate;
	this->owner = merchant->owner;
	
	items = merchant->items;
}

cmerchant::~cmerchant()
{
}

void cmerchant::mergeFrom(const cmerchant *them) {
	map<uint16, uint32>::const_iterator scur, send;
	map<uint16, uint32>::iterator cur, end;
	
	scur = them->items.begin();
	send = them->items.end();
	for(; scur != send; scur++) {
		//TODO: finish this..
	}
}
