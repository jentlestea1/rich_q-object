/*
 * QEMU Object Model
 *
 * Copyright IBM, Corp. 2011
 *
 * Authors:
 *  Anthony Liguori   <aliguori@us.ibm.com>
 *
 * This work is licensed under the terms of the GNU GPL, version 2 or later.
 * See the COPYING file in the top-level directory.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "object.h"

#define MAX_INTERFACES 32

typedef struct InterfaceImpl InterfaceImpl;
typedef struct TypeImpl TypeImpl;

struct InterfaceImpl
{
    const char *typename;
};

struct TypeImpl
{
    const char *name;

    size_t class_size;

    size_t instance_size;

    void (*class_init)(ObjectClass *klass, void *data);
    void (*class_base_init)(ObjectClass *klass, void *data);
    void (*class_finalize)(ObjectClass *klass, void *data);

    void *class_data;

    void (*instance_init)(Object *obj);
    void (*instance_post_init)(Object *obj);
    void (*instance_finalize)(Object *obj);

    bool abstract;

    const char *parent;
    TypeImpl *parent_type;

    ObjectClass *class;

    int num_interfaces;
    InterfaceImpl interfaces[MAX_INTERFACES];
};

static Type type_interface;

static GHashTable *type_table_get(void)
{
    static GHashTable *type_table;

    if (type_table == NULL) {
        type_table = g_hash_table_new(g_str_hash, g_str_equal);
    }

    return type_table;
}

static bool enumerating_types;

static void type_table_add(TypeImpl *ti)
{
    g_assert(!enumerating_types);
    g_hash_table_insert(type_table_get(), (void *)ti->name, ti);
}

static TypeImpl *type_table_lookup(const char *name)
{
    return g_hash_table_lookup(type_table_get(), name);
}

static TypeImpl *type_new(const TypeInfo *info)
{
    TypeImpl *ti = g_malloc0(sizeof(*ti));
    int i;

    g_assert(info->name != NULL);

    if (type_table_lookup(info->name) != NULL) {
        fprintf(stderr, "Registering `%s' which already exists\n", info->name);
        abort();
    }

    ti->name = g_strdup(info->name);
    ti->parent = g_strdup(info->parent);

    ti->class_size = info->class_size;
    ti->instance_size = info->instance_size;

    ti->class_init = info->class_init;
    ti->class_base_init = info->class_base_init;
    ti->class_finalize = info->class_finalize;
    ti->class_data = info->class_data;

    ti->instance_init = info->instance_init;
    ti->instance_post_init = info->instance_post_init;
    ti->instance_finalize = info->instance_finalize;

    ti->abstract = info->abstract;

    /* Warining the interfaces array should have a sentinel NULL*/
    for (i = 0; info->interfaces && info->interfaces[i].type; i++) {
        ti->interfaces[i].typename = g_strdup(info->interfaces[i].type);
    }
    ti->num_interfaces = i;

    return ti;
}

static TypeImpl *type_register_internal(const TypeInfo *info)
{
    TypeImpl *ti;
    ti = type_new(info);

    type_table_add(ti);
    return ti;
}

TypeImpl *type_register(const TypeInfo *info)
{
    g_assert(info->parent);
    return type_register_internal(info);
}

TypeImpl *type_register_static(const TypeInfo *info)
{
    return type_register(info);
}

void type_register_static_array(const TypeInfo *infos, int nr_infos)
{
    int i;

    for (i = 0; i < nr_infos; i++) {
        type_register_static(&infos[i]);
    }
}

static TypeImpl *type_get_by_name(const char *name)
{
    if (name == NULL) {
        return NULL;
    }

    return type_table_lookup(name);
}

static TypeImpl *type_get_parent(TypeImpl *type)
{
    if (!type->parent_type && type->parent) {
        type->parent_type = type_get_by_name(type->parent);
        g_assert(type->parent_type != NULL);
    }

    return type->parent_type;
}

