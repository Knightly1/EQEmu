
#include "../debug.h"
#include "patches.h"

#include "Client62.h"
#include "Titanium.h"
#include "Live.h"


void RegisterAllPatches(EQStreamIdentifier &into) {
	Client62::Register(into);
	Titanium::Register(into);
	Live::Register(into);
}

void ReloadAllPatches() {
	Client62::Reload();
	Titanium::Reload();
	Live::Reload();
}



















