/*  EQEMu:  Everquest Server Emulator
    Copyright (C) 2001-2004  EQEMu Development Team (http://eqemulator.org)

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
#include "features.h"

#ifdef EMBPERL
#ifdef EMBPERL_XS

#include "perlparser.h"
#include "questmgr.h"
#include "embxs.h"
#include "entity.h"

/*

Some useful perl API info:

SvUV == string to unsigned value (char->ulong)
SvIV == string to signed value (char->long)
SvNV == string to real value (float,double)
SvPV_nolen == string with no length restriction


*/

PerlXSParser::PerlXSParser() : PerlembParser() {
	_empty_sv = newSV(0);
	ReloadQuests();	//not sure WHY I have to call this again
				//but if I dont, it dosent call the right map_funs
}

void PerlXSParser::map_funs() {

	perl->eval(
	"{"
	"package quest;"
	"&boot_quest;"			//load our quest XS
#ifdef EMBPERL_XS_CLASSES
	"package Mob;"
	"&boot_Mob;"			//load our Mob XS
	
	"package Client;"
	"our @ISA = qw(Mob);"	//client inherits mob.
	"&boot_Mob;"			//load our Mob XS
	"&boot_Client;"			//load our Client XS
	
	"package NPC;"
	"our @ISA = qw(Mob);"	//NPC inherits mob.
	"&boot_Mob;"			//load our Mob XS
	"&boot_NPC;"			//load our NPC XS
	
	"package Corpse;"
	"our @ISA = qw(Mob);"	//Corpse inherits mob.
	"&boot_Mob;"			//load our Mob XS
	"&boot_Corpse;"			//load our Mob XS
	
	"package EntityList;"
	"&boot_EntityList;"		//load our EntityList XS
	
	"package Group;"
	"&boot_Group;"		//load our Group XS
#endif
	"package main;"
	"}"
	);//eval
}

void PerlXSParser::SendCommands(const char * pkgprefix, const char *event, int32 npcid, NPC* other, Mob* mob)
{
	if(!perl)
		return;
	_ZP(PerlXSParser_SendCommands);
	
	quest_manager.StartQuest(other, mob?mob->CastToClient():NULL);
	
	try {

		std::string cmd = "package " + (std::string)(pkgprefix) + (std::string)(";");
		perl->eval(cmd.c_str());
		
#ifdef EMBPERL_XS_CLASSES
		char namebuf[64];
		
		//init a couple special vars: client, npc, entity_list
		Client *curc = quest_manager.GetInitiator();
		snprintf(namebuf, 64, "%s::client", pkgprefix);
		SV *client = get_sv(namebuf, true);
		if(curc != NULL) {
			sv_setref_pv(client, "Client", curc);
		} else {
			//clear out the value, mainly to get rid of blessedness
			sv_setsv(client, _empty_sv);
		}
		
		NPC *curn = quest_manager.GetNPC();
		snprintf(namebuf, 64, "%s::npc", pkgprefix);
		SV *npc = get_sv(namebuf, true);
		sv_setref_pv(npc, "NPC", curn);
		
		snprintf(namebuf, 64, "%s::entity_list", pkgprefix);
		SV *el = get_sv(namebuf, true);
		sv_setref_pv(el, "EntityList", &entity_list);
#endif
		
		//now call the requested sub
		perl->dosub(std::string(pkgprefix).append("::").append(event).c_str());

	} catch(const char * err) {

		//try to reduce some of the console spam... 
		//todo: tweak this to be more accurate at deciding what to filter (we don't want to gag legit errors)
		if(!strstr(err,"Undefined subroutine"))
			LogFile->write(EQEMuLog::Status, "Script error: %s::%s - %s", pkgprefix, event, err);
		
		quest_manager.EndQuest();
		return;
	}
	
	quest_manager.EndQuest();
}


#ifdef EMBPERL_XS_CLASSES

//Any creation of new Client objects gets the current quest Client
XS(XS_Client_new);
XS(XS_Client_new)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: Client::new()");
	{
		Client *		RETVAL;

		RETVAL = quest_manager.GetInitiator();
		ST(0) = sv_newmortal();
		if(RETVAL)
			sv_setref_pv(ST(0), "Client", (void*)RETVAL);
	}
	XSRETURN(1);
}

