#ifndef DERIVED2_H
#define DERIVED2_H

#define MY_CHILD_VIEW
#include "mydevice.h"

#define TYPE_DERIVED2 "derived2"

///////////////////////////////////////////////////////////////////////////////
//
//                              access control
//
///////////////////////////////////////////////////////////////////////////////

#undef Private
#undef Protected
#if defined(DERIVED2_VIEW)
#   include "self_view_access_control.h"
#elif defined(DERIVED2_CHILD_VIEW)
#   include "self_view_access_control.h"
#else
#   include "child_view_access_control.h"
#endif


typedef void (*MyDoSomething)(void);
typedef struct Derived2Class {
      MyClass parent_class;

      MyDoSomething parent_do_something;
} Derived2Class;


typedef struct Derived2 {
      My parent;

      int j;
}Derived2;

// distinguash obj between klass as parameter 
#define DERIVED2_GET_CLASS(obj) \
        OBJECT_GET_CLASS(Derived2Class, obj, TYPE_DERIVED2)
#define DERIVED2_CLASS(klass) \
        OBJECT_CLASS_CHECK(Derived2Class, klass, TYPE_DERIVED2)
#define DERIVED2(obj) \
        OBJECT_CHECK(Derived2, obj, TYPE_DERIVED2)

void type_derived2_init(void);

#endif
