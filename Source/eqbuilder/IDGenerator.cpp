// IDGenerator.cpp: implementation of the IDGenerator class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "IDGenerator.h"
#include "database.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

IDGenerator::IDGenerator(const char *shortname, const char *db_query, database *db)
: m_query(db_query),
  m_short(shortname)
{
	m_mode = DB;
	m_db = db;
	m_zoneid = 0;
	m_fixedid = 1;
	m_float = 1;
	m_zonemult = 1000;
	m_currentid = 0;
	m_startid = 0;
}

IDGenerator::~IDGenerator()
{

}

void IDGenerator::SaveToFile() {
	string fn = m_short;
	fn += "_ids.dat";
	FILE *f = fopen(fn.c_str(), "wb");
	if(f == NULL)
		return;
	fwrite(&m_mode, sizeof(m_mode), 1, f);
	fwrite(&m_fixedid, sizeof(m_fixedid), 1, f);
	fwrite(&m_float, sizeof(m_float), 1, f);
	fwrite(&m_zonemult, sizeof(m_zonemult), 1, f);
	fclose(f);
}

void IDGenerator::ReadFromFile() {
	string fn = m_short;
	fn += "_ids.dat";
	FILE *f = fopen(fn.c_str(), "rb");
	if(f == NULL)
		return;
	fread(&m_mode, sizeof(m_mode), 1, f);
	fread(&m_fixedid, sizeof(m_fixedid), 1, f);
	fread(&m_float, sizeof(m_float), 1, f);
	fread(&m_zonemult, sizeof(m_zonemult), 1, f);
	fclose(f);

	Reset();
}

void IDGenerator::Reset() {
	switch(m_mode) {
	case DB:
		m_currentid = QueryID();
		break;
	case Fixed:
		m_currentid = m_fixedid;
		break;
	case Floating:
		m_currentid = m_float;
		break;
	case Zone:
		m_currentid = m_zoneid * m_zonemult;
		break;
	}
	m_startid = m_currentid;
}

void IDGenerator::DataWritten() {
	switch(m_mode) {
	case DB:
		break;
	case Fixed:
		break;
	case Floating:
		m_float = m_currentid;
		break;
	case Zone:
		break;
	}
}

void IDGenerator::Restart() {
	m_currentid = m_startid;
}

int IDGenerator::GetNextID() {
	return(m_currentid++);
}

int IDGenerator::QueryID() {
	return(m_db->load_value(m_query.c_str()));
}