static bool type_has_parent(TypeImpl *type)
{
    return (type->parent != NULL);
}

static size_t type_class_get_size(TypeImpl *ti)
{
    if (ti->class_size) {
        return ti->class_size;
    }

    if (type_has_parent(ti)) {
        return type_class_get_size(type_get_parent(ti));
    }

    return sizeof(ObjectClass);
}

static size_t type_object_get_size(TypeImpl *ti)
{
    if (ti->instance_size) {
        return ti->instance_size;
    }

    if (type_has_parent(ti)) {
        return type_object_get_size(type_get_parent(ti));
    }

    return 0;
}

size_t object_type_get_instance_size(const char *typename)
{
    TypeImpl *type = type_get_by_name(typename);

    g_assert(type != NULL);
    return type_object_get_size(type);
}

static bool type_is_ancestor(TypeImpl *type, TypeImpl *target_type)
{
    g_assert(target_type);

    /* Check if target_type is a direct ancestor of type */
    while (type) {
        if (type == target_type) {
            return true;
        }

        type = type_get_parent(type);
    }

    return false;
}

static void type_initialize(TypeImpl *ti);

static void type_initialize_interface(TypeImpl *ti, TypeImpl *interface_type,
                                      TypeImpl *parent_type)
{
    InterfaceClass *new_iface;
    TypeInfo info = { };
    TypeImpl *iface_impl;

    info.parent = parent_type->name;
    info.name = g_strdup_printf("%s::%s", ti->name, interface_type->name);
    info.abstract = true;

    iface_impl = type_new(&info);
    iface_impl->parent_type = parent_type;
    type_initialize(iface_impl);
    g_free((char *)info.name);

    new_iface = (InterfaceClass *)iface_impl->class;
    new_iface->concrete_class = ti->class;
    new_iface->interface_type = interface_type;

    ti->class->interfaces = g_slist_append(ti->class->interfaces,
                                           iface_impl->class);
}



static void type_initialize(TypeImpl *ti)
{
    TypeImpl *parent;

    if (ti->class) {
        return;
    }

    ti->class_size = type_class_get_size(ti);
    ti->instance_size = type_object_get_size(ti);
    /* Any type with zero instance_size is implicitly abstract.
     * This means interface types are all abstract.
     */
    if (ti->instance_size == 0) {
        ti->abstract = true;
    }

    ti->class = g_malloc0(ti->class_size);

    parent = type_get_parent(ti);
    if (parent) {
        /* If the derived class instance is created by calling object_new
         * the parent class then is uninitalized, so it is necessary to 
         * initialize the parent first.
         */
        type_initialize(parent);
        GSList *e;
        int i;

        g_assert_cmpint(parent->class_size, <=, ti->class_size);

        /* Important action, copy the class struct of parent, every derived
         * class has a unique copy of the class struct of their parent.
         */
        memcpy(ti->class, parent->class, parent->class_size);

        /* When initialized, it keeps interfaces both from parent and
         * its own.
         */
        ti->class->interfaces = NULL;

        ti->class->properties = g_hash_table_new_full(
            g_str_hash, g_str_equal, g_free, NULL);

        /* interfaces from parent */
        for (e = parent->class->interfaces; e; e = e->next) {
            InterfaceClass *iface = e->data;
            ObjectClass *klass = OBJECT_CLASS(iface);

            type_initialize_interface(ti, iface->interface_type, klass->type);
        }

        /* interfaces from the type its own */
        for (i = 0; i < ti->num_interfaces; i++) {
            TypeImpl *t = type_get_by_name(ti->interfaces[i].typename);
            for (e = ti->class->interfaces; e; e = e->next) {
                TypeImpl *target_type = OBJECT_CLASS(e->data)->type;

                if (type_is_ancestor(target_type, t)) {
                    break;
                }
            }

            if (e) {
                continue;
            }

            type_initialize_interface(ti, t, t);
        }
    } else {
        ti->class->properties = g_hash_table_new_full(
            g_str_hash, g_str_equal, g_free, NULL);
    }

    ti->class->type = ti;

    while (parent) {
        if (parent->class_base_init) {
            parent->class_base_init(ti->class, ti->class_data);
        }
        parent = type_get_parent(parent);
    }

    if (ti->class_init) {
        ti->class_init(ti->class, ti->class_data);
    }
}

