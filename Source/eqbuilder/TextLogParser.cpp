// EQBuilderDlg.cpp : implementation file
//

#include "stdafx.h"
#include "EQBuilder.h"
#include "EQBuilderDlg.h"
#include <afxtempl.h>
#include <math.h>
#include  <io.h>
#include "globals.h"
#include "../common/types.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

void CEQBuilderDlg::parselog()
{
	CFileException* pError=NULL; 

	CString path = chemin + "\\logs\\" + zonename + "\\" + compiledlogs->get(selectedlog)->name;
	if (flog.Open(path,CFile::modeRead,pError) == 0) 
	{ 
	  exit(1);
	} 

	int maxpos = flog.GetLength();
	int curpos = 0;

	parsebegin();

	while( !parseend() ) {

		parseread();

		if ( isMobInit() ) {

			getMobInitData();

		} else if ( isMobAdd() ) {

			getMobAddData();

		} else if ( isDoor() ) {

			getDoorData();

		} else if ( isTeleporter() ) {

			getTeleporterData();

		} else if ( isMovement() ) {

			getMovementData();

		} else if ( isKilled() ) {

			getKilledData();

		} else if ( isWaypoint() ) {

			getWaypointData();

		} else if ( isShopOpen() ) {

			getShopOpenData();

		} else if ( isShopItem() ) {

			getShopItemData();

		}

		parsenext();

		if ( (int)flog.GetPosition() > curpos ) {
			curpos = flog.GetPosition();
			currentlog->compilepos = ( 50 * curpos ) / maxpos;
			pProgress->SetPos( currentlog->compilepos );
		}

	}

	flog.Close(); 

}

void CEQBuilderDlg::parsebegin() {
	currentpos = 0;
	readresult = 1;
	flog.SeekToBegin(); 
}

bool CEQBuilderDlg::parseend() {
	return ( readresult != 1 );
}

void CEQBuilderDlg::parsenext() {
	currentpos++;

}

void CEQBuilderDlg::parseread() {
	readresult = flog.ReadString(currentline);
	if ( currentkeys != NULL ) {
		delete [] currentkeys;
	}
	currentkeys = formater( currentline );
}

bool CEQBuilderDlg::isFinSection( ) {
	return ( getkey(0) == "Received" ) || ( getkey(0) == "Sent" );
}

bool CEQBuilderDlg::isMobInit( ) {

	return ( nbkeys() >= 2 ) && ( getkey(0) == "Zone." ) && ( getkey(1) == "MobInit." );

}

bool CEQBuilderDlg::isMobAdd() {

	return ( nbkeys() >= 3 ) && ( getkey(0) == "Zone." ) && ( getkey(1) == "Add" ) && ( getkey(2) == "Mob." );

}

bool CEQBuilderDlg::isDoor() {

	return ( nbkeys() >= 2 ) && ( getkey(0) == "Zone." ) && ( getkey(1) == "ObjectInit." );

}

bool CEQBuilderDlg::isTeleporter() {

	return ( nbkeys() >= 2 ) && ( getkey(0) == "Zone." ) && ( getkey(1) == "TeleporterInit." );

}

bool CEQBuilderDlg::isMovement() {

	return ( nbkeys() >= 1 ) && ( getkey(0) == "MobMovement" );

}

bool CEQBuilderDlg::isKilled() {

	return ( nbkeys() >= 2 ) && ( getkey(0) == "Zone." ) && ( getkey(1) == "Killed." );

}

bool CEQBuilderDlg::isWaypoint() {

	return ( nbkeys() >= 2 ) && ( getkey(0) == "MobLocation" ) && ( getkey(1) == "of" );

}

bool CEQBuilderDlg::isShopOpen() {
	return ( nbkeys() >= 2 ) && ( getkey(0) == "Open" ) && ( getkey(1) == "shop" );
}

bool CEQBuilderDlg::isShopItem() {
	return ( nbkeys() >= 3 ) && ( getkey(0) == "Zone" ) && ( getkey(1) == "Item." ) && ( getkey(2) == "100" );
}

