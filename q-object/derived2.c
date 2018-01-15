#include <stdio.h>
#include "derived2.h"

static void derived2_do_something(void)
{
    printf("in derived2 do something\n");
      // do something
}

static void derived2_class_init(ObjectClass *oc, void *data)
{
      MyClass *mc = MY_CLASS(oc);
      Derived2Class *d2c = DERIVED2_CLASS(oc);

      d2c->parent_do_something = mc->do_something;
      mc->do_something = derived2_do_something;
      printf("in derived2 mc address %p\n", mc);
      printf("derived2  init\n");
}


static void derived2_instance_init(Object* obj)
{
     MY(obj)->j = 20;
     DERIVED2(obj)->j = 20;
}

static const TypeInfo derived2_type_info = {
      .name = TYPE_DERIVED2,
      .parent = TYPE_MY,
      .instance_size = sizeof(Derived2),
      .abstract = false,
      .class_size = sizeof(Derived2Class),
      .instance_init = derived2_instance_init,
      .class_init = derived2_class_init,
  };

static void derived2_register(void)
{
    type_register_static(&derived2_type_info);
}

void type_derived2_init(void)
{
   type_init(derived2_register);
}

