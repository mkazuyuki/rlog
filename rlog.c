/*
 * Raft Client
 *
 * Client reads the log from the pipe, sends it to a Raft server.
 * 
 * Client:
 * > date > pipe
 * 
 * Server:
 * > ./rlog
 * [I] Named PIPE was ready.
 * Wed Jun  4 10:45:21 JST 2025
 * [D] 29 bytes read
 * 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/poll.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>

// The theoretical maximum size of a UDP datagram payload is 65507 bytes, 
// which is 65535 bytes - 20 bytes for the IP header - 8 bytes for the UDP header.
#define BUFFER_SIZE 65507
#define IP	"127.0.0.1"
#define PORT	12345

void send_log(const char *data, const char *ip, int port)
{
	int sockfd;
	struct sockaddr_in server_addr;

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (sockfd < 0) {
		perror("socket");
		return;
	}

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	server_addr.sin_addr.s_addr = inet_addr(ip);
	
	if ( -1 == sendto(sockfd, data, strlen(data), 0, (struct sockaddr *) &server_addr, sizeof(server_addr)))
		perror("sendto");
	close(sockfd);
}

int main(int argc, char *argv[])
{
	char *pipefile = "pipe";
	const int buflen = BUFFER_SIZE;
	char buf[buflen];
	int fd = -1;

	if (mkfifo(pipefile, 0666) == -1 && errno != EEXIST) {
		perror("mkfifo");
		return 1;
	}
	printf("[I] Named PIPE was ready.\n");

	fd = open(pipefile, O_RDONLY | O_NONBLOCK);
	if (fd == -1) {
		perror("open");
		return 1;
	}

	struct pollfd fds[1];
	fds[0].fd = fd;
	fds[0].events = POLLIN;

	while (1) {
		if (-1 == poll(fds, 1, -1)){
			if (errno == EINTR)
				continue;
			perror("poll");
			break;
		}
		if (fds[0].revents & POLLIN) {
			ssize_t bytes_read = read(fd, buf, buflen - 1);
			if (bytes_read == -1) {
				perror("read");
				break;
			}
			buf[bytes_read]='\0';
			printf("%s", buf);
			printf("[D] %ld bytes read\n", bytes_read);
			fflush(stdout);

			char *ip = IP;
			int port = PORT;
			send_log(buf, ip, port);
		}
	}

	if (fd >= 0) {
		close(fd);
	};
	unlink(pipefile);
	return 0;
}