bool CEQBuilderDlg::isMerchant ( CString cle ) {


	return ( ( cle == "Tailoring" ) 
		|| ( cle == "Banker" )
		|| ( cle == "Bard" )
		|| ( cle == "Town" )
		|| ( cle == "Druid" )
		|| ( cle == "Ranger" )
		|| ( cle == "Armor" )
		|| ( cle == "Smithing" )
		|| ( cle == "Fletching" )
		|| ( cle == "Alchemy" )
		|| ( cle == "Jewelcrafting" )
		|| ( cle == "Satchels" )
		|| ( cle == "General" )
		|| ( cle == "Brewing" )
		|| ( cle == "Monk" )
		|| ( cle == "Pottery" )
		|| ( cle == "Berserker" )
		|| ( cle == "Weapons" ) );

}


void CEQBuilderDlg::getMobInitData()
{
//	afficherkeys();

	if ( currentlog->mobinit == NULL ) {
		currentlog->mobinit = new mob_list();
	}

	if ( currentlog->mobadd == NULL ) {
		currentlog->mobadd = new mob_list();
	}

	parsenext();
	parseread();

	while ( !isFinSection() ) {

		cmob* mob = new cmob();
		mob->npc = new cnpc();

		getMobGeneralData(mob);
		
		if ( IsValidSpawn( mob ) ) {
			
//if(string(mob->npc->nom).find(string("Questioner_Uila")) == string::npos)
//	continue;
			currentlog->mobinit->add( mob );
			currentlog->nbmobinit++;
		} else {
			delete mob;
		}

	}
}

void CEQBuilderDlg::getMobAddData()
{

	if ( currentlog->mobadd == NULL ) {
		currentlog->mobadd = new mob_list();
	}

	parsenext();
	parseread();

	cmob* mob = new cmob();
	mob->npc = new cnpc();

	
	getMobGeneralData(mob);

	if ( IsValidSpawn( mob ) ) {
//if(string(mob->npc->nom).find(string("Questioner_Uila")) == string::npos)
//	return;
		currentlog->mobadd->add( mob );
		currentlog->nbmobadd++;
	} else {
		delete mob;
	}

}