//Any creation of new NPC objects gets the current quest NPC
XS(XS_NPC_new);
XS(XS_NPC_new)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: NPC::new()");
	{
		NPC *		RETVAL;

		RETVAL = quest_manager.GetNPC();
		ST(0) = sv_newmortal();
		if(RETVAL)
			sv_setref_pv(ST(0), "NPC", (void*)RETVAL);
	}
	XSRETURN(1);
}

//Any creation of new NPC objects gets the current quest NPC
XS(XS_EntityList_new);
XS(XS_EntityList_new)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: EntityList::new()");
	{
		EntityList *		RETVAL;

		RETVAL = &entity_list;
		ST(0) = sv_newmortal();
		if(RETVAL)
			sv_setref_pv(ST(0), "EntityList", (void*)RETVAL);
	}
	XSRETURN(1);
}

#endif //EMBPERL_XS_CLASSES


XS(XS__echo); // prototype to pass -Wmissing-prototypes
XS(XS__echo) {
	dXSARGS;
	
	if (items != 1)
		Perl_croak(aTHX_ "Usage: say(str)");
	
	quest_manager.echo(SvPV_nolen(ST(0)));
	
	XSRETURN_EMPTY;
}

XS(XS__say); // prototype to pass -Wmissing-prototypes
XS(XS__say) {
	dXSARGS;
	
	if (items != 1)
		Perl_croak(aTHX_ "Usage: say(str)");
	
	quest_manager.say(SvPV_nolen(ST(0)));
	
	XSRETURN_EMPTY;
}

XS(XS__me); // prototype to pass -Wmissing-prototypes
XS(XS__me) {
	dXSARGS;
	
	if (items != 1)
		Perl_croak(aTHX_ "Usage: %s(str)", "me");
	
	quest_manager.me(SvPV_nolen(ST(0)));
	
	XSRETURN_EMPTY;
}

XS(XS__summonitem); // prototype to pass -Wmissing-prototypes 
XS(XS__summonitem)
{
	dXSARGS;
	if (items == 1)
		quest_manager.summonitem(SvUV(ST(0)));
	else if(items == 2)
		quest_manager.summonitem(SvUV(ST(0)), SvUV(ST(1)));
	else
		Perl_croak(aTHX_ "Usage: summonitem(itemid, [charges])");
	XSRETURN_EMPTY;
}

XS(XS__write);
XS(XS__write)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: write(file, str)");

	char *		file = (char *)SvPV_nolen(ST(0));
	char *		str = (char *)SvPV_nolen(ST(1));

	quest_manager.write(file, str);

	XSRETURN_EMPTY;
}

XS(XS__spawn);
XS(XS__spawn)
{
	dXSARGS;
	if (items != 6)
		Perl_croak(aTHX_ "Usage: spawn(npc_type, grid, unused, x, y, z)");
	
	int16		RETVAL;
	dXSTARG;
	
	int	npc_type = (int)SvIV(ST(0));
	int	grid = (int)SvIV(ST(1));
	int	unused = (int)SvIV(ST(2));
	float	x = (float)SvNV(ST(3));
	float	y = (float)SvNV(ST(4));
	float	z = (float)SvNV(ST(5));

	RETVAL = quest_manager.spawn2(npc_type, grid, unused, x, y, z, 0);
	XSprePUSH; PUSHu((UV)RETVAL);
	
	XSRETURN(1);
}

XS(XS__spawn2);
XS(XS__spawn2)
{
	dXSARGS;
	if (items != 7)
		Perl_croak(aTHX_ "Usage: spawn2(npc_type, grid, unused, x, y, z, heading)");
	
	int16		RETVAL;
	dXSTARG;
	
	int	npc_type = (int)SvIV(ST(0));
	int	grid = (int)SvIV(ST(1));
	int	unused = (int)SvIV(ST(2));
	float	x = (float)SvNV(ST(3));
	float	y = (float)SvNV(ST(4));
	float	z = (float)SvNV(ST(5));
	float	heading = (float)SvNV(ST(6));

	RETVAL = quest_manager.spawn2(npc_type, grid, unused, x, y, z, heading);
	XSprePUSH; PUSHu((UV)RETVAL);
	
	XSRETURN(1);
}

