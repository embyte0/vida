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
*    funz.c:                                   *
*      -some functions                         *
*                                              *
\*____________________________________________*/

#include "include.h"

// print in the left corner the argument
// if sl=1 print, sleep, clean
void info(char sl, const char *s, ...)
{
   char complete[128];
   u_short space = strlen (VERSION)+10;
   
   va_list ap;
   va_start(ap, s);
   vsprintf(complete, s, ap);

   mvaddstr(0, space, "\n");
   mvaddstr(0, space, complete);
   refresh();
   va_end(ap);
   if (sl)
     {
	sleep(1);
	mvaddstr(0, space, "\n");
     }
}

// create a new datapipe
// stay always in accept and pass at handler() the new connection
// we can't stop this with pthread_cancel :( (FIX IT!!)
void datapipe()
{
   char s[128];
   struct hostent *host_addr;
   int i = ndp;
   struct sockaddr_in sin1;

   // struct sockaddr_in sin2;
   int sd;
   int addlen = sizeof(struct sockaddr_in);

   // no connection still
   dp[i].cn = 0;

   // print this message
   sprintf(s, "Creating a datapipe throught port %i and %s:%i",
	   dp[i].localport, dp[i].host, dp[i].remoteport);
   info(0, s);

   // try to resolve host
   if (!(host_addr = gethostbyname(dp[i].host)))
     {
	sprintf(s, "Cannot resolve %s", dp[i].host);
	info(1, s);
	return;
     }
   // create the listen socket
   if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
     {
	sprintf(s, "Error in socket(): %s", strerror(errno));
	info(1, s);
	return;
     }

   memset(&sin1, 0, sizeof(sin1));
   sin1.sin_port = htons(dp[i].localport);
   sin1.sin_family = AF_INET;

   // bind and put in listen mode the socket
   if (bind(sd, (struct sockaddr *) &sin1, sizeof(sin1)))
     {
	sprintf(s, "Error in bind(): %s", strerror(errno));
	info(1, s);
	return;
     }
   listen(sd, 20);

   // riempio la struttura di uscita con i dati del vettore datapipe
   dp[i].sout.sin_port = htons(dp[i].remoteport);
   dp[i].sout.sin_family = AF_INET;
   memset(&(dp[i].sout.sin_zero), '\0', (size_t) 8);
   memcpy(&(dp[i].sout.sin_addr), host_addr->h_addr,
	  (size_t) sizeof(host_addr->h_length));

   // put the new datapipe in datapipe list
   mvwprintw(wlist, i + 2, 4, "%d -> %s:%d (0) %c", dp[i].localport,
	     dp[i].host, dp[i].remoteport, CHECKPWD(i) ? 'P' : ' ');
   wrefresh(wlist);

   info(1, "Datapipe activate");

   // increase datapipe number
   ndp++;
   dp[i].stat = 1;

   memset(&sin1, 0, sizeof(sin1));

   // fprintf(stderr,"Metto in accept dp[%d].sd ke vale %d\n",i,dp[i].sd);
   // accept cicle (the thread block here)
   while ((dp[i].from_sd[dp[i].cn] =
	   accept(sd, (struct sockaddr *) &sin1, &addlen)) != -1)
     {
	// fprintf(stderr,"Dentro accept, dp[%d].sd vale %d\n",i,dp[i].sd);
	//control that this datapipe wasn't deleted
	if (dp[i].stat == 2)
	  {
	     shutdown(dp[i].from_sd[dp[i].cn], 2);
	     pthread_exit(NULL);
	  }
	info(0, "Connection from %s",
	     inet_ntoa((struct in_addr) sin1.sin_addr));

	// add this connection to datapipe
	strcpy(s, (char *) inet_ntoa((struct in_addr) sin1.sin_addr));
	dp[i].ch[dp[i].cn] = (char *) malloc(strlen(s) + 1);
	strcpy(dp[i].ch[dp[i].cn], s);
	dp[i].cp[dp[i].cn] = ntohs(sin1.sin_port);

	// connection list window is open
	if (win == 2)
	  {
	     mvwprintw(wlist, dp[i].cn + 3, 6, "%s:%d (0/0)",
		       dp[i].ch[dp[i].cn], dp[i].cp[dp[i].cn]);
	     wrefresh(wlist);
	  }
	// datapipe list window is open
	else
	  {
	     if (dpos == i && win == 1)
	       {
		  wattrset(wlist, A_REVERSE);
		  mvwprintw(wlist, i + 2, 4, "%d -> %s:%d (%d) %c",
			    dp[i].localport, dp[i].host, dp[i].remoteport,
			    dp[i].cn + 1, CHECKPWD(i) ? 'P' : ' ');
		  wattrset(wlist, A_NORMAL);
	       }
	     else
	       mvwprintw(wlist, i + 2, 4, s);

	     wrefresh(wlist);
	  }

	// increase the number of connection for this datapipe
	dp[i].cn++;

	// pass the new connection to handler()
	pthread_create(&h, NULL, (void *) handler, &i);
     }

   // fprintf(stderr,"Fuori accept, dp[%d].sd vale %d\n",i,dp[i].sd);
   info(1, "Datapipe closed!!");

   return;
}

