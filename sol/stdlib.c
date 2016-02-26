#include "stdlib.h"
#include "string.h"
#include "ctype.h"

long strtol(const char *str, char **endptr, int base)
{
	int sign = 0;

	while (isspace(*str))
		++str;

	if (*str == '-' || *str == '+')
		sign = (*str++ == '-');

	const long value = (long)strtoul(str, endptr, base);

	return sign ? -1l * value : value;
}

unsigned long strtoul(const char *str, char **endptr, int base)
{
	static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

	unsigned long answer = 0;
	const char *pos;

	if (base == 0 && *str == '0') {
		if (*(str + 1) && (*(str + 1) == 'x' || *(str + 1) == 'X')) {
			base = 16;
			str += 2;
		} else {
			base = 8;
			str += 1;
		}
	}

	if (base == 0)
		base = 10;

	while ((pos = strchr(digits, tolower(*str)))) {
		answer = answer * base + (pos - digits);
		++str;
	}

	if (endptr)
		*endptr = (char *)str;

	return answer;
}
