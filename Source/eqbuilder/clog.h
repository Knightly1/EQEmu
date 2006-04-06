// clog.h: interface for the clog class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLOG_H__142308AA_BAC9_440B_A049_F6ABDB924A58__INCLUDED_)
#define AFX_CLOG_H__142308AA_BAC9_440B_A049_F6ABDB924A58__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "mob_list.h"
#include "waypoint_list.h"
#include "kill_list.h"

typedef enum {
	logNPCType = 0,
	logPathing,	//1
	logRaid		//2
} logType;

class clog  
{
public:
	clog();
	virtual ~clog();

	//log file info
	CString name;
	bool compiled;
	bool is_build_file;		//is this from a build file
	bool is_db_file;		//is this loaded from the DB
	int compilepos;
	
	//initial mobs
	int nbmobinit;
	mob_list* mobinit;
	
	//new spawns
	int nbmobadd;
	mob_list* mobadd;
	
	//type of this log
	logType type;		//1=movement log, 2 = 

};

#endif // !defined(AFX_CLOG_H__142308AA_BAC9_440B_A049_F6ABDB924A58__INCLUDED_)
