#include "../includes/ping.h"

// volatile int	signal_running = 1;
p_data	*ping_signal = NULL;

void	handle_sigs(int sig) {
	if (sig == SIGINT) {
		printf("--- %s ping statistics ---\n", ping_signal->hostname);
		printf("%d packets transmitted, %d packets received, %d\% packet loss\n", ping_signal->pack_trans, ping_signal->pack_recv, ping_signal->pack_loss);
		printf("round-trip min/avg/max/stddev = %d/%d/%d/%d ms\n", ping_signal->rtt_min, ping_signal->rtt_avg, ping_signal->rtt_max, ping_signal->rtt_stddev);
		// free_all
		exit(0);
	}
	else if (sig == SIGQUIT) {
		// free_all
		exit(131);
		// signal_running = 0;
	}
}

int main(int ac, char **av) {
	if (ac != 2) {
		fprintf(stderr, "ping: usage error: Destination address required");
		return (EXIT_FAILURE);
	}

	p_data	ping = initStruct(&av[1]);
	recupAddrInfo(&ping, &av[1]);

	ping_signal = &ping;
	signal(SIGINT, handle_sigs);
	signal(SIGQUIT, handle_sigs);

	ping.sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ping.sock_fd == -1) {
		perror("socket");
		// free_all
		return (1);
	}

	struct timeval time;
	time.tv_sec = 1;
	time.tv_usec = 0;
	if (setsockopt(ping.sock_fd, IPPROTO_ICMP, SO_RCVTIMEO, &time,sizeof(time)) < 0) {
		perror("setsockopt");
		// free_all
		return (1);
	}
// pour les bnus setsockopt pour le ttl
// https://stackoverflow.com/questions/31066061/setting-ttl-on-outgoing-udp-packets
	// while (1) {
	sendEcho(&ping);
	// 	stateTime();
	// 	receveEcho();
	// 	waitForSecond();
	// }

	return (0);
}