
#include "ExtractCollector.h"
#include "ExtractDB.h"
#include "../common/MiscFunctions.h"

#define FLOAT_CLOSE_ENOUGH 0.01	//arbitrary

ExtractCollector::ExtractCollector(EmuOpcode interested_op, string itable_name)
: my_op(interested_op), table_name(itable_name)
{
	float_epsilon = FLOAT_CLOSE_ENOUGH;
}
	
//called with a packet of type 'my_op'
//default implementation assumes it is a single item to extract
void ExtractCollector::GivePacket(EmuOpcode emu_op, unsigned char *data, uint32 len) {
	if(emu_op != my_op)
		return;	//not interested
	ExtractItem *item = NewItem();
	if(item->FromPacket(data, len) == 0 || !item->valid) {
		safe_delete(item);
		return;
	}
	collected.push_back(item);
}

void ExtractCollector::GenerateInserts(FILE *into, bool make_replaces) {
	vector<ExtractItem *>::iterator cur,end;
	
	cur = collected.begin();
	end = collected.end();
	for(; cur != end; cur++) {
		GenerateAnInsert(into, make_replaces, *cur);
	}
}

void ExtractCollector::GenerateAnInsert(FILE *into, bool make_replaces, ExtractItem *item) {
	map<uint16, FieldInfo>::iterator cur, end;
	map<uint16, string>::iterator valres;
	
	string field_names,values;
	bool first = true;
	
	cur = fields.begin();
	end = fields.end();
	for(; cur != end; cur++) {
		string value;
		
		//figure out what the value of this field is in this item
		valres = item->data.find(cur->first);
		if(valres == item->data.end()) {
			//the value is not present in this item... what to do
			if(cur->second.on_missing_action == OnMissingError) {
				printf("Error Generating Insert: field %s is missing", cur->second.name.c_str());
				return;
			} else if(cur->second.on_missing_action == OnMissingOmit) {
				continue;
			} else {	//assume OnMissingUseDefault
				value = cur->second.default_value;
			}
		} else {
			value = valres->second;
		}
		
		//add the item to the query
		if(first)
			first = false;
		else {
			field_names += ",";
			values += ", ";
		}
		field_names += cur->second.name;
		EscapeString(value, value.c_str(), value.length());
		values += "'" + value + "'";
		
	}
	
	//print this thing out.
	if(make_replaces)
		fprintf(into, "REPLACE");
	else
		fprintf(into, "INSERT");
		
	fprintf(into, " INTO %s (%s) VALUES(%s);\n", table_name.c_str(), field_names.c_str(), values.c_str());
}

void ExtractCollector::GenerateUpdates(FILE *into, ExtractorDB *db) {
	vector<ExtractItem *>::iterator cur,end;
	
	cur = collected.begin();
	end = collected.end();
	for(; cur != end; cur++) {
		GenerateAnUpdate(into, db, *cur);
	}
}

void ExtractCollector::GenerateClauses(string &field_names, string &where_clause, ExtractItem *item) {
	map<uint16, FieldInfo>::iterator cur, end;
	
	map<uint16, string>::iterator valres;
	
	bool first = true;
	bool first_where = true;
	
	//build a comma seperated list of field names we care about
	//also build a 'where' clause for the primary key of this item
	cur = fields.begin();
	end = fields.end();
	for(; cur != end; cur++) {
		//we only need its value if its a primay field
		if(cur->second.primary) {
			string value;
			
			//figure out what the value of this field is in this item
			valres = item->data.find(cur->first);
			if(valres == item->data.end()) {
				//the value is not present in this item... what to do
				if(cur->second.on_missing_action == OnMissingError) {
					printf("Error Generating Insert: field %s is missing\n", cur->second.name.c_str());
					return;
				} else if(cur->second.on_missing_action == OnMissingOmit) {
					continue;
				} else {	//assume OnMissingUseDefault
					value = cur->second.default_value;
				}
			} else {
				value = valres->second;
			}
			if(first_where)
				first_where = false;
			else {
				where_clause += " AND ";
			}
			
			if(cur->second.value_type == vString) {
				//non-case sensitive for regular strings
				where_clause += "lower("+cur->second.name+")";
				EscapeString(value, value.c_str(), value.length());
				where_clause += "=lower('" + value + "')";
			} else if(cur->second.value_type == vFloat) {
				//fuzzy match for non-exact floats
				EscapeString(value, value.c_str(), value.length());
				where_clause += "ABS("+cur->second.name+"-"+value+") < "+ftoa(FLOAT_CLOSE_ENOUGH);
			} else {
				where_clause += cur->second.name;
				EscapeString(value, value.c_str(), value.length());
				where_clause += "='" + value + "'";
			}
		}
		
		if(first)
			first = false;
		else {
			field_names += ",";
		}
		field_names += cur->second.name;
	}
}

