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
*    include.h:                                *
*      -declare includes                       *
*      -define some vars                       *
*      -declare some needed structs            *
*      -declare global vars                    *
*      -do prototype of functions              *
*                                              *
*                                              *
\*____________________________________________*/


/* Very good libraries which make ports very easy ;)  */

#include <libnet.h>
#include <pcap.h>
#include <pthread.h>
#include <ncurses.h>

#define VERSION "0.8devel"
#define MAXLINE 1029

#define CHECKPWD(n) (dp[n].check=='Y' || dp[n].check=='y')
struct redirect {

    int localport;
    int remoteport;
    char *host;

    // numero di connessioni di questo datapipe
    // number of connection of this datapipe
    ushort cn;

    // connessioni di questo datapipe (hosts)
    // connection of this datapipe (hosts)
    char *ch[10];

    // connessioni di questo datapipe (ports)
    // connection of this datapipe (ports)

    ushort cp[10];

    // byte ricevuti
    // received byted
    long int rb[10];

    // byte trasmessi
    // sent bytes
    long int tb[10];

    //socket descriptor
    int from_sd[10];
    int dest_sd[10];
    struct sockaddr_in sout;

    /*
     * datapipe connection status value: 
     0- print "client" data 
     1- print "server" data 
     2- print all data 
     3- print all data in different color 
     4- print "matched" data
     ---- 
     5-hijacking (keep client)
     6-hijacking (keep server) 
     7-hijacking (keep both) (skleroooooooooooo)
     8-closing this connection
     */
    int status[10];
    int logfile[10];
    //datapipe status
    int stat;
    FILE *fout;
	char mstring[50];
    char check;
    char pwd[32];
} dp[10];

//number of used datapipe
int ndp;



pthread_t dp_thread[10], h;

// windows
WINDOW *wmenu, *wlist, *wdata;

//this shows where is focus
// 0 = wmenu, 1 = wlist (list datapipe), 2 = wlist (list connection)
int win;

// mpos shows cursor's position in menu

// dpos shows cursor's position in datapipe's list 

// cpos shows cursor's position in connection's list
ushort mpos, dpos, cpos;



// this thread wait connections and pass it at handler
void datapipe();

//gestisce le connessioni
void handler(int *io);

// free all
void finish();

// erase \r from string (because an ncurses problem)
void normal(char *s, int len);
void statzero();

void info(char sleep, const char *s, ...);


/*
 * ripped and riadacted from dnshijacker v1.2 by pedram amini
 * (pedram@redhive.com) www.redhive.com 
 */

//add a dns spoofed host
void dnshijack_add(char *, char *);
//the thread that sniff dns activity
void dnshijacker();


/*
 * defines 
 */
#define SNAPLEN    128
#define PROMISC      1
#define T_A          1		/* host address */


/*
 * our chewycenter ... holds important information 
 */
struct cc_struct {
    u_long src_address;		/* source address */
    u_long dst_address;		/* destination address */
    u_short src_port;		/* source port */
    u_short dst_port;		/* destination port */
    u_short dns_id;		/* dns id */
    int is_a;			/* are we an A */
    int have_answer;		/* do we have an answer? */
    int payload_size;		/* size of our payload */
    u_char payload[512];	/* payload storage */
    char current_question[128];	/* current question being asked */
    char *current_answer;	/* current answer we are using */
};



/*
 * dns packet header |--- 1 byte ----|
 * +---------------|---------------|---------------|---------------+ | DNS ID 
 * | QR,OP,AA,TC,RD|RA,0,AD,CD,CODE|
 * |---------------|---------------|---------------|---------------| | #
 * Questions | # Answers |
 * |---------------|---------------|---------------|---------------| | #
 * Authority | # Resource |
 * +---------------|---------------|---------------|---------------+ 
 */

struct dnshdr {
    unsigned id:16;		/* query identification number */
    unsigned rd:1;		/* recursion desired */
    unsigned tc:1;		/* truncated message */
    unsigned aa:1;		/* authoritative answer */
    unsigned opcode:4;		/* purpose of message */
    unsigned qr:1;		/* response flag */
    unsigned rcode:4;		/* response code */
    unsigned cd:1;		/* checking disabled by resolver */
    unsigned ad:1;		/* authentic data from named */
    unsigned unused:1;		/* unused bits (MBZ as of 4.9.3a3) */
    unsigned ra:1;		/* recursion available */
    unsigned qdcount:16;	/* number of question entries */
    unsigned ancount:16;	/* number of answer entries */
    unsigned nscount:16;	/* number of authority entries */
    unsigned arcount:16;	/* number of resource entries */
};
