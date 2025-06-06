#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/poll.h>
#include <unistd.h>
#include <time.h>

#define PORT 12345
#define BUFFER_SIZE 65507

// Role of a member
typedef enum {
	FOLLOWER,
	CANDIDATE,
	LEADER
} RaftRole;

// Information of a member
typedef struct _ClusterMember {
	struct sockaddr_in addr;
	struct _ClusterMember* next;
} ClusterMember;

// Status of me member
typedef struct {
	RaftRole role;
	int current_term;
	int voted_for;
	time_t election_timeout;
} RaftState;

RaftState raft_state;
ClusterMember* cluster_members;

// メンバーの追加
ClusterMember* add_member(struct sockaddr_in addr) {
	ClusterMember* new_member = (ClusterMember*)malloc(sizeof(ClusterMember));
	if (new_member == NULL) {
		perror("malloc");
		return NULL;
	}
	new_member->addr = addr;
	new_member->next = NULL;

	if (cluster_members == NULL) {
		cluster_members = new_member;
		return new_member;
	}

	ClusterMember* current = cluster_members;
	ClusterMember* prev = NULL;
	for (; current != NULL; prev = current, current = current->next) {
		if (memcmp(&current->addr, &addr, sizeof(struct sockaddr_in)) == 0) {
			printf("[E] Member exists already.\n");
			free(new_member);
			return NULL;
		}
	}
	// prevはリストの最後を指している
	prev->next = new_member;
	return new_member;
}

// メンバーの削除
int remove_member(struct sockaddr_in addr) {
	ClusterMember* current = cluster_members;
	ClusterMember* prev = NULL;
	
	while (current != NULL) {
		if (memcmp(&current->addr, &addr, sizeof(struct sockaddr_in)) == 0) {
			if (prev == NULL) {
				cluster_members = current->next;
			} else {
				prev->next = current->next;
			}
			free(current);
			return 0;
		}
		prev = current;
		current = current->next;
	}
	return -1;
}

// メンバーシップ変更
void handle_membership_change(const char* message) {
	// メッセージ形式: "JOIN" または "LEAVE"
	char word[5];
	char ip[16];
	char port[6];
	sscanf(message, "%s %s %s", word, ip, port);

	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET; // Add this line
	addr.sin_addr.s_addr = inet_addr(ip);
	addr.sin_port = htons(atoi(port));

	if (strncmp(message, "JOIN", 4) == 0) {
		// メンバー追加
		ClusterMember* new_member = add_member(addr);
		if (new_member != NULL) {
			printf("[I] Member joined: %s:%d\n", inet_ntoa(new_member->addr.sin_addr), ntohs(new_member->addr.sin_port));
		}
	} else if (strncmp(message, "LEAVE", 5) == 0) {
		// メンバー削除
		if(remove_member(addr)){
			printf("[E] Failed to remove: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		} else {
			printf("[I] Member left: %s:%d\n", inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
		}
	} else if (strncmp(message, "DUMP", 4) == 0) {
		// メンバー一覧の表示
		ClusterMember* current = cluster_members;
		int index = 0;
		while (current != NULL) {
			printf("[I] Member index[%d] %s:%d\n", index,
				   inet_ntoa(current->addr.sin_addr),
				   ntohs(current->addr.sin_port));
			current = current->next;
			index++;
		}
	}
}

// メモリの解放
void cleanup_members() {
	ClusterMember* current = cluster_members;
	while (current != NULL) {
		ClusterMember* next = current->next;
		free(current);
		current = next;
	}
	cluster_members = NULL;
}

int main(int argc, char* argv[])
{
	int sockfd = -1;
	struct sockaddr_in server_addr, client_addr;
	char buffer[BUFFER_SIZE];
	socklen_t addr_len = sizeof(client_addr);

	// 初期化
	memset(&raft_state, 0, sizeof(raft_state));
	cluster_members = NULL;

	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);
	//server_addr.sin_port = htons(atoi(argv[1]));

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
			if (recv_len >= BUFFER_SIZE) recv_len = BUFFER_SIZE - 1;
			buffer[recv_len] = '\0';
			
			// メンバーシップの変更処理
			if (strncmp(buffer, "JOIN", 4) == 0
			 || strncmp(buffer, "LEAVE", 5) == 0
			 || strncmp(buffer, "DUMP", 4) == 0) {
				handle_membership_change(buffer);
			} else {
				printf("%s", buffer);
				printf("[D] %ld Bytes received from %s:%d\n", recv_len, inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
			}
			fflush(stdout);
		}
	}

	cleanup_members();
	close(sockfd);
	return 0;
}
