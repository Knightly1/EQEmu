// database.cpp : implementation file
//

#include "stdafx.h"
#include "database.h"
#include  <io.h>
#include <string.h>

#include <mysqld_error.h>

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define strncasecmp	_strnicmp
#define strcasecmp  _stricmp

/////////////////////////////////////////////////////////////////////////////
// database

database::database()
{
	mysql_init(&mysql);
	this->connected = false;

	_merchantid = 0;
	_spawn2id = 0;

	
	m_raceCachesFilled = false;
	m_classCachesFilled = false;
	map<uint8, string> m_raceToNames;
	map<uint8, string> m_classToNames;
	map<string, uint8> m_raceToIDs;
	map<string, uint8> m_classToIDs;
}

database::~database()
{
	mysql_close(&mysql);
}

void database::Connexion( CString host, CString user, CString password, CString database ) {

	unsigned long port=0;
	bool compression = false;
	const char *unix_socket = NULL; 
	unsigned long client_flag = 0; 

    //connection sur le serveur
    if ( mysql_real_connect(&mysql,host,user,password,database,port,unix_socket,client_flag) != 0 ) {
		connected = true;
	} else {
		connected = false;
	}

}

void database::runquery( CString query, MYSQL_RES** result, unsigned __int64* affected_rows, unsigned __int64* last_insert_id ) {

	unsigned long querylen = strlen( query );

	if (!mysql_real_query(&mysql, query, querylen)) {
		if (result && mysql_field_count(&mysql)) {
			*result = mysql_store_result(&mysql);
		} else if (result)
			*result = 0;
		if (affected_rows)
			*affected_rows = mysql_affected_rows(&mysql);
		if (last_insert_id)
			*last_insert_id = mysql_insert_id(&mysql);
	}

}


zone_list* database::GetZones() {
	CString query;
	MYSQL_RES* result;
	MYSQL_ROW row;

	czone* zone;
	zone_list* zones = new zone_list();

	query = "SELECT zoneidnumber, short_name, long_name FROM zone ORDER BY long_name";
	runquery( query, &result, NULL, NULL );

	while ( row = mysql_fetch_row( result ) ) {
		zone = new czone();
		zone->id = atoi( row[0] );
		zone->short_name = row[1];
		zone->long_name = row[2];
		zones->add(zone);
	}

	return zones;
}

void database::ClearLogs( CString zonename )
{
	CString query;
	MYSQL_RES* result;
//	MYSQL_ROW row;

	query.Format( "DELETE FROM logs WHERE zone='%s'", zonename );
	runquery( query, &result, NULL, NULL );
}

log_list* database::GetLogs( CString zonename )
{
	CString query;
	MYSQL_RES* result;
	MYSQL_ROW row;

	clog* log;
	log_list* zonelogs = new log_list();

	query.Format( "SELECT name, type FROM logs WHERE zone='%s'", zonename );
	runquery( query, &result, NULL, NULL );

	while ( row = mysql_fetch_row( result ) ) {
		log = new clog();
		log->name = row[0];
		log->type = (logType) atoi( row[1] );

		if(strstr(row[0], ".bf") == NULL && strstr(row[0], ".BF")) {
			zonelogs->txt_add( log );
		} else {
			zonelogs->bf_add( log );
		}
	}

	return zonelogs;

}

bool database::logexists( CString zonename, CString filename ) {

	CString query;
	MYSQL_RES* result;
	unsigned __int64 nbrows;

	query.Format( "SELECT * FROM logs WHERE zone='%s' AND name='%s'", zonename, filename );
	runquery( query, &result, &nbrows, NULL );
	
	return (nbrows == 1);

}

void database::SaveLog( CString zonename, CString filename )
{
	CString query;
	MYSQL_RES* result;

	query.Format( "INSERT INTO logs ( zone, name ) VALUES ( '%s', '%s' )", zonename, filename );
	runquery( query, &result, NULL, NULL );
}

void database::UpdateLog( CString zonename, clog* log )
{
	CString query;
	MYSQL_RES* result;

	query.Format( "UPDATE logs SET type=%d WHERE zone='%s' AND name='%s'", log->type, zonename, log->name );
	runquery( query, &result, NULL, NULL );
}


