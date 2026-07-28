#include "../includes/ping.h"

// ntohs (network to host short) comme htons utiliser pour send mais cette fois
// on recoit donc il faut remettre les bits en ordre
static void    doAnswer(struct icmphdr *icmp_r_buff, char * buff, p_data *ping, ssize_t t) {
    if (ntohs(icmp_r_buff->un.echo.id) == getpid()) {
        struct iphdr *ip = (struct iphdr *)buff;
        // permet de decoder la premiere couche (ip) du buff
        ping->size = t - 20; // les 20 octets de l'en-tete ip
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
            ping->size, ping->ip, ping->seq, ping->ttl, rtt);
        ping->rtt_total += rtt;
        ping->rtt_count++;
        ping->rtt_sq_total += (rtt * rtt);
    }
}

// reponse paquet
// +-------------------------------------------------------------------+
// | 1. En-tête IP de la Réponse (20 octets)                           |
// +-------------------------------------------------------------------+
// | 2. En-tête ICMP d'Erreur (8 octets : Type 11, Code 0...)          |
// +-------------------------------------------------------------------+
// | 3. En-tête IP d'Origine (20 octets) [Inclus dans le corps ICMP]   |
// +-------------------------------------------------------------------+
// | 4. En-tête ICMP d'Origine (8 octets) [Inclus dans le corps ICMP]  |
// +-------------------------------------------------------------------+

static void TimeExceededVerbose(char *buff, struct icmphdr *icmp_r_buff, int t) {
    struct iphdr *ip = (struct iphdr *)buff;
    int ip_len = ip->ihl * 4;

    // https://datatracker.ietf.org/doc/html/rfc791#section-3.1 + rfc icmp "Time Exceeded Message"
    //! ihl (internet header lenght) ne contien pas de nombre mais stock 32 bits, c'est a dire 4 octets
    //? la taillle standard d'une ip est 20 octets
    // 32 bits dans ce cas fait 20 / 4 = 5 (valeur de ihl)
    // je pourrais faire char *icmp_start = buff + 20 mais parfois l'en-tete ip est plus grande
    //! ce qui nous interesse c'est / 4 car si un octet arrive avec des options, c'est ihl qui sera different
    struct iphdr *origin_ip = (struct iphdr *)(buff + ip_len + sizeof(struct icmphdr));
    int origin_ip_len = origin_ip->ihl * 4;
    
    struct icmphdr *origin_icmp = (struct icmphdr *)((char *)origin_ip + origin_ip_len);
    unsigned char *ip_raw = (unsigned char *)origin_ip;

    char return_addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &ip->saddr,return_addr, INET_ADDRSTRLEN);
    printf("%d bytes from %s: Time to live exceeded\n",(t - ip_len), return_addr);
    
    int size = sizeof(origin_icmp);
    printf("IP Hdr Dump:\n");
    for (int i = 0; i < origin_ip_len; i++) {
        printf("%02x", ip_raw[i]);
        if ((i + 1) % 2 == 0)
            printf(" ");
    }
    printf("\n");
    char src[INET_ADDRSTRLEN], dst[INET_ADDRSTRLEN];
    // https://man7.org/linux/man-pages/man3/inet_ntop.3.html
    inet_ntop(AF_INET, &origin_ip->saddr, src, INET_ADDRSTRLEN);
    inet_ntop(AF_INET, &origin_ip->daddr, dst, INET_ADDRSTRLEN);
    printf("Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src	Dst	Data\n");
    printf(" %1x  %1x  %02x %04x %04x   %1x %04x  %02x  %02x %04x %s  %s\n",
        origin_ip->version,
        origin_ip->ihl,
        origin_ip->tos,
        ntohs(origin_ip->tot_len),
        ntohs(origin_ip->id),
// Flg et off -> Flags + Fragment Offset dans l'ip header (16 bits) (voir readme)
        (ntohs(origin_ip->frag_off) >> 13) & 0x07, // decalage de 13 bits on garde les 3 de gauche avec un masque --> en binaire 7 = 0000 0111
        ntohs(origin_ip->frag_off) & 0x1FFF, // 0x1FFF correspond a 0001 1111 1111 1111 --> ici le masque s'applique au 13 bits faibles
        origin_ip->ttl,
        origin_ip->protocol,
        ntohs(origin_ip->check),
        src, dst);

        int origin_size = t - ip_len - sizeof(struct icmphdr) - origin_ip_len;
        uint16_t origin_id = ntohs(origin_icmp->un.echo.id);
        uint16_t origin_seq = ntohs(origin_icmp->un.echo.sequence);
        printf("ICMP: type %d, code %d, size %d, id 0x%04x, sq 0x%04x\n",
        origin_icmp->type, origin_icmp->code,
        origin_size, origin_id, origin_seq);
}

