#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char **argv)
{
	struct sockaddr_in remote;
	const int port = 2016;
	const int sk = socket(AF_INET, SOCK_STREAM, 0);

	if (sk == -1) {
		perror("failed to create socket");
		exit(1);
	} 

	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);
	remote.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (connect(sk, (const struct sockaddr *)&remote, sizeof(remote)) == -1) {
		perror("failed to connect socket");
		exit(1);
	}

	for (int i = 0; i != argc; ++i)
		write(sk, argv[i], strlen(argv[i]));

	close(sk);

	return 0;
}