CString database::getZoneName( int zoneid )
{
	CString query;
	MYSQL_RES* result;
	MYSQL_ROW row;

	query.Format( "SELECT short_name FROM zone WHERE zoneidnumber=%d", zoneid );
	runquery( query, &result, NULL, NULL );	

	row = mysql_fetch_row( result );

	return row[0];

}

int database::GetRace( CString nomrace )
{
	CString query;
//	MYSQL_RES* result;
//	MYSQL_ROW row;

	if ( nomrace == "Froglok 330" ) {
		return 330;
	}

	if ( nomrace.Left( 6 ) == "Will O" ) {
		return 69;
	}

	if(m_raceCachesFilled) {
		map<string, uint8>::iterator res;
		res = m_raceToIDs.find(string((const char *)nomrace));
		if(res == m_raceToIDs.end()) {
			return(-1);
		}
		return(res->second);
	}

	LoadRaceCache();

	return(GetRace(nomrace));
}

int database::getclasse( CString classenom ) {

	CString query;
	MYSQL_RES* result;
	MYSQL_ROW row;

	classenom.MakeLower();
	
	if(m_classCachesFilled) {
		map<string, uint8>::iterator res;
		res = m_classToIDs.find(string((const char *)classenom));
		if(res == m_classToIDs.end()) {
			return(0);
		}
		return(res->second);
	}

	LoadClassCache();

	return(getclasse(classenom));

}

void database::LoadRaceCache() {
	CString query;
	MYSQL_RES* result;
	MYSQL_ROW row;

	query.Format( "SELECT name,id FROM races" );
	runquery( query, &result, NULL, NULL );	

	while((row = mysql_fetch_row( result ))) {
		uint8 id = atoi(row[1]);
		m_raceToIDs[row[0]] = id;
		m_raceToNames[id] = row[0];
	}

	m_raceCachesFilled = true;
}

void database::LoadClassCache() {
	CString query;
	MYSQL_RES* result;
	MYSQL_ROW row;

	query.Format( "SELECT name,id FROM classes" );
	runquery( query, &result, NULL, NULL );	

	while((row = mysql_fetch_row( result ))) {
		CString n(row[0]);
		n.MakeLower();
		string s((const char *)n);
		uint8 id = atoi(row[1]);
		m_classToIDs[s] = id;
		m_classToNames[id] = s;
	}

	m_classCachesFilled = true;
}

void database::InvalidateCaches() {
	m_raceCachesFilled = false;
	m_classCachesFilled = false;
}