XS(XS__unique_spawn);
XS(XS__unique_spawn)
{
	dXSARGS;
	if (items != 6 && items != 7)
		Perl_croak(aTHX_ "Usage: unique_spawn(npc_type, grid, unused, x, y, z[, heading])");
	
	int16		RETVAL;
	dXSTARG;
	
	int	npc_type = (int)SvIV(ST(0));
	int	grid = (int)SvIV(ST(1));
	int	unused = (int)SvIV(ST(2));
	float	x = (float)SvNV(ST(3));
	float	y = (float)SvNV(ST(4));
	float	z = (float)SvNV(ST(5));
	float	heading = 0;
	if(items == 7)
		heading = (float)SvNV(ST(6));

	RETVAL = quest_manager.unique_spawn(npc_type, grid, unused, x, y, z, heading);
	XSprePUSH; PUSHu((UV)RETVAL);
	
	XSRETURN(1);
}

XS(XS__setstat);
XS(XS__setstat)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: setstat(stat, value)");

	int	stat = (int)SvIV(ST(0));
	int	value = (int)SvIV(ST(1));

	quest_manager.setstat(stat, value);

	XSRETURN_EMPTY;
}

XS(XS__castspell);
XS(XS__castspell)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: castspell(spell_id, target_id)");

	int	spell_id = (int)SvIV(ST(0));
	int	target_id = (int)SvIV(ST(1));

	quest_manager.castspell(spell_id, target_id);

	XSRETURN_EMPTY;
}

XS(XS__selfcast);
XS(XS__selfcast)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: selfcast(spell_id)");

	int	spell_id = (int)SvIV(ST(0));

	quest_manager.selfcast(spell_id);

	XSRETURN_EMPTY;
}

XS(XS__addloot);
XS(XS__addloot)
{
	dXSARGS;
	
	if(items == 1)
		quest_manager.addloot(SvIV(ST(0)));
	else if(items == 2)
		quest_manager.addloot(SvIV(ST(0)), SvIV(ST(1)));
	else
		Perl_croak(aTHX_ "Usage: addloot(item_id, charges = 0)");

	XSRETURN_EMPTY;
}

XS(XS__zone);
XS(XS__zone)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: zone(zone_name)");

	char *		zone_name = (char *)SvPV_nolen(ST(0));

	quest_manager.Zone(zone_name);

	XSRETURN_EMPTY;
}

XS(XS__settimer);
XS(XS__settimer)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: settimer(timer_name, seconds)");

	char *		timer_name = (char *)SvPV_nolen(ST(0));
	int	seconds = (int)SvIV(ST(1));

	quest_manager.settimer(timer_name, seconds);

	XSRETURN_EMPTY;
}

XS(XS__stoptimer);
XS(XS__stoptimer)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: stoptimer(timer_name)");

	char *		timer_name = (char *)SvPV_nolen(ST(0));

	quest_manager.stoptimer(timer_name);

	XSRETURN_EMPTY;
}

XS(XS__emote);
XS(XS__emote)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: emote(str)");

	char *		str = (char *)SvPV_nolen(ST(0));

	quest_manager.emote(str);

	XSRETURN_EMPTY;
}

XS(XS__shout);
XS(XS__shout)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: shout(str)");

	char *		str = (char *)SvPV_nolen(ST(0));

	quest_manager.shout(str);

	XSRETURN_EMPTY;
}

XS(XS__shout2);
XS(XS__shout2)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: shout2(str)");

	char *		str = (char *)SvPV_nolen(ST(0));

	quest_manager.shout2(str);

	XSRETURN_EMPTY;
}

XS(XS__depop);
XS(XS__depop)
{
	dXSARGS;
	if (items < 0 || items > 1)
		Perl_croak(aTHX_ "Usage: depop(npc_type= 0)");

	int	npc_type;

	if (items < 1)
		npc_type = 0;
	else
		npc_type = (int)SvIV(ST(0));
	

	quest_manager.depop(npc_type);

	XSRETURN_EMPTY;
}

XS(XS__settarget);
XS(XS__settarget)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: settarget(type, target_id)");

	char *		type = (char *)SvPV_nolen(ST(0));
	int	target_id = (int)SvIV(ST(1));

	quest_manager.settarget(type, target_id);

	XSRETURN_EMPTY;
}

XS(XS__follow);
XS(XS__follow)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: follow(entity_id)");

	int	entity_id = (int)SvIV(ST(0));

	quest_manager.follow(entity_id);

	XSRETURN_EMPTY;
}

XS(XS__sfollow);
XS(XS__sfollow)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: sfollow()");


	quest_manager.sfollow();

	XSRETURN_EMPTY;
}

XS(XS__cumflag);
XS(XS__cumflag)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: cumflag()");


	quest_manager.cumflag();

	XSRETURN_EMPTY;
}

