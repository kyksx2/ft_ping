#ifndef PING_H
#define PING_H

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <signal.h>
#include <stdbool.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
#include <netinet/ip_icmp.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <errno.h>

typedef struct s_args
{
	bool	verbose;
	bool	ttl_use;
	int		ttl;
	char	*hostname;
}	t_args;

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

	bool	verbose;

	double	rtt_total;
	int		rtt_count;
	double	rtt_sq_total; // rtt au carre pour stddev
	double	rtt_min; // reponse la + rapide
	double	rtt_max; // reponse la plus lente
	double	rtt_avg; // moyenne de tous les rtt
	double	rtt_stddev; // ecart type des rtt
}	p_data;
//! penser a calculer les temps d'envoi du paquet, le temp general, le pourcentage de paquets perdu

extern p_data			*ping_signal;

p_data	initStruct(t_args args);
void	recupAddrInfo(p_data *ping);

void	sendEcho(p_data *ping);

void    receveEcho(p_data *ping);

#endif