void database::extractdoors( door_list* doors, teleport_list* teleports, CString path, CString zonename, bool delquerryok )
{
	int id;
	CString zone;
	int opentype;
	CString model;
	double pos_x;
	double pos_y;
	double pos_z;
	double pos_heading;
	int teleportid;
	CString dest_zone;
	double dest_x;
	double dest_y;
	double dest_z;
	double dest_heading;

	CString outputrep;
	outputrep.Format( "%s\\output", path  );

	if( _access( outputrep, 0 ) == -1 ) {
		CreateDirectory( outputrep, NULL );
	}

	CString zonerep;
	zonerep.Format( "%s\\%s", outputrep, zonename );

	if( _access( zonerep, 0 ) == -1 ) {
		CreateDirectory( zonerep, NULL );
	}


	CString filename;
	filename.Format( "%s\\%s-doors.sql", zonerep, zonename );

	CFile f;
	f.Open( filename, CFile::modeCreate | CFile::modeWrite, NULL );
	
	CString query;
	if ( delquerryok == true ) {
		query.Format( "DELETE FROM `doors` WHERE zone='%s';\n", zonename );
		f.Write( query, lstrlen(query) );
	}

	for ( int i=0; i<doors->getsize();i++ ) {
		cdoor* door = doors->get(i);
		id = door->id;
		zone = door->zone;
		opentype = door->type;
		model = door->model;
		pos_x = door->loc->x;
		pos_y = door->loc->y;
		pos_z = door->loc->z;
		pos_heading = door->loc->heading;
		teleportid = door->teleportid;
		if ( teleportid > 0 ) {
			for( int j = 0;j<teleports->getsize();j++ ) {
				cteleport* tp = teleports->get(j);
				if ( tp->id == teleportid ) {
					dest_zone = tp->zone;
					dest_x = tp->loc->x;
					dest_y = tp->loc->y;
					dest_z = tp->loc->z;
					dest_heading = tp->loc->heading;
					break;
				}
			}
		}
		CString ckeys;
		CString cvalues;
		if ( teleportid > 0 ) {
			ckeys = "`doorid`, `zone`, `name`, `pos_y`, `pos_x`, `pos_z`, `heading`, `opentype`, `dest_zone`, `dest_x`, `dest_y`, `dest_z`, `dest_heading`";
			cvalues.Format( "'%d', '%s', '%s', '%f', '%f', '%f', '%f', '%d', '%s', '%f', '%f', '%f', '%f'", id, zone, model, pos_x, pos_y, pos_z, pos_heading, opentype, dest_zone, dest_y, dest_x, dest_z, dest_heading );
		} else {
			ckeys = "`doorid`, `zone`, `name`, `pos_y`, `pos_x`, `pos_z`, `heading`, `opentype`";
			cvalues.Format( "'%d', '%s', '%s', '%f', '%f', '%f', '%f', '%d'", id, zone, model, pos_x, pos_y, pos_z, pos_heading, opentype );
		}

		query = "INSERT INTO `doors` (" + ckeys + ") VALUES (" + cvalues + ");\n";

		f.Write( query, lstrlen(query) );
	}

	f.Close();

}

void database::extractteleports( teleport_list* teleports, CString path, CString zonename, bool delquerryok )
{
	int number;
	CString zone;
	double tp_x;
	double tp_y;
	double tp_z;
	double tp_heading;
	CString tp_zone;

	CString outputrep;
	outputrep.Format( "%s\\output", path  );

	if( _access( outputrep, 0 ) == -1 ) {
		CreateDirectory( outputrep, NULL );
	}

	CString zonerep;
	zonerep.Format( "%s\\%s", outputrep, zonename );

	if( _access( zonerep, 0 ) == -1 ) {
		CreateDirectory( zonerep, NULL );
	}


	CString filename;
	filename.Format( "%s\\%s-teleports.sql", zonerep, zonename );

	CFile f;
	f.Open( filename, CFile::modeCreate | CFile::modeWrite, NULL );

	CString query;
	if ( delquerryok == true ) {
		query.Format( "DELETE FROM `zone_points` WHERE zone='%s';\n", zonename );
		f.Write( query, lstrlen(query) );
	}

	for( int j = 0;j<teleports->getsize();j++ ) {
		cteleport* tp = teleports->get(j);
		number = tp->id;
		zone = zonename;
		tp_zone = tp->zone;
		tp_x = tp->loc->x;
		tp_y = tp->loc->y;
		tp_z = tp->loc->z;
		tp_heading = tp->loc->heading;

		CString ckeys;
		CString cvalues;
		ckeys = "`zone`, `number`, `target_y`, `target_x`, `target_z`, `target_heading`, `target_zone`";
		cvalues.Format( "'%s', '%d', '%f', '%f', '%f', '%f', '%s'", zone, number, tp_x, tp_y, tp_z, tp_heading, tp_zone );

		query = "INSERT INTO `zone_points` (" + ckeys + ") VALUES (" + cvalues + ");\n";

		f.Write( query, lstrlen(query) );
	}

	f.Close();

}