// type 3: Destination unreachable
//    code: - 0 -> net unreachable
//          - 1 -> host unreachable
//          - 2 -> protocol unreachable
//          - 3 -> port unreachable
//          - 4 -> fragmentation needed and DF set
//          - 5 -> source route failed
//  From <ip> icmp_seq=X <code concerner>
// type 11 -> time exceeded
//    code: - 0 -> time to live exceeded in transit
//          - 1 -> fragment reassembly time exceeded
//  From <ip> icmp_seq=X Time to live exceeded
// rfc : https://datatracker.ietf.org/doc/html/rfc792
static void    handleIcmpError(struct icmphdr *icmp_buff, p_data *ping, char *buff, int t) {
    // error ICMP
    if (icmp_buff->type == ICMP_DEST_UNREACH) {
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
        if (ping->verbose)
            TimeExceededVerbose(buff, icmp_buff, t);
        else
            printf("From %s icmp_seq=%d Time to live exceeded\n", ping->ip, ping->seq);
    }
}

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
void    receveEcho(p_data *ping) {
    char buff[1024];
    struct sockaddr_in add_buff;
    socklen_t len = sizeof(add_buff);

    ssize_t t = recvfrom(ping->sock_fd, buff, sizeof(buff), 0,(struct sockaddr *)&add_buff, &len);
    // https://man7.org/linux/man-pages/man3/recvfrom.3p.html
    if(t == -1) {
        if (errno == EINTR)
            return;
    }
    if (t == 0)
        printf("Error: empty packet receive\n");
    else {
        struct icmphdr *icmp_r_buff = (struct icmphdr *)(buff + 20);
        if (icmp_r_buff->type == ICMP_ECHO)
            receveEcho(ping);
        if (icmp_r_buff->type == ICMP_ECHOREPLY) {
            doAnswer(icmp_r_buff, buff, ping, t);
        }
        else {
            handleIcmpError(icmp_r_buff, ping, buff, t);
        }
    }
}

// static void TimeExceededVerbose(char *buff, struct icmphdr *icmp_r_buff, p_data *ping) {
//     struct iphdr *ip_hdr = (struct iphdr *)buff; // En-tête IP de la réponse
        
//         // En-tête IP de TON paquet original (situé juste après l'en-tête ICMP d'erreur)
//         int ip_hdr_len = ip_hdr->ihl * 4;
//         struct iphdr *orig_ip_hdr = (struct iphdr *)(buff + ip_hdr_len + sizeof(struct icmphdr));
        
//         // En-tête ICMP d'origine
//         int orig_ip_len = orig_ip_hdr->ihl * 4;
//         struct icmphdr *orig_icmp = (struct icmphdr *)((char *)orig_ip_hdr + orig_ip_len);

//         // 2. Formatage IP Hdr Dump
//         printf("IP Hdr Dump:\n ");
//         // Affiche l'en-tête IP sous forme d'octets hexadécimaux (20 octets)
//         unsigned char *ip_raw = (unsigned char *)ip_hdr;
//         for (int i = 0; i < ip_hdr_len; i++) {
//             printf("%02x", ip_raw[i]);
//             if ((i + 1) % 2 == 0) printf(" "); // Espace tous les 2 octets
//         }
//         printf("\n");

//         // 3. Décodage détaillé des champs de l'en-tête IP
//         char src_str[INET_ADDRSTRLEN], dst_str[INET_ADDRSTRLEN];
//         inet_ntop(AF_INET, &(ip_hdr->saddr), src_str, INET_ADDRSTRLEN);
//         inet_ntop(AF_INET, &(ip_hdr->daddr), dst_str, INET_ADDRSTRLEN);

//         printf(" Vr HL TOS  Len   ID Flg  off TTL Pro  cks      Src      Dst Data\n");
//         printf("  %1x  %1x  %02x %04x %04x   %1x %04x  %02x  %02x %04x %s  %s\n",
//                ip_hdr->version,
//                ip_hdr->ihl,
//                ip_hdr->tos,
//                ntohs(ip_hdr->tot_len),
//                ntohs(ip_hdr->id),
//                (ntohs(ip_hdr->frag_off) >> 13) & 0x07, // Flags (DF, MF)
//                ntohs(ip_hdr->frag_off) & 0x1FFF,       // Offset
//                ip_hdr->ttl,
//                ip_hdr->protocol,
//                ntohs(ip_hdr->check),
//                src_str,
//                dst_str);

//         // 4. Détails ICMP (Paquet original retourné)
//         uint16_t orig_id  = ntohs(orig_icmp->un.echo.id);
//         uint16_t orig_seq = ntohs(orig_icmp->un.echo.sequence);

//         printf("ICMP: type %d, code %d, size %d, id 0x%04x, seq 0x%04x\n",
//                icmp_r_buff->type, 
//                icmp_r_buff->code, 
//                ping->size, 
//                orig_id,   // On affiche l'ID de TON paquet original
//                orig_seq); // On affiche le SEQ de TON paquet original
// }