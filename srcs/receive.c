#include "../includes/ping.h"

static void    doAnswer(struct icmphdr *icmp_r_buff, char * buff, p_data *ping, ssize_t t) {
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
        ping->rtt_count++;
        ping->rtt_sq_total += (rtt * rtt);
    }
}

static void    handleIcmpError(struct icmphdr *icmp_buff, p_data *ping) {
    // error ICMP
    if (icmp_buff->type == ICMP_DEST_UNREACH) {
        // type 3: Destination unreachable
        //    code: - 0 -> net unreachable
        //          - 1 -> host unreachable
        //          - 2 -> protocol unreachable
        //          - 3 -> port unreachable
        //          - 4 -> fragmentation needed and DF set
        //          - 5 -> source route failed
        //  From <ip> icmp_seq=X <code concerner>
        switch (icmp_buff->code) {
        case ICMP_NET_UNREACH:
            printf("From %s icmp_seq=%d  Destination Network Unreachable\n", ping->ip, ping->seq);
            break;
        case ICMP_HOST_UNREACH:
            printf("From %s icmp_seq=%d  Destination Host Unreachable\n", ping->ip, ping->seq);
            break;
        case ICMP_PROT_UNREACH:
            printf("From %s icmp_seq=%d  Destination Protocol Unreachable\n", ping->ip, ping->seq);
            break;
        case ICMP_PORT_UNREACH:
            printf("From %s icmp_seq=%d  Destination Port Unreachable\n", ping->ip, ping->seq);
            break;
        case ICMP_FRAG_NEEDED:
            printf("From %s icmp_seq=%d  Fragmentation Needed/DF set\n", ping->ip, ping->seq);
            break;
        case ICMP_SR_FAILED:
            printf("From %s icmp_seq=%d  Source Route Failed\n", ping->ip, ping->seq);
            break;
        default:
            printf("From %s Destination Unreachable (code=%d)\n", ping->ip, ping->seq);
            break;
        }
    }
    else if (icmp_buff->type == ICMP_TIME_EXCEEDED) {
        // type 11 -> time exceeded
        //    code: - 0 -> time to live exceeded in transit
        //          - 1 -> fragment reassembly time exceeded
        //  From <ip> icmp_seq=X Time to live exceeded
        // rfc : https://datatracker.ietf.org/doc/html/rfc792
        printf("From %s icmp_seq=%d Time to live exceeded\n", ping->ip, ping->seq);
    }
}

void    receveEcho(p_data *ping) {
    char buff[1024];
    struct sockaddr_in add_buff;
    socklen_t len = sizeof(add_buff);

    ssize_t t = recvfrom(ping->sock_fd, buff, sizeof(buff), 0,(struct sockaddr *)&add_buff, &len);
    // https://man7.org/linux/man-pages/man3/recvfrom.3p.html
    // if(t == -1) {
    //     if (errno == EWOULDBLOCK || errno == EAGAIN) {
    //         printf("Request timeout for icmp_seq %d\n", ping->seq);
    //     }
    //     else if (errno == EINTR)
    //         return;
    //     else
    //         perror("recvfrom fatal error");
    // }
    // else if (t == 0)
    //     printf("Error: empty packet receive\n");
    // else {
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
            doAnswer(icmp_r_buff, buff, ping, t);
        }
        else {
            handleIcmpError(icmp_r_buff, ping);
        }
    // }
}