void CEQBuilderDlg::getMobGeneralData(cmob* mob) {
		//	afficherkeys();

		CString mobidhexa = getkey(1);
		int mobid; 
		sscanf(mobidhexa, "%x", &mobid); 
		mob->id = mobid;
	
		mob->npc->type = atoi( getkey(2) );

		int levelpos = findnext( "L", 0 );
		int dec = levelpos-4;
		CString mobnom;
		if ( levelpos != 4 ) {
			if ( isMerchant( getkey(4) ) ) {
				mobnom = getkey(3);
			} else {
				mobnom = value( 3, dec+1 );
			}
		} else {
			mobnom = getkey(3);
		}
		int numpos = mobnom.FindOneOf("0123456789");
		//could prolly get last name here if we wanted...
		mob->npc->nom = mobnom.Left(numpos);
//		mob->npc->nom = mobnom.Left( mobnom.GetLength() - 2 );	//-2 removes numbers
		mob->npc->level = atoi( getkey(5+dec) );
		mob->npc->gender = atoi( getkey(7+dec) );
		mob->hp = atoi( getkey(9+dec) );

		CString ownedidhexa = getkey(11+dec);
		int ownedid; 
		sscanf(ownedidhexa, "%x", &ownedid); 
		mob->owned = ( ownedid > 0 );
		
		mob->npc->size = atof( getkey(13+dec) );
		CString walkrun = getkey(14+dec);
		mob->npc->walkspeed = atof( walkrun.Mid( 0, 4 ) );
		mob->npc->runspeed = atof( walkrun.Mid( 5, 4 ) );

		parsenext();
		parseread();

//		afficherkeys();
		int typepos = findnext( "Type", 0 );
		int deca = typepos-2;

		if ( typepos == 2 ) {

			mob->npc->race = db->GetRace( getkey(1) );

		} else if ( typepos == 3 ) {

			mob->npc->race = db->GetRace( value(1,deca+1) );

		} else if ( typepos == 4 ) {
			
			if ( getkey(1) == "Unknown" ) {
				mob->npc->race = atoi( getkey(3) );
			} else {
				mob->npc->race = db->GetRace( value(1,deca+1) );
			}

		}

		mob->type = atoi( getkey(3+deca) );
		int deitypos = findnext( "Deity", 3+deca);
		if(deitypos == (6+deca)) {
			mob->npc->classe = db->getclasse( getkey(5+deca) );
		} else {
			mob->npc->classe = db->getclasse( value( 5+deca, deitypos-5-deca ) );
		}

		parsenext();
		parseread();

		mob->loc = new cloc();
		mob->loc->x = atof( getkey(2) );
		mob->loc->y = atof( getkey(3) );
		mob->loc->z = atof( getkey(4) ) / 10.0f;	//eqcollector seems to log this Z coord high
		mob->loc->heading = atof( getkey(9) ) / 2;

		mob->speed = new cloc();
		mob->speed->x = atof( getkey(13) );
		mob->speed->y = atof( getkey(14) );
		mob->speed->z = atof( getkey(15) );
		mob->speed->heading = atof( getkey(11) ) / 2;

		parsenext();
		parseread();

		mob->npc->texture = atoi( getkey( 19 ) );
		if ( mob->npc->texture == -1 ) {
			mob->npc->texture = 0;
		}

		mob->npc->helmtexture = atoi( getkey( 21 ) );
		if ( mob->npc->helmtexture == -1 ) {
			mob->npc->helmtexture = 0;
		}

		parsenext();
		parseread();

		//skip the equipment line, not sure if this should be optional
		if(getkey(0) == "Equipment") {
			parsenext();
			parseread();
		}

		mob->npc->luclin_haircolor = atoi( getkey( 2 ) );
		mob->npc->luclin_beard = 1;	//default
		mob->npc->luclin_beardcolor = atoi( getkey( 4 ) );
		mob->npc->luclin_eyecolor = atoi( getkey( 6 ) );
		mob->npc->luclin_eyecolor2 = mob->npc->luclin_eyecolor;
		mob->npc->luclin_hairstyle = atoi( getkey( 9 ) );
		mob->npc->face = atoi( getkey( 11 ) );

		parsenext();
		parseread();

		CString isnpcidhexa = getkey(11);
		int isnpcid; 
		sscanf(ownedidhexa, "%x", &isnpcid); 
		mob->isnpc = ( isnpcid == 255 );

//		afficherkeys();

		parsenext();
		parseread();
		
		//do nothing with the unknowns

		//handle the newer logs with an additional unknown
		if(nbkeys() > 0 && getkey(0) == "Unknown_10B") {
			//skip over the extra unknowns line too
			parsenext();
			parseread();
		}

		parsenext();
		parseread();

/*		parsenext();
		parseread();
		parsenext();
		parseread();*/


		
	//hack... if we spawn a mob who's ID is taken, assume the old mob with that ID was killed and we missed it some how...
	cmob* existing_mob = NULL;
	existing_mob = currentlog->mobinit->getmobbyid( mob->id );
	if ( existing_mob == NULL && currentlog->mobadd != NULL) {
		existing_mob = currentlog->mobadd->getmobbyid( mob->id );
	}
	if(existing_mob != NULL) {
		if(!existing_mob->killed)
			existing_mob->killed = true;
	}
}



void CEQBuilderDlg::getDoorData()
{
//	afficherkeys();

	if ( doors != NULL ) {
		return;
	}

	parsenext();
	parseread();

	doors = new door_list();

	while ( !isFinSection() ) {

		cdoor* door = new cdoor();

//		afficherkeys();

		door->id = atoi( getkey(1) );
		door->zone = zonename;
		door->model = getkey(5);
		door->type = atoi( getkey(3) );

		parsenext();
		parseread();

//		afficherkeys();

		door->loc = new cloc();
		door->loc->x = atof( getkey(1) );
		door->loc->y = atof( getkey(2) );
		door->loc->z = atof( getkey(3) );	//this might need to be divided by 10 like everything else...
		door->loc->heading = atof( getkey(4) );

		parsenext();
		parseread();

//		afficherkeys();

		door->teleportid = atoi( getkey(8) );
	
		doors->add( door );

		parsenext();
		parsenext();
		parseread();
		parseread();


	}

}

