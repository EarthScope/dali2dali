/***************************************************************************
 * dali2dali.c
 *
 * DataLink to DataLink.
 *
 * Connect to a DataLink server and forward data to a DataLink server.
 *
 * Written by Chad Trabant
 *   IRIS Data Management Center
 *
 * modified 2011.004
 ***************************************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#include <libdali.h>
#include <libmseed.h>

#define PACKAGE   "dali2dali"
#define VERSION   "0.1"

static int  parameter_proc (int argcount, char **argvec);
static char *getoptval (int argcount, char **argvec, int argopt);
static void term_handler (int sig);
static void print_timelog (const char *msg);
static void usage (void);

static short int verbose   = 0;  /* Flag to control general verbosity */
static int stateint        = 0;  /* Packet interval to save statefile */
static char *statefile     = 0;	 /* State file for saving/restoring stream states */
static char *matchpattern  = 0;  /* Source ID matching expression */
static char *rejectpattern = 0;  /* Source ID rejecting expression */
static int   writeack      = 0;  /* Flag to control the request for write acks */

static DLCP *srcdlcp;
static DLCP *destdlcp;


int
main (int argc, char **argv)
{
  DLPacket dlpacket;
  char packetdata[MAXPACKETSIZE];
  int packetcnt = 0;

#ifndef WIN32
  /* Signal handling, use POSIX calls with standardized semantics */
  struct sigaction sa;

  sa.sa_flags   = SA_RESTART;
  sigemptyset (&sa.sa_mask);

  sa.sa_handler = term_handler;
  sigaction (SIGINT, &sa, NULL);
  sigaction (SIGQUIT, &sa, NULL);
  sigaction (SIGTERM, &sa, NULL);

  sa.sa_handler = SIG_IGN;
  sigaction (SIGHUP, &sa, NULL);
  sigaction (SIGPIPE, &sa, NULL);
#endif

  /* Process specified parameters */
  if ( parameter_proc (argc, argv) < 0 )
    {
      fprintf (stderr, "Argument processing failed\n");
      fprintf (stderr, "Try '-h' for detailed help\n");
      return -1;
    }

  /* Connect to source DataLink server */
  if ( dl_connect (srcdlcp) < 0 )
    {
      dl_log (2, 0, "Error connecting to source DataLink server: %s\n", srcdlcp->addr);
      return -1;
    }

  /* Connect to destination DataLink server */
  if ( dl_connect (destdlcp) < 0 )
    {
      dl_log (2, 0, "Error connecting to destination DataLink server: %s\n", destdlcp->addr);
      return -1;
    }

  /* Reposition connection */
  if ( srcdlcp->pktid > 0 )
    {
      if ( dl_position (srcdlcp, srcdlcp->pktid, srcdlcp->pkttime) < 0 )
	dl_log (2, 0, "Cannot resume connection to previous state\n");
      else
	dl_log (2, 0, "Connection resumed to packet ID %lld\n", (long long int)srcdlcp->pktid);
    }

  /* Send match pattern if supplied */
  if ( matchpattern )
    {
      if ( dl_match (srcdlcp, matchpattern) < 0 )
        return -1;
    }

  /* Send reject pattern if supplied */
  if ( rejectpattern )
    {
      if ( dl_reject (srcdlcp, rejectpattern) < 0 )
        return -1;
    }

  /* Collect packets in streaming mode */
  while ( dl_collect (srcdlcp, &dlpacket, packetdata, sizeof(packetdata), 0) == DLPACKET )
    {
      if ( verbose > 1 )
	{
	  char timestr[50];

	  dl_dltime2seedtimestr (dlpacket.datastart, timestr, 1);

	  dl_log (1, 0, "Forwarding packet %s, %s, %d bytes\n",
		  dlpacket.streamid, timestr, dlpacket.datasize);
	}

      /* Send packet to the destination DataLink server, reconnecting if needed */
      while ( dl_write (destdlcp, packetdata, dlpacket.datasize, dlpacket.streamid,
			dlpacket.datastart, dlpacket.dataend, writeack) < 0 )
	{
	  if ( verbose )
	    dl_log (2, 0, "Re-connecting to destination DataLink server\n");

	  /* Re-connect to destination DataLink server and sleep if error connecting */
	  if ( destdlcp->link != -1 )
	    dl_disconnect (destdlcp);

	  if ( dl_connect (destdlcp) < 0 )
	    {
	      dl_log (2, 0, "Error re-connecting to destination DataLink server, sleeping 10 seconds\n");
	      sleep (10);
	    }
	}

      /* Save intermediate state files */
      if ( statefile && stateint )
	{
	  if ( ++packetcnt >= stateint )
	    {
	      dl_savestate (srcdlcp, statefile);
	      packetcnt = 0;
	    }

	  packetcnt++;
	}
    }

  /* Shut down the connection to source DataLink server */
  if ( srcdlcp->link != -1 )
    dl_disconnect (srcdlcp);

  /* Shut down the connection to destination DataLink server */
  if ( destdlcp->link != -1 )
    dl_disconnect (destdlcp);

  /* Save state file for source connection */
  if ( statefile )
    dl_savestate (srcdlcp, statefile);

  return 0;
}  /* End of main() */


