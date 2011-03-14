#ifndef BASE_UTF8CONVERT_H
#define BASE_UTF8CONVERT_H

#include <string>
	
int is_utf8(const char * string);
std::string UTF8toLatin1( const char* szStr );
std::string Latin1toUTF8( const char* szStr );

#endif // BASE_UTF8CONVERT_H