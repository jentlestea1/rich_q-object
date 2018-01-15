#include <stdio.h>

/* 源文件一般包含其类本身的头文件，并定义该类的一个视角*/
#define DERIVED1_VIEW
#include "derived1.h"

static void derived1_do_something(void)
{
    printf("in derived1 do something\n");
      // do something
}

static bool is_writable()
{
   return false;
}

static void derived1_class_init(ObjectClass *oc, void *data)
{
      WritableClass *wc = WRITABLE_CLASS(oc);
      MyClass *mc = MY_CLASS(oc);
      Derived1Class *d1c = DERIVED1_CLASS(oc);

      wc->is_writable = is_writable;
      d1c->parent_do_something = mc->do_something;
      mc->do_something = derived1_do_something;

      printf("in derived1 mc address %p\n", mc);
      printf("derived1  init\n");
      
}


static void derived1_instance_init(Object* obj)
{
     MY(obj)->j = 10;
     DERIVED1(obj)->i = 10;
}

static const TypeInfo derived1_type_info = {
      .name = TYPE_DERIVED1,
      .parent = TYPE_MY,
      .instance_size = sizeof(Derived1),
      .abstract = false,
      .class_size = sizeof(Derived1Class),
      .instance_init = derived1_instance_init,
      .class_init = derived1_class_init,
  };

static void derived1_register(void)
{
    type_register_static(&derived1_type_info);
}

void type_derived1_init(void)
{
   type_init(derived1_register);
}