XS(XS__flagnpc);
XS(XS__flagnpc)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: flagnpc(flag_num, flag_value)");

	int	flag_num = (int)SvIV(ST(0));
	int	flag_value = (int)SvIV(ST(1));

	quest_manager.flagnpc(flag_num, flag_value);

	XSRETURN_EMPTY;
}

XS(XS__flagcheck);
XS(XS__flagcheck)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: flagcheck(flag_to_check, flag_to_set)");

	int	flag_to_check = (int)SvIV(ST(0));
	int	flag_to_set = (int)SvIV(ST(1));

	quest_manager.flagcheck(flag_to_check, flag_to_set);

	XSRETURN_EMPTY;
}

XS(XS__changedeity);
XS(XS__changedeity)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: changedeity(diety_id)");

	int	diety_id = (int)SvIV(ST(0));

	quest_manager.changedeity(diety_id);

	XSRETURN_EMPTY;
}

XS(XS__exp);
XS(XS__exp)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: exp(amt)");

	int	amt = (int)SvIV(ST(0));

	quest_manager.exp(amt);

	XSRETURN_EMPTY;
}

XS(XS__level);
XS(XS__level)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: level(newlevel)");

	int	newlevel = (int)SvIV(ST(0));

	quest_manager.level(newlevel);

	XSRETURN_EMPTY;
}

XS(XS__traindisc);
XS(XS__traindisc)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: traindisc(discipline_tome_item_id)");

	int	discipline_tome_item_id = (int)SvIV(ST(0));
	
	quest_manager.traindisc(discipline_tome_item_id);
	
	XSRETURN_EMPTY;
}

XS(XS__isdisctome);
XS(XS__isdisctome)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: isdisctome(item_id)");

	bool RETVAL;
	int	item_id = (int)SvIV(ST(0));

	RETVAL = quest_manager.isdisctome(item_id);

	ST(0) = boolSV(RETVAL);
	sv_2mortal(ST(0));
	XSRETURN(1);
}

XS(XS__safemove);
XS(XS__safemove)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: safemove()");


	quest_manager.safemove();

	XSRETURN_EMPTY;
}

XS(XS__rain);
XS(XS__rain)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: rain(weather)");

	int	weather = (int)SvIV(ST(0));

	quest_manager.rain(weather);

	XSRETURN_EMPTY;
}

XS(XS__snow);
XS(XS__snow)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: snow(weather)");

	int	weather = (int)SvIV(ST(0));

	quest_manager.snow(weather);

	XSRETURN_EMPTY;
}

XS(XS__surname);
XS(XS__surname)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: surname(name)");

	char *		name = (char *)SvPV_nolen(ST(0));

	quest_manager.surname(name);

	XSRETURN_EMPTY;
}

XS(XS__permaclass);
XS(XS__permaclass)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: permaclass(class_id)");

	int	class_id = (int)SvIV(ST(0));

	quest_manager.permaclass(class_id);

	XSRETURN_EMPTY;
}

XS(XS__permarace);
XS(XS__permarace)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: permarace(race_id)");

	int	race_id = (int)SvIV(ST(0));

	quest_manager.permarace(race_id);

	XSRETURN_EMPTY;
}

XS(XS__permagender);
XS(XS__permagender)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: permagender(gender_id)");

	int	gender_id = (int)SvIV(ST(0));

	quest_manager.permagender(gender_id);

	XSRETURN_EMPTY;
}

XS(XS__scribespells);
XS(XS__scribespells)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: scribespells()");


	quest_manager.scribespells();

	XSRETURN_EMPTY;
}

XS(XS__givecash);
XS(XS__givecash)
{
	dXSARGS;
	if (items != 4)
		Perl_croak(aTHX_ "Usage: givecash(copper, silver, gold, platinum)");

	int	copper = (int)SvIV(ST(0));
	int	silver = (int)SvIV(ST(1));
	int	gold = (int)SvIV(ST(2));
	int	platinum = (int)SvIV(ST(3));

	quest_manager.givecash(copper, silver, gold, platinum);

	XSRETURN_EMPTY;
}

XS(XS__pvp);
XS(XS__pvp)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: pvp(mode)");

	char *		mode = (char *)SvPV_nolen(ST(0));

	quest_manager.pvp(mode);

	XSRETURN_EMPTY;
}

XS(XS__movepc);
XS(XS__movepc)
{
	dXSARGS;
	if (items != 4)
		Perl_croak(aTHX_ "Usage: movepc(zone_id, x, y, z)");

	int	zoneid = (int)SvIV(ST(0));
	float	x = (float)SvNV(ST(1));
	float	y = (float)SvNV(ST(2));
	float	z = (float)SvNV(ST(3));

	quest_manager.movepc(zoneid, x, y, z);

	XSRETURN_EMPTY;
}