void database::extractnpcs( npc_list* npcs, CString path, CString zonename, bool delquerryok, bool usedb ) {

	int id;
	CString nom;
	int level;
	double size;
	double walkspeed;
	double runspeed;
	int race;
	int classe;
	int bodytype;
	int hp;
	int gender;
	int texture;
	int helmtexture;
	int face;
    int luclin_hairstyle;
	int luclin_haircolor;
	int luclin_eyecolor;
	int luclin_eyecolor2;
	int luclin_beard;
	int luclin_beardcolor;

	CString outputrep;
	outputrep.Format( "%s\\output", path  );

	if( _access( outputrep, 0 ) == -1 ) {
		CreateDirectory( outputrep, NULL );
	}

	CString zonerep;
	zonerep.Format( "%s\\%s", outputrep, zonename );

	if( _access( zonerep, 0 ) == -1 ) {
		CreateDirectory( zonerep, NULL );
	}


	CString filename;
	filename.Format( "%s\\%s-npcs.sql", zonerep, zonename );

	CFile f;
	f.Open( filename, CFile::modeCreate | CFile::modeWrite, NULL );

	CString query;

	if ( delquerryok == true ) {
		query = "DELETE FROM npc_types USING npc_types,spawn2,spawnentry WHERE npc_types.id=spawnentry.npcID AND spawn2.spawngroupID=spawnentry.spawngroupID AND spawn2.zone='"+ zonename +"';\n";
		f.Write( query, lstrlen(query) );
	}

	for( int i = 0;i<npcs->getsize();i++ ) {
		cnpc* npc = npcs->get(i);
		
		if(npc->db)
			continue;   //it came from the DB, it dosent need to go in it again
		
		id = npc->id;
		nom = npc->nom;
		level = npc->level;
		gender = npc->gender;
		size = npc->size;
		walkspeed = npc->walkspeed;
		runspeed = npc->runspeed;
		race = npc->race;
		classe = npc->classe;
		bodytype = npc->type;
		hp = level*(10+level);
		texture = npc->texture;
		helmtexture = npc->helmtexture;
		face = npc->face;
		luclin_hairstyle = npc->luclin_hairstyle;
		luclin_haircolor = npc->luclin_haircolor;
		luclin_eyecolor = npc->luclin_eyecolor;
		luclin_eyecolor2 = npc->luclin_eyecolor2;
		luclin_beard = npc->luclin_beard;
		luclin_beardcolor = npc->luclin_beardcolor;
		
		if ( usedb && isnpcindb( id ) ) {
			continue;
		}

  		CString ckeys;
		CString cvalues;
		ckeys = "`id`, `name`, `level`, `gender`, `size`, `walkspeed`, `runspeed`, "
		"`race`, `class`, `bodytype`, `hp`, `texture`, `helmtexture`, `face`, "
		"`luclin_hairstyle`, `luclin_haircolor`, `luclin_eyecolor`, `luclin_eyecolor2`, `luclin_beard`, `luclin_beardcolor`";
		cvalues.Format( "'%d', '%s', '%d', '%d', '%f', '%f', '%f', "
		"'%d', '%d', '%d', '%d', '%d', '%d', '%d', "
		"'%d', '%d', '%d', '%d', '%d', '%d'",
			id,nom,level,gender,size,walkspeed,runspeed,
			race,classe,bodytype,hp,texture,helmtexture,face,
			luclin_hairstyle,luclin_haircolor,luclin_eyecolor,luclin_eyecolor2,luclin_beard,luclin_beardcolor );

		query = "INSERT INTO `npc_types` (" + ckeys + ") VALUES (" + cvalues + ");\n";

		f.Write( query, lstrlen(query) );
	}

	f.Close();

}

bool database::isnpcindb( int npcid ) {

	CString query;
	MYSQL_RES* result;
	unsigned __int64 nbrows;

	query.Format( "SELECT id FROM npc_types WHERE id='%d'", npcid );
	runquery( query, &result, &nbrows, NULL );

	return ( nbrows == 1 );

}

//OMG this is the most rediculous method I have ever seen.
int database::getfreenpcid( int start, IntArray* usednpcids ) {

	CString query;
	MYSQL_RES* result;
	unsigned __int64 nbrows;

	int npcid = start;
	bool free;
	
	while ( true ) {

		free = true;

		query.Format( "SELECT id FROM npc_types WHERE id='%d'", npcid );
		runquery( query, &result, &nbrows, NULL );

		if ( nbrows == 0 ) {
			for ( int i=0; i<usednpcids->GetSize(); i++ ) {
				if ( npcid == usednpcids->GetAt(i) ) {
					free = false;
				}
			}
			if ( free == true ) {
				usednpcids->Add( npcid );
				break;
			}
		}

		npcid++;

	}

	return npcid;

}

