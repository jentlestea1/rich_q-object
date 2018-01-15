#ifndef MY_H
#define MY_H

#include "object.h"

#define TYPE_MY "my"

///////////////////////////////////////////////////////////////////////////////
//
//                           access control
//
///////////////////////////////////////////////////////////////////////////////

#if defined(MY_VIEW)
#   include "self_view_access_control.h"
#elif defined(MY_CHILD_VIEW)
#   include "child_view_access_control.h"
#else
#   include "other_view_access_control.h"
#endif

typedef void (*MyDoSomething)(void);
typedef struct MyClass {
      ObjectClass parent_class;

      MyDoSomething do_something;
} MyClass;

typedef struct My {
      Object Private(parent);

      //private part     
      int Private(i);

      //protected part
      int Protected(j);

      //public part
      int k;
}My;

// distinguash obj between klass as parameter 
#define MY_GET_CLASS(obj) \
        OBJECT_GET_CLASS(MyClass, obj, TYPE_MY)
#define MY_CLASS(klass) \
        OBJECT_CLASS_CHECK(MyClass, klass, TYPE_MY)
#define MY(obj) \
        OBJECT_CHECK(My, obj, TYPE_MY)

void type_my_init(void);

#define TYPE_WRITABLE "writable"

typedef struct WritableClass {
    InterfaceClass parent_class;
    bool (*is_writable)();
} WritableClass;

#define WRITABLE_CLASS(klass) \
        OBJECT_CLASS_CHECK(WritableClass, klass, TYPE_WRITABLE)

#define WRITABLE_GET_CLASS(obj) \
        OBJECT_GET_CLASS(WritableClass, obj, TYPE_WRITABLE)

#endif


