/*
embperl.cpp
---------------
wraps a perl interpreter for use in eqemu
Eglin
*/

#ifndef EMBPERL_CPP
#define EMBPERL_CPP

#ifdef EMBPERL

#include <cstdio>
#include <cstdarg>
#include <vector>
#include "../common/debug.h"
#include "embperl.h"
#include "embxs.h" 
#include "features.h"

//#pragma message("You may want to ensure that you add perl\\lib\\CORE to your include path")
//#pragma message("You may want to ensure that your build settings look like `perl -MExtUtils::Embed -e ccopts -e ldopts`")
//link against your Perl Lib
//#pragma comment(lib, "perl56.lib")
#ifdef WIN32
#pragma comment(lib, "perl58.lib")
#endif

#ifdef EMBPERL_XS
EXTERN_C XS(boot_quest);
#ifdef EMBPERL_XS_CLASSES
EXTERN_C XS(boot_Mob);
EXTERN_C XS(boot_NPC);
EXTERN_C XS(boot_Client);
EXTERN_C XS(boot_Corpse);
EXTERN_C XS(boot_EntityList);
EXTERN_C XS(boot_Group);
/*XS(XS_Client_new);
//XS(XS_Mob_new);
XS(XS_NPC_new);
//XS(XS_Corpse_new);
XS(XS_EntityList_new);
//XS(XS_Group_new);*/
#endif
#endif
#ifdef EMBPERL_COMMANDS
XS(XS_command_add);
#endif

#ifdef EMBPERL_IO_CAPTURE
XS(XS_EQEmuIO_PRINT);
#endif //EMBPERL_IO_CAPTURE

//so embedded scripts can use xs extensions (ala 'use socket;')
EXTERN_C void boot_DynaLoader(pTHX_ CV* cv);
EXTERN_C void xs_init(pTHX) 
{ 
	char file[256];
	strncpy(file, __FILE__, 256);
	file[255] = '\0';
	
	char buf[128];	//shouldent have any function names longer than this.
	
	//add the strcpy stuff to get rid of const warnings....
	
	newXS(strcpy(buf, "DynaLoader::boot_DynaLoader"), boot_DynaLoader, file); 
	newXS(strcpy(buf, "quest::boot_qc"), boot_qc, file);
#ifdef EMBPERL_XS
	newXS(strcpy(buf, "quest::boot_quest"), boot_quest, file);
#ifdef EMBPERL_XS_CLASSES
	newXS(strcpy(buf, "Mob::boot_Mob"), boot_Mob, file);
	newXS(strcpy(buf, "NPC::boot_Mob"), boot_Mob, file);
	newXS(strcpy(buf, "NPC::boot_NPC"), boot_NPC, file);
///	newXS(strcpy(buf, "NPC::new"), XS_NPC_new, file);
	newXS(strcpy(buf, "Corpse::boot_Mob"), boot_Mob, file);
	newXS(strcpy(buf, "Corpse::boot_Corpse"), boot_Corpse, file);
	newXS(strcpy(buf, "Client::boot_Mob"), boot_Mob, file);
	newXS(strcpy(buf, "Client::boot_Client"), boot_Client, file);
//	newXS(strcpy(buf, "Client::new"), XS_Client_new, file);
	newXS(strcpy(buf, "EntityList::boot_EntityList"), boot_EntityList, file);
//	newXS(strcpy(buf, "EntityList::new"), XS_EntityList_new, file);
	newXS(strcpy(buf, "Group::boot_Group"), boot_Group, file);
#endif
#endif
#ifdef EMBPERL_COMMANDS
	newXS(strcpy(buf, "commands::command_add"), XS_command_add, file);
#endif
#ifdef EMBPERL_IO_CAPTURE
	newXS(strcpy(buf, "EQEmuIO::PRINT"), XS_EQEmuIO_PRINT, file);
#endif
}

