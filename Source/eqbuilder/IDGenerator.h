// IDGenerator.h: interface for the IDGenerator class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_IDGENERATOR_H__8A6668E1_A0BA_47D0_A30E_707161135732__INCLUDED_)
#define AFX_IDGENERATOR_H__8A6668E1_A0BA_47D0_A30E_707161135732__INCLUDED_

#include <string>

using namespace std;

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class database;

class IDGenerator  
{
	typedef enum {
		DB,
		Fixed,
		Floating,
		Zone
	} idMode;
public:
	IDGenerator(const char *shortname, const char *db_query, database *db);
	virtual ~IDGenerator();
	
	void SaveToFile();
	void ReadFromFile();

	void SetToDB() { m_mode = DB; }
	void SetToFixed() { m_mode = Fixed; }
	void SetToFloating() { m_mode = Floating; }
	void SetToZone() { m_mode = Zone; }
	
	void SetFixedID(int fixed) { m_fixedid = fixed; }
	void SetFloatingID(int floating) { m_float = floating; }
	void SetZoneID(int zoneid) { m_zoneid = zoneid; }
	void SetZoneMultiple(int mult) { m_zonemult = mult; }
	
	void LoadFixedFromDB() { SetFixedID(QueryID()); }
	void LoadFloatingFromDB()  { SetFloatingID(QueryID()); }

	void Restart();		//re-start an ID sequence to value from last reset
	void DataWritten();	//gives us an event of 'these IDs have been written out'
	void Reset();		//re-determine the starting ID of this sequence
	int GetNextID();	//returns the next ID in the sequence
	
protected:
	int QueryID();

	idMode m_mode;
	int m_zoneid;
	int m_fixedid;
	int m_float;
	int m_zonemult;

	int m_currentid;
	int m_startid;

	string m_query;
	string m_short;
	database *m_db;


};

#endif // !defined(AFX_IDGENERATOR_H__8A6668E1_A0BA_47D0_A30E_707161135732__INCLUDED_)
