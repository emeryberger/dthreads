#ifndef _RABIN_H
#define _RABIN_H

#include "dedupdef.h"

enum {
	NWINDOW		= 32,
	MinSegment	= 1024,
	RabinMask	= 0xfff,	// must be less than <= 0x7fff 
};

void		rabininit(int, u32int*, u32int*);

int		rabinseg(uchar*, int, int, u32int*, u32int*);

#endif //_RABIN_H
