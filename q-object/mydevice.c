#include <stdio.h>

#define MY_VIEW
#include "mydevice.h"

static void my_do_something(void)
{
    printf("in my do something\n");
      // do something
}

static void my_class_init(ObjectClass *oc, void *data)
{
      MyClass *mc = MY_CLASS(oc);

      mc->do_something = my_do_something;

      printf("in my mc address %p\n", mc);
      printf("my type init\n");
}


static void my_instance_init(Object* obj)
{
     MY(obj)->i = 10;
     MY(obj)->k = 101;
}

static InterfaceInfo interfaces[] = {
     {
         .type = TYPE_WRITABLE,
     },
    NULL
};

static bool is_writable()
{
   return true;
}

static void writable_class_init(ObjectClass *oc, void* data)
{
   WritableClass *wc = WRITABLE_CLASS(oc);
   wc->is_writable = is_writable;    

}

static const TypeInfo writable_interface_info = {
      .name = TYPE_WRITABLE,
      .parent = TYPE_INTERFACE,
      .class_size = sizeof(WritableClass),
      .class_init = writable_class_init,
      .abstract = true,
};

static const TypeInfo my_type_info = {
      .name = TYPE_MY,
      .parent = TYPE_OBJECT,
      .instance_size = sizeof(My),
      .abstract = true,
      .class_size = sizeof(MyClass),
      .instance_init = my_instance_init,
      .class_init = my_class_init,
      .interfaces = interfaces, 
  };

static void my_register(void)
{
    type_register_static(&writable_interface_info);
    type_register_static(&my_type_info);
}

void type_my_init(void)
{
   type_init(my_register);
}

