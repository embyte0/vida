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
*    vida.c:                                   *
*      -initialize vars/curses/windows         *
*      -menu gestion                           *
*      -datapipe menu gestion                  *
*      -connection list menu gestion           *
*      -keys control                           *
*                                              *
*                                              *
\*____________________________________________*/

#include "include.h"

int main()
{
    ushort i = 0;

    char *menu[5] = {
	"Create datapipe", "Dns spoofing", "Help", "Go shell", "Quit"
    };
    char s[128];

    ndp = dpos = cpos = mpos = win = 0;

    // use signal to prevent interrupt with ctrl+c
    signal(SIGINT, finish);

    // initializes curses
    if (!initscr()) {
	fprintf(stderr, "\nError while initializate ncurses!\n\n");
	return -1;
    }

    noecho();
    keypad(stdscr, TRUE);
    start_color();
    init_pair(1, COLOR_GREEN, COLOR_BLACK);

    // create two windows
    wmenu = newwin((int) LINES / 2 - 3, (int) COLS / 3, 1, 0);
    wlist =
	newwin((int) LINES / 2 - 3, (int) COLS * (2 / 3), 1,
	       (int) COLS / 3 - 1);

    // create the scroll window
    wdata = newwin((int) LINES / 2 + 1, COLS, (int) LINES / 2 - 1, 0);
    scrollok(wdata, TRUE);

    // draw the border of win
    wborder(wmenu, 0, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE);
    wborder(wlist, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE, 0);

    // write the title
    attrset(COLOR_PAIR(1));
    mvprintw(0, 1, "Vida %s -[ ", VERSION);
    attrset(A_NORMAL);

    //hide cursor
    leaveok(wmenu, TRUE);
    leaveok(wlist, TRUE);
    leaveok(wdata, TRUE);
    leaveok(stdscr, TRUE);
    refresh();

    //main loop
    while (1) {
	// print menu
	if (win == 0) {
	    if (ndp) {
		if (dp[dpos].check == 'y' || dp[dpos].check == 'Y')
		    sprintf(s, "%d -> %s:%d (%d) P", dp[dpos].localport,
			    dp[dpos].host, dp[dpos].remoteport,
			    dp[dpos].cn);
		else
		    sprintf(s, "%d -> %s:%d (%d)", dp[dpos].localport,
			    dp[dpos].host, dp[dpos].remoteport,
			    dp[dpos].cn);
		mvwaddstr(wlist, dpos + 2, 4, s);
	    }

	    for (i = 0; i < 5; i++) {
		if (i == mpos) {
		    wattrset(wmenu, A_REVERSE);
		    mvwaddstr(wmenu, i + 2, 4, menu[i]);
		    wattrset(wmenu, A_NORMAL);
		}

		else
		    mvwaddstr(wmenu, i + 2, 4, menu[i]);
	    }
	}
	// print datapipes list
	else if (win == 1) {
	    // delete menu selection
	    mvwaddstr(wmenu, mpos + 2, 4, menu[mpos]);
	    for (i = 0; i < ndp; i++) {
		if (dp[i].check == 'y' || dp[i].check == 'Y')
		    sprintf(s, "%d -> %s:%d (%d) P", dp[i].localport,
			    dp[i].host, dp[i].remoteport, dp[i].cn);
		else
		    sprintf(s, "%d -> %s:%d (%d)", dp[i].localport,
			    dp[i].host, dp[i].remoteport, dp[i].cn);
		if (i == dpos) {
		    wattrset(wlist, A_REVERSE);
		    mvwaddstr(wlist, i + 2, 4, s);
		    wattrset(wlist, A_NORMAL);
		}

		else
		    mvwaddstr(wlist, i + 2, 4, s);
	    }
	}
	// print connection list for selected datapipe
	else {
	    if (dp[i].check == 'y' || dp[i].check == 'Y')
		sprintf(s, "%d -> %s:%d (%d) P", dp[i].localport,
			dp[i].host, dp[i].remoteport, dp[i].cn);
	    else
		sprintf(s, "%d -> %s:%d", dp[dpos].localport,
			dp[dpos].host, dp[dpos].remoteport);
	    wattrset(wlist, COLOR_PAIR(1));
	    mvwaddstr(wlist, 2, 4, s);
	    wattrset(wlist, A_NORMAL);

	    // cicla fino al numero di connessioni del datapipe
	    // selezionato
	    for (i = 0; i < dp[dpos].cn; i++) {
		sprintf(s, "%s:%d (%ld/%ld)", dp[dpos].ch[i],
			dp[dpos].cp[i], dp[dpos].rb[i], dp[dpos].tb[i]);
		if (i == cpos) {
		    wattrset(wlist, A_REVERSE);
		    mvwaddstr(wlist, i + 3, 6, s);
		    wattrset(wlist, A_NORMAL);
		}

		else
		    mvwaddstr(wlist, i + 3, 6, s);
	    }
	}
	wrefresh(wmenu);
	wrefresh(wlist);
	// wrefresh (wdata);
	//
	/*
	 * menu
	 */
	if (win == 0) {
	    switch (i = getch()) {
	    case KEY_UP:
		mpos = mpos == 0 ? 4 : mpos - 1;
		break;
	    case KEY_DOWN:
		mpos = mpos == 4 ? 0 : mpos + 1;
		break;

		/*
		 * switch between windows (TAB)
		 */
	    case 9:
		if (ndp)
		    win = 1;
		break;

		/*
		 * keep the function keys
		 */
	    case 'c':
	    case 'd':
	    case 'h':
	    case 'g':
	    case 'q':
		if (i == 'c')
		    mpos = 0;

		else if (i == 'd')
		    mpos = 1;
		else if (i == 'h')
		    mpos = 2;
		else if (i == 'g')
		    mpos = 3;
		else if (i == 'q')
		    mpos = 4;

		/*
		 * no break here...because the show must go on :P
		 */

		/*
		 * enter
		 */
	    case 10:
		if (mpos == 0) {
		    if (ndp == 10) {
			info(1,
			     "Too many datapipe...I can't add this one!");
			continue;
		    }
		    echo();

		    do {
			info(0, "Insert the local port: ");
			getnstr(s, 5);
			info(0, "\n");
		    }
		    while ((dp[ndp].localport = atoi(s)) == 0);

		    do {
			info(0, "Insert the remote port: ");
			getnstr(s, 5);
			info(0, "\n");
		    }
		    while ((dp[ndp].remoteport = atoi(s)) == 0);

		    do {
			info(0, "Insert the remote host: ");
			getnstr(s, 25);
			info(0, "\n");
		    }
		    while (strlen(s) < 2);

		    // procedure for password ask
		    do {
			info(0,
			     "Datapipe need correct password to connect to? (not printed) (y/n) ");
			dp[ndp].check = getch();
			info(0, "\n");
		    }
		    while (dp[ndp].check != 'y'
			   && dp[ndp].check != 'Y'
			   && dp[ndp].check != 'n'
			   && dp[ndp].check != 'N');

		    if (dp[ndp].check == 'y' || dp[ndp].check == 'Y') {
			info(0, "Insert password now : ");
			noecho();
			getnstr(dp[dpos].pwd, 32);
			echo();
			info(0, "\n");
		    }

		    dp[ndp].host = (char *) malloc(strlen(s) + 2);
		    strcpy(dp[ndp].host, s);
		    noecho();

		    /*
		     * make thread
		     */
		    pthread_create(&dp_thread[ndp], NULL,
				   (void *) datapipe, NULL);
		    wborder(wmenu, 0, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE);
		}

		/*
		 * dnshijacking
		 */
		else if (mpos == 1) {
		    bzero(s, sizeof(s));
		    do {
			info(0,
			     "DNS_Spoofer - Insert domain name to spoof: ");
			echo();
			getnstr(s, 30);
			noecho();
			info(0, "\n");
		    }
		    while (strlen(s) < 5);

		    do {
			info(0, "DNS_Spoofer - Insert the *resolved* ip: ");
			echo();
			getnstr(s + 35, 17);
			noecho();
			info(0, "\n");
		    }
		    while (strlen(s + 35) < 7);

		    dnshijack_add(s, s + 35);
		}

		/*
		 * help
		 */

		else if (mpos == 2) {
		    // call vida's man page
		    system("man vida");

		    // repaint all
		    werase(stdscr);
		    refresh();
		    wborder(wmenu, 0, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE);
		    wborder(wlist, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE, 0);
		    attrset(COLOR_PAIR(1));
		    mvprintw(0, 1, "Vida %s -[ ", VERSION);
		    attrset(A_NORMAL);
		    // wrefresh(wmenu);
		    // wrefresh(wlist);
		    // refresh();
		}

		/*
		 * go shell
		 */
		else if (mpos == 3) {
		    clear();
		    refresh();
		    endwin();
		    system("/bin/bash");
		    refresh();
		    wborder(wmenu, 0, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE);
		    wborder(wlist, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE, 0);
		    for (i = 0; i < ndp; i++) {
			if (dp[i].check == 'y' || dp[i].check == 'Y')
			    sprintf(s, "%d -> %s:%d (%d) P",
				    dp[i].localport, dp[i].host,
				    dp[i].remoteport, dp[i].cn);
			else
			    sprintf(s, "%i -> %s:%i (%i)", dp[i].localport,
				    dp[i].host, dp[i].remoteport,
				    dp[i].cn);
			mvwaddstr(wlist, i + 2, 4, s);
		    }
		    attrset(COLOR_PAIR(1));
		    mvprintw(0, 1, "Vida %s -[ ", VERSION);
		    attrset(A_NORMAL);
		}

		/*
		 * quit
		 */
		else
		    finish();
		break;

	    }
	}

	/*
	 * list datapipe
	 */
	else if (win == 1) {
	    switch (getch()) {
	    case KEY_UP:
		dpos = dpos == 0 ? ndp - 1 : dpos - 1;
		break;
	    case KEY_DOWN:
		dpos = dpos == ndp - 1 ? 0 : dpos + 1;
		break;

		// enter
	    case 10:
	    case KEY_RIGHT:

		// if there's any connection..
		if (dp[dpos].cn) {
		    wclear(wlist);
		    wborder(wlist, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE, 0);
		    win = 2;
		}

		else
		    info(1, "No connection for this datapipe");
		break;

		// switch between windows
	    case 9:
		win = 0;
		break;

		// canc
	    case 330:
	    case 'd':

		/*
		 * remove dp[dpos] and all it connections
		 */

		// decremento il numero di datapipe
		ndp--;

		for (i = 0; i < dp[dpos].cn; i++) {
		    shutdown(dp[dpos].from_sd[i], 2);
		    shutdown(dp[dpos].dest_sd[i], 2);
		    close(dp[dpos].from_sd[i]);
		    close(dp[dpos].dest_sd[i]);
		    free(dp[dpos].ch[i]);
		}
		free(dp[dpos].host);
		dp[dpos].stat = 2;
		dp[dpos].from_sd[dp[i].cn]=-1;		
		
		for(i=dpos;i<ndp;i++)  
		    dp[i]=dp[i+1];

		wclear(wlist);
		wborder(wlist, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE, 0);
		wrefresh(wlist);
		dpos = 0;

		// we have deleted the last datapipe
		// the focus goes on menu
		if (ndp == 0)
		    win = 0;
		break;
	    }
	}
	// list connection for selected datapipe
	else {
	    switch (getch()) {

	    case KEY_UP:
		cpos = cpos == 0 ? dp[dpos].cn - 1 : cpos - 1;
		break;

	    case KEY_DOWN:
		cpos = cpos == dp[dpos].cn - 1 ? 0 : cpos + 1;
		break;

		// tab
	    case KEY_LEFT:
	    case 9:
		wclear(wlist);
		wborder(wlist, 0, 0, 0, 0, ACS_HLINE, 0, ACS_HLINE, 0);
		win = 1;
		break;

		// canc
	    case 330:
	    case 'd':

		dp[dpos].status[cpos] = 8;
		// fprintf(stderr, "Setto a 8 dp[%d].status[cpos]\n", dpos, cpos);
		//
		// sent something just to unblock the select
		write(dp[dpos].dest_sd[cpos], "\n",
		      sizeof("\n"));
		// fprintf(stderr, "gli mando la stringa\n");
		//
		// posiziona il cursore della lista delle connessioni sul primo
		// elemento
		//
		while (dp[dpos].stat != 0)
		    usleep(100000);
		cpos = 0;
		break;

		// enter
	    case 10:
		// print data of this connection in wdata
		if (dp[dpos].status[cpos] == 4)
		    dp[dpos].status[cpos] = 0;
		else
		    dp[dpos].status[cpos]++;

		i = dp[dpos].status[cpos];

		statzero();

		switch (i) {

		case 0:
		    mvwaddstr(wdata, (LINES / 2), 0,
			      " -= printing client data =-\n");
		    break;

		case 1:
		    mvwaddstr(wdata, (LINES / 2), 0,
			      " -= printing server data =-\n");
		    break;

		case 2:
		    mvwaddstr(wdata, (LINES / 2), 0,
			      " -= printing all data =-\n");
		    break;

		case 3:
		    mvwaddstr(wdata, (LINES / 2), 0,
			      " -= printing all data in different color =-\n");
		    break;

		case 4:
		    mvwaddstr(wdata, (LINES / 2), 0,
			      " -= printing matches packages =-\n");
		    wrefresh(wdata);
		    info(0, "Type matched string :");
		    echo();
		    getnstr(dp[dpos].mstring, 200);
		    noecho();
		    info(0, "\n");
		    break;
		}
		wrefresh(wdata);

		dp[dpos].status[cpos] = i;

		break;

		// 3- print both data in different color
	    case 'k':
		mvwaddstr(wdata, (LINES / 2), 0,
			  " -= printing all data in different color =-\n");
		wrefresh(wdata);
		statzero();
		dp[dpos].status[cpos] = 3;
		break;

		// 1- print "server" data
	    case 's':
		mvwaddstr(wdata, (LINES / 2), 0,
			  " -= printing server data =-\n");
		wrefresh(wdata);
		statzero();
		dp[dpos].status[cpos] = 1;
		break;

		// 0- print "client" data
	    case 'c':
		mvwaddstr(wdata, (LINES / 2), 0,
			  " -= printing client data =-\n");
		wrefresh(wdata);
		statzero();
		dp[dpos].status[cpos] = 0;
		break;

		// 2- print both data
	    case 'b':
		mvwaddstr(wdata, (LINES / 2), 0,
			  " -= printing all data =-\n");
		wrefresh(wdata);
		statzero();
		dp[dpos].status[cpos] = 3;
		break;

		// 4- print matches packages
	    case 'm':
		mvwaddstr(wdata, (LINES / 2), 0,
			  " -= printing matches packages =-\n");
		wrefresh(wdata);
		statzero();
		dp[dpos].status[cpos] = 4;
		break;

		// j for simple hijacking
	    case 'j':

		sprintf(s, "hijacking: %s:%d-localhost:%d-%s:%d",
			dp[dpos].ch[cpos], dp[dpos].cp[cpos],
			dp[dpos].localport, dp[dpos].host,
			dp[dpos].remoteport);
		info(1, s);
		do {
		    sprintf(s, "Direction: 1) %s, 2) %s",
			    dp[dpos].ch[cpos], dp[dpos].host);
		    info(0, s);
		    i = getch();
		}
		while (i != '1' && i != '2');

		echo();
		info(0, "Write data: ");
		getnstr(s, 128);
		strcat(s, "\n");
		noecho();

		switch (i) {
		case '1':
		    win = write(dp[dpos].from_sd[cpos], s, strlen(s));

		    if (win < 1) {
			sprintf(s, "Simple hijacking error: %s",
				strerror(errno));
			info(1, s);
			break;
		    }

		case '2':
		    win = write(dp[dpos].dest_sd[cpos], s, strlen(s));
		    if (win < 1) {
			sprintf(s, "Simple hijacking error: %s",
				strerror(errno));
			info(1, s);
			break;
		    }
		}
		info(1, "Data send!");
		// restore win value
		win = 2;
		break;

		// l for logging and not-logging
	    case 'l':
		if (!dp[dpos].logfile[cpos]) {

		    info(1, "Logging to file active");
		    dp[dpos].logfile[cpos] = 1;
		} else {
		    info(1, "Logging to file disable");
		    dp[dpos].logfile[cpos] = 0;
		}

		break;

		// end switch
	    }

	    // end else
	}

	// end while
    }

    return 0;
}