XS(XS__gmmove);
XS(XS__gmmove)
{
	dXSARGS;
	if (items != 3)
		Perl_croak(aTHX_ "Usage: gmmove(x, y, z)");

	float	x = (float)SvNV(ST(0));
	float	y = (float)SvNV(ST(1));
	float	z = (float)SvNV(ST(2));

	quest_manager.gmmove(x, y, z);

	XSRETURN_EMPTY;
}

XS(XS__movegrp);
XS(XS__movegrp)
{
	dXSARGS;
	if (items != 4)
		Perl_croak(aTHX_ "Usage: movegrp(zoneid, x, y, z)");

	int	zoneid = (int)SvIV(ST(0));
	float	x = (float)SvNV(ST(1));
	float	y = (float)SvNV(ST(2));
	float	z = (float)SvNV(ST(3));

	quest_manager.movegrp(zoneid, x, y, z);

	XSRETURN_EMPTY;
}

XS(XS__doanim);
XS(XS__doanim)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: doanim(anim_id)");

	int	anim_id = (int)SvIV(ST(0));

	quest_manager.doanim(anim_id);

	XSRETURN_EMPTY;
}

XS(XS__addskill);
XS(XS__addskill)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: addskill(skill_id, value)");

	int	skill_id = (int)SvIV(ST(0));
	int	value = (int)SvIV(ST(1));

	quest_manager.addskill(skill_id, value);

	XSRETURN_EMPTY;
}

XS(XS__setlanguage);
XS(XS__setlanguage)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: setlanguage(skill_id, value)");

	int	skill_id = (int)SvIV(ST(0));
	int	value = (int)SvIV(ST(1));

	quest_manager.setlanguage(skill_id, value);

	XSRETURN_EMPTY;
}

XS(XS__setskill);
XS(XS__setskill)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: setskill(skill_id, value)");

	int	skill_id = (int)SvIV(ST(0));
	int	value = (int)SvIV(ST(1));

	quest_manager.setskill(skill_id, value);

	XSRETURN_EMPTY;
}

XS(XS__setallskill);
XS(XS__setallskill)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: setallskill(value)");

	int	value = (int)SvIV(ST(0));

	quest_manager.setallskill(value);

	XSRETURN_EMPTY;
}

XS(XS__attack);
XS(XS__attack)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: attack(client_name)");

	char *		client_name = (char *)SvPV_nolen(ST(0));

	quest_manager.attack(client_name);

	XSRETURN_EMPTY;
}

XS(XS__save);
XS(XS__save)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: save()");


	quest_manager.save();

	XSRETURN_EMPTY;
}

XS(XS__faction);
XS(XS__faction)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: faction(faction_id, faction_value)");

	int	faction_id = (int)SvIV(ST(0));
	int	faction_value = (int)SvIV(ST(1));

	quest_manager.faction(faction_id, faction_value);

	XSRETURN_EMPTY;
}

XS(XS__setsky);
XS(XS__setsky)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: setsky(new_sky)");

		unsigned char		new_sky = (unsigned char)SvUV(ST(0));

	quest_manager.setsky(new_sky);

	XSRETURN_EMPTY;
}

XS(XS__setguild);
XS(XS__setguild)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: setguild(new_guild_id, new_rank)");

		unsigned long		new_guild_id = (unsigned long)SvUV(ST(0));
	char	new_rank = (char)*SvPV_nolen(ST(1));

	quest_manager.setguild(new_guild_id, new_rank);

	XSRETURN_EMPTY;
}

XS(XS__settime);
XS(XS__settime)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: settime(new_hour, new_min)");

	char	new_hour = (char)*SvPV_nolen(ST(0));
	char	new_min = (char)*SvPV_nolen(ST(1));

	quest_manager.settime(new_hour, new_min);

	XSRETURN_EMPTY;
}

XS(XS__itemlink);
XS(XS__itemlink)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: itemlink(item_id)");

	int	item_id = (int)SvIV(ST(0));

	quest_manager.itemlink(item_id);

	XSRETURN_EMPTY;
}