int database::getnpcid( cnpc* npc ) {

	CString query;
	MYSQL_RES* result;
	MYSQL_ROW row;
	unsigned __int64 nbrows = 0;

	query.Format( "SELECT id FROM npc_types WHERE name='%s' AND level='%d' AND gender='%d' AND size='%f' AND race='%d' AND class='%d' AND bodytype='%d' AND texture='%d' AND helmtexture='%d'", npc->nom, npc->level, npc->gender, npc->size, npc->race, npc->classe, npc->type, npc->texture, npc->helmtexture );
	runquery( query, &result, &nbrows, NULL );

	if ( nbrows == 1 ) {
		row = mysql_fetch_row( result );
		return atoi(row[0]);
	}

	return 0;

}

int database::getbestgroupid( IntArray* usedgroupids, IntArray* npcids ) {

	int deca = 0;
	int sgid = npcids->GetAt(0);
	int newsgid;
	bool found = false;

	while ( found == false ) {
		newsgid = sgid+deca;

		found = true;
		for ( int i=0; i<usedgroupids->GetSize(); i++ ) {
			if ( newsgid == usedgroupids->GetAt(i) ) {
				found = false;
				break;
			}
		}

			
		deca++;

	}

	return newsgid;

}

int database::getNextSpawn2ID(int min) {
	if(_spawn2id < min)
		_spawn2id = min;
	return(_spawn2id++);
}

void database::extractASpawn(cspawn *spawn, CString &zonename, CFile &f) {
	int spawngroup_id;
	CString spawngroup_name;
	int spawnentry_spawngroupid;
	int spawnentry_npcid;
	int spawnentry_chance;

	int spawn2_id;
	int spawn2_spawngroupid;
	CString zone;
	double spawn2_x;
	double spawn2_y;
	double spawn2_z;
	double spawn2_heading;
	int spawn2_pathgrid;
	IntArray* npcids;
	
	zone = zonename;
	if ( spawn->grid != NULL ) {
		spawn2_pathgrid = spawn->grid->id;
	} else {
		spawn2_pathgrid = 0;
	}

	// npcids
	npcids = new IntArray();
	for ( int j = 0;j<spawn->mobs->getsize(); j++ ) {
		npcids->Add( spawn->mobs->get(j)->npc->id );
	}

	// groupid
/*		spawn2_spawngroupid = getbestgroupid( usedgroupids, npcids );
	usedgroupids->Add( spawn2_spawngroupid );
	spawn2_id = spawn2_spawngroupid;*/
	
	CString query;
  	CString ckeys;
	CString cvalues;

	if(!spawn->db) {
		//spawn group is not in the DB, put it there...
		spawn2_spawngroupid = spawn->id;
		
		//create spawn2 entries
		int r,locmax;
		locmax = spawn->locs->getsize();
		for(r = 0; r < locmax; r++) {
			cloc *loc = spawn->locs->get(r);
			spawn2_x = loc->x;
			spawn2_y = loc->y;
			spawn2_z = loc->z;
			spawn2_heading = loc->heading;
			spawn2_id = getNextSpawn2ID(spawn->id);

			ckeys = "`id`, `spawngroupID`, `zone`, `x`, `y`, `z`, `heading`, `pathgrid`";
			cvalues.Format( "'%d', '%d', '%s', '%f', '%f', '%f', '%f', '%d'",spawn2_id,spawn2_spawngroupid,zone,spawn2_y,spawn2_x,spawn2_z,spawn2_heading,spawn2_pathgrid );
			query = "INSERT INTO `spawn2` (" + ckeys + ") VALUES (" + cvalues + ");\n";
			f.Write( query, lstrlen(query) );
		}

		//create spawn group
		spawngroup_id = spawn2_spawngroupid;
		spawngroup_name.Format( "%s%d", zone, spawn2_spawngroupid );

		ckeys = "`id`, `name`";
		cvalues.Format( "'%d', '%s'",spawngroup_id,spawngroup_name );
		query = "INSERT INTO `spawngroup` (" + ckeys + ") VALUES (" + cvalues + ");\n";
		f.Write( query, lstrlen(query) );
	}
	
	int chance_cut = 100 / spawn->mobs->getsize();
	int chance_change = 100 % spawn->mobs->getsize();
	//generate each spawn entry
	for ( int k = 0;k<spawn->mobs->getsize(); k++ ) {
		cmob *m = spawn->mobs->get(k);
		if ( !m->valid ) {
			continue;   //skip invalids
		}
		if(m->db)
			continue;   //skip entries allready in the db

		spawnentry_spawngroupid = spawn2_spawngroupid;
		spawnentry_npcid = npcids->GetAt(k);
		spawnentry_chance = chance_cut;
		if(k < chance_change)
			spawnentry_chance++;

		ckeys = "`spawngroupID`, `npcID`, `chance`";
		cvalues.Format( "'%d', '%d', '%d'",spawnentry_spawngroupid,spawnentry_npcid,spawnentry_chance );
		query = "INSERT INTO `spawnentry` (" + ckeys + ") VALUES (" + cvalues + ");\n";
		f.Write( query, lstrlen(query) );

	}

	delete npcids;
}

