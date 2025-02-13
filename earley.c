#include "vwg.h"
#include "states.h"
#include <stdio.h>

extern FILE *poutfile;
extern struct ruleset *hypersyntax;
extern struct hyperrule *toplevelhr;
extern struct hypernotion *emptypn;
extern uint debugbits;

extern bool hnmatch(), eqhn();		/* from hnmatch */
extern struct hyperrule *substrule();	/* from hnmatch */
int hashval3(void *w1,void *w2,void *w3);

static struct dictnode **wvec;
static int wptr;
static bool predictmode;
static struct stateset *statesets[MAXSENTLEN+2];
static struct state *result;
static struct ruleset *strictsyntax[(MAXHITEMS*MAXRHS) + 1];
static void addpreterminals(void );
static void processstates(void );
static void td_predictor( struct state *st);
static bool predicateholds( struct hypernotion *hn, int d);
static void testsign( struct hypernotion *hn, int d, bool *okp);
static void bu_predictor( struct state *st);
static void gap_scanner( struct state *st, char *how);
static void term_scanner( struct state *st);
static void completer( struct state *st);
static struct statelist *slnode( struct statelist *x, struct state *left, struct state *down);
static struct hyperrule *canonize_rule( struct hyperrule *hr);
static struct stateset *newstateset(void );
static struct state *addstate(struct stateset *, struct hyperrule *,int,int,char*);
static void echoterm(void );
static int rulelength( struct hyperrule *hr);
static bool isstrict( struct hyperrule *hr);
static bool eqrule( struct hyperrule *hr1, struct hyperrule *hr2);
static bool eqrhs( struct hyperalt *r1, struct hyperalt *r2);


global struct state *parsesentence( struct dictnode **w, struct dictnode *pw)
  { /* This is Earley's famous Algorithm. */
    /* w is the sentence to parse; pw is the "predict word" set by ".pw" */
    int i;
    for (i=0; i <= MAXHITEMS*MAXRHS; i++) strictsyntax[i] = NULL;
    result = NULL; wvec = w; wptr = 0; predictmode = false;
    statesets[0] = newstateset();
    addstate(statesets[0], canonize_rule(toplevelhr), 0, 0, "INI");
    until (wptr >= MAXSENTLEN || statesets[wptr] -> sshd == NULL)
      { statesets[++wptr] = newstateset();
	if (pw != NULL && wvec[wptr-1] == pw) predictmode = true;
	if (predictmode) wvec[wptr-1] = NULL;
	unless (predictmode) addpreterminals();
	processstates();
	echoterm();
	if (debugbits & 0xc) printf("\n============\n");
      }
    if (statesets[wptr] -> sshd != NULL) fprintf(poutfile, " TOO LONG!");
    if (result == NULL) fprintf(poutfile, " NO");
    putc('\n', poutfile);
    return result;
  }

static void addpreterminals(void )
  { struct ruleset *rs;
    for (rs = hypersyntax; rs != NULL; rs = rs -> lnk)
      { struct hyperrule *hr = rs -> hr;
	struct hyperalt *rhs = hr -> rhs;
	if (rhs -> rlen == 1)
	  { struct hypernotion *rhn = rhs -> rdef[0];
	    if ((rhn -> hlen < 0 && rhn -> term == wvec[wptr-1]) ||	/* it's a terminal */
		(rhn -> hlen == 1))					/* it's ``*''	   */
	      addstate(statesets[wptr-1], canonize_rule(hr), 0, wptr-1, "APT");
	  }
      }
  }

