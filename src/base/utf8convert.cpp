#include "utf8convert.h"
#include "system.h"

int is_utf8(const char * string)
{
	if(!string)
		return 0;
	
	const unsigned char * bytes = (const unsigned char *)string;
	while(*bytes)
	{
		if(     (// ASCII
				 bytes[0] == 0x09 ||
				 bytes[0] == 0x0A ||
				 bytes[0] == 0x0D ||
				 (0x20 <= bytes[0] && bytes[0] <= 0x7E)
				 )
		   ) 
		{
			bytes += 1;
			continue;
		}
		
		if(     (// non-overlong 2-byte
				 (0xC2 <= bytes[0] && bytes[0] <= 0xDF) &&
				 (0x80 <= bytes[1] && bytes[1] <= 0xBF)
				 )
		   ) 
		{
			bytes += 2;
			continue;
		}
		
		if(     (// excluding overlongs
				 bytes[0] == 0xE0 &&
				 (0xA0 <= bytes[1] && bytes[1] <= 0xBF) &&
				 (0x80 <= bytes[2] && bytes[2] <= 0xBF)
				 ) ||
		   (// straight 3-byte
			((0xE1 <= bytes[0] && bytes[0] <= 0xEC) ||
			 bytes[0] == 0xEE ||
			 bytes[0] == 0xEF) &&
			(0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
			(0x80 <= bytes[2] && bytes[2] <= 0xBF)
			) ||
		   (// excluding surrogates
			bytes[0] == 0xED &&
			(0x80 <= bytes[1] && bytes[1] <= 0x9F) &&
			(0x80 <= bytes[2] && bytes[2] <= 0xBF)
			)
		   ) 
		{
			bytes += 3;
			continue;
		}
		
		if(     (// planes 1-3
				 bytes[0] == 0xF0 &&
				 (0x90 <= bytes[1] && bytes[1] <= 0xBF) &&
				 (0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
				 (0x80 <= bytes[3] && bytes[3] <= 0xBF)
				 ) ||
		   (// planes 4-15
			(0xF1 <= bytes[0] && bytes[0] <= 0xF3) &&
			(0x80 <= bytes[1] && bytes[1] <= 0xBF) &&
			(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
			(0x80 <= bytes[3] && bytes[3] <= 0xBF)
			) ||
		   (// plane 16
			bytes[0] == 0xF4 &&
			(0x80 <= bytes[1] && bytes[1] <= 0x8F) &&
			(0x80 <= bytes[2] && bytes[2] <= 0xBF) &&
			(0x80 <= bytes[3] && bytes[3] <= 0xBF)
			)
		   ) 
		{
			bytes += 4;
			continue;
		}
		
		return 0;
	}
	
	return 1;
}

std::string UTF8toLatin1( const char* szStr )
{
	
	const unsigned char* pSource = (const unsigned char*)szStr;
	std::string strResult;
	int nLen = strlen( szStr );
	strResult.reserve( nLen + nLen/10 );
	
	unsigned long ucs4;
	int c, state, octets;
	
	ucs4 = 0;
	state = 0;
	octets = 0;
	
	c = *(pSource);
	
	while (c) 
	{
		switch (state) 
		{
			case 0:	/* start of utf8 char */
				ucs4 = 0;	/* reset ucs4 char */
				if ((c & 0xfe) == 0xfc) 
				{		/* 6 octets */
					ucs4 = (c & 0x01) << 30;
					octets = 6;
					state = 5;	/* look for 5 more */
				} 
				else if ((c & 0xfc) == 0xf8) 
				{	/* 5 octets */
					ucs4 = (c & 0x03) << 24;
					octets = 5;
					state = 4;
				} 
				else if ((c & 0xf8) == 0xf0) 
				{	/* 4 octets */
					ucs4 = (c & 0x07) << 18;
					octets = 4;
					state = 3;
				} 
				else if ((c & 0xf0) == 0xe0) 
				{	/* 3 octets */
					ucs4 = (c & 0x0f) << 12;
					octets = 3;
					state = 2;
				} 
				else if ((c & 0xe0) == 0xc0) 
				{	/* 2 octets */
					ucs4 = (c & 0x1f) << 6;
					octets = 2;
					state = 1;	/* look for 1 more */
				} 
				else if ((c & 0x80) == 0x00) 
				{	/* 1 octet */
					ucs4 = (c & 0x7f);
					octets = 1;
					state = 0;	/* we have a result */
				} 
				else {				/* error */
					;
				}
				break;
			case 1:
				if ((c & 0xc0) == 0x80) 
				{
					ucs4 = ucs4 | (c & 0x3f);
					if (ucs4 < 0x80 || ucs4 > 0x7ff) 
					{
						ucs4 = 0xffffffff;
					}
				} 
				else 
				{
					ucs4 = 0xffffffff;
				}
				state = 0;	/* we're done and have a result */
				break;
			case 2:
				if ((c & 0xc0) == 0x80) 
				{
					ucs4 = ucs4 | ((c & 0x3f) << 6);
					state = 1;
				} 
				else 
				{
					ucs4 = 0xffffffff;
					state = 0;
				}
				break;
			case 3:
				if ((c & 0xc0) == 0x80) 
				{
					ucs4 = ucs4 | ((c & 0x3f) << 12);
					state = 2;
				} 				
				else 
				{
					ucs4 = 0xffffffff;
					state = 0;
				}
				break;
			case 4:
				if ((c & 0xc0) == 0x80) 
				{
					ucs4 = ucs4 | ((c & 0x3f) << 18);
					state = 3;
				} 
				else 
				{
					ucs4 = 0xffffffff;
					state = 0;
				}
				break;
			case 5:
				if ((c & 0xc0) == 0x80) 
				{
					ucs4 = ucs4 | ((c & 0x3f) << 24);
					state = 4;
				} 
				else 
				{
					ucs4 = 0xffffffff;
					state = 0;
				}
				break;
			default:	/* error, can't happen */
				ucs4 = 0xffffffff;
				state = 0;
				break;
		}
		if (state == 0) 
		{
			switch (octets) 
			{
				case 1:
					if (ucs4 < 0x0 || ucs4 > 0x7f)
						ucs4 = 0xffffffff;
					break;
				case 2:
					if (ucs4 < 0x80 || ucs4 > 0x7ff)
						ucs4 = 0xffffffff;
					break;
				case 3:
					if (ucs4 < 0x800 || ucs4 > 0xffff)
						ucs4 = 0xffffffff;
					break;
				case 4:
					if (ucs4 < 0x10000 || ucs4 > 0x1fffff)
						ucs4 = 0xffffffff;
					break;
				case 5:
					if (ucs4 < 0x200000 || ucs4 > 0x3ffffff)
						ucs4 = 0xffffffff;
					break;
				case 6:
					if (ucs4 < 0x4000000 || ucs4 > 0x7fffffff)
						ucs4 = 0xffffffff;
					break;
				default:
					ucs4 = 0xffffffff;
					break;
			}
			if (ucs4 != 0xffffffff) 
			{
				strResult += (char)ucs4;
			}
		}
		c = *(++pSource);
	}
	return strResult;
}
	
std::string Latin1toUTF8( const char* szStr )
{
	const unsigned char* pSource = (const unsigned char*)szStr;
	std::string strResult;
	int nLen = strlen( szStr );
	strResult.reserve( nLen + nLen/10 );
	int cSource = *(pSource);
	while ( cSource )
	{
		if ( cSource >= 128 )
		{
			strResult += (char)(((cSource&0x7c0)>>6) | 0xc0);
			strResult += (char)((cSource&0x3f) | 0x80);
		}
		else
			strResult += cSource;
		cSource = *(++pSource);
	}
	return strResult;
}
	
