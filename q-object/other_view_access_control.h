/**
 *  In the view of the child of a class, the visiable parts are just the 
 *  protected and the public part of it's parent class. 
 *  And this header is left without protection intentionally.
 */

#define INVISIABLE_MEMBER(member) member##_is_not_visible_in_current_view##\
                                          _and_I_know_what_I_am_doing

#define Private(member) INVISIABLE_MEMBER(member)
#define Protected(member) INVISIABLE_MEMBER(member)