static void object_init_with_type(Object *obj, TypeImpl *ti)
{
    if (type_has_parent(ti)) {
        object_init_with_type(obj, type_get_parent(ti));
    }

    if (ti->instance_init) {
        ti->instance_init(obj);
    }
}

static void object_post_init_with_type(Object *obj, TypeImpl *ti)
{
    if (ti->instance_post_init) {
        ti->instance_post_init(obj);
    }

    if (type_has_parent(ti)) {
        object_post_init_with_type(obj, type_get_parent(ti));
    }
}

static void object_initialize_with_type(void *data, size_t size, TypeImpl *type)
{
    Object *obj = data;

    g_assert(type != NULL);
    type_initialize(type);

    g_assert_cmpint(type->instance_size, >=, sizeof(Object));
    g_assert(type->abstract == false);
    g_assert_cmpint(size, >=, type->instance_size);

    memset(obj, 0, type->instance_size);
    obj->class = type->class;
    object_ref(obj);
    obj->properties = g_hash_table_new_full(g_str_hash, g_str_equal,
                                            NULL, NULL);
    object_init_with_type(obj, type);
    object_post_init_with_type(obj, type);
}

void object_initialize(void *data, size_t size, const char *typename)
{
    TypeImpl *type = type_get_by_name(typename);

    object_initialize_with_type(data, size, type);
}



static void object_deinit(Object *obj, TypeImpl *type)
{
    if (type->instance_finalize) {
        type->instance_finalize(obj);
    }

    if (type_has_parent(type)) {
        object_deinit(obj, type_get_parent(type));
    }
}

static void object_finalize(void *data)
{
    Object *obj = data;
    TypeImpl *ti = obj->class->type;

    // object_property_del_all(obj);
    object_deinit(obj, ti);

    g_assert_cmpint(obj->ref, ==, 0);
    if (obj->free) {
        obj->free(obj);
    }
}

static Object *object_new_with_type(Type type)
{
    Object *obj;

    g_assert(type != NULL);
    type_initialize(type);

    obj = g_malloc(type->instance_size);
    object_initialize_with_type(obj, type->instance_size, type);
    obj->free = g_free;

    return obj;
}

Object *object_new(const char *typename)
{
    TypeImpl *ti = type_get_by_name(typename);

    return object_new_with_type(ti);
}


Object *object_new_with_props(const char *typename,
                              Object *parent,
                              const char *id,
                              Error **errp,
                              ...)
{
    va_list vargs;
    Object *obj;

    va_start(vargs, errp);
    obj = object_new_with_propv(typename, parent, id, errp, vargs);
    va_end(vargs);

    return obj;
}


Object *object_new_with_propv(const char *typename,
                              Object *parent,
                              const char *id,
                              Error **errp,
                              va_list vargs)
{
    Object *obj;
    ObjectClass *klass;
    Error *local_err = NULL;

    klass = object_class_by_name(typename);
    if (!klass) {
        error_setg(errp, "invalid object type: %s", typename);
        return NULL;
    }

    if (object_class_is_abstract(klass)) {
        error_setg(errp, "object type '%s' is abstract", typename);
        return NULL;
    }
    obj = object_new(typename);

#if 0
    if (object_set_propv(obj, &local_err, vargs) < 0) {
        goto error;
    }

    object_property_add_child(parent, id, obj, &local_err);
    if (local_err) {
        goto error;
    }

    if (object_dynamic_cast(obj, TYPE_USER_CREATABLE)) {
        user_creatable_complete(obj, &local_err);
        if (local_err) {
            object_unparent(obj);
            goto error;
        }
    }
#endif
    object_unref(OBJECT(obj));
    return obj;

 error:
    error_propagate(errp, local_err);
    object_unref(obj);

    return NULL;
}