/***************************************************************************
 * parameter_proc:
 *
 * Process the command line parameters.
 *
 * Returns 0 on success, and -1 on failure.
 ***************************************************************************/
static int
parameter_proc (int argcount, char **argvec)
{
  char *srcaddress = 0;
  char *destaddress = 0;
  char *tptr;
  int error = 0;

  if (argcount <= 1)
    error++;

  /* Process all command line arguments */
  for (optind = 1; optind < argcount; optind++)
    {
      if (strcmp (argvec[optind], "-V") == 0)
        {
          fprintf (stderr, "%s version: %s\n", PACKAGE, VERSION);
          exit (0);
        }
      else if (strcmp (argvec[optind], "-h") == 0)
        {
          usage ();
          exit (0);
        }
      else if (strncmp (argvec[optind], "-v", 2) == 0)
	{
	  verbose += strspn (&argvec[optind][1], "v");
	}
      else if (strcmp (argvec[optind], "-x") == 0)
	{
	  statefile = getoptval(argcount, argvec, optind++);
	}
      else if (strcmp (argvec[optind], "-m") == 0)
	{
	  matchpattern = getoptval(argcount, argvec, optind++);
	}
      else if (strcmp (argvec[optind], "-r") == 0)
	{
	  rejectpattern = getoptval(argcount, argvec, optind++);
	}
      else if (strncmp (argvec[optind], "-", 1) == 0)
	{
	  fprintf (stderr, "Unknown option: %s\n", argvec[optind]);
	  exit (1);
	}
      else if ( ! srcaddress )
        {
          srcaddress = argvec[optind];
        }
      else if ( ! destaddress )
        {
          destaddress = argvec[optind];
        }
      else
	{
	  fprintf (stderr, "Unknown option: %s\n", argvec[optind]);
	  exit (1);
	}
    }

  /* Make sure a source DataLink server was specified */
  if ( ! srcaddress )
    {
      fprintf (stderr, "No source DataLink server specified\n\n");
      fprintf (stderr, "%s version %s\n\n", PACKAGE, VERSION);
      fprintf (stderr, "Usage: %s [options] slhost dlhost\n\n", PACKAGE);
      fprintf (stderr, "Try '-h' for detailed help\n");
      exit (1);
    }

  /* Make sure a destination DataLink server was specified */
  if ( ! destaddress )
    {
      fprintf (stderr, "No destination DataLink server specified\n\n");
      fprintf (stderr, "%s version %s\n\n", PACKAGE, VERSION);
      fprintf (stderr, "Usage: %s [options] slhost dlhost\n\n", PACKAGE);
      fprintf (stderr, "Try '-h' for detailed help\n");
      exit (1);
    }

  /* Initialize the verbosity for the dl_log function */
  dl_loginit (verbose, &print_timelog, "", &print_timelog, "");

  /* Set stdout (where logs go) to always flush after a newline */
  setvbuf(stdout, NULL, _IOLBF, 0);

  /* Allocate and initialize DataLink connection description */
  if ( ! (srcdlcp = dl_newdlcp (srcaddress, argvec[0])) )
    {
      fprintf (stderr, "Cannot allocation source DataLink descriptor\n");
      exit (1);
    }

  /* Allocate and initialize destination DataLink connection description */
  if ( ! (destdlcp = dl_newdlcp (destaddress, argvec[0])) )
    {
      fprintf (stderr, "Cannot allocation destination DataLink descriptor\n");
      exit (1);
    }

  /* Load the match stream list from a file if the argument starts with '@' */
  if ( matchpattern && *matchpattern == '@' )
    {
      char *filename = matchpattern + 1;

      if ( ! (matchpattern = dl_read_streamlist (srcdlcp, filename)) )
        {
          dl_log (2, 0, "Cannot read matching list file: %s\n", filename);
          exit (1);
        }
    }

  /* Load the reject stream list from a file if the argument starts with '@' */
  if ( rejectpattern && *rejectpattern == '@' )
    {
      char *filename = rejectpattern + 1;

      if ( ! (rejectpattern = dl_read_streamlist (srcdlcp, filename)) )
        {
          dl_log (2, 0, "Cannot read rejecting list file: %s\n", filename);
          exit (1);
        }
    }

  /* Report the program version */
  dl_log (1, 0, "%s version: %s\n", PACKAGE, VERSION);

  /* If errors then report the usage message and quit */
  if ( error )
    {
      usage ();
      exit (1);
    }

  /* Attempt to recover sequence numbers from state file */
  if ( statefile )
    {
      /* Check if interval was specified for state saving */
      if ( (tptr = strchr (statefile, ':')) != NULL )
	{
	  char *tail;

	  *tptr++ = '\0';

	  stateint = (unsigned int) strtoul (tptr, &tail, 0);

	  if ( *tail || (stateint < 0 || stateint > 1e9) )
	    {
	      dl_log (2, 0, "state saving interval specified incorrectly\n");
	      return -1;
	    }
	}

      if ( dl_recoverstate (srcdlcp, statefile) < 0 )
	{
	  dl_log (2, 0, "state recovery failed\n");
	}
    }

  return 0;
}  /* End of parameter_proc() */


