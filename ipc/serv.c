#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

static void handle(int sk)
{
	char buf[1024];
	int size;

	while ((size = read(sk, buf, sizeof(buf) - 1)) > 0) {
		buf[size] = '\0';
		puts(buf);
	}
}

int main(void)
{
	struct sockaddr_in local;
	const int port = 2016;
	const int sk = socket(AF_INET, SOCK_STREAM, 0);

	if (sk == -1) {
		perror("failed to create socket");
		exit(1);
	} 

	local.sin_family = AF_INET;
	local.sin_port = htons(port);
	local.sin_addr.s_addr = htonl(INADDR_LOOPBACK);

	if (bind(sk, (const struct sockaddr *)&local, sizeof(local)) == -1) {
		perror("failed to bind socket");
		exit(1);
	}

	if (listen(sk, 0) == -1) {
		perror("listen failed");
		exit(1);
	}

	while (1) {
		struct sockaddr_in remote;
		socklen_t size = sizeof(remote);

		const int client = accept(sk, (struct sockaddr *)&remote, &size);

		if (client == -1) {
			perror("accept failed");
			exit(1);
		}

		printf("Connection from %s:%d\n",
			inet_ntoa(remote.sin_addr),
			ntohs(remote.sin_port));

		handle(client);
		close(client);
	}
	close(sk);

	return 0;
}
