#ifndef _SHAREDLIBRARY_H
#define _SHAREDLIBRARY_H

#ifdef WIN32
#include <windows.h>
#endif

class SharedLibrary {
public:
	SharedLibrary();
	virtual ~SharedLibrary();
	
	//two call styles for GetSym, one returns bool, other NULL for fail
	bool GetSym(const char *name, void **sym);
	void *GetSym(const char *name);
	
	const char *GetError();
	
	virtual bool Load(const char *file);
	virtual void Unload();
	
	inline bool	Loaded() { return (hDLL != 0); }
	
protected:
#ifdef WIN32
	HINSTANCE hDLL;
#else
	void* hDLL;
#endif
};

#endif