#if 0
int object_set_props(Object *obj,
                     Error **errp,
                     ...)
{
    va_list vargs;
    int ret;

    va_start(vargs, errp);
    ret = object_set_propv(obj, errp, vargs);
    va_end(vargs);

    return ret;
}
#endif

Object *object_dynamic_cast(Object *obj, const char *typename)
{
    if (obj && object_class_dynamic_cast(object_get_class(obj), typename)) {
        return obj;
    }

    return NULL;
}

Object *object_dynamic_cast_assert(Object *obj, const char *typename,
                                   const char *file, int line, const char *func)
{
#if 0
    trace_object_dynamic_cast_assert(obj ? obj->class->type->name : "(null)",
                                     typename, file, line, func);
#endif

#ifdef CONFIG_QOM_CAST_DEBUG
    int i;
    Object *inst;

    for (i = 0; obj && i < OBJECT_CLASS_CAST_CACHE; i++) {
        if (atomic_read(&obj->class->object_cast_cache[i]) == typename) {
            goto out;
        }
    }

    inst = object_dynamic_cast(obj, typename);

    if (!inst && obj) {
        fprintf(stderr, "%s:%d:%s: Object %p is not an instance of type %s\n",
                file, line, func, obj, typename);
        abort();
    }

    assert(obj == inst);

    if (obj && obj == inst) {
        for (i = 1; i < OBJECT_CLASS_CAST_CACHE; i++) {
            atomic_set(&obj->class->object_cast_cache[i - 1],
                       atomic_read(&obj->class->object_cast_cache[i]));
        }
        atomic_set(&obj->class->object_cast_cache[i - 1], typename);
    }

out:
#endif
    return obj;
}

ObjectClass *object_class_dynamic_cast(ObjectClass *class,
                                       const char *typename)
{
    ObjectClass *ret = NULL;
    TypeImpl *target_type;
    TypeImpl *type;

    if (!class) {
        return NULL;
    }

    /* A simple fast path that can trigger a lot for leaf classes.  */
    type = class->type;
    if (type->name == typename) {
        return class;
    }

    target_type = type_get_by_name(typename);
    if (!target_type) {
        /* target class type unknown, so fail the cast */
        return NULL;
    }

    /* If the type_interface is the ancestor of the target_type to be cast,
     * then we iterate through the interfaces to find the target class.
     * Otherwise, we just check whether the target_type is the ancestor of 
     * the type.
     */
    if (type->class->interfaces &&
            type_is_ancestor(target_type, type_interface)) {
        int found = 0;
        GSList *i;

        for (i = class->interfaces; i; i = i->next) {
            ObjectClass *target_class = i->data;

            if (type_is_ancestor(target_class->type, target_type)) {
                ret = target_class;
                found++;
            }
         }

        /* The match was ambiguous, don't allow a cast */
        if (found > 1) {
            ret = NULL;
        }
    } else if (type_is_ancestor(type, target_type)) {
        ret = class;
    }

    return ret;
}

ObjectClass *object_class_dynamic_cast_assert(ObjectClass *class,
                                              const char *typename,
                                              const char *file, int line,
                                              const char *func)
{
    ObjectClass *ret;

#if 0
    trace_object_class_dynamic_cast_assert(class ? class->type->name : "(null)",
                                           typename, file, line, func);
#endif

#ifdef CONFIG_QOM_CAST_DEBUG
    int i;

    for (i = 0; class && i < OBJECT_CLASS_CAST_CACHE; i++) {
        if (atomic_read(&class->class_cast_cache[i]) == typename) {
            ret = class;
            goto out;
        }
    }
#else
    if (!class || !class->interfaces) {
        return class;
    }
#endif

    ret = object_class_dynamic_cast(class, typename);
    if (!ret && class) {
        fprintf(stderr, "%s:%d:%s: Object %p is not an instance of type %s\n",
                file, line, func, class, typename);
        abort();
    }

#ifdef CONFIG_QOM_CAST_DEBUG
    if (class && ret == class) {
        for (i = 1; i < OBJECT_CLASS_CAST_CACHE; i++) {
            atomic_set(&class->class_cast_cache[i - 1],
                       atomic_read(&class->class_cast_cache[i]));
        }
        atomic_set(&class->class_cast_cache[i - 1], typename);
    }
out:
#endif
    return ret;
}