void CEQBuilderDlg::getTeleporterData()
{

	if ( teleports != NULL ) {
		return;
	}

	parsenext();
	parseread();

	teleports = new teleport_list();

	while ( !isFinSection() ) {

		cteleport* tp = new cteleport();

//		afficherkeys();

		tp->id = atoi( getkey(1) );

		CString zoneidhexa = getkey(3);
		int zoneid; 
		sscanf(zoneidhexa, "%x", &zoneid); 
		tp->zone = db->getZoneName( zoneid );

		tp->loc = new cloc();
		tp->loc->x = atof( getkey(5) );
		tp->loc->y = atof( getkey(6) );
		tp->loc->z = atof( getkey(7) );	//this might need to be divided by 10 like everything else...
		tp->loc->heading = atof( getkey(8) );

		teleports->add( tp );

		parsenext();
		parseread();


	}

}

void CEQBuilderDlg::getMovementData()
{
	cwaypoint* move = new cwaypoint();


	CString mobidhexa = getkey(2);
	int mobid;
	sscanf(mobidhexa, "%x", &mobid );
	move->id = mobid;

	move->loc = new cloc();
	move->loc->x = atof( getkey(5) );
	move->loc->y = atof( getkey(6) );
	move->loc->z = atof( getkey(7) ) / 10.0f;	//eqcollector seems to log this Z coord high;
	move->loc->heading = atof( getkey(12) );

	cmob* mob = NULL;

	if(currentlog->mobinit != NULL)
		mob = currentlog->mobinit->getmobbyid( move->id );
	if ( mob == NULL && currentlog->mobadd != NULL) {
		mob = currentlog->mobadd->getmobbyid( move->id );
	}

	if ( mob!=NULL ) {

		mob->moved = true;

		if ( ( currentlog->type == 0 ) || ( currentlog->type == 2 ) ) {
			delete [] move;
			return;
		}

		if ( mob->waypoints == NULL ) {
			mob->waypoints = new waypoint_list();
		}

		mob->waypoints->add( move );

	} else {
		delete [] move;
	}

	parsenext();
	parseread();

}

void CEQBuilderDlg::getKilledData()
{

	parsenext();
	parseread();

	CString mobidhexa = getkey(1);
	int killid;
	sscanf(mobidhexa, "%x", &killid );

	cmob* mob = NULL;

	if(currentlog->mobinit != NULL)
		mob = currentlog->mobinit->getmobbyid( killid );
	if ( mob == NULL && currentlog->mobadd != NULL) {
		mob = currentlog->mobadd->getmobbyid( killid );
	}

	if ( mob != NULL ) {
		mob->killed = true;
	}

	parsenext();
	parseread();

}

void CEQBuilderDlg::getWaypointData() {

	cwaypoint* wp = new cwaypoint();

	CString mobidhexa = getkey(2);
	int mobid;
	sscanf(mobidhexa, "%x", &mobid );
	wp->id = mobid;

	wp->loc = new cloc();
	wp->loc->x = atof( getkey(5) );
	wp->loc->y = atof( getkey(6) );
	wp->loc->z = atof( getkey(7) ) / 10.0f;	//eqcollector seems to log this Z coord high
	wp->loc->heading = atof( getkey(10) );

	wp->pause = true;

	cmob* mob = NULL;
	
	if(currentlog->mobinit != NULL)
		mob = currentlog->mobinit->getmobbyid( wp->id );
	if ( mob == NULL && currentlog->mobadd != NULL) {
		mob = currentlog->mobadd->getmobbyid( wp->id );
	}

	if ( mob != NULL ) {

		mob->roamed = true;

		if ( ( currentlog->type == 0 ) || ( currentlog->type == 2 ) ) {
			delete [] wp;
			return;
		}

		if ( mob->waypoints == NULL ) {
			mob->waypoints = new waypoint_list();
		}

		if ( mob->waypoints->nbpauses() == 0 ) {
			if ( isMovingAtInit( mob ) == true ) {
				mob->loc->x = wp->loc->x;
				mob->loc->y = wp->loc->y;
				mob->loc->z = wp->loc->z;
				mob->loc->heading = wp->loc->heading;
			}
		}

		mob->waypoints->add( wp );

	} else {

		delete [] wp;

	}

	parsenext();
	parseread();

}

