#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main(void)
{
	char buf[4096];
	int size, fds[2];

	pipe(fds);
	if (fork()) {
		while ((size = read(0, buf, sizeof(buf) - 1)) > 0) {
			buf[size] = '\0';
			printf("Send: %s\n", buf);
			write(fds[1], buf, size);
		}
	} else {
		while ((size = read(fds[0], buf, sizeof(buf) - 1)) > 0) {
			buf[size] = '\0';
			printf("Received: %s\n", buf);
		}
	}
	close(fds[0]);
	close(fds[1]);

	return 0;
}
