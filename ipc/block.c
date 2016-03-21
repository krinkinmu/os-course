#include <sys/types.h>
#include <signal.h>
#include <unistd.h>
#include <stdio.h>

static void try_it_again(int sig)
{
	static const char msg[] = "I'm still alive!\n"; 
	write(1, msg, sizeof(msg) - 1);
}

static void block_signals(void)
{
	sigset_t set;
	sigfillset(&set);
	sigprocmask(SIG_BLOCK, &set, NULL);
}

int main(void)
{
	signal(SIGTERM, &try_it_again);

	printf("I'm %d, kill me if you can!\n", (int)getpid());
	block_signals();
	while (1);

	return 0;
}