static void processstates(void )
  { struct state *st;
    for (st = statesets[wptr-1] -> sshd; st != NULL; st = st -> sflk)
      { struct hyperalt *rhs = st -> pval -> rhs;
	int j = st -> jval;
	if (j < rhs -> rlen)
	  { struct hypernotion *hn = rhs -> rdef[j];
	    if (hn -> hlen >= 0)
	      { /* a hypernotion */
		if (ispn(hn)) td_predictor(st);
		if (hnmatch(emptypn, hn)) gap_scanner(st, "GSC");
	      }
	    else
	      { /* a terminal */
		if (predictmode)
		  { /* "force", i.e. predict, a particular terminal */
		    unless (wvec[wptr-1] == NULL) fprintf(poutfile, " AMBIG!"); /* ambiguous prediction */
		    wvec[wptr-1] = hn -> term;
		  }
		if (hn -> term == wvec[wptr-1]) term_scanner(st);
	      }
	  }
	else
	  { if (ispn(st -> pval -> lhs))
	      { unless (predictmode) bu_predictor(st);
		completer(st);
	      }
	  }
      }
  }

static void td_predictor( struct state *st)
  { struct hypernotion *hn = st -> pval -> rhs -> rdef[st -> jval];
    if (hn -> flags & hn_dterm)
      { struct ruleset *rs;
	if (debugbits & 8) fprintf(stdout,"\nTd_predicting state st = %e\n", st);
	for (rs = hn -> fxref; rs != NULL; rs = rs -> lnk)
	  { struct hyperrule *hr = rs -> hr;
	    if (hnmatch(hn, hr -> lhs))
	      { struct state *nst = addstate(statesets[wptr-1], canonize_rule(substrule(hr)), 0, wptr-1, "TDP");
		if (debugbits & 8) fprintf(stdout,"   adding new state nst = %e   bval=%d\n", nst, nst -> bval);
	      }
	  }
      }
    else if (predicateholds(hn, 0)) gap_scanner(st, "PRD");
  }

static bool predicateholds( struct hypernotion *hn, int d)
  { /* recursive-descent parser for testing predicates */
    bool ok;
    if (debugbits & 4)
      { fprintf(stdout,"   "); dots(d, stdout);
	fprintf(stdout,"Does predicate hold?  hn=``%h''\n", hn);
      }
    if (hn -> hlen == 1) ok = true; /* EMPTY */
    else
      { struct ruleset *rs;
	ok = false;
	for (rs = hn -> fxref; rs != NULL && !ok; rs = rs -> lnk)
	  { struct hyperrule *hr = rs -> hr;
	    if (hnmatch(hn, hr -> lhs))
	      { int k;
		hr = substrule(hr);
		ok = true;
		for (k=0; k < hr -> rhs -> rlen && ok; k++)
		  { struct hypernotion *rhn = hr -> rhs -> rdef[k];
		    unless (predicateholds(rhn, d+1)) ok = false;
		  }
		testsign(hr -> lhs, d+1, &ok);
	      }
	  }
      }
    testsign(hn, d, &ok);
    if (debugbits & 4)
      { fprintf(stdout,"   "); dots(d, stdout);
	fprintf(stdout,"Returns %b\n", ok);
      }
    return ok;
  }

static void testsign( struct hypernotion *hn, int d, bool *okp)
  { if (hn -> flags & hn_negpred)
      { if (debugbits & 4)
	  { fprintf(stdout,"   "); dots(d, stdout);
	    fprintf(stdout,"hn ``%h'' is neg, inverting\n", hn);
	  }
	*okp ^= true;
      }
  }

static void bu_predictor( struct state *st)
  { /* add states for all hyperrules whose first RHSs match our LHS */
    struct hyperrule *p = st -> pval;
    int b = st -> bval;
    struct ruleset *rs;
    if (debugbits & 8) fprintf(stdout,"\nBu_predicting state st = %e\n", st);
    for (rs = p -> lhs -> bxref; rs != NULL; rs = rs -> lnk)
      { struct hyperrule *hr = rs -> hr;
	if (hr -> rhs -> rlen >= 1)
	  { struct hypernotion *rhn = hr -> rhs -> rdef[0];
	    if (rhn -> hlen >= 0 && hnmatch(st -> pval -> lhs, rhn))
	      { struct state *nst = addstate(statesets[b], canonize_rule(substrule(hr)), 0, b, "BUP");
		if (debugbits & 8) fprintf(stdout,"   adding new state nst = %e   bval=%d\n", nst, nst -> bval);
	      }
	  }
      }
  }

