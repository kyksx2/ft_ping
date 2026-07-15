#include "../includes/ping.h"

void    receveEcho(p_data *ping) {
    char buff[1024];
    struct sockaddr_in add_buff;
    socklen_t len = sizeof(add_buff);

    ssize_t t = recvfrom(ping->sock_fd, buff, sizeof(buff), 0,(struct sockaddr *)&add_buff, &len);
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
    if (icmp_r_buff->type == ICMP_ECHOREPLY) {
        if (ntohs(icmp_r_buff->un.echo.id) == getpid()) {
        // ntohs (network to host short) comme htons utiliser pour send mais cette fois
        // on recoit donc il faut remettre les bits en ordre
            struct iphdr *ip = (struct iphdr *)buff;
            // permet de decoder la premiere couche (ip) du buff
            int size = t - 20; // les 20 octets de l'en-tete ip
            ping->ttl = ip->ttl;
            ping->pack_recv++;
            struct timeval time_send = *(struct timeval*)((char *)icmp_r_buff + sizeof(struct icmphdr));
            struct timeval time_receive;
            gettimeofday(&time_receive, NULL);
            double rtt = (time_receive.tv_sec - time_send.tv_sec) * 1000.0 +
                        (time_receive.tv_usec - time_send.tv_usec) / 1000.0;
            if (ping->rtt_min == 0) {
                ping->rtt_min = rtt;
            }
            else if (ping->rtt_min > rtt) {
                ping->rtt_min = rtt;
            }
            if (ping->rtt_max < rtt) {
                ping->rtt_max = rtt;
            }
            printf("%d bytes from %s: icmp_seq=%d ttl=%d time=%.3f\n",
                size, ping->ip, ping->seq, ping->ttl, rtt);
            ping->rtt_total += rtt;
        }
        else {
            // autres cas possible, juste passer silencieusement
        }
    }
    else {
        // error ICMP
    }
    }
}