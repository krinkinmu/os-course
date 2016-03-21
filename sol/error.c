#include "kernel.h"
#include "error.h"

static const char *err_str[] = {
	"No error",
	"Out of memory",
	"No such file or directory",
	"Operation not supported",
	"Resource is busy",
	"File or directory already exists",
	"Invalid value",
	"IO error",
	"Exec format error",
	"No such system call",
	"Unknown error"
};

const char *errstr(int errc)
{
	static const int max_errc = sizeof(err_str)/sizeof(err_str[0]) - 1;

	if (errc < 0)
		errc = -errc;

	return err_str[MIN(errc, max_errc)];
}
