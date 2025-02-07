#include "vwg.h"
#include <stdio.h>

#define NUMSEGS	   400	  /* num. of segments in heap */
#define SEGSIZE	   10000  /* size of a segment in heap */

static char *segbase[NUMSEGS];
static int segptr[NUMSEGS];
static int botmark;	/* start searching for space at this segment number */
static int topmark;	/* seg. num. above the highest from which storage has been allocated */

extern char *malloc();

static void fail( int n);
static void swap( int *xx, int *yy);

global void initheap(void)
  { int k;
    for (k=0; k < NUMSEGS; k++) segbase[k] = NULL;
    botmark = topmark = 0;
  }

global char *cheap( int nb) /* called by "heap" macro */
  { int k; char *x;
    bool found = false;
    for (k = botmark; k < NUMSEGS && !found; k++)
      { if (segbase[k] == NULL)
	  { segbase[k] = malloc(SEGSIZE);
	    segptr[k] = 0;
	  }
	if (segbase[k] == NULL) fail(1);
	if (segptr[k] + nb <= SEGSIZE)
	  { x = segbase[k] + segptr[k];
	    segptr[k] += nb;
	    /* move segment to front of list for efficiency */
	    unless (k == botmark)
	      { swap(&segbase[k], &segbase[botmark]);
		swap(&segptr[k], &segptr[botmark]);
	      }
	    found = true;
	  }
      }
    unless (found) fail(2);
    if (k > topmark) topmark = k;	/* topmark is seg. num. above highest allocated */
    return x;
  }

static void fail( int n)
  { fprintf(stderr, "vwg: no room! (%d)\n", n);
    exit(1);
  }

static void swap( int *xx, int *yy)
  { int t = *xx;
    *xx = *yy; *yy = t;
  }

global void freezeheap(void )
  { botmark = topmark;
  }

global void resetheap(void )
  { int k;
    for (k = botmark; k < topmark; k++) segptr[k] = 0; /* don't free, just mark as empty */
    topmark = botmark;
  }

global void wrheap(void )
  { int tot = 0; int k;
    for (k=0; k < topmark; k++) tot += segptr[k];
    printf("%d bytes (%d/%d)", tot, topmark, NUMSEGS);
  }

