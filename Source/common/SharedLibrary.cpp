#include "SharedLibrary.h"
#include <stdio.h>

#ifdef WIN32
	#define snprintf	_snprintf
	#define vsnprintf	_vsnprintf
	#define strncasecmp	_strnicmp
	#define strcasecmp  _stricmp

	#define EmuLibName "EMuShareMem"
#else
	#define EmuLibName "libEMuShareMem.so"

	#include "../common/unix.h"
	#include <dlfcn.h>
    #define GetProcAddress(a,b) dlsym(a,b)
	#define LoadLibrary(a) dlopen(a, RTLD_NOW) 
	#define  FreeLibrary(a) dlclose(a)
	#define GetLastError() dlerror()
#endif

SharedLibrary::SharedLibrary() {
	hDLL = NULL;
}

SharedLibrary::~SharedLibrary() {
	Unload();
}

bool SharedLibrary::Load(const char *name)
{
#ifdef WIN32
	SetLastError(0);
#endif
	
	hDLL = LoadLibrary(name);
	
	if(!hDLL) {
		const char *load_error = GetError();
		fprintf(stderr, "[Error] Load Shared Library '%s' failed.  Error=%s\n", name, load_error?load_error:"Null Return, no error");
		return false;
	}
#ifdef WIN32
    else { SetLastError(0); } // Clear the win9x error
#endif
	
	return(true);
}

void SharedLibrary::Unload() {
	if (hDLL != NULL) {
		FreeLibrary(hDLL);
#ifndef WIN32
		const char* error;
		if ((error = GetError()) != NULL)
			fprintf(stderr, "FreeLibrary() error = %s", error);
#endif
		hDLL = NULL;
	}
}

void *SharedLibrary::GetSym(const char *name) {
	if (!Loaded())
		return(NULL);
	
	void *r = GetProcAddress(hDLL, name);

	if(GetError() != NULL)
		r = NULL;

	return(r);
}

bool SharedLibrary::GetSym(const char *name, void **sym)
{
	bool result=false;
	if (Loaded()) {
		*sym = GetProcAddress(hDLL, name);
		result= (GetError() == NULL);
	}

	return result;
}

const char *SharedLibrary::GetError()
{
#ifdef WIN32
	//not thread safe, dont care.
	static char ErrBuf[128];
	unsigned long err = GetLastError();
	if(err == 0)
		return(NULL);
	sprintf(ErrBuf, "Error #%lu", err);
	return(ErrBuf);
#else
	return GetLastError();
#endif
}
