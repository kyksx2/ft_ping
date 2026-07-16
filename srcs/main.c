#include "../includes/ping.h"

p_data	*ping_signal = NULL;

static void calculAvgStddev(p_data *ping_signal) {
	ping_signal->rtt_avg = ping_signal->rtt_total / ping_signal->pack_recv;
	// avg = moyenne -> (n + n1 + n2 + n3 + ....) / n total
	// stddev = ecart type -> ((rtt2) - ((rtt2) / N) / N - 1)
	// a la fin il faut mettre le resultat au carre
	// rtt2 --> somme des rtt au carre
	// N --> total nombre de rtt
	// formule donne par gemini pour eviter la liste chainee + malloc
	if (ping_signal->rtt_count <= 1) {
		return;
	}
	else {
		double variance = (ping_signal->rtt_sq_total - (ping_signal->rtt_sq_total / ping_signal->rtt_count)) / ping_signal->rtt_count - 1;
		ping_signal->rtt_stddev = sqrt(variance);
	}
}

void	handle_sigs(int sig) {
	if (sig == SIGINT) {
		calculAvgStddev(ping_signal);
		printf("--- %s ping statistics ---\n", ping_signal->hostname);
		printf("%d packets transmitted, %d packets received, %d%% packet loss\n", ping_signal->pack_trans, ping_signal->pack_recv, ping_signal->pack_loss);
		if (ping_signal->pack_recv > 0)
			printf("round-trip min/avg/max/stddev = %.3f/%.3f/%.3f/%.3f ms\n", ping_signal->rtt_min, ping_signal->rtt_avg, ping_signal->rtt_max, ping_signal->rtt_stddev);
		close(ping_signal->sock_fd);
		exit(0);
	}
	else if (sig == SIGQUIT) {
		close(ping_signal->sock_fd);
		printf("[1]	%d	ping %s\n", getpid(), ping_signal->hostname);
		exit(131);
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

static t_args	getArgs(int ac, char **av) {
	if (ac < 2) {
		fprintf(stderr, "ft_ping: usage error: Destination address required");
		exit(EXIT_FAILURE);
	}

	t_args start;
	start.verbose = false;
	start.hostname = NULL;
	start.ttl = 0;
	start.ttl_use = false;

	for (int i = 1; av[i]; i++) {
		if (strcmp(av[i], "-v") == 0)
			start.verbose = true;
		else if (strcmp(av[i], "--ttl") == 0) {
			start.ttl_use = true;
			if (i + 1 < ac) {
				start.ttl = atoi(av[i + 1]);
				i++;
			}
			else {
				fprintf(stderr,"ft_ping: option '--ttl' requires an argument\n");
				exit(EXIT_FAILURE);
			}
		}
		else {
			if (start.hostname == NULL)
				start.hostname = av[i];
			else {
                fprintf(stderr, "ft_ping: extra target '%s'\n", av[i]);
                exit(EXIT_FAILURE);
            }
		}
	}

	if (start.hostname == NULL) {
		fprintf(stderr, "ft_ping: missing host operand\n");
		exit(EXIT_FAILURE);
	}
	return start;
}

int main(int ac, char **av) {

	t_args args = getArgs(ac, av);
	p_data	ping = initStruct(args);
	recupAddrInfo(&ping);

	ping_signal = &ping;
	signal(SIGINT, handle_sigs);
	signal(SIGQUIT, handle_sigs);

	ping.sock_fd = socket(AF_INET, SOCK_RAW, IPPROTO_ICMP);
	if (ping.sock_fd == -1) {
		perror("socket");
		return (1);
	}

	struct timeval time;
	time.tv_sec = 1;
	time.tv_usec = 0;
	if (setsockopt(ping.sock_fd, IPPROTO_ICMP, SO_RCVTIMEO, &time,sizeof(time)) < 0) {
		close(ping.sock_fd);
		perror("setsockopt");
		return (1);
	}
	if (args.ttl_use) {
		if (setsockopt(ping.sock_fd, IPPROTO_IP, IP_TTL, &ping.ttl, sizeof(ping.ttl)) < 0) {
			close(ping.sock_fd);
			perror("setsockopt");
			return (1);
		}

	}
	// pour les bonus setsockopt pour le ttl
	// https://stackoverflow.com/questions/31066061/setting-ttl-on-outgoing-udp-packets
	if (ping.verbose) {
		uint16_t id = (uint16_t)getpid();
		printf("PING %s (%s): 56 data bytes, id 0x%04x = %d\n", ping.hostname, ping.ip, id, id);
	}
	else if (!args.verbose)
		printf("PING %s (%s): 56 data bytes\n", ping.hostname, ping.ip);
	// 56 est la taille du msg 64 = 56 + 8 (octets de l'en-tete)
	while (1) {
		sendEcho(&ping);
		receveEcho(&ping);
		ping.seq++;
        if (ping.pack_trans > 0)
	        ping.pack_loss = ((ping.pack_trans - ping.pack_recv) * 100) / ping.pack_trans;
		sleep(1);
	}
	return (0);
}