static void gap_scanner( struct state *st, char *how)
  { struct hyperrule *p = st -> pval;
    int j = st -> jval, b = st -> bval;
    struct state *nst = addstate(statesets[wptr-1], canonize_rule(substrule(p)), j+1, b, how);
    if (debugbits & 8)
      { fprintf(stdout,"\nScanning gap, state st = %e\n", st);
	fprintf(stdout,"   adding new state nst = %e   bval=%d\n", nst, nst -> bval);
      }
    nst -> treep = slnode(nst -> treep, st, NULL);
  }

static void term_scanner( struct state *st)
  { struct hyperrule *p = st -> pval;
    int j = st -> jval, b = st -> bval;
    struct state *nst = addstate(statesets[wptr], p, j+1, b, "TSC");
    if (debugbits & 8)
      { fprintf(stdout,"\nScanning term, state st = %e\n", st);
	fprintf(stdout,"   adding new state nst = %e   bval=%d\n", nst, nst -> bval);
      }
    nst -> treep = slnode(nst -> treep, st, NULL);
  }

static void completer( struct state *st)
  { struct hyperrule *p = st -> pval;
    int b = st -> bval;
    if (debugbits & 8) fprintf(stdout,"\nCompleting state st = %e   bval=%d\n", st, b);
    if (p == toplevelhr && b == 0)
      { fprintf(poutfile, " YES");
	result = st;
      }
    else
      { struct state *sb;
	for (sb = statesets[b] -> sshd; sb != NULL; sb = sb -> sflk)
	  { struct hyperrule *pb = sb -> pval;
	    int jb = sb -> jval, bb = sb -> bval;
	    if (jb < pb -> rhs -> rlen)
	      { struct hypernotion *rhn = pb -> rhs -> rdef[jb];
		if (rhn -> hlen >= 0 && hnmatch(p -> lhs, rhn))
		  { struct state *nst = addstate(statesets[wptr-1], canonize_rule(substrule(pb)), jb+1, bb, "COM");
		    if (debugbits & 8)
		      { fprintf(stdout,"   predictor state   sb = %e\n", sb);
			fprintf(stdout,"   adding new state nst = %e   bval=%d\n", nst, nst -> bval);
			if (nst -> treep != NULL) fprintf(stdout,"   DUPLICATE\n");
		      }
		    nst -> treep = slnode(nst -> treep, sb, st);
		    if (b == wptr-1) nst -> treep -> down = NULL; /* don't record empty descendants (keeps tree tidy) */
		  }
	      }
	  }
      }
  }

static struct statelist *slnode( struct statelist *x, struct state *left, struct state *down)
  { struct statelist *y = heap(1, struct statelist);
    y -> link = x;
    y -> left = left;
    y -> down = down;
    return y;
  }

static struct hyperrule *canonize_rule( struct hyperrule *hr)
  { struct hyperrule *chr;
    if (isstrict(hr))
      { int n = rulelength(hr);
	struct ruleset *x = strictsyntax[n];
	until (x == NULL || eqrule(x -> hr, hr)) x = x -> lnk;
	if (x == NULL)
	  { /* a new rule; add to strict syntax */
	    x = heap(1, struct ruleset);
	    x -> hr = hr;
	    x -> lnk = strictsyntax[n]; strictsyntax[n] = x;
	  }
	chr = x -> hr;
      }
    else chr = hr; /* can't canonize non-strict rules! */
    return chr;
  }

static bool eqrule( struct hyperrule *hr1, struct hyperrule *hr2)
  { return eqhn(hr1 -> lhs, hr2 -> lhs) && eqrhs(hr1 -> rhs, hr2 -> rhs);
  }

