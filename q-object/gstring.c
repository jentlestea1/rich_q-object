#include <string.h>
#include "gstring.h"


gboolean
g_str_equal (gconstpointer v1, gconstpointer v2)
{
  const gchar *string1 = v1;
  const gchar *string2 = v2;
  
  return strcmp (string1, string2) == 0;
}

/* 31 bit hash function */
guint
g_str_hash (gconstpointer key)
{
  const char *p = key;
  guint h = *p;

  if (h)
    for (p += 1; *p != '\0'; p++)
      h = (h << 5) - h + *p;

  return h;
}
