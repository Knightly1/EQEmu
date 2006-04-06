// eqbuilderini.h: interface for the eqbuilderini class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EQBUILDERINI_H__1DDA9DB0_5666_4AEB_8A49_C794E44E0857__INCLUDED_)
#define AFX_EQBUILDERINI_H__1DDA9DB0_5666_4AEB_8A49_C794E44E0857__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class eqbuilderini  
{
public:
	eqbuilderini( CString eqbpath );
	virtual ~eqbuilderini();

	void setdbparams( CString host, CString user, CString password, CString db );
	void setgeneralparams( CString eqmaps_path, CString eqemumaps_path );
	void setsqlparams( int useopt, int zoneid, int npcid, int spawnid, int gridid, bool sqldelete, bool usedb );

	// database
	CString host;
	CString user;
	CString password;
	CString db;

	// sql options
	int useopt;
	int zoneid;
	int npcid;
	int spawnid;
	int gridid;
	bool sqldelete;
	bool usedb;
	
	//general options
	CString eqmaps_path;
	CString eqemumaps_path;

private:
	CString path;
	CString getcle( CString line );
	CString getvalue( CString line );
	void createdefault();
	void save();
	void load();


};

#endif // !defined(AFX_EQBUILDERINI_H__1DDA9DB0_5666_4AEB_8A49_C794E44E0857__INCLUDED_)
