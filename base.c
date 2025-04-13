#include "vwg.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <unistd.h>
//#include <internet.h>

#define starts(s1,s2)	(strncmp(s1,s2,strlen(s2)) == 0)

global FILE *pinfile, *poutfile;
global struct ruleset *hypersyntax;
global struct dictnode *metalist, *ghead;
global char *protolist[32];
global struct hyperrule *toplevelhr;
global struct hypernotion *emptypn;
global uint debugbits;
global int linenum;
global bool anyerrors;
int fprintf(FILE *restrict f, const char *restrict fmt, ...);

static int fdnum;
static bool sopt, fdopt, xopt;
//static struct tcpuser tuser;
static char *fname;

extern void initheap();		/* from heap	  */
extern void readgrammar(const char *fn);		/* from readgr	  */
extern void checkmetasyntax();	/* from checkmeta */
extern void factorize();		/* from factorize */
extern void checkgrammar();		/* from checkgr	  */
extern void parse();			/* from parse	  */
void writemetagr(void);
void writegrammar(void);

static void readcmdline(int argc, char *argv[]);
static void usage();
static void setpinpout(void );
static void giveup(const char *msg, ...);
global int main( int argc, char *argv[])
  { readcmdline(argc, argv);
    setpinpout();
    initheap(); initclock();
    anyerrors = false;
    readgrammar(fname);
    if (anyerrors) return(2);
    linenum = -1;
    checkmetasyntax();
    if (anyerrors) return(2);
    factorize();
    if(xopt)  writemetagr(); 
    checkgrammar();
    if(xopt) { writegrammar(); return 0; }
    if (anyerrors) return(2);
    fprintf(poutfile, "OK\n"); fflush(poutfile);
    parse();
    return(0);
  }

static void readcmdline( int argc, char *argv[])
  { int ak = 0;
    debugbits = 0; sopt = fdopt = false;
    if (ak < argc) ak++; /* skip program name */
    while (ak < argc && argv[ak][0] == '-')
      { if (starts(argv[ak], "-db"))
	  { int ni = sscanf(&argv[ak][3], "%x", &debugbits);
	    unless (ni == 1) usage();
	    ak++;
	  }
/*	else if (starts(argv[ak], "-s"))
	  { int ni = sscanf(&argv[ak][2], "%x:%hd", &tuser.faddr, &tuser.fport);
	    unless (ni == 2) usage();
	    sopt = true;
	    ak++;
	  }
*/	else if (starts(argv[ak], "-fd"))
	  { int ni = sscanf(&argv[ak][3], "%d", &fdnum);
	    unless (ni == 1) usage();
	    fdopt = true;
	    ak++;
	  }
	else if (starts(argv[ak], "-x"))
	  { 
	    xopt = true;
	    ak++;
	  }
	else usage();
      }
    if (sopt && fdopt) giveup("can't specify both -s and -fd");
    if (ak < argc) fname = argv[ak++]; else usage();
    if (ak < argc) usage();
  }

static void usage(void )
  { fprintf(stderr, "Usage: vwg [-db<hexbits>] [-s<host:port>] [-fd<num>] fn\n");
    exit(1);
  }

static void setpinpout(void )
  {/* if (sopt)
      { int code;
	fdnum = tcp_sock();
	if (fdnum < 0) giveup("tcp_sock failed");
	tuser.lport = tuser.param = 0;
	code = tcp_connect(fdnum, &tuser);
	if (code < 0) giveup("tcp_connect failed");
      }
*/    if (sopt || fdopt)
      { pinfile = fdopen(fdnum, "r"); poutfile = fdopen(fdnum, "w");
	if (pinfile == NULL || poutfile == NULL) giveup("fdopen failed");
      }
    else
      { pinfile = stdin;
	poutfile = stdout;
      }
  }

static void giveup(const char *msg,...)
  { fprintf(stderr, "vwg: %s\n", msg);
    exit(1);
  }

