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

//#pragma message("You may want to ensure that you add perl\\lib\\CORE to your include path")
//#pragma message("You may want to ensure that your build settings look like `perl -MExtUtils::Embed -e ccopts -e ldopts`")
//link against your Perl Lib
//#pragma comment(lib, "perl56.lib")
#ifdef WIN32
#pragma comment(lib, "perl58.lib")
#endif

//so embedded scripts can use xs extensions (ala 'use socket;')
EXTERN_C void boot_DynaLoader(pTHX_ CV* cv);
EXTERN_C void xs_init(pTHX) 
{ 
	char *file = __FILE__; 
	newXS("DynaLoader::boot_DynaLoader", boot_DynaLoader, file); 
	newXS("quest::boot_qc", boot_qc, file);
}

Embperl::Embperl()
{
	//arguments for interpreter start
	char * args[] = {"", "-e", "0"};
	my_perl = perl_alloc();
	if(!my_perl)
		throw "Failed to init Perl (perl_alloc)";
	perl_construct(my_perl);
	if(perl_parse(my_perl, xs_init, 3, args, NULL))
		throw "perl_parse failed";
	perl_run(my_perl);
	eval_pv("sub my_eval {eval $_[0];}",true);
	//ruin the perl exit command:
	eval_pv("sub my_exit {}",true);
	if(gv_stashpv("CORE::GLOBAL", FALSE)) {
		GV *exitgp = gv_fetchpv("CORE::GLOBAL::exit", TRUE, SVt_PVCV);
		GvCV(exitgp) = perl_get_cv("my_exit", TRUE);
		GvIMPORTED_CV_on(exitgp);
	}
	try { init_eval_file(); }
	catch(const char *err)
	{ 
		//remember... lasterr() is no good if we crap out here, in construction
		LogFile->write(EQEMuLog::Status, "perl error: %s", err);
		throw "failed to install eval_file hook"; 
	}
#ifdef EMBPERL_PLUGIN
	try { eval("package plugin; use IO::Scalar;$plugin::printbuff='';tie *PLUGIN,'IO::Scalar',\\$plugin::printbuff;"); }
	catch(const char *err) { throw "failed to install plugin printhook, do you lack IO::Scalar?"; }
	LogFile->write(EQEMuLog::Status, "Loading perlemb plugins.");
	try
	{
		eval_file("plugin", "plugin.pl");
	}
	catch(const char *err)
	{ 
		LogFile->write(EQEMuLog::Status, "Warning - plugin.pl: %s", err);
	}
	try
	{
		//should probably read the directory in c, instead, so that
		//I can echo filenames as I do it, but c'mon... I'm lazy and this 1 line reads in all the plugins
		eval("if(opendir(D,'plugins')){my@d=readdir(D);closedir(D);foreach(@d){main::eval_file('plugin','plugins/'.$_)if/\\.pl$/;}}");
	}
	catch(const char *err)
	{ 
		LogFile->write(EQEMuLog::Status, "Perl warning: %s", err);
	}
#endif //EMBPERL_PLUGIN
}

Embperl::~Embperl()
{
	perl_destruct(my_perl);
	perl_free(my_perl);
}

void Embperl::init_eval_file(void) const
{//ala perlembed
	eval(
		"our %Cache;"
		"use Symbol qw(delete_package);"
		"sub eval_file {"
			"my($package, $filename) = @_;"
			"$filename=~s/\'//g;"
			"my $mtime = -M $filename;"
			"if(defined $Cache{$package}{mtime}&&$Cache{$package}{mtime} <= $mtime && !($package eq 'plugin')){ return; }"

			"else {"
				"local *FH;open FH, $filename or die \"open '$filename' $!\";"
				"local($/) = undef;my $sub = <FH>;close FH;"
				"my $eval = qq{package $package; sub handler { $sub; }};"
				"{ my($filename,$mtime,$package,$sub); eval $eval; }"
				"die $@ if $@;"
				"$Cache{$package}{mtime} = $mtime; ${$package.'::isloaded'} = 1;}}"
		);
 }

void Embperl::eval_file(const char * packagename, const char * filename) const
{
	std::vector<std::string> args;
	args.push_back(packagename);
	args.push_back(filename);
	dosub("eval_file", &args);
}

void Embperl::dosub(const char * subname, const std::vector<std::string> * args, int mode) const
{//as seen in perlembed docs
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
	if(err)
	{
		errmsg = "Perl runtime error: ";
		errmsg += SvPVX(ERRSV);
		throw errmsg.c_str();
	}
}

//evaluate an expression. throw error on fail
void Embperl::eval(const char * code) const
{
	std::vector<std::string> arg;
	arg.push_back(code);
// MYRA - added EVAL & KEEPERR to eval per Eglin's recommendation	
	dosub("my_eval", &arg, G_SCALAR|G_DISCARD|G_EVAL|G_KEEPERR);
//end Myra
}


#endif //EMBPERL

#endif //EMBPERL_CPP