const char *object_get_typename(const Object *obj)
{
    return obj->class->type->name;
}

ObjectClass *object_get_class(Object *obj)
{
    return obj->class;
}

bool object_class_is_abstract(ObjectClass *klass)
{
    return klass->type->abstract;
}

const char *object_class_get_name(ObjectClass *klass)
{
    return klass->type->name;
}

ObjectClass *object_class_by_name(const char *typename)
{
    TypeImpl *type = type_get_by_name(typename);

    if (!type) {
        return NULL;
    }

    type_initialize(type);

    return type->class;
}

ObjectClass *object_class_get_parent(ObjectClass *class)
{
    TypeImpl *type = type_get_parent(class->type);

    if (!type) {
        return NULL;
    }

    type_initialize(type);

    return type->class;
}

typedef struct OCFData
{
    void (*fn)(ObjectClass *klass, void *opaque);
    const char *implements_type;
    bool include_abstract;
    void *opaque;
} OCFData;

static void object_class_foreach_tramp(gpointer key, gpointer value,
                                       gpointer opaque)
{
    OCFData *data = opaque;
    TypeImpl *type = value;
    ObjectClass *k;

    type_initialize(type);
    k = type->class;

    if (!data->include_abstract && type->abstract) {
        return;
    }

    if (data->implements_type && 
        !object_class_dynamic_cast(k, data->implements_type)) {
        return;
    }

    data->fn(k, data->opaque);
}

void object_class_foreach(void (*fn)(ObjectClass *klass, void *opaque),
                          const char *implements_type, bool include_abstract,
                          void *opaque)
{
    OCFData data = { fn, implements_type, include_abstract, opaque };

    enumerating_types = true;
    g_hash_table_foreach(type_table_get(), object_class_foreach_tramp, &data);
    enumerating_types = false;
}


static void object_class_get_list_tramp(ObjectClass *klass, void *opaque)
{
    GSList **list = opaque;

    *list = g_slist_prepend(*list, klass);
}

GSList *object_class_get_list(const char *implements_type,
                              bool include_abstract)
{
    GSList *list = NULL;

    object_class_foreach(object_class_get_list_tramp,
                         implements_type, include_abstract, &list);
    return list;
}

void object_ref(Object *obj)
{
    if (!obj) {
        return;
    }
    // atomic_inc(&obj->ref);
    obj->ref--;
}

void object_unref(Object *obj)
{
    if (!obj) {
        return;
    }
    g_assert_cmpint(obj->ref, >, 0);

    /* parent always holds a reference to its children */
    // atomic_fetch_dec(&obj->ref) 
    int ref = obj->ref--;
    if (ref == 1) {
        object_finalize(obj);
    }
}

static void object_instance_init(Object *obj)
{
    //object_property_add_str(obj, "type", qdev_get_type, NULL, NULL);
    printf("object instance init\n");
}

static void register_types(void)
{
    static TypeInfo interface_info = {
        .name = TYPE_INTERFACE,
        .class_size = sizeof(InterfaceClass),
        .abstract = true,
    };

    static TypeInfo object_info = {
        .name = TYPE_OBJECT,
        .instance_size = sizeof(Object),
        .instance_init = object_instance_init,
        .abstract = true,
    };

    printf("calling register_types\n");
    type_interface = type_register_internal(&interface_info);
    type_register_internal(&object_info);
}

void type_object_init(void){
    type_init(register_types);
}
