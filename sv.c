#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/select.h>
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

	printf("[D] started waiting for recveve.\n");

	while (1) {
		if (sockfd == -1) {
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
		}
		memset(buffer, 0, BUFFER_SIZE);
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(sockfd, &read_fds);
		int i = select(sockfd + 1, &read_fds, NULL, NULL, NULL);
		if (i == -1) {
			perror("select");
			break;
		}
		if (FD_ISSET(sockfd, &read_fds)) {

			ssize_t recv_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *) &client_addr, &addr_len);
			if (recv_len < 0) {
				perror("recvfrom");
				continue;
			}
			if (recv_len == 0) {
				close(sockfd);
				sockfd = -1;
			}
			if (recv_len == BUFFER_SIZE){
				buffer[BUFFER_SIZE-1] = '\0';
			}
			printf("%s[D] %ld Bytes received from %s:%d\n", buffer, recv_len, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
			fflush(stdout);
		}
	}

	close(sockfd);
	return 0;
}