void CEQBuilderDlg::getShopOpenData() {
	//make sure we are collecting merchant data.
	/*if ( ( currentlog->type == 0 ) || ( currentlog->type == 2 ) ) {
		parsenext();
		parseread();
		return;
	}*/

	cmerchant* m = new cmerchant();

	CString mobidhexa = getkey(4);
	int mobid;
	sscanf(mobidhexa, "%x", &mobid );

	m->rate = atof( getkey(10) );

	cmob* mob = NULL;
	
	if(currentlog->mobinit != NULL)
		mob = currentlog->mobinit->getmobbyid( mobid );
	if ( mob == NULL && currentlog->mobadd != NULL) {
		mob = currentlog->mobadd->getmobbyid( mobid );
	}

	if ( mob == NULL ) {
		delete m;
		return;
	}
	
	m->id = db->getNextMerchantID();
	mob->npc->merchant = m;
	m->owner = mob->npc;
	cur_merchant = m;
	
	parsenext();
	parseread();

}

void CEQBuilderDlg::getShopItemData() {
	//skip the line that got us here, we dont need it.
	parsenext();
	parseread();


	int slot_id = 0;
	int merchant_slot = 0;
	int item_id = 0;
	
	CString item = value(0, nbkeys());
	
	LPTSTR pstr, tok;
	TCHAR seperator[] = _T("|");

	pstr = item.GetBuffer(0);
	
	int index = 0;
	tok = _tcstok(pstr, seperator);
	while(tok != NULL) {
		if(index == 2) {
			slot_id = atoi(tok);
		} else if(index == 5) {
			merchant_slot = atoi(tok);
		} else if(index == 11 || index == 12) {
			item_id = atoi(tok);
			if(item_id >= 1000)
				break;
		} else if(index > 13)
			break;
		index++;
		tok = _tcstok(NULL, seperator);
	}
	item.ReleaseBuffer(-1);

	
	if(cur_merchant != NULL && item_id != 0) {
		cur_merchant->items[slot_id] = item_id;
	}
	
	parsenext();
	parseread();
}

StringArray* CEQBuilderDlg::formater( CString &s ) {
/*
	CString s2;
	CString c;

CString original(s);
string res;*/

	StringArray* sformate = new StringArray();

	int r;
	int len = s.GetLength();
	char *buffer = s.GetBuffer(len);
	char *word = NULL;
	for(r = 0; r < len; r++) {
		switch(*buffer) {
		case ' ':
		case '%':
		case '\'':
		case '=':
		case '>':
		case ',':
		case '[':
		case ']':
		case '}':
		case '{':
		case ':':
		case '(':
		case ')':
			if(word != NULL) {
				//we had a word going, terminate it and record it.
				*buffer = '\0';
				sformate->Add(CString(word));
				word = NULL;	//reset to no word...
			} else {
				//no word... do nothing?
			}
			break;
		default:
			if(word == NULL)
				word = buffer;	//start a new word
			break;
		}
		buffer++;
	}
	if(word != NULL)
		sformate->Add(CString(word));


	/*
	int i = 0;
	int ok = false;

	while ( i < s.GetLength() ) {
		c = s.Mid( i, 1 );
		
		if ( ( c == " " ) || ( c == "%" ) || ( c == "'" ) || ( c == "=" ) || ( c == ">" ) || ( c == ",") || ( c == "[" )|| ( c == "]" )|| ( c == "}" ) || ( c == "{" ) || ( c == ":" ) || ( c == "(" ) || ( c == ")" ) ) {
			if ( ok ) {
				sformate->Add( s2 );
				ok = false;
			}
		} else {

			if ( !ok ) {
				s2 = c;
				ok = true;
			} else {
				CString temp = s2;
				s2.Format("%s%s", temp, c );
			}
		}
		i++;
	}
	sformate->Add( s2 );
*/
	return sformate;
}

int CEQBuilderDlg::findnext( CString tofind, int start )
{
	CString temp;
	int i = start;
	while ( temp != tofind )
	{
		temp = getkey(i);
		i++;
	}

	return i-1;
}

CString CEQBuilderDlg::value( int start, int size ) {

	CString value = getkey( start );
	
	if((currentkeys->GetSize()-start) < size) {
		size = currentkeys->GetSize()-start;
	}
	for ( int i=1; i<size; i++ ) {
		value += " ";
		value += getkey( start+i );
	}

	return value;
}

CString CEQBuilderDlg::getkey( int index ) { 
	return currentkeys->GetAt( index );
}

int CEQBuilderDlg::nbkeys() { 
	return currentkeys->GetSize();
}

