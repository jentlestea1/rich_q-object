#include <stdio.h>
#include "object.h"
#include "mydevice.h"
#include "derived1.h"
#include "derived2.h"
#include "error.h"

// used in error.c
Error *error_fatal;
Error *error_abort;
int errno;

int main()
{  
    type_object_init();
    type_my_init();
    type_derived1_init();
    type_derived2_init();

#if 0
    Object* obj = object_new("my");
    MY_GET_CLASS(obj)->do_something();
    printf("data member is %d\n", MY(obj)->i);
#endif
    Object* obj1 = object_new("derived1");
    MY_GET_CLASS(obj1)->do_something();
    //printf("data member is %d\n", MY(obj1)->i);

    printf("data member is %p\n", &MY(obj1)->k);
    
    Object* obj2 = object_new("derived2");
    MY_GET_CLASS(obj2)->do_something();
    //printf("data member is %d\n", MY(obj2)->i);


    if (WRITABLE_GET_CLASS(obj1)->is_writable()){
       printf("that is true\n");    
    }

    printf("sizeof MyClass is %ld\n", sizeof(MyClass));
    printf("sizeof d1Class is %ld\n", sizeof(Derived1Class));
    printf("sizeof d2Class is %ld\n", sizeof(Derived2Class));

    return 0;
}

