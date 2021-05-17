#ifndef __USRHOOKS_H
#define __USRHOOKS_H

/*---------------------------------------------------------------------
   Error definitions
---------------------------------------------------------------------*/

enum USRHOOKS_Errors
{
   USRHOOKS_Warning = -2,
   USRHOOKS_Error = -1,
   USRHOOKS_Ok = 0
};

/*---------------------------------------------------------------------
   Function Prototypes
---------------------------------------------------------------------*/

int USRHOOKS_GetMem(void **ptr, unsigned long size);
int USRHOOKS_FreeMem(void *ptr);

#endif
