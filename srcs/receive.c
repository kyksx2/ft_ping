#include "../includes/ping.h"

void    receveEcho(p_data *ping) {
    char buff[1024];
    struct sockaddr_in add_buff;
    socklen_t len = sizeof(add_buff);

    ssize_t t = recvfrom(ping->sock_fd, buff, sizeof(buff), 0,(struct sockaddr *)&add_buff, len);
// https://man7.org/linux/man-pages/man3/recvfrom.3p.html
    if(t == -1) {
        if (errno == EWOULDBLOCK || errno == EAGAIN) {
            printf("Request timeout for icmp_seq %d\n", ping->seq);
        }
        else if (errno == EINTR)
            return;
        else
            perror("recvfrom fatal error");
    }
    else if (t == 0)
        printf("Error: empty packet receive\n");
    else {
        struct icmphdr *icmp_r_buff = (struct icmphdr *)(buff + 20);
    // l'en tete IP fait 20 octets, il faut la saute, le reste 8 octets
    // tableau rfc : https://datatracker.ietf.org/doc/html/rfc792
    //  0                   1                   2                   3
    //  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |     Type      |     Code      |          Checksum             |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |           Identifier          |        Sequence Number        |
    // +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    // |     Data ...
    // +-+-+-+-+-

    }
}