/* fprintf which knows about "%h" to print hypernotions */

#include "vwg.h"
#include "states.h"
#include <stdio.h>
#include <stdarg.h>

extern char *protolist[32];
static void spaces( int d, FILE *f);
static void fwritea( struct hyperalt *alt, int d, FILE *f);
static void fwriteb( bool b, int d, FILE *f);
static void fwritec( int c, int d, FILE *f);
static void fwrited( int n, int d, FILE *f);
static void fwritee( struct state *st, int d, FILE *f);
static void fwriteh( struct hypernotion *hn, int d, FILE *f);
static void fwritem( struct metarhs *y, int d, FILE *f);
static void fwrites( char *s, int d, FILE *f);
static void fwritet( uint64 t, int d, FILE *f);
static void fwritex( uint n, int d, FILE *f);

static void writeneg( bool s, int n, int d, FILE *f);
static void wrhn( struct hypernotion *hn, FILE *f);

/*
global void printf( char *s, word p1, p2, p3, p4)
  { fprintf(stdout, s, p1, p2, p3, p4);
  }
*/
int fprintf(FILE *restrict f, const char *restrict fmt, ...)
{
        int ret;
        va_list ap;
        va_start(ap, fmt);
        ret = vfprintf(f, fmt, ap);
        va_end(ap);
        return ret;
}

global int vfprintf( FILE *restrict f, const char *restrict s, va_list ap0)
  { int j = 0, k = 0;
    va_list ap;
    va_copy(ap, ap0);
    char ch = s[j++];
    until (ch == '\0')
      { if (ch == '%')
	  { int d = 0;
	    ch = s[j++];
	    while (ch >= '0' && ch <= '9') { d = d*10 + (ch-'0'); ch = s[j++]; }
	    if (ch == '%') putc('%', f);
	    else
#define FWR(c,typ) fwrite##c( va_arg(ap,typ),d,f); break;
  switch (ch)
      { default:    break;
	case 'a':   FWR(a,struct hyperalt *);
	case 'b':   FWR(b,bool);
	case 'c':   FWR(c,int);
	case 'd':   FWR(d,int);
	case 'e':   FWR(e,struct state *);
	case 'h':   FWR(h,struct hypernotion *);
	case 'm':   FWR(m,struct metarhs *);
	case 's':   FWR(s,char*);
	case 't':   FWR(t,uint64);
	case 'x':   FWR(x,uint);
      }
	  }
	else putc(ch, f);
	ch = s[j++];
      }
    va_end(ap);
    return k;
  }

static void fwritea( struct hyperalt *alt, int d, FILE *f)
  { int i;
    for (i=0; i < alt -> rlen; i++)
      { if (i > 0) fprintf(f, ", ");
	fwriteh(alt -> rdef[i], d, f);
      }
  }

static void fwriteb( bool b, int d, FILE *f)
  { putc(b ? 'Y' : 'N', f);
  }

static void fwritec( int c, d, FILE *f)
  { putc(c, f);
  }

static void fwrited( int n, int d, FILE *f)
  { if (n < 0) writeneg(true, n, d-1, f);
    if (n > 0) writeneg(false, -n, d, f);
    if (n == 0) { spaces(d-1, f); putc('0', f); }
  }

static void writeneg( bool s, int n, int d, FILE *f)
  { if (n == 0)
      { spaces(d, f);
	if (s) putc('-', f);
      }
    else
      { writeneg(s, n/10, d-1, f);
	putc(-(n%10) + '0', f);
      }
  }

static void spaces( int d, FILE *f)
  { while (d > 0) putc(' ', f), d--; }

static void fwritee( struct state *st, int d, FILE *f)
  { struct hyperrule *hr = st -> pval;
    struct hyperalt *rhs = hr -> rhs;
    int i;
    fprintf(f, "%h:", hr -> lhs);
    for (i=0; i < rhs -> rlen + 1; i++)
      { if (i == st -> jval) fprintf(f, " <DOT>");
	if (i < rhs -> rlen) fprintf(f, " %h", rhs -> rdef[i]);
	if (i < rhs -> rlen - 1) putc(',', f);
      }
  }

static void fwriteh( struct hypernotion *hn, int d, FILE *f)
  { if (hn -> hlen < 0) fprintf(f, "\"%s\"", hn -> term -> str);
    else wrhn(hn, f);
  }

static void wrhn( struct hypernotion *hn, FILE *f)
  { int k; bool sep;
    int n1 = 0;
    if (hn -> flags & hn_negpred)
      { /* change "where" into "unless" */
	fprintf(f, "unless");
	n1 = 5;
      }
    for (k = n1; k < hn -> hlen; k++)
      { struct hitem *z = &hn -> hdef[k];
	if (z -> sy == s_ssm)
	  { if (k > n1 && sep) putc(' ', f);
	    if( z -> it_s > 32) putc(z -> it_s, f);
            else fprintf(f, "{}%s", z -> it_z -> str);
	    sep = false; /* i.e. only if followed by a meta */
	  }
	if (z -> sy == s_meta)
	  { if (k > n1) putc(' ', f);
	    fprintf(f, "%s", z -> it_z -> str);
	    sep = true;
	  }
      }
  }

static void fwritem( struct metarhs *y, int d, FILE *f)
  { bool had = false;
    until (y == NULL)
      { if (had) fprintf(f, "; ");
	fprintf(f, "%h", y -> hn);
	y = y -> lnk; had = true;
      }
  }

static void fwrites( char *s, int d, FILE *f)
  { int i = 0;
    until (s[i] == '\0')
      { int c = s[i++];
	putc(c, f);
      }
  }

static void fwritet( uint64 t, int d, FILE *f)
  { /* write LL(1) starters */
    static char *tab = "abcdefghijklmnopqrstuvwxyz<>_*.@ABCDEFGHIJLMNOPQR";   /* * is stopper; @ is null bit */
    int k = 0;
    until (t == 0)
      { if (t & 1) { putc(tab[k], f);
        if( k >= 32 ) fprintf(f, " /%s/ ", protolist[k-32] ); }
	t >>= 1; k++;
      }
  }

static void fwritex( uint n, int d, FILE *f)
  { static char *hextab = "0123456789abcdef";
    if (d > 0)
      { fwritex(n >> 4, d-1, f);
	putc(hextab[n & 0xf], f);
      }
  }

