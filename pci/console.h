#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#include "list.h"

struct console {
	struct list_head link;
	void (*write)(const char *, unsigned long);
};

void register_console(struct console *console);
void unregister_console(struct console *console);
void console_write(const char *str, unsigned long size);

#endif /*__CONSOLE_H__*/
