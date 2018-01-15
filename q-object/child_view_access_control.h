/**
 *  In the view of others, the visiable part is just the public part of a class. 
 *  And this header is left without protection intentionally.
 */

#define VISIABLE_MEMBER(member) member
#define INVISIABLE_MEMBER(member) member##_is_not_visible_in_current_view##\
                                          _and_I_know_what_I_am_doing

#define Private(member) INVISIABLE_MEMBER(member)
#define Protected(member) VISIABLE_MEMBER(member)

