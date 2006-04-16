#ifndef TEXT_MAP_FILE_H
#define TEXT_MAP_FILE_H

#include "../common/types.h"

#include <vector>

using namespace std;

class TextMapPoint {
public:
	float x;
	float y;
	float z;
};

class TextMapColor {
public:
	int r;
	int g;
	int b;
};

class TextMapLine {
public:
	TextMapPoint start;
	TextMapPoint end;
	TextMapColor color;
};

class TextMapReader {
public:
	
	bool ReadFile(const char *directory, const char *zone);
	
	vector<TextMapLine>::const_iterator begin_lines() const { return(_lines.begin()); }
	vector<TextMapLine>::const_iterator end_lines() const { return(_lines.end()); }
	
protected:
	vector<TextMapLine> _lines;
};



#endif


