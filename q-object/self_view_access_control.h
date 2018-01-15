
/**
 *  In the class it self's view, it can see any thing defined in it's class.
 *  And this header is left without protection intentionally.
 */
#define VISIABLE_MEMBER(member) member

#define Private(member) VISIABLE_MEMBER(member)
#define Protected(member) VISIABLE_MEMBER(member)

