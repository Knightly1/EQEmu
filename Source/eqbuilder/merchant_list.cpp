// merchant_list.cpp: implementation of the merchant_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "merchant_list.h"
#include "cmerchant.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

merchant_list::merchant_list()
{
	this->size = 0;

}

void merchant_list::add( cmerchant* grid ) {
	list.Append( grid );
	size++;
}

cmerchant* merchant_list::get( int pos ) {

    LinkedListIterator<cmerchant*> iterator(list);
	iterator.Reset();

	cmerchant* m = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			m = iterator.GetData();
			return m;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

merchant_list::~merchant_list()
{
	this->list.Clear();
}
