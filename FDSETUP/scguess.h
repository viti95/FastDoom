#ifndef __SCGUESS_H__
#define __SCGUESS_H__

/*
 * This stuff just checks for environment variables.  If they're there,
 * then it kinda figures hopefully that the card is there and it fills
 * in the values as defaults to choose from in Scott's & Paul's cool
 * install program.
 */

/*
 * Returns 1 if it senses the BLASTER environment variable, 0 if it
 * doesn't.  If it does return 1, it will also fill in as many fields
 * as it can extract from the environment variable.  Any fields *not*
 * filled in will be set to -1.  Of course, if the midi field is filled,
 * that means only that it's an SB16 and does not confirm whether the
 * WaveBlaster is present.
 */

int SmellsLikeSB(int *addr, int *irq, int *dma, int *midi);

/*
 * Returns 1 if it senses the ULTRASND environment variable, 0 if it
 * doesn't.  If it does return 1, it will also fill in as many fields
 * as it can extract from the environment variable.  Any fields *not*
 * filled in will be set to -1.  Do we need the address, irq, and dma?
 * Paul's GF1_Detect code doesn't seem terribly interested in this
 * information.  Who cares- it's bonus stuff if you're curious, and
 * militant GUSaholes can eat cake.
 */

int SmellsLikeGUS(int *addr, int *irq, int *dma);

#endif
