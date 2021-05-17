#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "scguess.h"

int getSBParam(char *string, char field)
{
  char *p;
  int rc;

  p = strchr(string, field);

  if (!p) return -1;
  else p++;

  if (field == 'A' || field == 'P') sscanf(p, "%x", &rc); // hex field
  else sscanf(p, "%d", &rc); // decimal field

  return rc;
}

/*
 * Returns 1 if it senses the BLASTER environment variable, 0 if it
 * doesn't.  If it does return 1, it will also fill in as many fields
 * as it can extract from the environment variable.  Any fields *not*
 * filled in will be set to -1.  Of course, if the midi field is filled,
 * that means only that it's an SB16 and does not confirm whether the
 * WaveBlaster is present.
 */

int SmellsLikeSB(int *addr, int *irq, int *dma, int *midi)
{
  char *var = getenv("BLASTER");

  if (!var) return 0;

  *addr = getSBParam(var, 'A');
  *irq = getSBParam(var, 'I');
  *dma = getSBParam(var, 'D');
  *midi = getSBParam(var, 'P');

  return 1;
}

/*
 * Returns 1 if it senses the ULTRASND environment variable, 0 if it
 * doesn't.  If it does return 1, it will also fill in as many fields
 * as it can extract from the environment variable.  Any fields *not*
 * filled in will be set to -1.
 */

int SmellsLikeGUS(int *addr, int *irq, int *dma)
{
  char *var = getenv("ULTRASND");
  int dummy;

  if (!var) return 0;
  else
  {
    sscanf(var, "%x,%d,%d,%d,%d", addr, dma, &dummy, irq, &dummy);
    return 1;
  }

}
