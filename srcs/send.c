#include "../includes/ping.h"

uint16_t	calculateChecksum(uint16_t *buff, int buffLen) {
	uint32_t sum = 0;
 	// checksum additionne 32 nombres de 16 bits

	while(buffLen > 1) { // ajouter les block de 16 bits a sum
		sum += *buff++;
		buffLen -= 2;
	}

	if (buffLen > 0) {
 	// dans ce cas, si bufflen est supp a 0, elle vaudra 1 donc tiendra sur un octet
 	// uint16 tien sur 2 et le compilateur doit le savoir donc je force la lecture sur un seul octet
		sum += *(unsigned char*)buff;
	}

	while (sum >> 16) {
 	// on decalle tous les bits de 16 cases vers la droites
 	// ici il faut imagine sum comme ca : 0x0003FFFE -> 0003 16 bits de gauche (retenue, ce qui depasse)
 	// FFFE 16 bits de droite taille max utilisee pour le checksum final
 	// si on decalle de 16 vers la droite on trouve 0x0003 donc on ne touche plus a FFFE
		sum = (sum & 0xFFFF) + (sum >> 16);
		// 0xFFFF agit comme un masque donc si sum = 0x0003FFFE le masque correspond a
		// 0x00001111 donc sum pensera etre 0x000FFFE et ainsi on aditionne FFFE avec
		// les bits de gauche decaller a droite donc 0x0000FFFE + 0x00000003
	}
	// regle du one's complement (on inverse les bits) grace au tidle (~)
	return (uint16_t)(~sum);
}

void	sendEcho(p_data *ping) {
	char	buff[64]; // on envoi un msg de 64 octets
	memset(buff, 0, sizeof(buff));

 	// https://fr.wikipedia.org/wiki/Internet_Control_Message_Protocol
	struct icmphdr s;
	s.type = ICMP_ECHO; // demande d'echo
	s.code = 0;
	s.checksum = 0;
	s.un.echo.id = htons(getpid()); // le pid permetde verifier que c'est bien moi a la reception
	s.un.echo.sequence = htons(ping->seq); // juste le compteur a incrementer a chaque ping
 	// utilisation de htons() pour passer en uint16_t version net
 	// La fonction htons() (host to network short) convertit l’entier court non signé
 	// 'hostshort' de l’ordre des octets de l’hôte à l’ordre des octets du réseau. 
 	// https://www.man-linux-magique.net/man3/htons.html
	
	memcpy(buff, &s, sizeof(s));
	
	struct timeval time;
	gettimeofday(&time, NULL);
	memcpy(buff + sizeof(s), &time, sizeof(time));

	struct icmphdr *icmp_buff = (struct icmphdr *)buff;
	icmp_buff->checksum = calculateChecksum((uint16_t *)buff, sizeof(buff));
	// checksum a faire
	sendto(ping->sock_fd, buff, sizeof(buff), 0, (struct sockaddr *)&ping->targetAddr, sizeof(ping->targetAddr));
	// verif send to
	ping->pack_trans++;
}