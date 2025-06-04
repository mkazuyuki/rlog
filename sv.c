#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <unistd.h>

#define PORT 12345
#define BUFFER_SIZE 65507

int main()
{
	int sockfd = -1;
	struct sockaddr_in server_addr, client_addr;
	char buffer[BUFFER_SIZE];
	socklen_t addr_len = sizeof(client_addr);

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd == -1) {
		perror("socket");
		return 1;
	}
	if (bind(sockfd, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
		perror("bind");
		close(sockfd);
		exit(EXIT_FAILURE);
	}
	struct pollfd fds[1];
	fds[0].fd  = sockfd;
	fds[0].events = POLLIN;

	printf("[D] started waiting for recveve.\n");

	while (1) {
		if (-1 == poll(fds, 1, -1)){
			perror("poll");
			break;
		}
		if (fds[0].revents & POLLIN) {
			ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
			if (recv_len < 0) {
				perror("recvfrom");
				continue;
			}
			buffer[recv_len] = '\0';
			printf("%s", buffer);
			printf("[D] %ld Bytes received from %s:%d\n", recv_len, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
			fflush(stdout);
		}
	}

	close(sockfd);
	return 0;
}
