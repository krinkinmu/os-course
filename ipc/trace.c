#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <sys/reg.h>

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	const pid_t child = fork();
	if (!child) {
		char **args = calloc(argc, sizeof(char *));
		int i;

		for (i = 0; i != argc - 1; ++i)
			args[i] = strdup(argv[1 + i]);
		args[argc - 1] = 0;

		ptrace(PTRACE_TRACEME, 0, 0, 0);
		if (execvp(args[0], args) == -1)
			return 1;
		return 0;
	}

	if (child > 0) {
		const int syscall = SIGTRAP | 0x80;
		int status;

		waitpid(child, &status, 0);
		ptrace(PTRACE_SETOPTIONS, child, 0, PTRACE_O_TRACESYSGOOD);
		while (!WIFEXITED(status)) {
			long rax;

			ptrace(PTRACE_SYSCALL, child, 0, 0);
			waitpid(child, &status, 0);

			if (!WIFSTOPPED(status) || WSTOPSIG(status) != syscall)
				continue;

			rax = ptrace(PTRACE_PEEKUSER, child, 8 * ORIG_RAX, 0);
			printf("syscall %ld\n", rax);
			ptrace(PTRACE_SYSCALL, child, 0, 0);
			waitpid(child, &status, 0);
		}
	}

	return 0;
}
