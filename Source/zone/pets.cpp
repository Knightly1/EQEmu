/*  EQEMu:  Everquest Server Emulator
Copyright (C) 2001-2004  EQEMu Development Team (http://eqemu.org)

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY except by those people which sell it, which
	are required to give you total support for your newly bought product;
	without even the implied warranty of MERCHANTABILITY or FITNESS FOR
	A PARTICULAR PURPOSE.  See the GNU General Public License for more details.

	  You should have received a copy of the GNU General Public License
	  along with this program; if not, write to the Free Software
	  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/
#include "../common/debug.h"
#include "spdat.h"
#include "masterentity.h"
#include "../common/packet_dump.h"
#include "../common/moremath.h"
#include "../common/Item.h"
#include "worldserver.h"
#include "../common/skills.h"
#include "../common/bodytypes.h"
#include "../common/classes.h"
#include <math.h>
#include <assert.h>
#ifndef WIN32
#include <stdlib.h>
#include "../common/unix.h"
#endif

#include "StringIDs.h"


///////////////////////////////////////////////////////////////////////////////
// pet related functions

char *GetRandPetName()
{
	char petnames[77][64] = { "Gabeker","Gann","Garanab","Garn","Gartik",
            "Gebann","Gebekn","Gekn","Geraner","Gobeker","Gonobtik","Jabantik",
            "Jasarab","Jasober","Jeker","Jenaner","Jenarer","Jobantik",
            "Jobekn","Jonartik","Kabann","Kabartik","Karn","Kasarer","Kasekn",
            "Kebekn","Keber","Kebtik","Kenantik","Kenn","Kentik","Kibekab",
            "Kobarer","Kobobtik","Konaner","Konarer","Konekn","Konn","Labann",
            "Lararer","Lasobtik","Lebantik","Lebarab","Libantik","Libtik",
            "Lobn","Lobtik","Lonaner","Lonobtik","Varekab","Vaseker","Vebobab",
            "Venarn","Venekn","Vener","Vibobn","Vobtik","Vonarer","Vonartik",
            "Xabtik","Xarantik","Xarar","Xarer","Xeber","Xebn","Xenartik",
            "Xeratik","Xesekn","Xonartik","Zabantik","Zabn","Zabeker","Zanab",
            "Zaner","Zenann","Zonarer","Zonarn" };
	int r = (rand()  % (77 - 1)) + 1;
	printf("Pet being created: %s\n",petnames[r]); // DO NOT COMMENT THIS OUT!
	if(r > 77) // Just in case
		r=77;
	return petnames[r];
}

int CalcPetHp(int levelb, int classb, int STA)
{
	int multiplier = 0;
	int base_hp = 0;
	switch(classb) {
		case WARRIOR:{
			if (levelb < 20)
				multiplier = 22;
			else if (levelb < 30)
				multiplier = 23;
			else if (levelb < 40)
				multiplier = 25;
			else if (levelb < 53)
				multiplier = 27;
			else if (levelb < 57)
				multiplier = 28;
			else
				multiplier = 30;
			break;
		}
		case DRUID:
		case CLERIC:
		case SHAMAN:{
			multiplier = 15;
			break;
		}
		case PALADIN:
		case SHADOWKNIGHT:{
			if (levelb < 35)
				multiplier = 21;
			else if (levelb < 45)
				multiplier = 22;
			else if (levelb < 51)
				multiplier = 23;
			else if (levelb < 56)
				multiplier = 24;
			else if (levelb < 60)
				multiplier = 25;
			else
				multiplier = 26;
			break;
		}
		case MONK:
		case BARD:
		case ROGUE:
		case BEASTLORD:{
			if (levelb < 51)
				multiplier = 18;
			else if (levelb < 58)
				multiplier = 19;
			else
				multiplier = 20;
			break;
		}
		case RANGER:{
			if (levelb < 58)
				multiplier = 20;
			else
				multiplier = 21;
			break;
		}
		case MAGICIAN:
		case WIZARD:
		case NECROMANCER:
		case ENCHANTER:{
			multiplier = 12;
			break;
		}
		default:{
			if (levelb < 35)
				multiplier = 21;
			else if (levelb < 45)
				multiplier = 22;
			else if (levelb < 51)
				multiplier = 23;
			else if (levelb < 56)
				multiplier = 24;
			else if (levelb < 60)
				multiplier = 25;
			else
				multiplier = 26;
			break;
		}
	}

	if (multiplier == 0)
	{
		cerr << "Multiplier == 0 in CalcPetHp,using Generic...." << endl;
		multiplier=12;
	}

	base_hp = 5 + (multiplier*levelb) + ((multiplier*levelb*STA) + 1)/300;
	return base_hp;
}



void Mob::MakePet(int16 spell_id, const char* pettype) {
/*Baron-Sprite: Pet types were conflicting all over...
 * was rushed it appears.  I have corrected pet types and
 * assigned ranges for pet types. PLEASE follow these ranges 
 * if you ever add/modify pets: The Pettype ranges for each pet 
 * are shown next to their summoning name, take a minute and look. 
 * Since mage pets are not as percise as necromancers, I am going i
 * to reserve them a large range for future additions I will add.
 *
 * However their base values will still be reserved as 0-3 and epic 
 * will stay on 4. Also I may want to note that even though pet 
 * ranges and types are reserved, it doesn't mean they will be used, 
 * however please respect the reserved spots.  Thanks!
 * TODO: Make anything missing on this list.
*/
	//Baron-Sprite:  Mage pets done by: gej302.  Am I missing anyone else??  Sorry only one I saw.
	Make_Pet_Struct petstruct;
		//int hp_percent = 100;
		if (strncmp(pettype, "SumEarthR", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 0. ALSO 74-87.
			int8 tmp = atoi(&pettype[9]);
	    	switch (tmp) { 
				 case  2:
					database.MakePet(&petstruct,74,1);
					break;
				 case  3:
					database.MakePet(&petstruct,75,1);
					break;
				 case  4:
					database.MakePet(&petstruct,76,1);
					break;
				 case  5:
					database.MakePet(&petstruct,77,1);
					break;
				 case  6:
					database.MakePet(&petstruct,78,1);
					break;
				 case  7:
					database.MakePet(&petstruct,79,1);
					break;
				 case  8:
					database.MakePet(&petstruct,80,1);
					break;
				 case  9:
					database.MakePet(&petstruct,81,1);
					break;
				 case 10:
					database.MakePet(&petstruct,82,1);
					break;
				 case 11:
					database.MakePet(&petstruct,83,1);
					break;
				 case 12:
					database.MakePet(&petstruct,84,1);
					break;
				 case 13:
					database.MakePet(&petstruct,85,1);
					break;
				 case 14:
					database.MakePet(&petstruct,86,1);
					break;
				 case 15:
					database.MakePet(&petstruct,87,1);
					break;
				default:
					Message(0, "Error: Unknown Earth Pet formula");
			}
    } else if (strncmp(pettype, "SumFireR", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 1. ALSO 88-101.
		int8 tmp = atoi(&pettype[8]);
		switch (tmp) { 
			 case  2:
				database.MakePet(&petstruct,88,1);
				break;
			 case  3:
				database.MakePet(&petstruct,89,1);
				break;
			 case  4:
				database.MakePet(&petstruct,90,1);
				break;
			 case  5:
				database.MakePet(&petstruct,91,1);
				break;
			 case  6:
				database.MakePet(&petstruct,92,1);
				break;
			 case  7:
				database.MakePet(&petstruct,93,1);
				break;
			 case  8:
				database.MakePet(&petstruct,94,1);
				break;
			 case  9:
				database.MakePet(&petstruct,95,1);
				break;
			 case 10:
				database.MakePet(&petstruct,96,1);
				break;
			 case 11:
				database.MakePet(&petstruct,97,1);
				break;
			 case 12:
				database.MakePet(&petstruct,98,1);
				break;
			 case 13:
				database.MakePet(&petstruct,99,1);
				break;
			 case 14:
				database.MakePet(&petstruct,100,1);
				break;
			 case 15:
				database.MakePet(&petstruct,101,1);
				break;
			default:
				Message(0, "Error: Unknown Fire Pet formula");
		}
    } else if (strncmp(pettype, "SumAirR", 7) == 0) { //Baron-Sprite: This Pettype is reserved to 3. ALSO 60-73.
		int8 tmp = atoi(&pettype[7]);
    	switch (tmp) {
			 case  2:
				database.MakePet(&petstruct,60,1);
				break;
			 case  3:
				database.MakePet(&petstruct,61,1);
				break;
			 case  4:
				database.MakePet(&petstruct,62,1);
				break;
			 case  5:
				database.MakePet(&petstruct,63,1);
				break;
			 case  6:
				database.MakePet(&petstruct,64,1);
				break;
			 case  7:
				database.MakePet(&petstruct,65,1);
				break;
			 case  8:
				database.MakePet(&petstruct,66,1);
				break;
			 case  9:
				database.MakePet(&petstruct,67,1);
				break;
			 case 10:
				database.MakePet(&petstruct,68,1);
				break;
			 case 11:
				database.MakePet(&petstruct,69,1);
				break;
			 case 12:
				database.MakePet(&petstruct,70,1);
				break;
			 case 13:
				database.MakePet(&petstruct,71,1);
				break;
			 case 14:
				database.MakePet(&petstruct,72,1);
				break;
			 case 15:
				database.MakePet(&petstruct,73,1);
				break;
			default:
				Message(0, "Error: Unknown Air Pet formula");
		}
    } else if (strncmp(pettype, "SumWaterR", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 2. ALSO 102-115.
		int8 tmp = atoi(&pettype[9]);
		switch (tmp) {
			 case  2:
				database.MakePet(&petstruct,102,1);
				break;
			 case  3:
				database.MakePet(&petstruct,103,1);
				break;
			 case  4:
				database.MakePet(&petstruct,104,1);
				break;
			 case  5:
				database.MakePet(&petstruct,105,1);
				break;
			 case  6:
				database.MakePet(&petstruct,106,1);
				break;
			 case  7:
				database.MakePet(&petstruct,107,1);
				break;
			 case  8:
				database.MakePet(&petstruct,108,1);
				break;
			 case  9:
				database.MakePet(&petstruct,109,1);
				break;
			 case 10:
				database.MakePet(&petstruct,110,1);
				break;
			 case 11:
				database.MakePet(&petstruct,111,1);
				break;
			 case 12:
				database.MakePet(&petstruct,112,1);
				break;
			 case 13:
				database.MakePet(&petstruct,113,1);
				break;
			 case 14:
				database.MakePet(&petstruct,114,1);
				break;
			 case 15:
				database.MakePet(&petstruct,115,1);
				break;
			default:
				Message(0, "Error: Unknown Water Pet formula");
		}
    } else if (strncmp(pettype, "Familiar",8) == 0) { // neotokyo: reserved type 217 for familiars
		int8 tmp = atoi(&pettype[8]);
		int16 tmprace = 46;
		int16 tmpclass = WARRIOR;
		int16 tmptype = 1;
		int8 tmptexture = 1;
		int8 tmplevel = 27;
		//these should prolly get put into the database
		switch (tmp) {
			case 1:
				tmplevel = 27;
				break;
			case 2:
				tmplevel = 47;
				break;
			case 3:
				tmplevel = 50;
				tmprace = 89;
				tmptype = 122;
				tmpclass = 4;
				tmptexture = 4;
				break;
			case 4:
				tmplevel = 58;
				tmprace = 89;
				tmptype = 123;
				tmpclass = 4;
				tmptexture = 4;
				break;
			case 5:
				tmplevel = 60;
				tmprace = 89;
				tmptype = 124;
				break;
			case 6:
				tmplevel = 47;
				tmptype = 122;
				tmprace = 367;
				tmpclass = 11;
				tmptexture = 4;
				break;
			case 7:
				tmplevel = 47;
				tmptype = 122;
				tmprace = 89;
				tmptexture = 3;
				break;

		}
		MakePet(spell_id, tmplevel, tmpclass, tmprace, tmptexture, tmptype, 3, 217);
		return;

/*        MakePet(spell_id, 27,1,46,1,120,3,217); //Baron-Sprite: This Pettype is reserved to 120-124.
		return;
    } else if (strncmp(pettype, "Familiar2",9) == 0) {
        MakePet(spell_id, 47,1,46,1,121,3,217);
		return;
    } else if (strncmp(pettype, "Familiar3",9) == 0) {
        MakePet(spell_id, 50,1,89,4,122,3,217);
		return;
    } else if (strncmp(pettype, "Familiar4",9) == 0) {
        MakePet(spell_id, 58,1,89,4,123,3,217);
		return;
    } else if (strncmp(pettype, "Familiar5",9) == 0) {
        MakePet(spell_id, 60,1,89,1,124,3,217);
		return;*/
    } else if (strncmp(pettype, "SpiritWolf", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 40-45.  Looks sloppy, sorry.
		int8 tmp = atoi(&pettype[11]);
		switch (tmp) {
		case 42:
			database.MakePet(&petstruct,45,3);
			break;
		case 37:
			database.MakePet(&petstruct,44,3);
			break;
		case 34:
			database.MakePet(&petstruct,43,3);
			break;
		case 30:
			database.MakePet(&petstruct,42,3);
			break;
		case 27:
			database.MakePet(&petstruct,41,3);
			break;
		case 24:
			database.MakePet(&petstruct,40,3);
			break;
	    default:
			cout << "Unknown pettype: " << tmp<< " : Generating default type." << endl;
			MakePet(spell_id, 24, 1, 42, 0, 40, 7, 3);
			return;
		}
    } else if (strncmp(pettype, "BLpet", 5) == 0) { //Baron-Sprite: This Pettype is reserved to 125-137
		int8 ptype = atoi(&pettype[5]);
		int crace = this->GetRace();
		int prace=0;

		int mat=0;
        float size_mod = 1;
		#ifdef _EQDEBUG
			cout << "Setting stats for BL Pet for Race: " << crace << endl;
		#endif

		switch ( crace ) {
		case VAHSHIR:
			prace = 63;
			size_mod = 1.7f;
			break;
		case TROLL:
			prace=91;
			break;
		case OGRE:
			prace=43;
			mat=3;
			break;
		case IKSAR:
			prace=42;
			mat=0;
			break;
		case BARBARIAN:
			prace=42;
			mat=2;
            size_mod = 1.5f;
			break;
		default:
#ifdef _EQDEBUG
			cout << "No pet type modifications defined for race: " << crace << endl;
#endif
			break;
		}
#ifdef _EQDEBUG
			cout << "Summoning BeastLord Pet: " << (int)ptype << endl;
#endif
		switch ( ptype ) {
		case 51:
			database.MakePet(&petstruct,136,5,6*size_mod);
			break;
		case 49:
			database.MakePet(&petstruct,135,5,5.8*size_mod);
			break;
		case 47:
			database.MakePet(&petstruct,134,5,5.6*size_mod);
			break;
		case 45:
			database.MakePet(&petstruct,133,5,5.4*size_mod);
			break;
		case 43:
			database.MakePet(&petstruct,132,5,5.2*size_mod);
			break;
		case 41:
			database.MakePet(&petstruct,131,5,5*size_mod);
			break;
		case 39:
			database.MakePet(&petstruct,130,5,4.5*size_mod);
			break;
		case 31:
			database.MakePet(&petstruct,129,5,4*size_mod);
			break;
		case 26:
			database.MakePet(&petstruct,128,5,3.5*size_mod);
			break;
		case 22:
			database.MakePet(&petstruct,127,5,3*size_mod);
			break;
		case 16:
			database.MakePet(&petstruct,126,5,2.6*size_mod);
			break;
		case 9:
			database.MakePet(&petstruct,125,5,2.3*size_mod);
			break;
		default:
			MakePet(spell_id, 10, 1, prace, mat, 125, 2*size_mod, 5);
	        cout << "ptype not found: Making default BL pet." << endl;
			break;
		}
		petstruct.race = prace;
		petstruct.texture = mat;
    } else if (strncmp(pettype, "BLBasePet", 9) == 0) { //Baron-Sprite: This Pettype does not need a reserve, it is only a warning message.
			Message(13, "Beastlord pets are summon via the Spirit of Sharik, Khaliz, Keshuval, Herikol, Yekan, Kashek, Omakin, Zehkes, Khurenz, Khati Sha, Arag, or Sorsha spell line.  The Summon Warder ability was taken out of live sometime ago and replaced with this method. ");
			return;
    } else if (strncmp(pettype, "Animation", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 46-59.
		int8 ptype = atoi(&pettype[9]);

		switch ( ptype ) {
		case 14:
			database.MakePet(&petstruct,59,2);
			break;
		case 13:
			database.MakePet(&petstruct,58,2);
			break;
		case 12:
			database.MakePet(&petstruct,57,2);
			break;
		case 11:
			database.MakePet(&petstruct,56,2);
			break;
		case 10:
			database.MakePet(&petstruct,55,2);
			break;
		case 9:
			database.MakePet(&petstruct,54,2);
			break;
		case 8:
			database.MakePet(&petstruct,53,2);
			break;
		case 7:
			database.MakePet(&petstruct,52,2);
			break;
		case 6:
			database.MakePet(&petstruct,51,2);
			break;
		case 5:
			database.MakePet(&petstruct,50,2);
			break;
		case 4:
			database.MakePet(&petstruct,49,2);
			break;
		case 3:
			database.MakePet(&petstruct,48,2);
			break;
		case 2:
			database.MakePet(&petstruct,47,2);
			break;
		case 1:
			database.MakePet(&petstruct,46,2);
			break;
		default:
			MakePet(spell_id, 1, 1, 127, 0, 46, 6, 2);
			cout << "ptype not found: Making default animation pet." << endl;
			return;
		}
    } else if (strncmp(pettype, "SumSword", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 18.
        // for testing make an chanter pet
		MakePet(spell_id, 59, 1, 127,0,46,0,2);
    } else if (strncmp(pettype, "skel_pet_", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 22-39.
		char sztmp[50];
		strcpy(sztmp, pettype);
		sztmp[11] = 0;
		int8 tmp = atoi(&sztmp[9]);
		//Baron-Sprite: No need for level algorithim - Since pet levels are now fixed in EQLive.
		//Baron-Sprite: MakePet(level, class, race, texture, pettype, size, type) 0 Can be a placeholder.
		if (tmp >= 65) {
			database.MakePet(&petstruct,39,4);
        } else if (tmp >= 63) {
			database.MakePet(&petstruct,38,4);
        } else if (tmp >= 61) {
			database.MakePet(&petstruct,37,4);
		} else if (tmp >= 47) {
			database.MakePet(&petstruct,36,4);
		} else if (tmp >= 44) {
			database.MakePet(&petstruct,35,4);
        } else if (tmp >= 43) {
			database.MakePet(&petstruct,34,4);
        } else if (tmp >= 41) {
			database.MakePet(&petstruct,33,4);
        } else if (tmp >= 37) {
			database.MakePet(&petstruct,32,4);
        } else if (tmp >= 33) {
			database.MakePet(&petstruct,31,4);
		} else if (tmp >= 29) {
			database.MakePet(&petstruct,30,4);
        } else if (tmp >= 25) {
			database.MakePet(&petstruct,29,4);
        } else if (tmp >= 22) {
			database.MakePet(&petstruct,28,4);
        } else if (tmp >= 19) {
			database.MakePet(&petstruct,27,4);
        } else if (tmp >= 16) {
			database.MakePet(&petstruct,26,4);
        } else if (tmp >= 11) {
			database.MakePet(&petstruct,25,4);
        } else if (tmp >= 9) {
			database.MakePet(&petstruct,24,4);
        } else if (tmp >= 5) {
			database.MakePet(&petstruct,23,4);
        } else if (tmp >= 1) {
			database.MakePet(&petstruct,22,4);
        } else {
			MakePet(spell_id, 1, 1, 60);
			return;
        }
	// in_level, in_class, in_race, in_texture, in_pettype, in_size, type
	/*	else if (strncmp(pettype, "MonsterSum", 9) == 0) { //Baron-Sprite: This Pettype is reserved to 5-7.
	}
	*/
    } else if (strncmp(pettype, "Mistwalker", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 8.
		database.MakePet(&petstruct,8,9);
/* solar: this needs to be fixed right, commenting out for now
		zone->AddAggroMob();
	    this->GetPet()->AddToHateList(target, 1);
		Mob* sictar = entity_list.GetMob(this->GetPetID());
		if (target)
			target->AddToHateList(sictar, 1, 0);
*/
    } else if (strncmp(pettype, "TunareBane", 10) == 0) { //Baron-Sprite: This Pettype is reserved to 13.
		database.MakePet(&petstruct,13,12);
    } else if (strncmp(pettype, "DruidPet", 8) == 0) { //Baron-Sprite: This Pettype is reserved to 11.
		database.MakePet(&petstruct,11,11);
    } else if (strncmp(pettype, "SumMageMultiElement", 19) == 0) {
		database.MakePet(&petstruct,4,15);
	//Pet types from WR, not sure if they are real or not...
	} else if (strncmp(pettype, "SumHammer", 9) == 0) {
		MakePet(spell_id, 50, WARRIOR, 127, 0, 6, 0, 13, 3);
		return;
		//another cleric pet: cleric_hammer_67_ (spell 5256)
		//and SumCelestialSpirit (spell 5865)
	} else if (strncmp(pettype, "SumSword2", 9) == 0) {
		MakePet(spell_id, 54, WARRIOR, 127, 0, 5, 0, 500, 3);
		return;
	} else if (strncmp(pettype, "SumSword", 8) == 0) {
		MakePet(spell_id, 52, WARRIOR, 127, 0, 5, 0, 100, 3);
		return;
	} else if (strncmp(pettype, "CrysSpider", 9) == 0) {
		MakePet(spell_id, 50, WARRIOR, 38, 5, 0, 0, 2500, 8);
		return;
	} else {
		Message(13, "Unknown pet type: %s", pettype);
	}
	MakePet(spell_id, petstruct.level,petstruct.class_,petstruct.race,petstruct.texture,petstruct.pettype,petstruct.size,petstruct.type,petstruct.min_dmg,petstruct.max_dmg);
}

void Mob::MakePet(int16 spell_id, int8 in_level, int8 in_class, int16 in_race,
                  int8 in_texture, int8 in_pettype, float in_size,
                  int8 type, int32 min_dmg, int32 max_dmg) {
	if (this->GetPetID() != 0) {
		return;
	}
	
	NPCType* npc_type = new NPCType;
	memset(npc_type, 0, sizeof(NPCType));
	if (in_level>1)
		npc_type->hp_regen = 2;//(int)(in_level/5); fixed elsewhere if they arent engaged
	else
		npc_type->hp_regen = 1;

	if (in_race == 127 || in_race == 216)
		npc_type->gender = 0;
	else if (in_race == 42 && in_texture == 5)
		npc_type->gender = 1;
	else
		npc_type->gender = 2;

	if (in_pettype >= 1 && in_pettype <= 4) {
		strcpy(npc_type->name, this->GetName());
		npc_type->name[19] = '\0';
		strcat(npc_type->name, "`s_familiar");
	} else if (this->IsClient())
		strcpy(npc_type->name, GetRandPetName());
	else {
		strcpy(npc_type->name, this->GetCleanName());
		npc_type->name[25] = '\0';
		strcat(npc_type->name, "'s_pet");
	}

	npc_type->level = in_level;
	npc_type->race = in_race;
	npc_type->class_ = in_class;
	npc_type->texture = in_texture;
	npc_type->helmtexture = in_texture;
	npc_type->runspeed = 1.25f;
	//the proper type for necro pets is BT_SummonedUndead
	npc_type->bodytype = BT_Summoned; /* pets are summoned */
	npc_type->min_dmg = min_dmg;
	npc_type->max_dmg = max_dmg;

	npc_type->walkspeed = 0.7f;
	npc_type->size = in_size;
	//npc_type->npc_spells_id = this->GetNPCSpellsID();
	npc_type->npc_spells_id = 0;

	npc_type->max_hp = CalcPetHp(npc_type->level, npc_type->class_);
	npc_type->cur_hp = npc_type->max_hp;
	npc_type->fixedZ = 1;
	int pettype = in_pettype; //Baron-Sprite: Needed for necro pet types.
//int yourlevel = this->GetLevel();
	NPCType pet;

	switch(type) {
       case 217: {
    	// wizards familiars
    	char f_name[50];
    	strcpy(f_name,this->GetCleanName());
    	strcat(f_name,"'s Familiar");
    	strcpy(npc_type->name, f_name);
    	npc_type->min_dmg = 0;  //Baron-Sprite: Naughty Familiar.  No Attack 4 u.
    	npc_type->max_dmg = 0;
    	npc_type->max_hp = 1000;
    	break;
       }
       case 1: { //Bentareth: Mage pets, as close live as I can find, need spell procs added
             //2 types of procs, last 3 in each category does a new type of proc
             //Air and earth do damage ~50 hp, water does double previous, and fire needs several wizard spells added
               npc_type->hp_regen = 6; //default case (true until lvl 39 pet)
			   npc_type->gender=2;
			    if(pettype==4){ //Mage epic pet
					database.GetPetStats(&pet,4);
					npc_type->hp_regen=50;
					npc_type->npc_spells_id = 21;
				}
				else if(pettype>=60 && pettype<74){ //Air Pets
					database.GetPetStats(&pet,pettype);
					npc_type->npc_spells_id = 14;
				}
				else if(pettype>=74 && pettype<88){ //Earth Pets
					database.GetPetStats(&pet,pettype);
					npc_type->npc_spells_id = 15;
				}
				else if(pettype>=88 && pettype<99){ //Fire Pets
					database.GetPetStats(&pet,pettype);
					npc_type->npc_spells_id = 18;
				}
				else if(pettype>=99 && pettype<102){ //Fire Pets
					database.GetPetStats(&pet,pettype);
					npc_type->npc_spells_id = 20;
					if(pettype==101)
						sprintf(npc_type->npc_attacks, "E");
					npc_type->hp_regen = 30;
				}
				else if(pettype>=102 && pettype<116){ //Water Pets
					database.GetPetStats(&pet,pettype);
					npc_type->npc_spells_id = 18;
					if(pettype==115){
						npc_type->hp_regen = 100;
						sprintf(npc_type->npc_attacks, "E");
					}
					else if(pettype>=110){
						npc_type->hp_regen = 30;
						sprintf(npc_type->npc_attacks, "E");
					}
					npc_type->npc_spells_id = 16;
				}
				else{
					printf("Unknown pet number of %i\n",pettype);
					break;
				}
				npc_type->max_hp = pet.max_hp;
				npc_type->cur_hp = pet.cur_hp;
				npc_type->min_dmg = pet.min_dmg;
				npc_type->max_dmg = pet.max_dmg;
				break;
	}
	case 2: { //Baron-Sprite: Enchanter Pets.  Some info from casters realm.
		npc_type->gender = 0; //devn00b: nfi what to do about race here.
		npc_type->equipment[7] = 34;// devn00b: or these equip fields might have to add them as an option in the db
		npc_type->equipment[8] = 202;
		if (pettype >=57 && pettype<=59) {
			database.GetPetStats(&pet,pettype);
			npc_type->equipment[7] = 26;
			npc_type->equipment[8] = 26;
		}
		else if ((pettype>=51 && pettype<=56) || (pettype>=46 && pettype<=48))
			database.GetPetStats(&pet,pettype);
		else if (pettype >= 49 && pettype<=50){
			database.GetPetStats(&pet,pettype);
			npc_type->equipment[7] = 3;
		}
		else{
			printf("Unknown pet number of %i\n",pettype);
			break;
		}
		npc_type->max_hp = pet.max_hp;
		npc_type->cur_hp = pet.cur_hp;
		npc_type->min_dmg = pet.min_dmg;
		npc_type->max_dmg = pet.max_dmg;
		break;
	}
	case 3: { //Baron-Sprite: Shaman pets.  Credits for information go mostly to eq.castersrealm.com.
	if(pettype==45){
		database.GetPetStats(&pet,pettype);
        sprintf(npc_type->npc_attacks, "E"); // should enrage, i guess
		npc_type->max_hp = pet.max_hp;
		npc_type->cur_hp = pet.cur_hp;
		npc_type->min_dmg = pet.min_dmg;
		npc_type->max_dmg = pet.max_dmg;
	}
	else if(pettype>=40 && pettype<45){
		database.GetPetStats(&pet,pettype);
		npc_type->max_hp = pet.max_hp;
		npc_type->cur_hp = pet.cur_hp;
		npc_type->min_dmg = pet.min_dmg;
		npc_type->max_dmg = pet.max_dmg;
	}
	else{
			cout << "Fallthrough case for Shaman Pet." << endl;
			npc_type->max_hp = 25;
			npc_type->cur_hp = 25;
			npc_type->min_dmg = 1;
			npc_type->max_dmg = 3;
	}
	break;
	}
	case 4: { //Baron-Sprite: Necromancer pets.  Some of the info is from eqnecro.com
		npc_type->npc_spells_id = 23;
		npc_type->bodytype = BT_SummonedUndead; /* both summoned and undead */
		if(pettype >= 37 && pettype<=39){ //Baron-Sprite: This is defined above in the Makepet statement.  I use it to single out the individual pet spells.
			database.GetPetStats(&pet,pettype);
			sprintf(npc_type->npc_attacks, "E");
			npc_type->max_hp = pet.max_hp;
			npc_type->cur_hp = pet.cur_hp;
			npc_type->min_dmg = pet.min_dmg;
			npc_type->max_dmg = pet.max_dmg;
		}
		else if(pettype >= 22 && pettype<=36)
		{
			database.GetPetStats(&pet,pettype);
			npc_type->max_hp = pet.max_hp;
			npc_type->cur_hp = pet.cur_hp;
			npc_type->min_dmg = pet.min_dmg;
			npc_type->max_dmg = pet.max_dmg;
		}
		else
		{
			npc_type->max_hp = 25;
			npc_type->cur_hp = 25;
			npc_type->min_dmg = 1;
			npc_type->max_dmg = 3;
		}
		break;
	}
	case 5: //Baron-Sprite: Beastlord pets.  Credits for information go mostly to eq.castersrealm.com.  What's new? :)
	{
		char f_name[50];
	    strcpy(f_name,this->GetCleanName());
	    strcat(f_name,"`s warder");
	    strcpy(npc_type->name, f_name);
		if (pettype >= 125 && pettype<=136)
		{
			database.GetPetStats(&pet,pettype);
			npc_type->max_hp = pet.max_hp;
			npc_type->cur_hp = pet.cur_hp;
			npc_type->min_dmg = pet.min_dmg;
			npc_type->max_dmg = pet.max_dmg;
		}
		else
		{
			npc_type->max_hp = 300;
			npc_type->cur_hp = 300;
			npc_type->min_dmg = 5;
			npc_type->max_dmg = 10;
		}
		if (in_race == 42 && in_texture == 0)
			npc_type->gender = 1;
		else
			npc_type->gender = 2;
		break;
	}
	case 8: { //Baron-Sprite:  Mistwalker :D
		char f_name[50];
		strcat(f_name," pet");
	    strcpy(npc_type->name, f_name);
		database.GetPetStats(&pet,9);
		npc_type->max_hp = pet.max_hp;
		npc_type->cur_hp = pet.cur_hp;
		npc_type->min_dmg = pet.min_dmg;
		npc_type->max_dmg = pet.max_dmg;
		break;
	}
	case 11:{ // Druid Pet...  Baron-Sprite: Info from www.eqdruids.com said about 100 hp.
		database.GetPetStats(&pet,11);
		npc_type->max_hp = pet.max_hp;
		npc_type->cur_hp = pet.cur_hp;
		npc_type->min_dmg = pet.min_dmg;
		npc_type->max_dmg = pet.max_dmg;
		break;	
	}
	case 12:{
		char f_name[50];
	    strcpy(f_name,this->GetCleanName());
	    strcat(f_name," pet");
	    strcpy(npc_type->name, f_name);
		database.GetPetStats(&pet,13);
		npc_type->max_hp = pet.max_hp;
		npc_type->cur_hp = pet.cur_hp;
		npc_type->min_dmg = pet.min_dmg;
		npc_type->max_dmg = pet.max_dmg;
		break;
	}
	case 13: { //Cleric Hammer
		npc_type->max_hp = 100;
		npc_type->cur_hp = 100;
		npc_type->max_dmg = 75;
		break;
	}
	default:
		printf("Unknown type/pettype of: %i,%i.  Using Default Formula...\n",type,pettype);
		npc_type->max_hp = 5*((in_level*in_level)/2);
		npc_type->cur_hp = npc_type->max_hp;
		npc_type->min_dmg = 1;
		npc_type->max_dmg = (int)(in_level*1.2);
		break;
	}
	switch (npc_type->class_)
	{
		case CLERIC:
		case PALADIN:
		case RANGER:
		case SHADOWKNIGHT:
		case DRUID:
		case BARD:
		case SHAMAN:
		case NECROMANCER:
		case WIZARD:
		case MAGICIAN:
		case ENCHANTER:
		case BEASTLORD:
			npc_type->Mana = npc_type->level*25;
			break;
	}
	
	NPC* npc = new NPC(npc_type, 0,
                       this->GetX()+10, this->GetY()+10,
                       this->GetZ(), this->GetHeading());
	safe_delete(npc_type);
	npc->SetPetType(in_pettype);
	npc->SetOwnerID(this->GetID());
	entity_list.AddNPC(npc);
    if (type != 217)
	    this->SetPetID(npc->GetID());
    else
	    this->SetFamiliarID(npc->GetID());	
	npc->SetPetSpellID(spell_id);
}




