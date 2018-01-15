#include "type_init.h"
#include <stdio.h>

void type_init(void (*type_register_func)(void))
{
   if (type_register_func == NULL) return;

   type_register_func();
}