void database::extractspawns( spawn_list* fspawns, spawn_list* gspawns, CString path, CString zonename, bool delquerryok ) {

	CString outputrep;
	outputrep.Format( "%s\\output", path  );

	if( _access( outputrep, 0 ) == -1 ) {
		CreateDirectory( outputrep, NULL );
	}

	CString zonerep;
	zonerep.Format( "%s\\%s", outputrep, zonename );

	if( _access( zonerep, 0 ) == -1 ) {
		CreateDirectory( zonerep, NULL );
	}

	CString filename;
	filename.Format( "%s\\%s-spawns.sql", zonerep, zonename );

	CFile f;
	f.Open( filename, CFile::modeCreate | CFile::modeWrite, NULL );

	CString query;
  	CString ckeys;
	CString cvalues;

//	IntArray* usedgroupids = new IntArray();

	if ( delquerryok == true ) {
		query.Format( "DELETE FROM spawngroup WHERE name LIKE '%s%%';\nDELETE FROM spawnentry USING spawn2,spawnentry WHERE spawnentry.spawngroupID = spawn2.id AND spawn2.zone = '%s';\nDELETE FROM spawn2 WHERE zone = '%s';\n", zonename, zonename, zonename );
		f.Write( query, lstrlen(query) );
	}

	for( int i = 0;i<fspawns->getsize();i++ ) {
		cspawn* spawn = fspawns->get(i);
		extractASpawn(spawn, zonename, f);
	}

	for( int l = 0;l<gspawns->getsize();l++ ) {
		cspawn* spawn = gspawns->get(l);
		extractASpawn(spawn, zonename, f);
	}

//	delete usedgroupids;

	f.Close();

}

