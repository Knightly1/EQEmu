/*
Embperl.h
---------------
eqemu perl wrapper
Eglin
*/

#ifndef EMBPERL_H
#define EMBPERL_H

#ifdef EMBPERL

#include <string>
#include <vector>

//headers from the Perl distribution
#include <EXTERN.h> 
#define WIN32IO_IS_STDIO

extern "C" {	//the perl headers dont do this for us...
#include <perl.h>
#include <XSUB.h>
};

//perl defines this macro and dosent clean it up, lazy bastards.
#ifdef Copy
#undef Copy
#endif


//so embedded scripts can use xs extensions (ala 'use socket;')
EXTERN_C void boot_DynaLoader(pTHX_ CV* cv);
EXTERN_C void xs_init(pTHX);

class Embperl
{
private:
	//if we fail inside a script evaluation, this will hold the croak msg (not much help if we die during construction, but that's our own fault)
	mutable std::string errmsg;
	//kludgy workaround for the fact that we can't directly do something like SvIV(get_sv($big[0]{ass}->{struct}))
	SV * my_get_sv(const char * varname) const {
		eval(std::string("$scratch::temp = ").append(varname).append(";").c_str());
		return get_sv("scratch::temp", false);
	}
	//install a perl func
	void init_eval_file(void) const;
protected:
	//the embedded interpreter
	PerlInterpreter * my_perl;
public:
	Embperl(void); //This can throw errors!  Buyer beware
	~Embperl(void);
	//return the last error msg
	std::string lasterr(void) const { return errmsg;};
	//evaluate an expression. throws string errors on fail
	void eval(const char * code) const;
	//execute a subroutine.  throws lasterr on failure
	void dosub(const char * subname, const std::vector<std::string> * args = NULL, int mode = G_SCALAR|G_DISCARD|G_EVAL) const;
	//returns the contents of the perl variable named in varname as a c int
	int geti(const char * varname) const { return SvIV(my_get_sv(varname));};
	//returns the contents of the perl variable named in varname as a c double
	double getd(const char * varname) const { return SvNV(my_get_sv(varname));};
	//returns the contents of the perl variable named in varname as a string
	std::string getstr(const char * varname) const {
		SV * temp = my_get_sv(varname);
		return std::string(SvPV_nolen(temp),SvLEN(temp));
	}
	//loads a file and compiles it into our interpreter (assuming it hasn't already been read in)
	//idea borrowed from perlembed
	void eval_file(const char * packagename, const char * filename) const;
};

#endif //EMBPERL

#endif //EMBPERL_H
