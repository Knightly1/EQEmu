
#include "StdAfx.h"

//#include <dlfcn.h>
#include <iostream>
#include <map>
#include "PluginManager.h"
#include "../common/SharedLibrary.h"

using namespace std;

PluginManager::PluginManager() {
	GetHandlerCallbacks(&callbacks);
}

PluginManager::~PluginManager()
{
map<string,SharedLibrary *>::iterator pitr;
	while((pitr=LoadedPlugins.begin())!=LoadedPlugins.end()) {
		UnloadHook on_unload;
		if (pitr->second->GetSym("on_unload",(void **)&on_unload)) {
			on_unload(&callbacks);
		}
		pitr->second->Unload();
		cout << pitr->first << " unloaded." << endl;
		LoadedPlugins.erase(pitr);
	}
}

bool PluginManager::load(const char *name)
{
bool result=false;
	
	//parse parameter
	char libname[256];
	char arg[512];
	const char *from = name;
	char *to = libname;
	for(; *from != '\0' && *from != '='; from++, to++) {
		*to = *from;
	}
	*to = '\0';
	if(*from == '=') {
		from++;
		to = arg;
		for(; *from != '\0' && *from != '='; from++, to++) {
			*to = *from;
		}
		*to = '\0';
	} else {
		arg[0] = '\0';
	}
	
	//see if we allready have it loaded
	map<string,SharedLibrary *>::iterator res;
	res = LoadedPlugins.find(libname);
	if(res != LoadedPlugins.end()) {
		if(res->second->Loaded()) {
			//allready loaded... do nothing.
			return(true);
		}
		//else, it was unloaded but remains in our map for some reason...
		//delete it and maybe it will re-load below.
		safe_delete(res->second);
		LoadedPlugins.erase(res);
	}
	
	//load the actual library
	SharedLibrary *lib=new SharedLibrary();
	lib->Load(libname);
	if (lib->Loaded()) {
		LoadedPlugins[libname]=lib;
		cout << name << " loaded." << endl;
		result=true;
		LoadHook on_load;
		if (lib->GetSym("on_load",(void **)&on_load)) {
			on_load(&callbacks, arg);
		}
	} else {
		cerr << "Could not load plugin '" << libname << "': " << lib->GetError() << endl;
		safe_delete(lib);
	}

	return result;

}

bool PluginManager::unload(const char *name)
{
map<string,SharedLibrary *>::iterator pitr;
bool result=false;
	if ((pitr=LoadedPlugins.find(name))!=LoadedPlugins.end()) {
		UnloadHook on_unload;
		if (pitr->second->GetSym("on_unload",(void **)&on_unload)) {
			on_unload(&callbacks);
		}
		pitr->second->Unload();
		cout << pitr->first << " unloaded." << endl;
		LoadedPlugins.erase(pitr);
	} else {
		cerr << "Plugin '" << name << "' not loaded." << endl;
	}

	return result;
}
