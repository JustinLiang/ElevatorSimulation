#ifndef __STRINGCAT__
#define __STRINGCAT__

#include <iostream>
#include <string>
#include <sstream>

inline std::string itos(int i) // convert int to string
{
	std::stringstream s;
	s << i;
	return s.str();
}

#endif