static bool eqrhs( struct hyperalt *r1, struct hyperalt *r2)
  { int k;
    unless (r1 -> rlen == r2 -> rlen) return false;
    for (k=0; k < r1 -> rlen; k++)
      { unless (eqhn(r1 -> rdef[k], r2 -> rdef[k])) return false;
      }
    return true;
  }

static bool isstrict( struct hyperrule *hr)
  { int k;
    unless (ispn(hr -> lhs)) return false;
    for (k=0; k < hr -> rhs -> rlen; k++)
      { struct hypernotion *hn = hr -> rhs -> rdef[k];
	unless(hn -> hlen < 0 || ispn(hn)) return false;
      }
    return true;
  }

static int rulelength( struct hyperrule *hr)
  { int k;
    int n = hr -> lhs -> hlen;
    for (k=0; k < hr -> rhs -> rlen; k++)
      { struct hypernotion *hn = hr -> rhs -> rdef[k];
	if (hn -> hlen >= 0) n += hn -> hlen;
      }
    return n;
  }

static struct stateset *newstateset(void)
  { int i;
    struct stateset *sts = heap(1, struct stateset);
    sts -> sshd = NULL; sts -> sstl = &sts -> sshd;
    for (i=0; i < HASHSIZE; i++) sts -> hash[i] = NULL;
    return sts;
  }

#define eqst(st, p, j, b) \
    ( st -> pval == p && \
      st -> jval == j && \
      st -> bval == b )

static struct state *addstate( struct stateset *sts, struct hyperrule *p, int j, int b, char *how)
  { int hv = hashval3( p, (void*)j, (void*)b);
    struct state **zz = &sts -> hash[hv];
    struct state *z = *zz;
    struct state *st;
    until (z == NULL || eqst(z, p, j, b))
      { zz = &z -> shlk;
	z = *zz;
      }
    if (z != NULL) st = z; /* the already-existing state */
    else
      { /* it's a new state; append it to state set */
	struct state *nst = heap(1, struct state);
	nst -> treep = NULL;
	nst -> mark = false;
	nst -> pval = p; nst -> jval = j; nst -> bval = b;
	nst -> how = how;
	*zz = nst; nst -> shlk = NULL;
	*(sts -> sstl) = nst; sts -> sstl = &nst -> sflk; nst -> sflk = NULL;
	st = nst; /* the new state */
      }
    return st;
  }

static void echoterm(void )
  { struct dictnode *term = wvec[wptr-1];
    unless (term == NULL)
      { putc(' ', poutfile);
	unless (term -> flg & f_term) putc('?', poutfile);
	fprintf(poutfile, "%s", term -> str);
      }
  }

global void printstates(void )
  { int i;
    for (i=0; i <= wptr; i++)
      { struct state *x;
	fprintf(poutfile, "\nIn state set %d:\n", i);
	for (x = statesets[i] -> sshd; x != NULL; x = x -> sflk)
	  fprintf(poutfile, "   [%s] %e\n", x -> how, x);
      }
  }

global void printrules(void )
  { int i;
    for (i=0; i <= MAXHITEMS*MAXRHS; i++)
      { struct ruleset *x;
	for (x = strictsyntax[i]; x != NULL; x = x -> lnk)
	  { struct hyperrule *hr = x -> hr;
	    struct hyperalt *rhs = hr -> rhs;
	    int rlen = rhs -> rlen;
	    fprintf(poutfile, "\n%h:\n", hr -> lhs);
	    /* we cd use "%a" format to print rhs, but rhsides get rather long! */
	    if (rlen > 0)
	      { int j;
		for (j=0; j < rlen-1; j++) fprintf(poutfile, "   %h,\n", rhs -> rdef[j]);
		fprintf(poutfile, "   %h.\n", rhs -> rdef[rlen-1]);
	      }
	    else fprintf(poutfile, "   .\n");
	  }
      }
  }

