#ifndef EXTRACTCOLLECTORS_H
#define EXTRACTCOLLECTORS_H

#include "../common/types.h"
#include "../common/emu_opcodes.h"

#include <stdio.h>
#include <map>
#include <string>
#include <vector>

using namespace std;

class ExtractorDB;

/*
	In its simplest form, an extractor just accepts packets.
	This defines such an interface
*/
class ExtractBase {
public:
	//ExtractBase();
	virtual ~ExtractBase() {}
	
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) = 0;
};

class ExtractCollector : public ExtractBase {
public:
	/*
		Only public cause VC++ wont let you inherit from a protected
		class in your parent class
	*/
	class ExtractItem {
	public:
		ExtractItem() { valid = true; }
		virtual ~ExtractItem() {}
		//returns the number of bytes of 'data' which are consumed
		//by this item in data, 0 == error
		virtual uint32 FromPacket(unsigned char *packet, uint32 len) = 0;
		
		map<uint16, string> data;
		bool valid;
	};
	
	
	
	
	ExtractCollector(EmuOpcode interested_op, string table_name);
	virtual ~ExtractCollector() {}
	
	//default implementation assumes it is a single item to extract
	virtual void GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len);
	
	//inline EmuOpcode GetInterestedOp() const { return(my_op); }
	
	//returns a new ExtractItem subclass object for this implementation
	virtual ExtractItem *NewItem() = 0;
	
	virtual void GenerateInserts(FILE *into, bool make_replaces);
	virtual void GenerateAnInsert(FILE *into, bool make_replaces, ExtractItem *item);
	
	virtual void GenerateUpdates(FILE *into, ExtractorDB *db);
	virtual void GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item);
	
	virtual void GenerateTexts(FILE *into, ExtractorDB *db);
	virtual void GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item);
	
	//helper function to split up packets which contain multiple items
	//in general should not be used except by other ExtractCollectors
	void SplitPacket(uint16 count, unsigned char *data, uint32 len);
protected:
	//these types moved here to make the code easier to read
	typedef enum { OnMissingError, OnMissingOmit, OnMissingUseDefault
		} OMA;
	//used for comparison purposes only:
	typedef enum { 
			vString, vStringCaseSensitive, 
			vFloat, vFloatExact, 
			vInt, vIntUnsigned
		} VT;
		
	class FieldInfo {
	public:
		FieldInfo() {
			primary = false;
		}
		FieldInfo(string nam, string def, bool prim, OMA oma, VT vt = vString) {
			name = nam;
			default_value = def;
			primary = prim;
			on_missing_action = oma;
			value_type = vt;
		}
		string name;
		string default_value;
		bool primary;	//is part of the primary key?
		OMA on_missing_action;
		VT value_type;
	};
	
	virtual void GenerateClauses(string &field_names, string &where_clause, ExtractItem *item);
	
	virtual bool compare(const char *l, const char *r, VT vt);
	float float_epsilon;
	
	const EmuOpcode my_op;
	
	vector<ExtractItem *> collected;
	map<uint16, FieldInfo> fields;
	const string table_name;
	
	//escape a string for mysql
	void EscapeString(string &to, const char *from, int len);
	int EscapeBlock(char *buf, const char *from, int len);
	//number to string converters
	static string ultoa(uint32 n);
	static string itoa(sint32 n);
	static string ftoa(float n);
};


#endif


