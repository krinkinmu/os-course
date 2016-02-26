#include "vsinkprintf.h"
#include "kernel.h"
#include "string.h"
#include "stdlib.h"
#include "ctype.h"

#include <stdint.h>

enum format_flag {
	FF_SIGNED = (1 << 0),
	FF_ZEROPAD = (1 << 1),
	FF_SIGN = (1 << 2),
	FF_PREFIX = (1 << 3)
};

enum format_type {
	FT_NONE,
	FT_CHAR,
	FT_STR,
	FT_PERCENT,
	FT_PTR,
	FT_INTMAX,
	FT_SIZE,
	FT_PTRDIFF,
	FT_LLONG,
	FT_LONG,
	FT_INT,
	FT_SHORT,
	FT_BYTE,
	FT_INVALID
};

struct format_spec {
	unsigned long type;
	unsigned long flags;
	unsigned base;
	int qualifier;
	int width;
};

static void vsinkprintf_write(struct vsinkprintf_sink *sink,
			const char *data, size_t size)
{ sink->write(sink, data, size); }

static void vsinkprintf_putchar(struct vsinkprintf_sink *sink, int c)
{
	const char buf = c;

	sink->write(sink, &buf, 1);
}

static void vsinkprintf_repeat(struct vsinkprintf_sink *sink, int c,
			size_t count)
{
	for (size_t i = 0; i != count; ++i)
		vsinkprintf_putchar(sink, c);
}

static void vsinkprintf_puts_nonewline(struct vsinkprintf_sink *sink,
			const char *str)
{
	const size_t len = strlen(str);

	sink->write(sink, str, len);
}

static int format_decode(const char *fmt, struct format_spec *spec)
{
	static const char *length_mod = "hljzt";

	const char *start = fmt;

	spec->type = FT_NONE;
	spec->flags = 0;
	spec->width = 0;
	spec->qualifier = 0;
	spec->base = 10;

	while (*fmt && *fmt != '%')
		++fmt;

	if (fmt != start || !*fmt)
		return fmt - start;

	while (1) {
		int found = 1;
		++fmt;

		switch (*fmt) {
		case '+':
			spec->flags |= FF_SIGN;
			break;
		case '#':
			spec->flags |= FF_PREFIX;
			break;
		default:
			found = 0;
		}

		if (!found)
			break;
	};

	if (isdigit(*fmt))
		spec->width = strtol(fmt, (char **)&fmt, 10);

	if (strchr(length_mod, *fmt)) {
		spec->qualifier = *fmt++;

		if (*fmt == spec->qualifier) {
			spec->qualifier = toupper(spec->qualifier);
			++fmt;
		}
	}

	switch (*fmt) {
	case 'c':
		spec->type = FT_CHAR;
		return ++fmt - start;
	case 's':
		spec->type = FT_STR;
		return ++fmt - start;
	case '%':
		spec->type = FT_PERCENT;
		return ++fmt - start;
	case 'p':
		spec->type = FT_PTR;
		spec->base = 16;
		spec->flags = FF_PREFIX;
		return ++fmt - start;
	case 'd':
	case 'i':
		spec->flags |= FF_SIGNED;
	case 'o':
	case 'x':
	case 'X':
		if (*fmt == 'o')
			spec->base = 8;
		if (*fmt == 'X' || *fmt == 'x')
			spec->base = 16;
	case 'u':
		break;
	default:
		spec->type = FT_INVALID;
	}

	switch (spec->qualifier) {
	case 'H':
		spec->type = FT_BYTE;
		break;
	case 'h':
		spec->type = FT_SHORT;
		break;
	case 'l':
		spec->type = FT_LONG;
		break;
	case 'L':
		spec->type = FT_LLONG;
		break;
	case 'j':
		spec->type = FT_INTMAX;
		break;
	case 'z':
		spec->type = FT_SIZE;
		break;
	case 't':
		spec->type = FT_PTRDIFF;
		break;
	default:
		spec->type = FT_INT;
		break;
	}

	return ++fmt - start;
}

static int untoa(uintmax_t value, char *str, int base)
{
	static const char digits[] = "0123456789abcdefghijklmnopqrstuvwxyz";

	int pos = 0;

	do {
		str[pos++] = digits[value % base];
		value /= base;
	} while (value);

	for (int i = 0; i != pos / 2; ++i) {
		const char tmp = str[i];

		str[i] = str[pos - i - 1];
		str[pos - i - 1] = tmp;
	}
	str[pos] = 0;
	return pos;
}

static void format_number(struct vsinkprintf_sink *sink, uintmax_t value,
			const struct format_spec *spec)
{
	const intmax_t svalue = value;

	char buffer[256];
	const char *sign = "";
	const char *prefix = "";
	const char *num;
	int len, padding;

	if ((spec->flags & FF_SIGNED) && svalue < 0) {
		len = untoa(-svalue, buffer, spec->base) + 1;
		sign = "-";
	} else {
		len = untoa(value, buffer, spec->base);
		if (spec->flags & FF_SIGN) {
			sign = "+";
			++len;
		}
	}
	num = buffer;

	if (spec->flags & FF_PREFIX) {
		if (spec->base == 8) {
			prefix = "0";
			++len;
		} else if (spec->base == 16) {
			prefix = "0x";
			len += 2;
		}
	}

	padding = 0;
	if (len < spec->width)
		padding = spec->width - len;

	vsinkprintf_repeat(sink, ' ', padding);
	vsinkprintf_puts_nonewline(sink, sign);
	vsinkprintf_puts_nonewline(sink, prefix);
	vsinkprintf_puts_nonewline(sink, num);
}

void vsinkprintf(struct vsinkprintf_sink *sink, const char *fmt, va_list args)
{
	struct format_spec spec;

	while (*fmt) {
		const char *save = fmt;
		const int read = format_decode(fmt, &spec);

		fmt += read;

		switch (spec.type) {
		case FT_NONE: {
			vsinkprintf_write(sink, save, read);
			break;
		}

		case FT_CHAR: {
			char c = va_arg(args, int);

			vsinkprintf_repeat(sink, ' ', MAX(spec.width, 0));
			vsinkprintf_putchar(sink, c);
			break;
		}

		case FT_STR: {
			const char *toprint = va_arg(args, const char *);
			const int len = strlen(toprint);

			vsinkprintf_repeat(sink, ' ', MAX(spec.width - len, 0));
			vsinkprintf_puts_nonewline(sink, toprint);
			break;
		}

		case FT_PERCENT:
			vsinkprintf_putchar(sink, '%');
			break;

		case FT_INVALID:
			vsinkprintf_putchar(sink, '?');
			break;

		default: {
			uintmax_t value;

			switch (spec.type) {
			case FT_PTR:
				value = (uintptr_t)va_arg(args, void *);
				break;
			case FT_LLONG:
				value = va_arg(args, long long);
				break;
			case FT_LONG:
				value = va_arg(args, long);
				break;
			case FT_INTMAX:
				value = va_arg(args, intmax_t);
				break;
			case FT_SIZE:
				value = va_arg(args, size_t);
				break;
			case FT_PTRDIFF:
				value = va_arg(args, ptrdiff_t);
				break;
			default:
				value = va_arg(args, int);
				break;
			}

			format_number(sink, value, &spec);
			break;
		}

		}
	}
}
