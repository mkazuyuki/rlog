/*
 * Raft Client
 *
 * Client reads the log from the pipe, sends it to a Raft server.
 * 
 * Client:
 * > date hello > pipe
 * 
 * Server:
 * > ./rlog
 * [I] Named PIPE was ready.
 * Wed Jun  4 10:45:21 JST 2025
 * [D] 29 bytes read
 * [D] 0 bytes read
 * 
 */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <sys/select.h>
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
	inet_pton(AF_INET, ip, &server_addr.sin_addr);
	
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

	while (1) {
		if (fd == -1) {
			fd = open(pipefile, O_RDONLY | O_NONBLOCK);
			if (fd == -1) {
				perror("open");
				return 1;
			}
		}

		memset(buf, 0, buflen);
		fd_set read_fds;
		FD_ZERO(&read_fds);
		FD_SET(fd, &read_fds);

		int ret = select(fd + 1, &read_fds, NULL, NULL, NULL);
		if (ret == -1) {
			if (errno == EINTR)
				continue;
			perror("select");
			break;
		}
		if (FD_ISSET(fd, &read_fds)) {
			ssize_t bytes_read = read(fd, buf, buflen - 1);
			if (bytes_read == -1) {
				perror("read");
				break;
			}
			if (bytes_read == 0) {
				close(fd);
				fd = -1;
			}
			//printf("%s[D] READ bytes=[%ld]\n", buf, bytes_read);
			//fflush(stdout);
			char *ip = IP;
			int port = PORT;
			//printf("[D] [%ld] [%s]\n", bytes_read, buf);
			printf("%s[D] %ld bytes read\n", buf, bytes_read);

			send_log(buf, ip, port);
		}
	}

	if (fd >= 0) {
		close(fd);
	};
	unlink(pipefile);
	return 0;
}
