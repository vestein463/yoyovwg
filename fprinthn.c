/* fprintf which knows about "%h" to print hypernotions */

#include "vwg.h"
#include "states.h"
#include <stdio.h>

static spaces(d, f) int d; FILE *f;
  { while (d > 0) putc(' ', f), d--; }

static fwritee(st, d, f) struct state *st; int d; FILE *f;
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

static fwriteh(hn, d, f) struct hypernotion *hn; int d; FILE *f;
  { if (hn -> hlen < 0) fprintf(f, "\"%s\"", hn -> term -> str);
    else wrhn(hn, f);
  }

static wrhn(hn, f) struct hypernotion *hn; FILE *f;
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
	    putc(z -> it_s, f);
	    sep = false; /* i.e. only if followed by a meta */
	  }
	if (z -> sy == s_meta)
	  { if (k > n1) putc(' ', f);
	    fprintf(f, "%s", z -> it_z -> str);
	    sep = true;
	  }
      }
  }

static fwritem(y, d, f) struct metarhs *y; int d; FILE *f;
  { bool had = false;
    until (y == NULL)
      { if (had) fprintf(stderr, "; ");
	fprintf(f, "%h", y -> hn);
	y = y -> lnk; had = true;
      }
  }

static fwritet(t, d, f) uint t; int d; FILE *f;
  { /* write LL(1) starters */
    static char *tab = "abcdefghijklmnopqrstuvwxyz<>_*.@";   /* * is stopper; @ is null bit */
    int k = 0;
    until (t == 0)
      { if (t & 1) putc(tab[k], f);
	t >>= 1; k++;
      }
  }
