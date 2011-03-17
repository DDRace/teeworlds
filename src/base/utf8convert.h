#ifndef BASE_UTF8CONVERT_H
#define BASE_UTF8CONVERT_H

#ifdef __cplusplus
extern "C" {
#endif
	
int is_utf8(const char *string);
void UTF8toLatin1(const char *utf8, char *latin);
void Latin1toUTF8(const char *latin, char *utf8);

#ifdef __cplusplus
}
#endif

#endif // BASE_UTF8CONVERT_H