/***************************************************************************
 * getoptval:
 *
 * Return the value to a command line option; checking that the value is
 * itself not an option (starting with '-') and is not past the end of
 * the argument list.
 *
 * argcount: total arguments in argvec
 * argvec: argument list
 * argopt: index of option to process, value is expected to be at argopt+1
 *
 * Returns value on success and exits with error message on failure
 ***************************************************************************/
static char *
getoptval (int argcount, char **argvec, int argopt)
{
  if ( argvec == NULL || argvec[argopt] == NULL ) {
    fprintf (stderr, "getoptval(): NULL option requested\n");
    exit (1);
  }

  if ( (argopt+1) < argcount && *argvec[argopt+1] != '-' )
    return argvec[argopt+1];

  fprintf (stderr, "Option %s requires a value\n", argvec[argopt]);
  exit (1);
}  /* End of getoptval() */


/***************************************************************************
 * term_handler:
 * Signal handler routine to control termination.
 ***************************************************************************/
static void
term_handler (int sig)
{
  dl_terminate (srcdlcp);
}


/***************************************************************************
 * print_timelog:
 *
 * Log message print handler used with dl_loginit().  Prefixes a local
 * time string to the message before printing.
 ***************************************************************************/
static void
print_timelog (const char *msg)
{
  char timestr[100];
  time_t loc_time;

  /* Build local time string and cut off the newline */
  time(&loc_time);
  strcpy(timestr, asctime(localtime(&loc_time)));
  timestr[strlen(timestr) - 1] = '\0';

  fprintf (stdout, "%s - %s", timestr, msg);
}


/***************************************************************************
 * usage:
 * Print the usage message and exit.
 ***************************************************************************/
static void
usage (void)
{
  fprintf (stderr, "%s version %s\n\n", PACKAGE, VERSION);
  fprintf (stderr, "Copy selected data from one DataLink server to another\n\n");
  fprintf (stderr, "Usage: %s [options] srchost desthost\n\n", PACKAGE);
  fprintf (stderr,
	   " ## General options ##\n"
	   " -V              Report program version\n"
	   " -h              Print this usage message\n"
	   " -v              Be more verbose, multiple flags can be used\n"
	   " -x sfile[:int]  Save/restore stream state information to this file\n"
	   "\n"
	   " ## Data stream selection ##\n"
	   " -m match        Specify stream ID matching pattern\n"
	   " -r reject       Specify stream ID rejecting pattern\n"
	   "                   Default is all data streams\n"
	   "\n"
	   " srchost   Address of the source DataLink server in host:port format\n\n"
	   " desthost  Address of the destination DataLink server in host:port format\n\n"
	   "             Default host is 'localhost' and default port is '16000'\n\n");

}  /* End of usage() */
