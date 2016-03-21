#include <sys/types.h>
#include <sys/wait.h>

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

int main()
{
	const pid_t pid = fork();
	if (pid < 0) {
		perror("failed to fork");
		exit(1);
	}

	srand(time(0));
	if (pid == 0) {
		const int rc = rand() & 0xff;
		printf("PROCESS %d says: return %d\n", getpid(), rc);
		exit(rc);
	} else {
		int status;
		waitpid(pid, &status, 0);
		printf("PROCESS %d says: My child's died, with return value %d\n", getpid(), WEXITSTATUS(status));
	}

	return 0;
}
