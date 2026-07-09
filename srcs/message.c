#include "../includes/ping.h"

void	sendEcho(p_data *ping) {
	char	buff[64];
	memset(&buff, 0, sizeof(buff));

// https://fr.wikipedia.org/wiki/Internet_Control_Message_Protocol
	struct icmphdr s;
	s.type = ICMP_ECHO; // demande d'echo
	s.code = 0;
	s.checksum = 0;
	s.un.echo.id = (uint16_t)getpid(); // le pid permetde verifier que c'est bien moi a la reception
	s.un.echo.sequence = ping->seq; // juste le compteur a incrementer a chaque ping

	memcpy(&buff, &s, sizeof(s));

	printf("buff: %s\n", buff);
}