XS(XS__signalwith);
XS(XS__signalwith)
{
	dXSARGS;
	
	if (items == 2) {
		int	npc_id = (int)SvIV(ST(0));
		int	signal_id = (int)SvIV(ST(1));
		quest_manager.signalwith(npc_id, signal_id);
	} else if(items == 3) {
		int	npc_id = (int)SvIV(ST(0));
		int	signal_id = (int)SvIV(ST(1));
		int	wait = (int)SvIV(ST(2));
		quest_manager.signalwith(npc_id, signal_id, wait);
	} else {
		Perl_croak(aTHX_ "Usage: signalwith(npc_id,signal_id[,wait_ms])");
	}

	XSRETURN_EMPTY;
}

XS(XS__signal);
XS(XS__signal)
{
	dXSARGS;
	
	if (items == 1) {
		int	npc_id = (int)SvIV(ST(0));
		quest_manager.signal(npc_id);
	} else if(items == 2) {
		int	npc_id = (int)SvIV(ST(0));
		int	wait = (int)SvIV(ST(1));
		quest_manager.signal(npc_id, wait);
	} else {
		Perl_croak(aTHX_ "Usage: signal(npc_id[,wait_ms])");
	}

	XSRETURN_EMPTY;
}

XS(XS__setglobal);
XS(XS__setglobal)
{
	dXSARGS;
	if (items != 4)
		Perl_croak(aTHX_ "Usage: setglobal(varname, newvalue, options, duration)");

	char *		varname = (char *)SvPV_nolen(ST(0));
	char *		newvalue = (char *)SvPV_nolen(ST(1));
	int	options = (int)SvIV(ST(2));
	char *		duration = (char *)SvPV_nolen(ST(3));

	quest_manager.setglobal(varname, newvalue, options, duration);

	XSRETURN_EMPTY;
}

XS(XS__targlobal);
XS(XS__targlobal)
{
	dXSARGS;
	if (items != 6)
		Perl_croak(aTHX_ "Usage: targlobal(varname, value, duration, npcid, charid, zoneid)");

	char *		varname = (char *)SvPV_nolen(ST(0));
	char *		value = (char *)SvPV_nolen(ST(1));
	char *		duration = (char *)SvPV_nolen(ST(2));
	int	npcid = (int)SvIV(ST(3));
	int	charid = (int)SvIV(ST(4));
	int	zoneid = (int)SvIV(ST(5));

	quest_manager.targlobal(varname, value, duration, npcid, charid, zoneid);

	XSRETURN_EMPTY;
}

XS(XS__delglobal);
XS(XS__delglobal)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: delglobal(varname)");

	char *		varname = (char *)SvPV_nolen(ST(0));

	quest_manager.delglobal(varname);

	XSRETURN_EMPTY;
}

XS(XS__ding);
XS(XS__ding)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: ding()");


	quest_manager.ding();

	XSRETURN_EMPTY;
}

XS(XS__rebind);
XS(XS__rebind)
{
	dXSARGS;
	if (items != 4)
		Perl_croak(aTHX_ "Usage: rebind(zoneid, x, y, z)");

	int	zoneid = (int)SvIV(ST(0));
	float	x = (float)SvNV(ST(1));
	float	y = (float)SvNV(ST(2));
	float	z = (float)SvNV(ST(3));

	quest_manager.rebind(zoneid, x, y, z);

	XSRETURN_EMPTY;
}

XS(XS__start);
XS(XS__start)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: start(wp)");

	int	wp = (int)SvIV(ST(0));

	quest_manager.start(wp);

	XSRETURN_EMPTY;
}

XS(XS__stop);
XS(XS__stop)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: stop()");


	quest_manager.stop();

	XSRETURN_EMPTY;
}

XS(XS__pause);
XS(XS__pause)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: pause(duration)");

	int	duration = (int)SvIV(ST(0));

	quest_manager.pause(duration);

	XSRETURN_EMPTY;
}

XS(XS__moveto);
XS(XS__moveto)
{
	dXSARGS;
	if (items != 3)
		Perl_croak(aTHX_ "Usage: moveto(x, y, z)");

	float	x = (float)SvNV(ST(0));
	float	y = (float)SvNV(ST(1));
	float	z = (float)SvNV(ST(2));

	quest_manager.moveto(x, y, z);

	XSRETURN_EMPTY;
}

XS(XS__resume);
XS(XS__resume)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: resume()");


	quest_manager.resume();

	XSRETURN_EMPTY;
}

XS(XS__addldonpoints);
XS(XS__addldonpoints)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: addldonpoints(points, theme)");

	long	points = (long)SvIV(ST(0));
	unsigned long		theme = (unsigned long)SvUV(ST(1));

	quest_manager.addldonpoints(points, theme);

	XSRETURN_EMPTY;
}