void
  // gli passo il numero di datapipe
  handler(int *io)
{
   // buffer
   char recvline[1028];

   // password receive form client
   char password[32];

   // number of received byte
   int n;

   // required for log
   ushort l = 0;

   int inf;

   // to control the two fd
   fd_set rfds;
   fd_set efds;

   // il valore n della funct select()
   int max_sd = 0;
   char s[128];

   // numero datapipe
   int i = *io;

   // numero connessione
   int c = dp[i].cn - 1;

   // azzera il numero di byte trasmessi/ricevuti
   dp[i].tb[c] = dp[i].rb[c] = 0;

   // azzera lo sniffing mode
   dp[i].status[c] = -1;

   // no log at now
   dp[i].logfile[c] = 0;

   // if psw is needed check for it (we must check only here and not in
   // datapipe(), if you want how emailme)
   if (CHECKPWD(i))
     {
	sprintf(s, "A password is required : ");
	write(dp[i].from_sd[c], s, strlen(s));
	n = read(dp[i].from_sd[c], password, 32);
	password[n - 2] = '\0';
	if (strcmp(password, dp[i].pwd))
	  {
	     // non sono uguali -> jump to the end (correct goto :^)
	     sprintf (s, "Wrong password.\n");
	     write(dp[i].from_sd[c], s, strlen(s));	     
	     shutdown(dp[i].from_sd[c], 2);
	     goto closing;
	  }
	else
	  {
	     sprintf (s, "OK waiting the connection.. :-)\n");
	     write(dp[i].from_sd[c], s, strlen(s));
	  }

     }
   // creo il socket descriptor di destinazione
   if ((dp[i].dest_sd[c] = socket(AF_INET, SOCK_STREAM, 0)) == -1)
     {
	shutdown(dp[i].from_sd[c], 2);
	info(1, "Error in socket(): %s", strerror(errno));
	return;
     }
   // provo a connettermi
   inf =
     connect(dp[i].dest_sd[c], (struct sockaddr *) &dp[i].sout,
	     (socklen_t) sizeof(dp[i].sout));
   if (inf == -1)
     {
	shutdown(dp[i].from_sd[c], 2);
	info(1, "Error in connect(): %s", strerror(errno));
	return;
     }
   info(1, "Connection estabilished");

   // imposto il max_sd per la select()
   if (dp[i].from_sd[c] > max_sd)
     max_sd = dp[i].from_sd[c];
   if (dp[i].dest_sd[c] > max_sd)
     max_sd = dp[i].dest_sd[c];
   max_sd++;

   // ciclo di trasmissione dati
   while (1)
     {

	// azzero i fd_set
	FD_ZERO(&rfds);
	FD_ZERO(&efds);

	// lego i due fd ai due capi delle connessioni
	FD_SET(dp[i].from_sd[c], &rfds);
	FD_SET(dp[i].dest_sd[c], &rfds);
	FD_SET(dp[i].from_sd[c], &efds);
	FD_SET(dp[i].dest_sd[c], &efds);

	// è necessario per il logging se no devi fputtare solo n byte
	bzero(&recvline, MAXLINE);

	// sta chiudendo...ho il tempo di chiudere le connessioni
	if (dp[i].status[c] == 8)
	  break;

	// controllo dei socket in polling
	if (select(max_sd, &rfds, NULL, &efds, NULL) == -1)
	  {
	     info(0, "Error in select(): %s", strerror(errno));
	     break;
	  }

	if (dp[i].status[c] == 8)
	  break;

	if (FD_ISSET(dp[i].dest_sd[c], &rfds)
	    || FD_ISSET(dp[i].dest_sd[c], &efds))
	  {

	     // dati in arrivo dal "server"
	     n = read(dp[i].dest_sd[c], recvline, MAXLINE);

	     // incremento il valore dei byte ricevuti
	     dp[i].tb[c] += n;

	     // se la finestra ke lista le connessioni e' aperta
	     // aggiorno i tb e rb
	     if (win == 2)
	       {
		  if (cpos == c)
		    {
		       wattrset(wlist, A_REVERSE);
		       mvwprintw(wlist, c + 3, 6, "%s:%d (%d/%d)",
				 dp[i].ch[c], dp[i].cp[c], dp[i].rb[c],
				 dp[i].tb[c]);
		       wattrset(wlist, A_NORMAL);
		    }
		  else
		    mvwprintw(wlist, c + 3, 6, "%s:%d (%d/%d)",
			      dp[i].ch[c], dp[i].cp[c], dp[i].rb[c],
			      dp[i].tb[c]);;
		  wrefresh(wlist);
	       }
	     // scrivo sull'altro fd
	     inf = write(dp[i].from_sd[c], recvline, n);
	     if (inf < 1)
	       {
		  info(0, "Error in write(): %s", strerror(errno));
		  break;
	       }
	     // se e' 1,2 o 3 (qui devo printarli in ogni caso
	     if (dp[i].status[c] < 4 && dp[i].status[c] > 0)
	       {
		  recvline[n] = '\0';
		  normal(recvline, strlen(recvline));
		  mvwprintw(wdata, LINES / 2, 0, recvline);
		  wrefresh(wdata);
	       }
	     else if (dp[i].status[c] == 4)
	       if (strstr(recvline, dp[i].mstring) != NULL)
		 {
		    recvline[n] = '\0';
		    normal(recvline, strlen(recvline));
		    mvwprintw(wdata, LINES / 2, 0, recvline);
		    wrefresh(wdata);
		 }
	  }

	else if (FD_ISSET(dp[i].from_sd[c], &rfds)
		 || FD_ISSET(dp[i].from_sd[c], &efds))
	  {

	     // dati in arrivo dal "client"
	     n = read(dp[i].from_sd[c], recvline, MAXLINE);

	     // incremento il valore dei byte ricevuti
	     dp[i].rb[c] += n;

	     // se la finestra ke lista le connessioni e' aperta
	     // aggiorno i tb e rb
	     if (win == 2)
	       {
		  if (cpos == c)
		    {
		       wattrset(wlist, A_REVERSE);
		       mvwprintw(wlist, c + 3, 6, "%s:%d (%d/%d)",
				 dp[i].ch[c], dp[i].cp[c], dp[i].rb[c],
				 dp[i].tb[c]);
		       wattrset(wlist, A_NORMAL);
		    }
		  else
		    mvwprintw(wlist, c + 3, 6, "%s:%d (%d/%d)",
			      dp[i].ch[c], dp[i].cp[c], dp[i].rb[c],
			      dp[i].tb[c]);
		  wrefresh(wlist);
	       }
	     // scrivo sull'altro fd
	     inf = write(dp[i].dest_sd[c], recvline, n);
	     if (inf < 1)
	       {
		  info(0, "Error in write(): %s", strerror(errno));
		  break;
	       }

	     if (dp[i].status[c] == 0 || dp[i].status[c] == 2)
	       {
		  recvline[n] = '\0';
		  normal(recvline, strlen(recvline));
		  mvwprintw(wdata, (LINES / 2), 0, recvline);
		  wrefresh(wdata);
	       }

	     else if (dp[i].status[c] == 3)
	       {
		  recvline[n] = '\0';
		  normal(recvline, strlen(recvline));
		  wattrset(wdata, COLOR_PAIR(1));
		  mvwprintw(wdata, (LINES / 2), 0, recvline);
		  wattrset(wdata, A_NORMAL);
		  wrefresh(wdata);
	       }
	     else if (dp[i].status[c] == 4)
	       if (strstr(recvline, dp[i].mstring) != NULL)
		 {
		    recvline[n] = '\0';
		    normal(recvline, strlen(recvline));
		    mvwprintw(wdata, LINES / 2, 0, recvline);
		    wrefresh(wdata);
		 }
	  }

	if (dp[i].logfile[c] && l == 0)
	  {
	     // apro il file descriptor associato alla connessione alla prima
	     // richiesta di logging
	     sprintf(s, "%s-localhost:%d-%s:%d", dp[i].ch[c],
		     dp[i].localport, dp[i].host, dp[i].remoteport);
	     dp[i].fout = fopen(s, "a");
	     fprintf(dp[i].fout, "\n------ Logging from ------\n");
	     fprintf(dp[i].fout, "client requesting: %s:%d\n", dp[i].ch[c],
		     dp[i].cp[i]);
	     fprintf(dp[i].fout, "datapipe on: 127.0.0.1:%d\n", dp[i].localport);
	     fprintf(dp[i].fout, "redirecting to: %s:%d\n", dp[i].host,
		     dp[i].remoteport);
	     fprintf(dp[i].fout, "---------- DATA ----------\n");

	     // incremento il parametro l di check
	     l++;
	  }
	// se la connessione è da loggare allora lo faccio
	if (dp[i].logfile[c])
	  fputs(recvline, dp[i].fout);

     }

   // fprintf(stderr, "Closing connection %d of datapipe %d\n", c, i);
   //
   // connessione interrotta
   shutdown(dp[i].from_sd[c], 2);
   shutdown(dp[i].dest_sd[c], 2);
   close(dp[i].from_sd[c]);
   close(dp[i].dest_sd[c]);

   // chiudo lo stream
   if (dp[i].fout != NULL)
     {

	fprintf(dp[i].fout, "byte trasmitted : %ld IN, %ld OUT\n", dp[i].rb[c],
		dp[i].tb[c]);
	fclose(dp[i].fout);
     }
   // i datapipe corrente
   // c connessione corrente
   //
   // deve cancellare la connessione corrente
   // e far shiftare tutto l'array delle connessioni indietro
   //
    /*
     * fprintf(stderr, "Comincio il ciclo del male\n"); for (n = 0, inf = 0; n
     * < dp[i].cn; n++) {
     *
     * // siamo dopo l'elemento eliminato if (inf) { fprintf(stderr, "ok,
     * dentro il ciclo del male, n=%d c=%d\n", n, c);
     *
     * dp[i].status[n - 1] = dp[i].status[n]; dp[i].cp[n - 1] = dp[dpos].cp[n];
     *
     * fprintf(stderr, "Prima della free\n"); if (dp[i].ch[n - 1] != NULL) {
     * fprintf(stderr, "Ok... libero davvero\n"); free(dp[i].ch[n - 1]); }
     *
     * fprintf(stderr, "la free nn e' il male\n");
     *
     * dp[i].ch[n - 1] = (char *) malloc(strlen(dp[i].ch[n]) + 1);
     * fprintf(stderr, "la malloc nn e' il male, forse strcpy\n");
     *
     * strcpy(dp[i].ch[n - 1], dp[i].ch[n]); fprintf(stderr, "strcpy nn e' il
     * male\n");
     *
     * dp[i].rb[n - 1] = dp[i].rb[n]; dp[i].tb[n - 1] = dp[i].tb[n];
     * dp[i].from_sd[n - 1] = dp[i].from_sd[n]; dp[i].dest_sd[n - 1] =
     * dp[i].dest_sd[n]; fprintf(stderr, "bene\n"); }
     *
     * if (n == c) { fprintf(stderr, "Ok....sono all'elemento eliminato %d\n",
     * c); free(dp[i].ch[n]); inf = 1; } }
     *
     * fprintf(stderr, "Fatto...ho finito il ciclo del male\n");
     *
     * // decrementa il numero di connessioni x quel datapipe
     */

   closing:

   dp[i].cn--;
   wclear(wlist);
   wborder(wlist, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE, 0);

   // e' aperta la finestra ke lista le connessioni
   if (win == 2)
     {

	// se era l'unica connessione
	// apre la finestra ke lista i datapipe
	if (!dp[i].cn)
	  {
	     win = 1;
	     for (n = 0; n < ndp; n++)
	       {
		  if (n == dpos)
		    {
		       wattrset(wlist, A_REVERSE);
		       mvwprintw(wlist, n + 2, 4, "%d -> %s:%d (%d) %c",
				 dp[n].localport, dp[n].host,
				 dp[n].remoteport, dp[n].cn,
				 CHECKPWD(n) ? 'P' : ' ');
		       wattrset(wlist, A_NORMAL);
		    }

		  else
		    mvwaddstr(wlist, n + 2, 4, s);
		  wrefresh(wlist);
	       }
	  }
	// nn e' l'unica connessione
	// riprinta tuta la lista delle connessioni
	else
	  {
	     for (n = 0; n < dp[i].cn; n++)
	       {
		  if (n == 0)
		    {
		       wattrset(wlist, A_REVERSE);
		       mvwprintw(wlist, n + 3, 6, "%s:%d (%d/%d) %c",
				 dp[n].ch[c], dp[n].cp[c], dp[n].rb[c],
				 dp[n].tb[c], CHECKPWD(n) ? 'P' : ' ');
		       wattrset(wlist, A_NORMAL);
		    }

		  else
		    mvwaddstr(wlist, n + 3, 6, s);
	       }
	  }
     }
   // e' aperta la finestra ke lista i datapipe
   else
     {
	for (n = 0; n < ndp; n++)
	  {
	     sprintf(s, "%d -> %s:%d (%d)", dp[n].localport, dp[n].host,
		     dp[n].remoteport, dp[n].cn);
	     if (n == dpos)
	       {
		  wattrset(wlist, A_REVERSE);
		  mvwprintw(wlist, n + 2, 4, "%d -> %s:%d (%d) %c",
			    dp[n].localport, dp[n].host, dp[n].remoteport,
			    dp[n].cn, CHECKPWD(n) ? 'P' : ' ');
		  wattrset(wlist, A_NORMAL);
	       }

	     else
	       mvwprintw(wlist, n + 2, 4, s);
	  }
     }

   // fprintf(stderr, "qua\n");
   wrefresh(wlist);
   dp[i].stat = 0;
   info(1, "Connection closed");
   return;
}

void finish()
{
   int i, a;

   info(0, "Closing program... ");

   // libero la memoria allocata
   for (i = 0; i < ndp; i++)
     {
	free(dp[i].host);
	for (a = 0; a < dp[i].cn; a++)
	  free(dp[i].ch[a]);
	if (dp[i].fout!=NULL)
	  fclose(dp[i].fout);
     }

   // cancello le finestre create prima
   delwin(wmenu);
   delwin(wlist);
   delwin(wdata);

   // pulisco lo schermo
   clear();
   refresh();

   // chiudo le ncurses
   endwin();
   exit(0);
}

void normal(char *s, int len)
{
   char str[len + 1];
   int i = 0, i2 = 0;

   memset(&str, 0, len + 1);

   for (; i < len; i++)
     if (s[i] != '\r')
       str[i2++] = s[i];

   strcpy(s, str);
}

void statzero()
{
   int i, y;

   for (i = 0; i < ndp; i++)
     for (y = 0; y < dp[i].cn; y++)
       dp[i].status[y] = -1;

}