void database::extractgrids( grid_list* grids, CString path, CString zonename, uint32 zone_id, bool delquerryok ) {

	int id;
	int type=0;
	int type2=0;
	double wp_x;
	double wp_y;
	double wp_z;
	double wp_heading;
	int wp_pause;

	CString outputrep;
	outputrep.Format( "%s\\output", path  );

	if( _access( outputrep, 0 ) == -1 ) {
		CreateDirectory( outputrep, NULL );
	}

	CString zonerep;
	zonerep.Format( "%s\\%s", outputrep, zonename );

	if( _access( zonerep, 0 ) == -1 ) {
		CreateDirectory( zonerep, NULL );
	}

	CString filename;
	filename.Format( "%s\\%s-grids.sql", zonerep, zonename );

	CFile f;
	f.Open( filename, CFile::modeCreate | CFile::modeWrite, NULL );

	CString query;
  	CString ckeys;
	CString cvalues;

	if ( delquerryok == true ) {
//disabled because im too lazy to write the query to clear out grid_entries
//		query.Format( "DELETE FROM grid USING grid,spawn2 WHERE grid.id=spawn2.pathgrid AND spawn2.zone='%s';\n", zonename );
//		f.Write( query, lstrlen(query) );
	}

	for( int i = 0;i<grids->getsize();i++ ) {
		cgrid* grid = grids->get(i);
		
		if(grid->db)
			continue;   //skip entries allready in the db
		
		id = grid->id;
		type = 3;
		type2 = 0;
		ckeys = "`id`, `zoneid`, `type`, `type2`";
		cvalues.Format( "%d, %d, %d, %d", id, zone_id, type, type2 );
		query = "INSERT INTO `grid` (" + ckeys + ") VALUES (" + cvalues + ");\n";
		f.Write( query, lstrlen(query) );

		int goal = grid->waypoints->getsize();
		for ( int j=0;j<goal;j++ ) {
				cwaypoint* wp = grid->waypoints->get(j);
				wp_x = wp->loc->x;
				wp_y = wp->loc->y;
				wp_z = wp->loc->z;
				wp_heading = wp->loc->heading;
				if ( wp->pause == true ) {
					wp_pause = 45;
				} else {
					wp_pause = 0;
				}
				
				ckeys = "`gridid`, `zoneid`, `number`, `x`, `y`, `z`, `heading`, `pause`";
				cvalues.Format( "%d, %d, %d, %f, %f, %f, %f, %d", id, zone_id, j, wp_y, wp_x, wp_z, wp_heading, wp_pause);
				query = "INSERT INTO `grid_entries` (" + ckeys + ") VALUES (" + cvalues + ");\n";
				f.Write( query, lstrlen(query) );
		}
	}

	f.Close();

}

int database::getNextMerchantID() {
	if(_merchantid == 0) {
		CString query;
		MYSQL_RES* result;
		MYSQL_ROW row;

		query.Format( "SELECT MAX(merchantid) FROM merchantlist" );
		runquery( query, &result, NULL, NULL );	

		row = mysql_fetch_row( result );
		if(row == NULL || row[0] == NULL) {
			//should print an error...
			_merchantid = 2;
			return(1);
		}

		_merchantid = atoi(row[0]);
		if(_merchantid == 0) {
			_merchantid = 2;
			return(1);
		}
	}
	return(_merchantid++);
}

void database::extractmerchants( merchant_list* merchants, CString path, CString zonename, bool delquerryok )
{

	CString outputrep;
	outputrep.Format( "%s\\output", path  );

	if( _access( outputrep, 0 ) == -1 ) {
		CreateDirectory( outputrep, NULL );
	}

	CString zonerep;
	zonerep.Format( "%s\\%s", outputrep, zonename );

	if( _access( zonerep, 0 ) == -1 ) {
		CreateDirectory( zonerep, NULL );
	}


	CString filename;
	filename.Format( "%s\\%s-merchants.sql", zonerep, zonename );

	CFile f;
	f.Open( filename, CFile::modeCreate | CFile::modeWrite, NULL );

	CString query;
/*	if ( delquerryok == true ) {
		query.Format( "DELETE FROM `zone_points` WHERE zone='%s';\n", zonename );
		f.Write( query, lstrlen(query) );
	}
*/
	for( int j = 0;j<merchants->getsize();j++ ) {
		cmerchant* cm = merchants->get(j);
		if(cm->id == 0) {
			continue;	//never used or referenced, why bother....
		}
		
		if(cm->db)
			continue;   //allready in the database

		int slot = 1;
		map<uint16, uint32>::iterator cur,end;
		CString ckeys;
		CString cvalues;
		ckeys = "`merchantid`, `slot`, `item`";

		cur = cm->items.begin();
		end = cm->items.end();
		for(; cur != end; cur++, slot++) {
			cvalues.Format( "'%d', '%d', '%d'", cm->id, slot, cur->second);

			query = "INSERT INTO `merchantlist` (" + ckeys + ") VALUES (" + cvalues + ");\n";

			f.Write( query, lstrlen(query) );
		}
	}
	f.Close();

}