XS(XS__setnexthpevent);
XS(XS__setnexthpevent)
{
	dXSARGS;
	if (items != 1)
		Perl_croak(aTHX_ "Usage: setnexthpevent(at)");

	int	at = (int)SvIV(ST(0));

	quest_manager.setnexthpevent(at);

	XSRETURN_EMPTY;
}

XS(XS__respawn);
XS(XS__respawn)
{
	dXSARGS;
	if (items != 2)
		Perl_croak(aTHX_ "Usage: respawn(npc_type, grid)");

	int	npc_type = (int)SvIV(ST(0));
	int	grid = (int)SvIV(ST(1));

	quest_manager.respawn(npc_type, grid);

	XSRETURN_EMPTY;
}

XS(XS__ChooseRandom);
XS(XS__ChooseRandom)
{
	dXSARGS;
	if (items < 1)
		Perl_croak(aTHX_ "Usage: ChooseRandom(... list ...)");
	
	int index = MakeRandomInt(0, items-1);
	
	//Im not 100% sure if I need to clear out ST(0) first...
	ST(0) = sv_2mortal(ST(index));
	
	XSRETURN(1);	//return 1 element from the stack (ST(0))
}

XS(XS__set_proximity);
XS(XS__set_proximity)
{
	dXSARGS;
	if (items != 4 && items != 6)
		Perl_croak(aTHX_ "Usage: set_proximity(minx, maxx, miny, maxy [, minz, maxz])");
	
	float minx = (float)SvNV(ST(0));
	float maxx = (float)SvNV(ST(1));
	float miny = (float)SvNV(ST(2));
	float maxy = (float)SvNV(ST(3));

	if(items == 4)
		quest_manager.set_proximity(minx, maxx, miny, maxy);
	else {
		float minz = (float)SvNV(ST(4));
		float maxz = (float)SvNV(ST(5));
		quest_manager.set_proximity(minx, maxx, miny, maxy, minz, maxz);
	}
	
	XSRETURN_EMPTY;
}

XS(XS__clear_proximity);
XS(XS__clear_proximity)
{
	dXSARGS;
	if (items != 0)
		Perl_croak(aTHX_ "Usage: clear_proximity()");
	
	quest_manager.clear_proximity();
	
	XSRETURN_EMPTY;
}

XS(XS__setanim);
XS(XS__setanim) //Cisyouc: mob->setappearance() addition
{
	dXSARGS;
	if(items != 2)
		Perl_croak(aTHX_ "Usage: quest::setanim(npc_type, animnum);");
	
	quest_manager.setanim(SvUV(ST(0)), SvUV(ST(1)));

	XSRETURN_EMPTY;
}

