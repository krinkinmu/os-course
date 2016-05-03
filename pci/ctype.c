#include "ctype.h"

int toupper(int c)
{
	if (c < 'a' || c > 'z')
		return c;
	return c + ('A' - 'a');
}

int tolower(int c)
{
	if (c < 'A' || c > 'Z')
		return c;
	return c + ('a' - 'A');
}

int isdigit(int c)
{ return c >= '0' && c <= '9'; }

int isspace(int c)
{
	switch (c) {
	case ' ':
	case '\t':
	case '\n':
	case '\v':
	case '\f':
	case '\r':
		return 1;
	}
	return 0;
}
