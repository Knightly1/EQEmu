#ifndef _MISC_H

#define _MISC_H
#include <string>
#include <map>

using namespace std;

#define ITEMFIELDCOUNT 116

void Unprotect(string &s, char what);

void Protect(string &s, char what);

bool ItemParse(const char *data, int length, map<int,map<int,string> > &items, int id_pos, int name_pos, int max_field, int level=0);

int Tokenize(string s, map<int,string> & tokens, char delim='|');

void LoadItemDBFieldNames();

void encode_length(unsigned long length, char *out);
unsigned long decode_length(char *in);
unsigned long  encode(char *in, unsigned long length, char *out);
void decode(char *in, char *out);
void encode_chunk(char *in, int len, char *out);
void decode_chunk(char *in, char *out);

int Deflate(unsigned char* in_data, int in_length, unsigned char* out_data, int max_out_length);
int Inflate(unsigned char* indata, int indatalen, unsigned char* outdata, int outdatalen, bool iQuiet=true);
#ifndef WIN32
int print_stacktrace();
#endif

#endif