/*

This is the callback perl will look for to setup the
quest package's XSUBs
*/
EXTERN_C XS(boot_quest); // prototype to pass -Wmissing-prototypes
EXTERN_C XS(boot_quest)
{
	dXSARGS;
	char file[256];
	strncpy(file, __FILE__, 256);
	file[255] = '\0';
	
	if(items != 1)
		LogFile->write(EQEMuLog::Error, "boot_quest does not take any arguments.");
	
	char buf[128];	//shouldent have any function names longer than this.
	
	//add the strcpy stuff to get rid of const warnings....
	
	XS_VERSION_BOOTCHECK ;
		newXS(strcpy(buf, "echo"), XS__say, file);
		newXS(strcpy(buf, "say"), XS__say, file);
		newXS(strcpy(buf, "me"), XS__say, file);
		newXS(strcpy(buf, "summonitem"), XS__summonitem, file);
		newXS(strcpy(buf, "write"), XS__write, file);
		newXS(strcpy(buf, "spawn"), XS__spawn, file);
		newXS(strcpy(buf, "spawn2"), XS__spawn2, file);
		newXS(strcpy(buf, "unique_spawn"), XS__unique_spawn, file);
		newXS(strcpy(buf, "setstat"), XS__setstat, file);
		newXS(strcpy(buf, "castspell"), XS__castspell, file);
		newXS(strcpy(buf, "selfcast"), XS__selfcast, file);
		newXS(strcpy(buf, "addloot"), XS__addloot, file);
		newXS(strcpy(buf, "zone"), XS__zone, file);
		newXS(strcpy(buf, "settimer"), XS__settimer, file);
		newXS(strcpy(buf, "emote"), XS__emote, file);
		newXS(strcpy(buf, "shout"), XS__shout, file);
		newXS(strcpy(buf, "shout2"), XS__shout2, file);
		newXS(strcpy(buf, "depop"), XS__depop, file);
		newXS(strcpy(buf, "settarget"), XS__settarget, file);
		newXS(strcpy(buf, "follow"), XS__follow, file);
		newXS(strcpy(buf, "sfollow"), XS__sfollow, file);
		newXS(strcpy(buf, "cumflag"), XS__cumflag, file);
		newXS(strcpy(buf, "flagnpc"), XS__flagnpc, file);
		newXS(strcpy(buf, "flagcheck"), XS__flagcheck, file);
		newXS(strcpy(buf, "changedeity"), XS__changedeity, file);
		newXS(strcpy(buf, "exp"), XS__exp, file);
		newXS(strcpy(buf, "level"), XS__level, file);
		newXS(strcpy(buf, "traindisc"), XS__traindisc, file);
		newXS(strcpy(buf, "isdisctome"), XS__isdisctome, file);
		newXS(strcpy(buf, "safemove"), XS__safemove, file);
		newXS(strcpy(buf, "rain"), XS__rain, file);
		newXS(strcpy(buf, "snow"), XS__snow, file);
		newXS(strcpy(buf, "surname"), XS__surname, file);
		newXS(strcpy(buf, "permaclass"), XS__permaclass, file);
		newXS(strcpy(buf, "permarace"), XS__permarace, file);
		newXS(strcpy(buf, "permagender"), XS__permagender, file);
		newXS(strcpy(buf, "scribespells"), XS__scribespells, file);
		newXS(strcpy(buf, "givecash"), XS__givecash, file);
		newXS(strcpy(buf, "pvp"), XS__pvp, file);
		newXS(strcpy(buf, "movepc"), XS__movepc, file);
		newXS(strcpy(buf, "gmmove"), XS__gmmove, file);
		newXS(strcpy(buf, "movegrp"), XS__movegrp, file);
		newXS(strcpy(buf, "doanim"), XS__doanim, file);
		newXS(strcpy(buf, "addskill"), XS__addskill, file);
		newXS(strcpy(buf, "setlanguage"), XS__setlanguage, file);
		newXS(strcpy(buf, "setskill"), XS__setskill, file);
		newXS(strcpy(buf, "setallskill"), XS__setallskill, file);
		newXS(strcpy(buf, "attack"), XS__attack, file);
		newXS(strcpy(buf, "save"), XS__save, file);
		newXS(strcpy(buf, "faction"), XS__faction, file);
		newXS(strcpy(buf, "setsky"), XS__setsky, file);
		newXS(strcpy(buf, "setguild"), XS__setguild, file);
		newXS(strcpy(buf, "settime"), XS__settime, file);
		newXS(strcpy(buf, "itemlink"), XS__itemlink, file);
		newXS(strcpy(buf, "signal"), XS__signal, file);
		newXS(strcpy(buf, "signalwith"), XS__signalwith, file);
		newXS(strcpy(buf, "setglobal"), XS__setglobal, file);
		newXS(strcpy(buf, "targlobal"), XS__targlobal, file);
		newXS(strcpy(buf, "delglobal"), XS__delglobal, file);
		newXS(strcpy(buf, "ding"), XS__ding, file);
		newXS(strcpy(buf, "rebind"), XS__rebind, file);
		newXS(strcpy(buf, "start"), XS__start, file);
		newXS(strcpy(buf, "stop"), XS__stop, file);
		newXS(strcpy(buf, "pause"), XS__pause, file);
		newXS(strcpy(buf, "moveto"), XS__moveto, file);
		newXS(strcpy(buf, "resume"), XS__resume, file);
		newXS(strcpy(buf, "addldonpoints"), XS__addldonpoints, file);
		newXS(strcpy(buf, "setnexthpevent"), XS__setnexthpevent, file);
		newXS(strcpy(buf, "respawn"), XS__respawn, file);
        newXS(strcpy(buf, "getItemName"), XS_qc_getItemName, file);
        newXS(strcpy(buf, "ChooseRandom"), XS__ChooseRandom, file);
        newXS(strcpy(buf, "set_proximity"), XS__set_proximity, file);
        newXS(strcpy(buf, "clear_proximity"), XS__clear_proximity, file);
        newXS(strcpy(buf, "setanim"), XS__setanim, file);
	XSRETURN_YES;
}




#endif
#endif