void ExtractCollector::GenerateAnUpdate(FILE *into, ExtractorDB *db, ExtractItem *item) {
	map<uint16, FieldInfo>::iterator cur, end;
	
	map<uint16, string>::iterator valres;
	
	string field_names;
	string where_clause;
	string update_clause;
	bool first_update = true;
	
	
	GenerateClauses(field_names, where_clause, item);
	
	//now we have the field list and a where clause for this
	//primary key entry, query the item.
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	bool valid = false;
	MYSQL_RES *result;
	MYSQL_ROW row = NULL;
	if (db->RunQuery(query, MakeAnyLenString(&query, 
		"SELECT %s FROM %s WHERE %s", field_names.c_str(), table_name.c_str(), where_clause.c_str()
		), errbuf, &result))
	{
		if ((row = mysql_fetch_row(result))) {
			//we found it
			valid = true;
		} else {
			mysql_free_result(result);
		}
	}
	safe_delete_array(query);
	if(!valid) {
		//not found, generate an INSERT and were done.
		GenerateAnInsert(into, false, item);
		return;
	}
	
	//we now have a valid result for our row in the DB... compare
	cur = fields.begin();
	end = fields.end();
	for(; cur != end; cur++) {
		if(cur->second.primary)
			continue;	//skip the primary key, we know it matches
		
		string value;
		
		//figure out what the value of this field is in this item
		valres = item->data.find(cur->first);
		if(valres == item->data.end()) {
			//the value is not present in this item... what to do
			if(cur->second.on_missing_action == OnMissingError) {
				printf("Error Generating Update: field '%s' is missing\n", cur->second.name.c_str());
				return;
			} else if(cur->second.on_missing_action == OnMissingOmit) {
				continue;
			} else {	//assume OnMissingUseDefault
				value = cur->second.default_value;
			}
		} else {
			value = valres->second;
		}
		
		//compare our new value to this value in the DB
		if(compare(value.c_str(), row[cur->first], cur->second.value_type))
			continue;		//they are the same
		
//printf("Comparison(%s): '%s' is different than DB value '%s'\n", cur->second.name.c_str(), value.c_str(), row[cur->first]);
		//the values differ, include it in the update
		if(first_update)
			first_update = false;
		else {
			update_clause += ", ";
		}
		update_clause += cur->second.name;
		EscapeString(value, value.c_str(), value.length());
		update_clause += "='"+value+"'";
	}
	mysql_free_result(result);
	
	
	//now generate the final update statement
	if(!first_update)
		fprintf(into, "UPDATE %s SET %s WHERE %s;\n", table_name.c_str(), update_clause.c_str(), where_clause.c_str());
}

void ExtractCollector::GenerateTexts(FILE *into, ExtractorDB *db) {
	vector<ExtractItem *>::iterator cur,end;
	
	cur = collected.begin();
	end = collected.end();
	for(; cur != end; cur++) {
		GenerateAText(into, db, *cur);
	}
}

void ExtractCollector::GenerateAText(FILE *into, ExtractorDB *db, ExtractItem *item) {
	map<uint16, FieldInfo>::iterator cur, end;
	
	map<uint16, string>::iterator valres;
	
	string field_names;
	string where_clause;
	string update_clause;
	bool first_update = true;
	
	
	GenerateClauses(field_names, where_clause, item);
	
	//now we have the field list and a where clause for this
	//primary key entry, query the item.
	char errbuf[MYSQL_ERRMSG_SIZE];
	char *query = 0;
	bool valid = false;
	MYSQL_RES *result;
	MYSQL_ROW row = NULL;
	if (db->RunQuery(query, MakeAnyLenString(&query, 
		"SELECT %s FROM %s WHERE %s", field_names.c_str(), table_name.c_str(), where_clause.c_str()
		), errbuf, &result))
	{
		if ((row = mysql_fetch_row(result))) {
			//we found it
			valid = true;
		} else {
			mysql_free_result(result);
		}
	}
	safe_delete_array(query);
	
	//we now have a valid result for our row in the DB... compare
	cur = fields.begin();
	end = fields.end();
	for(; cur != end; cur++) {
		if(cur->second.primary)
			continue;	//skip the primary key, we know it matches
		
		string value;
		
		//figure out what the value of this field is in this item
		valres = item->data.find(cur->first);
		if(valres == item->data.end()) {
			//the value is not present in this item... what to do
			if(cur->second.on_missing_action == OnMissingError) {
				printf("Error Generating Text: field %s is missing\n", cur->second.name.c_str());
				return;
			} else if(cur->second.on_missing_action == OnMissingOmit) {
				continue;
			} else {	//assume OnMissingUseDefault
				value = cur->second.default_value;
			}
		} else {
			value = valres->second;
		}
		
		//compare our new value to this value in the DB
		if(valid && compare(value.c_str(), row[cur->first], cur->second.value_type))
			continue;		//they are the same
		
		//the values differ
		if(first_update) {
			first_update = false;
			fprintf(into, "\nTable %s: key (%s)\n", table_name.c_str(), where_clause.c_str());
		}
		fprintf(into, "\t%s: DB: '%s', Live: '%s'\n", cur->second.name.c_str(), 
			valid?row[cur->first]:"?", value.c_str());
	}
	if(valid)
		mysql_free_result(result);
}


