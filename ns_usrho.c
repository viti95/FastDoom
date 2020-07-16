#include <stdlib.h>
#include "ns_usrho.h"

int USRHOOKS_GetMem(
    void **ptr,
    unsigned long size)

{
   void *memory;

   memory = malloc(size);
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

   free(ptr);

   return (USRHOOKS_Ok);
}