void database::loadZone(int zoneid, CString zonename, CEQBuilderDlg *d) {
//this code needs a lot of work before it will be ready...
/*	CString query;
	MYSQL_RES* result;
	MYSQL_ROW row;
	
	
	//fake a new log, stick the stuff in it
	clog *l = new clog();
	l->is_db_file = true;
	l->is_build_file = false;
	l->name = "From DB";
	l->mobinit = new mob_list();
	l->mobadd = new mob_list();
	l->type = 1;
	
	//Load all mobs for this zone, 
	query.Format( "SELECT spawngroup.id,npc_types.id,"
	"	spawn2.grid_id,npc_types.merchant_id "
	" FROM spawn2,spawngroup,spawnentry,npc_types"
	" WHERE spawnentry.npc_id = npc_types.id"
	" AND spawngroup.id = spawnentry.spawngroup_id"
	" AND spawn2.spawngroup_id = spawngroup.id"
	" AND spawn2.zoneid = %d"
	" ORDER BY spawngroup.id,spawnentry.id,npc_types.id", zoneid);
	
	runquery( query, &result, NULL, NULL );

	int lastgroup = -1;
	cspawn *curspawn = NULL;
	while ((row = mysql_fetch_row( result ))) {
		int idx = 0;
		
		int curgroup = atoi(row[idx++]);
		int npc_id = atoi(row[idx++]);
		int grid_id = atoi(row[idx++]);
		int merchant_id = atoi(row[idx++]);
		
		if(curgroup != lastgroup) {
			//spawn group changed...
			if(curspawn != NULL) {
				//we had a spawn group, its complete now...
				
				//now we can add it to our log.
				//put them in the add list because it does less with probabilities
				//and we shouldent need the probabilities anyhow
				l->mobadd->add(
			}
			//make a new, empty spawn group
			curspawn = new cspawn();
			curspawn->db = true;
			curspawn->loc = new cloc();
			curspawn->mobs = new mob_list();
			
			//remember grid info
			if(has grid) {
				curspawn->grid = new cgrid();
				curspawn->grid->db = true;
				curspawn->grid->id = grid_id;
			} else {
				curspawn->grid = NULL;
			}
						
			//set loc
			//set parameters
			....
			lastgroup = curgroup;
		}
		
		cmob *mob = new cmob();
		mob->npc = new cnpc();
		
		//ensure not moving at spawn so builder dosent ignore it
		mob->speed->x = 0;
		mob->speed->y = 0;
		mob->speed->z = 0;
		....
		mob->db = true;
		mob->npc->db = true;
		//remember merchant info
		if(has merchant) {
			mob->merchant = new cmerchant();
			mob->merchant->db = true;
			mob->merchant->owner = mob;
			mob->merchant->id = merchant_id;
		} else {
			mob->merchant = NULL;
		}
		curspawn->mobs.Add(mob);
	}
	
	
	//TODO: go through each entry in the spawn list
	//and see if that spawn had a grid, if so, load it.
	
	//TODO: go through each entry in the spawn list, and
	//each mob in the spawn, and see if it has a merchant...
	//if so, load the merchant items
	
*/
}


void database::loadFactionMapping(const char *zonename, map<string, int> &mapping) {
/*	CString query;
	MYSQL_RES* result = NULL;
	MYSQL_ROW row;
	

	query.Format( "SELECT match, npc_faction_id FROM builder_faction WHERE zone='%s'", zonename );
	runquery( query, &result, NULL, NULL );
	if(result == NULL)
		return;

	MatchStruct m;
	while ( row = mysql_fetch_row( result ) ) {
		m.match = row[1];
		m.id = atoi(row[0]);
		mapping.push_back(m);
	}*/
}

void database::loadLootMapping(const char *zonename, map<string, int> &mapping) {
/*	CString query;
	MYSQL_RES* result = NULL;
	MYSQL_ROW row;

	mapping.clear();

	query.Format( "SELECT match, npc_faction_id FROM builder_faction WHERE zone='%s'", zonename );
	runquery( query, &result, NULL, NULL );
	if(result == NULL)
		return;

	MatchStruct m;
	while ( row = mysql_fetch_row( result ) ) {
		m.match = row[1];
		m.id = atoi(row[0]);
		mapping.push_back(m);
	}*/
}





BEGIN_MESSAGE_MAP(database, CWnd)
	//{{AFX_MSG_MAP(database)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// database message handlers
