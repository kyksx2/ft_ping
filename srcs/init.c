#include "../includes/ping.h"

p_data	initStruct(char **av) {
	p_data ping;
	ping.sock_fd = 0;
	ping.hostname = av[0];
	ping.ip[0] = '\0';
	ping.ttl = 0;
	ping.seq = 0;
	ping.pack_trans = 0;
	ping.pack_recv = 0;
	ping.pack_loss = 0;
	ping.rtt_total = 0.0;
	ping.rtt_min = 0.0;
	ping.rtt_max = 0.0;
	ping.rtt_avg = 0.0;
	ping.rtt_stddev = 0.0;
	return ping;
}

void	recupAddrInfo(p_data *ping, char **av) {
// https://man7.org/linux/man-pages/man3/getaddrinfo.3.html
	struct addrinfo hints;
	struct addrinfo *result;

	memset(&hints, 0, sizeof(hints));
	hints.ai_protocol = IPPROTO_ICMP; // accepte tout protocol
	hints.ai_family= AF_INET; // ipv4
	hints.ai_socktype = SOCK_RAW; // use for network protocols

	int info = getaddrinfo(av[0], NULL, &hints, &result);
	if (info != 0) {
		fprintf(stderr, "ft_ping: unknown host\n");
		exit(EXIT_FAILURE);
	}

	if (result == NULL || result->ai_addr == NULL) {
		fprintf(stderr, "ft_ping: network error\n");
		if (result)
			freeaddrinfo(result);
		exit(EXIT_FAILURE);
	}

	struct sockaddr_in *ip = (struct sockaddr_in*)result->ai_addr;
	ping->targetAddr = *ip;
	
	if (inet_ntop(AF_INET, &(ip->sin_addr), ping->ip, INET_ADDRSTRLEN) == NULL) {
		perror("inet_ntop");
		freeaddrinfo(result);
		exit(1);
	}

	freeaddrinfo(result);
}