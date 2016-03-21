#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

int main()
{
	const pid_t pid = fork();
	if (pid < 0) {
		perror("failed to fork");
		exit(1);
	}

	if (pid == 0)
		printf("PROCESS %d says: I'm child\n", getpid());
	else
		printf("PROCESS %d says: I'm parent of %d\n", getpid(), pid);

	return 0;
}