Embperl::Embperl()
{
	in_use = true;	//in case one of these files generates an event
	//arguments for interpreter start
	char *args[] = { "", "-e", "0" };
	
	//setup perl...
	my_perl = perl_alloc();
	if(!my_perl)
		throw "Failed to init Perl (perl_alloc)";
	perl_construct(my_perl);
	if(perl_parse(my_perl, xs_init, 3, args, NULL))
		throw "perl_parse failed";
	perl_run(my_perl);
	
	//a little routine we use a lot.
	eval_pv("sub my_eval {eval $_[0];}",true);
	
	//ruin the perl exit command:
	eval_pv("sub my_exit {}",true);
	if(gv_stashpv("CORE::GLOBAL", FALSE)) {
		GV *exitgp = gv_fetchpv("CORE::GLOBAL::exit", TRUE, SVt_PVCV);
		GvCV(exitgp) = perl_get_cv("my_exit", TRUE);
		GvIMPORTED_CV_on(exitgp);
	}
	
	//declare our file eval routine.
	try {
		init_eval_file();
	}
	catch(const char *err)
	{ 
		//remember... lasterr() is no good if we crap out here, in construction
		LogFile->write(EQEMuLog::Quest, "perl error: %s", err);
		throw "failed to install eval_file hook"; 
	}
	
#ifdef EMBPERL_IO_CAPTURE
	//make a tieable class to capture IO and pass it into EQEMuLog
	eval_pv("package EQEmuIO; "
			"&boot_EQEmuIO;"
 			"sub TIEHANDLE { bless {}, $_[0]; } } "
  			"sub PRINTF { my $me = shift; $me->PRINT(sprintf(@_)); } "
  			"package plugin;"
  			"tie *STDOUT, 'EQEmuIO';"
  			"tie *STDERR, 'EQEmuIO';"
  		, true);
#endif //EMBPERL_IO_CAPTURE
	
#ifdef EMBPERL_PLUGIN
	try {
		eval(
			"package plugin; "
			"use IO::Scalar;"
			"$plugin::printbuff='';"
			"tie *PLUGIN,'IO::Scalar',\\$plugin::printbuff;");
	}
	catch(const char *err) {
		throw "failed to install plugin printhook, do you lack IO::Scalar?";
	}
	LogFile->write(EQEMuLog::Quest, "Loading perlemb plugins.");
	try
	{
		eval_file("plugin", "plugin.pl");
	}
	catch(const char *err)
	{ 
		LogFile->write(EQEMuLog::Quest, "Warning - plugin.pl: %s", err);
	}
	try
	{
		//should probably read the directory in c, instead, so that
		//I can echo filenames as I do it, but c'mon... I'm lazy and this 1 line reads in all the plugins
		eval(
			"if(opendir(D,'plugins')) { "
			"	my @d = readdir(D);"
			"	closedir(D);"
			"	foreach(@d){ "
			"		main::eval_file('plugin','plugins/'.$_)if/\\.pl$/;"
			"	}"
			"}"
		);
	}
	catch(const char *err)
	{ 
		LogFile->write(EQEMuLog::Quest, "Perl warning: %s", err);
	}
#endif //EMBPERL_PLUGIN
#ifdef EMBPERL_COMMANDS
	LogFile->write(EQEMuLog::Quest, "Loading perl commands...");
	try
	{
		eval_file("commands", "commands.pl");
		dosub("commands::commands_init");
	}
	catch(const char *err)
	{ 
		LogFile->write(EQEMuLog::Quest, "Warning - commands.pl: %s", err);
	}
#endif //EMBPERL_COMMANDS
	in_use = false;
}

Embperl::~Embperl()
{
#ifdef EMBPERL_IO_CAPTURE
	//clean up our handles so perl dosent puke its guts out
	eval(
  			"untie *STDOUT;"
  			"untie *STDERR;"
  	);
#endif
	perl_destruct(my_perl);
	perl_free(my_perl);
}

void Embperl::init_eval_file(void)
{//ala perlembed
	eval(
		"our %Cache;"
		"use Symbol qw(delete_package);"
		"sub eval_file {"
			"my($package, $filename) = @_;"
#ifdef EMBPERL_IO_CAPTURE
  			"tie *STDOUT, 'EQEmuIO';"
  			"tie *STDERR, 'EQEmuIO';"
#endif
			"$filename=~s/\'//g;"
			"my $mtime = -M $filename;"
			"if(defined $Cache{$package}{mtime}&&$Cache{$package}{mtime} <= $mtime && !($package eq 'plugin')){ return; }"

			"else {"
				"local *FH;open FH, $filename or die \"open '$filename' $!\";"
				"local($/) = undef;my $sub = <FH>;close FH;"
				"my $eval = qq{package $package; sub handler { $sub; }};"
				"{ my($filename,$mtime,$package,$sub); eval $eval; }"
				"die $@ if $@;"
				"$Cache{$package}{mtime} = $mtime; ${$package.'::isloaded'} = 1;}"
			"}"
#ifdef EMBPERL_IO_CAPTURE
  			"untie *STDOUT;"
  			"untie *STDERR;"
#endif
		);
 }

void Embperl::eval_file(const char * packagename, const char * filename)
{
	std::vector<std::string> args;
	args.push_back(packagename);
	args.push_back(filename);
	dosub("eval_file", &args);
}

void Embperl::dosub(const char * subname, const std::vector<std::string> * args, int mode)
{//as seen in perlembed docs
#if EQDEBUG >= 5
	if(InUse()) {
		LogFile->write(EQEMuLog::Debug, "Warning: Perl dosub called for %s when perl is allready in use.\n", subname);
	}
#endif
	in_use = true;
	bool err = false;
	dSP;                            /* initialize stack pointer      */
	ENTER;                          /* everything created after here */
	SAVETMPS;                       /* ...is a temporary variable.   */
	PUSHMARK(SP);                   /* remember the stack pointer    */
	if(args && args->size())
	{
		for(std::vector<std::string>::const_iterator i = args->begin(); i != args->end(); ++i)
		{/* push the arguments onto the perl stack  */
			XPUSHs(sv_2mortal(newSVpv(i->c_str(), i->length()))); 
		}
	}
	PUTBACK;                      /* make local stack pointer global */
	call_pv(subname, mode); /*eval our code*/
	SPAGAIN;                        /* refresh stack pointer         */
	if(SvTRUE(ERRSV))
	{
		err = true;
	}
	FREETMPS;                       /* free temp values        */
	LEAVE;                       /* ...and the XPUSHed "mortal" args.*/
	
	in_use = false;
	if(err)
	{
		errmsg = "Perl runtime error: ";
		errmsg += SvPVX(ERRSV);
		throw errmsg.c_str();
	}
}

//evaluate an expression. throw error on fail
void Embperl::eval(const char * code)
{
	std::vector<std::string> arg;
	arg.push_back(code);
// MYRA - added EVAL & KEEPERR to eval per Eglin's recommendation	
	dosub("my_eval", &arg, G_SCALAR|G_DISCARD|G_EVAL|G_KEEPERR);
//end Myra
}

bool Embperl::SubExists(const char *package, const char *sub) {
	HV *stash = gv_stashpv(package, false);
	if(!stash)
		return(false);
	int len = strlen(sub);
	return(hv_exists(stash, sub, len));
}


#endif //EMBPERL

#endif //EMBPERL_CPP
