// log_list.cpp: implementation of the log_list class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "log_list.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

log_list::log_list()
{
	this->txt_size = 0;
	this->bf_size = 0;
	
	list.dont_delete = true;
}

void log_list::txt_add( clog* log ) {
	log->is_build_file = false;
	list.Append( log );
	txt_list.Append( log );
	txt_size++;
}

void log_list::bf_add( clog* log ) {
	log->is_build_file = true;
	list.Append( log );
	bf_list.Append( log );
	bf_size++;
}

clog* log_list::get( int pos ) {

    LinkedListIterator<clog*> iterator(list);
	iterator.Reset();

	clog* log = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			log = iterator.GetData();
			return log;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

clog* log_list::bf_get( int pos ) {

    LinkedListIterator<clog*> iterator(bf_list);
	iterator.Reset();

	clog* log = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			log = iterator.GetData();
			return log;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

clog* log_list::txt_get( int pos ) {

    LinkedListIterator<clog*> iterator(txt_list);
	iterator.Reset();

	clog* log = NULL;
	int i = 0;
	while(iterator.MoreElements()) {

		if ( i == pos ) {
			log = iterator.GetData();
			return log;
		}

        iterator.Advance();
		i++;

	}

	return NULL;
}

log_list::~log_list()
{
	this->list.Clear();
}
