//extends the parser to include perl
//Eglin

#ifndef EMBPARSER_H
#define EMBPARSER_H

#ifdef EMBPERL

#include "client.h"
#include "parser.h"
#include "embperl.h"


class PerlembParser : public Parser
{
private:
	Embperl * perl;
	//export a symbol table of sorts
	void map_funs(void) const;
public:
	PerlembParser(void);
	~PerlembParser();
	Embperl * getperl(void) { return perl; };
	//todo, consider making the following two methods static (need to check for perl!=null, first, then)
	bool isloaded(const char *packagename) const { return perl->geti(std::string("$").append(packagename).append("::isloaded").c_str()); }
	bool isdefault(const char *packagename) const { return perl->geti(std::string("$").append(packagename).append("::isdefault").c_str()); }
	void Event(int event, int32 npcid, const char * data, Mob* npcmob, Mob* mob);
	void LoadScript(int npcid, const char * zone) const;
	//expose a var to the script (probably parallels addvar))
	//i.e. exportvar("qst1234", "name", "somemob"); 
	//would expose the variable $name='somemob' to the script that handles npc1234
	//I don't escape the strings, so use caution!!
	void ExportVar(const char * pkgprefix, const char * varname, const char * value) const;
	//get an appropriate namespage/packagename from an npcid
	std::string GetPkgPrefix(int32 npcid, bool defaultOK = true) const;
	//call the appropriate perl handler. afterwards, parse and dispatch the command queue
	//SendCommands("qst1234", "EVENT_SAY") would trigger sub EVENT_SAY() from the qst1234.pl file
	void SendCommands(const char * pkgprefix, const char *event, int32 npcid, Mob* other, Mob* mob);
	void ReloadQuests();
};

#endif //EMBPERL

#endif //EMBPARSER_H
