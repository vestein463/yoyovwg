#include "vwg.h"
#include <stdio.h>
#include <stdarg.h>
#include <assert.h>

#define time_t long
#include <sys/times.h>

extern bool anyerrors;
extern int linenum;
extern struct dictnode *metalist;

static struct dictnode *dictionary;
static int starttime;

forward struct dictnode *lookupword();
static int timenow(void );

global bool ispn( struct hypernotion *hn)
  { struct hitem *hd = hn -> hdef;
    int n = hn -> hlen; int i = 0;
    if (n < 0) printf(" BUG6!");
    while (i < n && hd[i].sy == s_ssm) i++;
    return (i >= n);
  }

global void initdictionary()
  { dictionary = NULL;
  }

global struct dictnode *newmetanotion( char *s)
  { struct dictnode *z = lookupword(s, true);
    unless (z -> flg & f_meta)
      { /* it's a new metanotion */
	z -> flg |= f_meta;
	z -> rhs = NULL;
	z -> nxt = metalist;
	metalist = z;
      }
    return z;
  }

global struct dictnode *newterminal( char *s)
  { struct dictnode *z = lookupword(s, true);
    z -> flg |= f_term;
    return z;
  }

global struct dictnode *lookupword( char *s, bool ins)
  { struct dictnode **m = &dictionary;
    bool found = false;
    until (found || *m == NULL)
      { int k = strcmp(s, (*m) -> str);
	if (k < 0) m = &((*m) -> lft);
	if (k > 0) m = &((*m) -> rgt);
	if (k == 0) found = true;
      }
    if (!found && ins)
      { struct dictnode *t = heap(1, struct dictnode);
	t -> lft = t -> rgt = NULL;
	t -> flg = 0;
	t -> str = heap(strlen(s)+1, char);
	strcpy(t -> str, s);
	*m = t;
      }
    return *m;
  }

global void error( const char *s, ...)
  { fprintf(stderr, "*** ");
    if (linenum >= 0) fprintf(stderr, "line %d: ", linenum);
    va_list ap;
    va_start(ap, s);
    vfprintf(stderr, s, ap);
    va_end(ap);
    putc('\n', stderr);
    anyerrors = true;
  }

global void initclock(void )
  { starttime = timenow();
  }

global void wrtime(void )
  { static int ktab[] = { 10, 10,  6, 10,  6, 10, 10 };
    static int ptab[] = { 11,  9,  8,  5,  4,  1,  0 };
    int now, dt, i; char string[14];
    now = timenow();	      /* time now */
    dt = (now-starttime+3)/6; /* time since beginning of run (1/10s) */
    strcpy(string, "nnh nnm nn.ns");
    for (i=0; i<7; i++)
      { int k = ktab[i], p = ptab[i];
	string[p] = dt%k + '0';
	dt /= k;
      }
    printf(string);
  }

static int timenow(void )
  { struct tms tms;
    times(&tms);
    return (tms.tms_utime + tms.tms_stime); /* user + system */
  }

#define MAXINT	   0x7fffffff
#define HASHFACTOR 65599	/* should be a prime number */

global int hashval3(void *w1,void *w2,void *w3)
  {
    uint sum = (((uint)w1 *HASHFACTOR) + (uint)w2) *HASHFACTOR + (uint)w3;
    return (sum & MAXINT) % HASHSIZE;
  }

global int hashval2(void *w1,void *w2)
  {
    uint sum = ((uint)w1 *HASHFACTOR) + (uint)w2;
    return (sum & MAXINT) % HASHSIZE;
  }

global int hashval( int n, word w1, w2, w3)
  { word *p = &w1;
    uint sum = 0; int k;
    for (k=0; k<n; k++) sum = (sum * HASHFACTOR) + p[k];
    return (sum & MAXINT) % HASHSIZE;
  }

global void dots( int n, FILE *fi)
  { int i;
    for (i=0; i<n; i++) putc('.', fi);
  }

