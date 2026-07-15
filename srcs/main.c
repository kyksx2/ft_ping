#include "../includes/ping.h"

// volatile int	signal_running = 1;
p_data	*ping_signal = NULL;

void	handle_sigs(int sig) {
	if (sig == SIGINT) {
		ping_signal->rtt_avg = ping_signal->rtt_total / ping_signal->pack_recv;
		printf("--- %s ping statistics ---\n", ping_signal->hostname);
		printf("%d packets transmitted, %d packets received, %d%% packet loss\n", ping_signal->pack_trans, ping_signal->pack_recv, ping_signal->pack_loss);
		printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", ping_signal->rtt_min, ping_signal->rtt_avg, ping_signal->rtt_max, ping_signal->rtt_stddev);
		// free_all
		exit(0);
	}
	else if (sig == SIGQUIT) {
		// free_all
		exit(131);
		// signal_running = 0;
	}
}

// struct sockaddr_in --> sert a definir la cible ou la source,
// precis car contient protocol, adresse et port

// struct sockaddr --> version generique de l'adresse reseau en C
// sert pour les fonctions qui refusent sockaadr_in car plus generique
// utilise uniquement : (struct sockaddr *)&ping->targetAddr

// struct icmphdr --> le message IMCP
// utiliser pour l'envoi des donnees du ping ou analyser la reception
// c'est elle qui calcul le checksum a l'envoi et a la reception

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
	printf("PING %s (%s): 56 data bytes\n", ping.hostname, ping.ip);
	// 56 est la taille du msg 64 = 56 + 8 (octets de l'en-tete)
	while (1) {
		sendEcho(&ping);
		receveEcho(&ping);
		ping.seq++;
        if (ping.pack_trans > 0)
	        ping.pack_loss = ((ping.pack_trans - ping.pack_recv) * 100) / ping.pack_trans;
		// waitForSecond();
		sleep(1);
	}

	return (0);
}