bool ExtractCollector::compare(const char *l, const char *r, VT vt) {
	//compares things in their native type... with hopes of doing something good I guess
	switch(vt) {
	case vStringCaseSensitive:
		return(!strcmp(l, r));
	case vFloat: {
		//default float match is fuzzy
		float lf, rf;
		lf = atof(l);
		rf = atof(r);
		lf -= rf;
		if(lf < 0)
			lf = 0 - rf;
		return(lf < FLOAT_CLOSE_ENOUGH);
	}
	case vFloatExact:
		return(atof(l) == atof(r));
	case vInt:
		return(atoi(l) == atoi(r));
	case vIntUnsigned:
		return(strtoul(l, NULL, 10) == strtoul(r, NULL, 10));
	case vString:
	default:
		break;
	}
	return(!strcasecmp(l, r));
}

/*
	Mysql escape routines so we dont need a stupid connection..
	Mysql pisses me off for requiring that crap... hello.. offline processing...
*/
void ExtractCollector::EscapeString(string &to, const char *from, int len) {
	//incurrs an extra copy of the resulting string, but im lazy
	char *buf = new char[len+len+2];
	
	int end = EscapeBlock(buf, from, len);
	buf[end] = '\0';
	
	to = buf;
	delete[] buf;
}

//assumes buf is big enough
int ExtractCollector::EscapeBlock(char *buf, const char *from, int len) {
	char *to = buf;
	const char *src = from;
	while(len > 0) {
		len--;
		
		//straight from mysql, without needing a connection... fuckers
	    switch (*src) {
	    case 0:                             /* Must be escaped for 'mysql' */
	      *to++= '\\';
	      *to++= '0';
	      break;
	    case '\n':                          /* Must be escaped for logs */
	      *to++= '\\';
	      *to++= 'n';
	      break;
	    case '\r':
	      *to++= '\\';
	      *to++= 'r';
	      break;
	    case '\\':
	      *to++= '\\';
	      *to++= '\\';
	      break;
	    case '\'':
	      *to++= '\\';
	      *to++= '\'';
	      break;
	    case '"':                           /* Better safe than sorry */
	      *to++= '\\';
	      *to++= '"';
	      break;
	    case '\032':                        /* This gives problems on Win32 */
	      *to++= '\\';
	      *to++= 'Z';
	      break;
	    default:
	      *to++= *src;
		}
		src++;
	}
	return(int(to) - int(buf));
}


//helper function to split up packets which contain multiple items
void ExtractCollector::SplitPacket(uint16 count, unsigned char *data, uint32 len) {
	ExtractItem *item;
	while(count > 0) {
		count--;
		item = NewItem();
		uint32 used = item->FromPacket(data, len);
		if(!item->valid) {
			//item marked as invalid, sliently discard it
			safe_delete(item);
		} else if(used == 0) {
			printf("Packet parser reported error. Aborting packet splitting.\n");
			safe_delete(item);
			return;
		} else if(used > len) {
			printf("Error splitting packet. It consumed %lu bytes, but we only have %lu\n", used, len);
			safe_delete(item);
			return;
		} else {
			collected.push_back(item);
		}
		data += used;
		len -= used;
	}
}


string ExtractCollector::ultoa(uint32 n) {
	static char nbuf[16];
	sprintf(nbuf, "%lu", n);
	return(string(nbuf));
}

string ExtractCollector::itoa(sint32 n) {
	static char nbuf[16];
	sprintf(nbuf, "%ld", n);
	return(string(nbuf));
}

string ExtractCollector::ftoa(float n) {
	static char nbuf[16];
	sprintf(nbuf, "%f", n);
	return(string(nbuf));
}









