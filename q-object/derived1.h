#ifndef DERIVED1_H
#define DERIVED1_H

/* 头文件一般包含其父类的头文件，并定义其父类的子类的一个视角*/
#define MY_CHILD_VIEW
#include "mydevice.h"

#define TYPE_DERIVED1 "derived1"


///////////////////////////////////////////////////////////////////////////////
//
//                             access control
//
///////////////////////////////////////////////////////////////////////////////

// Avoid macros Private and  Protected  redefinition warining 
#undef Private
#undef Protected
#if defined(DERIVED1_VIEW)
#   include "self_view_access_control.h"   
#elif defined(DERIVED1_CHILD_VIEW)
#   include "child_view_access_control.h"   
#else
#   include "other_view_access_control.h"   
#endif


typedef void (*MyDoSomething)(void);
typedef struct Derived1Class {
      MyClass parent_class;

      MyDoSomething parent_do_something;
} Derived1Class;


typedef struct Derived1 {
      My Private(parent);

      int Private(i);
}Derived1;

// distinguash obj between klass as parameter 
#define DERIVED1_GET_CLASS(obj) \
        OBJECT_GET_CLASS(Derived1Class, obj, TYPE_DERIVED1)
#define DERIVED1_CLASS(klass) \
        OBJECT_CLASS_CHECK(Derived1Class, klass, TYPE_DERIVED1)
#define DERIVED1(obj) \
        OBJECT_CHECK(Derived1, obj, TYPE_DERIVED1)

void type_derived1_init(void);

#endif
