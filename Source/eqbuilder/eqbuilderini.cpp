// eqbuilderini.cpp: implementation of the eqbuilderini class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "EQBuilder.h"
#include "eqbuilderini.h"
#include <io.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

eqbuilderini::eqbuilderini( CString eqbpath )
{
	this->path = eqbpath+"\\eqbuilder.ini";
	this->host = "";
	this->user = "";
	this->password = "";
	this->db = "";
/*	this->useopt = 1;
	this->zoneid = 400;
	this->npcid = 1000;
	this->spawnid = 1000;
	this->gridid = 1000;
	this->sqldelete = false;
	this->usedb = false;
*/	this->eqmaps_path = "";
	this->eqemumaps_path = "";
	load();
}

void eqbuilderini::setdbparams( CString host, CString user, CString password, CString db ) {
	this->host = host;
	this->user = user;
	this->password = password;
	this->db = db;
	save();
}

/*void eqbuilderini::setsqlparams( int useopt, int zoneid, int npcid, int spawnid, int gridid, bool sqldelete, bool usedb ) {
	this->useopt = useopt;
	this->zoneid = zoneid;
	this->npcid = npcid;
	this->spawnid = spawnid;
	this->gridid = gridid;
	this->sqldelete = sqldelete;
	this->usedb = usedb;
	save();
}*/

void eqbuilderini::setgeneralparams( CString eqmaps_path, CString eqemumaps_path ) {
	this->eqmaps_path = eqmaps_path;
	this->eqemumaps_path = eqemumaps_path;
	save();
}


void eqbuilderini::load() {
	
	CFileException* pError=NULL; 

	if( _access( path, 0 ) == -1 ) {
		createdefault();
		return;
	}

	CStdioFile f;
	CString line;
	CString cle;
	CString value;

	if (f.Open(path,CFile::modeRead,pError) == 0) 
	{ 
	  exit(1);
	}

	int readresult = 1;
	f.SeekToBegin();

	while ( readresult == 1 ) {

		readresult = f.ReadString(line);

		cle = getcle( line );
		value = getvalue( line );
		
		if ( cle == "host" ) {
			host = value;
		} else if ( cle == "user" ) {
			user = value;
		} else if ( cle == "password" ) {
			password = value;
		} else if ( cle == "db" ) {
			db = value;
/*		} else if ( cle == "useopt" ) {
			useopt = atoi( value );
		} else if ( cle == "zoneid" ) {
			zoneid = atoi( value );
		} else if ( cle == "npcid" ) {
			npcid = atoi( value );
		} else if ( cle == "spawnid" ) {
			spawnid = atoi( value );
		} else if ( cle == "gridid" ) {
			gridid = atoi( value );
		} else if ( cle == "sqldelete" ) {
			sqldelete = ( value == "true" );
		} else if ( cle == "usedb" ) {
			usedb = ( value == "true" );
*/		} else if ( cle == "eqmaps_path" ) {
			eqmaps_path = value;
		} else if ( cle == "eqemumaps_path" ) {
			eqemumaps_path = value;
		}

	}

	f.Close();

}

CString eqbuilderini::getcle( CString line ) {
	int seppos = line.Find( "=", 0 );

	if ( seppos > 0 ) {
		return line.Mid(0,seppos);
	}
	
	return "";
}

CString eqbuilderini::getvalue( CString line ) {
	int seppos = line.Find( "=", 0 );

	if ( seppos > 0 ) {
		return line.Mid(seppos+1, line.GetLength()-seppos );
	}

	return "";
}

void eqbuilderini::createdefault() {
	save();
}

void eqbuilderini::save() {

	CStdioFile f;
	CString line;

	CFileException* pError=NULL; 
	
	if (f.Open( "eqbuilder.ini", CFile::modeCreate|CFile::modeWrite, pError ) == 0) 
	{ 
	  exit(1);
	}

	line = "[Database]\n";
	f.WriteString(line);
	line = "host=" + this->host + "\n";
	f.WriteString(line);
	line = "user=" + this->user + "\n";
	f.WriteString(line);
	line = "password=" + this->password + "\n";
	f.WriteString(line);
	line = "db=" + this->db + "\n";
	f.WriteString(line);
	line = "\n";
	f.WriteString(line);

/*	line = "[SQL Options]\n";
	f.WriteString(line);
	line.Format( "useopt=%d\n", this->useopt );
	f.WriteString(line);
	line.Format( "zoneid=%d\n", this->zoneid );
	f.WriteString(line);
	line.Format( "npcid=%d\n", this->npcid );
	f.WriteString(line);
	line.Format( "spawnid=%d\n", this->spawnid );
	f.WriteString(line);
	line.Format( "gridid=%d\n", this->gridid );
	f.WriteString(line);
	if ( sqldelete == true ) {
		line.Format( "sqldelete=true\n" );
	} else {
		line.Format( "sqldelete=false\n" );
	}
	f.WriteString(line);
	if ( usedb == true ) {
		line.Format( "usedb=true\n" );
	} else {
		line.Format( "usedb=false\n" );
	}
	f.WriteString(line);
*/
	line = "[General Options]\n";
	f.WriteString(line);
	line = "eqmaps_path=" + this->eqmaps_path + "\n";
	f.WriteString(line);
	line = "eqemumaps_path=" + this->eqemumaps_path + "\n";
	f.WriteString(line);

}

eqbuilderini::~eqbuilderini()
{

}
