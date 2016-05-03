#include "console.h"

static LIST_HEAD(consoles);

void register_console(struct console *console)
{ list_add_tail(&console->link, &consoles); }

void unregister_console(struct console *console)
{ list_del(&console->link); }

void console_write(const char *str, unsigned long size)
{
	const struct list_head * const end = &consoles;
	struct list_head *pos = consoles.next;

	for (; pos != end; pos = pos->next) {
		struct console *console = LIST_ENTRY(pos, struct console, link);

		console->write(str, size);
	}
}
