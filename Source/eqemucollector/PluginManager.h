#ifndef _PLUGINMANAGER_H

#define _PLUGINMANAGER_H

#include <map>
#include <string>
#include "PacketHandler.h"

using namespace std;

class SharedLibrary;

//this is a plugin manager for packet collector's plugins
class PluginManager {
protected:
	HandlerCallbacks callbacks;
	map<string,SharedLibrary *> LoadedPlugins;

	typedef int(*LoadHook)(const HandlerCallbacks *calls, const char *arg);
	typedef int(*UnloadHook)(const HandlerCallbacks *calls);
public:
	PluginManager();
	~PluginManager();
	bool load(const char *name);
	bool unload(const char *name);
};

#endif
