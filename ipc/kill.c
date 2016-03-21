#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	if (argc < 2) {
		puts("Target process PID expected");
		exit(1);
	}

	for (int i = 1; i != argc; ++i) {
		const int pid = atoi(argv[i]);

		if (!pid) {
			printf("Wrong PID: %s\n", argv[i]);
			exit(1);
		}
		kill(pid, SIGKILL);
	}

	return 0;
}
