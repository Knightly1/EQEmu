// logs.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "logs.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// log

logs::logs( CString name )
{
	this->name = name;
}

void logs::parse()
{
	CArray<CString,CString&>* buffer = loadbuffer();
	int maxpos = buffer->GetSize();

	currentpos = 0;

	while( currentpos < maxpos ) {

		currentkey = get( buffer, currentpos );

		if ( isMobInit( buffer ) ) {

			currentpos += 2;
			getMobInitData( buffer );

		} else if ( isMobAdd( buffer ) ) {

			currentpos += 3;
			getMobAddData( buffer );

		} else if ( isDoor( buffer ) ) {

			currentpos += 2;
			getDoorData( buffer );

		} else if ( isTeleporter( buffer) ) {

			currentpos += 2;
			getTeleporterData( buffer );

		} else if ( isMovement( buffer ) ) {

			currentpos ++;
			getMovementData( buffer );

		} else if ( isKilled( buffer ) ) {

			currentpos += 2;
			getKilledData( buffer );

		} 

		currentpos++;

	}

}

BOOL logs::OnInitDialog() {
	
	return TRUE;
}

bool logs::isMobInit() {

	return ( currentkey == "Zone." ) && ( get( currentpos + 1 ) == "MobInit." );

}

bool logs::isMobAdd() {

	return ( currentkey == "Zone." ) && ( get( currentpos + 1 ) == "Add" ) && ( get( currentpos + 2 ) == "Mob." );

}

bool logs::isDoor() {

	return ( currentkey == "Zone." ) && ( get( currentpos + 1 ) == "ObjectInit." );

}

bool logs::isTeleporter() {

	return ( currentkey == "Zone." ) && ( get( currentpos + 1 ) == "TeleporterInit." );

}

bool logs::isMovement() {

	return ( currentkey == "MobMovement:" );

}

bool logs::isKilled()  {

	return ( currentkey == "Zone." ) && ( get( currentpos + 1 ) == "Killed." );

}

void logs::getMobInitData()
{
}

void logs::getMobAddData()
{
}

void logs::getDoorData()
{
	while( currentkey != "Received:" ) {
		currentpos ++;
		currentkey = get( currentpos );
	}
}

void logs::getTeleporterData()
{
}

void logs::getMovementData()
{
}

void logs::getKilledData()
{
}


int logs::findnext( CString tofind, int start )
{
	CString temp;
	int i = start;
	while ( temp != tofind )
	{
		temp = get(i);
		i++;
	}

	return i-1;
}

CString logs::value( CString key, int start, int size ) {

	int keypos = findnext( key, start );
	int valuepos = keypos + 1;

	CString value = get( valuepos );

	for ( int i=1; i<size; i++ ) {
		value.Format( "%s %s", value, get( valuepos + i ) );
	}

	return value;
}

CString logs::get( CArray<CString,CString&>* buffer, int index ) { 
	return buffer->GetAt( index );
}

CArray<CString,CString&>* logs::loadbuffer()
{
	FILE *f;
	CArray<CString,CString&>* buf = new CArray<CString,CString&>();

	if (!(f = fopen( name, "r" ))) {
		return NULL;
	}

	CString line;

	while (!feof (f)) 
	{
		fscanf(f, "%s", line);
		buf->Add(line);
	}

	fclose( f );

	return buf;
}

logs::~logs()
{
}


BEGIN_MESSAGE_MAP(logs, CWnd)
	//{{AFX_MSG_MAP(logs)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// logs message handlers
