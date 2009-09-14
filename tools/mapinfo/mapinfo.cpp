// mapinfo.cpp : Defines the entry point for the console application.
//

#include "../Tank/SaveFile.h"


#include <iostream>
#include <algorithm>

using namespace std;

void replace(string &str, const char *what, const char *by)
{
	string::size_type pos = 0;
	size_t lw = strlen(what);
	size_t lb = strlen(by);

	while( pos < str.length() )
	{
		pos = str.find(what, pos);
		if( string::npos == pos )
			return;
		str.replace(pos, lw, by);
		pos += lb;
	}
}

void escape(string &str)
{
	replace(str, "\r", "");
	replace(str, "\\", "\\\\");
	replace(str, "\n", "\\n");
	replace(str, "\t", "\\t");
	replace(str, "\"", "\\\"");
}

int main(int argc, char* argv[])
{
	if( argc < 2 )
	{
		cout << "usage: mapinfo <map_file>" << endl;
		return 0;
	}

	MapFile file;
	if( !file.Open(argv[1], false) )
	{
		cerr << "couldn't open map file" << endl;
		return -1;
	}

	string val;

	if( file.getMapAttribute("author", val) )
	{
		escape(val);
		cout << "author=\"" << val << "\" ";
	}

	if( file.getMapAttribute("e-mail", val) )
	{
		escape(val);
		cout << "email=\"" << val << "\" ";
	}

	if( file.getMapAttribute("link-url", val) )
	{
		escape(val);
		cout << "url=\"" << val << "\" ";
	}

	if( file.getMapAttribute("desc", val) )
	{
		escape(val);
		cout << "desc=\"" << val << "\" ";
	}

	if( file.getMapAttribute("theme", val) )
	{
		escape(val);
		cout << "theme=\"" << val << "\" ";
	}

	int x, y;
	if( file.getMapAttribute("width", x) && file.getMapAttribute("height", y) )
	{
		stringstream ss;
		ss << x << "x" << y;
		cout << "mapsize=\"" << ss.str() << "\" ";
	}


	return 0;
}

