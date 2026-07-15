#ifndef PING_H
#define PING_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>

typedef struct ping_data
{
	int		sock_fd;
	struct	sockaddr_in	targetAddr; // ip de la cible
	char	*hostname; // exemple: si ecrit ping google.com
	char	ip[INET_ADDRSTRLEN]; // ip reel

	int		ttl; // info dans 'struct icmphdr'
	int		seq; // info dans 'struct icmphdr'

	long	pack_trans; // package transmis
	long	pack_recv; // package recus
	int		pack_loss;

	double	rtt_total;
	double	sq_rtt_total; // rtt au carre pour stddev
	double	rtt_min; // reponse la + rapide
	double	rtt_max; // reponse la plus lente
	double	rtt_avg; // moyenne de tous les rtt
	double	rtt_stddev; // deviation moyenne
}	p_data;
//! penser a calculer les temps d'envoi du paquet, le temp general, le pourcentage de paquets perdu
extern p_data			*ping_signal;

p_data	initStruct(char **av);
void	recupAddrInfo(p_data *ping, char **av);

void	sendEcho(p_data *ping);
uint16_t	calculateChecksum(uint16_t *buff, int buffLen);

void    receveEcho(p_data *ping);

#endif