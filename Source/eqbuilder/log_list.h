// log_list.h: interface for the log_list class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_LOG_LIST_H__BBA05006_FF88_4C21_BEED_C3A6137246A5__INCLUDED_)
#define AFX_LOG_LIST_H__BBA05006_FF88_4C21_BEED_C3A6137246A5__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "clog.h"
#include "../common/linked_list.h"

class log_list  
{
public:
	log_list();
	virtual ~log_list();

	void txt_add( clog* log );
	void bf_add( clog* log );
	clog* get( int pos );
	clog* txt_get( int pos );
	clog* bf_get( int pos );
	int getsize() { return txt_size + bf_size; }
	int txt_getsize() { return txt_size; }
	int bf_getsize() { return bf_size; }

private:
	int txt_size;
	int bf_size;
	LinkedList<clog*> list;
	LinkedList<clog*> txt_list;
	LinkedList<clog*> bf_list;
};

#endif // !defined(AFX_LOG_LIST_H__BBA05006_FF88_4C21_BEED_C3A6137246A5__INCLUDED_)
