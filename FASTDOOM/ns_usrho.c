#include <stdlib.h>
#include "ns_usrho.h"
#include "options.h"
#include "z_zone.h"

int USRHOOKS_GetMem(
    void **ptr,
    unsigned long size)

{
   void *memory;

   memory = Z_MallocUnowned(size, PU_STATIC);
   if (memory == NULL)
   {
      return (USRHOOKS_Error);
   }

   *ptr = memory;

   return (USRHOOKS_Ok);
}

int USRHOOKS_FreeMem(
    void *ptr)

{
   if (ptr == NULL)
   {
      return (USRHOOKS_Error);
   }

   Z_Free(ptr);

   return (USRHOOKS_Ok);
}
