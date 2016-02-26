#include "vsinkprintf.h"
#include "console.h"
#include "string.h"
#include "stdio.h"

int putchar(int c)
{
	const char ch = c;

	console_write(&ch, 1);
	return 0;
}

int puts(const char *str)
{
	console_write(str, strlen(str));
	putchar('\n');
	return 0;
}

struct vsinkprintf_console_sink {
	struct vsinkprintf_sink sink;
	int count;
};

static void vsinkprintf_console_write(struct vsinkprintf_sink *sink,
			const char *data, size_t size)
{
	struct vsinkprintf_console_sink *con =
		(struct vsinkprintf_console_sink *)sink;

	console_write(data, size);
	con->count += size;
}

int vprintf(const char *fmt, va_list args)
{
	struct vsinkprintf_console_sink sink = {
		{ &vsinkprintf_console_write },
		0
	};

	vsinkprintf(&sink.sink, fmt, args);
	return sink.count;
}

int printf(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	const int ret = vprintf(fmt, args);

	va_end(args);

	return ret;
}

struct vsinkprintf_buffer_sink {
	struct vsinkprintf_sink sink;
	char *data;
	int size;
	int pos;
};

static void vsinkprintf_buffer_write(struct vsinkprintf_sink *sink,
			const char *data, size_t size)
{
	struct vsinkprintf_buffer_sink *buf =
		(struct vsinkprintf_buffer_sink *)sink;

	const int remain = MAX(buf->size - 1 - buf->pos, 0);

	if (remain)
		memcpy(buf->data + buf->pos, data, MINU(remain, size));
	buf->pos += size;
}

int vsnprintf(char *buf, size_t size, const char *fmt, va_list args)
{
	struct vsinkprintf_buffer_sink sink = {
		{ &vsinkprintf_buffer_write },
		buf,
		size,
		0
	};
	vsinkprintf(&sink.sink, fmt, args);
	sink.data[MIN(sink.size - 1, sink.pos)] = 0;
	return sink.pos;
}

int snprintf(char *buf, size_t size, const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);

	const int ret = vsnprintf(buf, size, fmt, args);

	va_end(args);

	return ret;
}
