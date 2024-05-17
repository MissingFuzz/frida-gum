/*
 * Copyright (C) 2022 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "gumquicksampler.h"

#include "gumquickmacros.h"

#include <gum/gum-prof.h>

GUMJS_DECLARE_CONSTRUCTOR (gumjs_sampler_construct)
GUMJS_DECLARE_FUNCTION (gumjs_sampler_sample)
GUMJS_DECLARE_FINALIZER (gumjs_sampler_finalize)

GUMJS_DECLARE_CONSTRUCTOR (gumjs_wallclock_sampler_construct)

static const JSClassDef gumjs_file_def =
{
  .class_name = "Sampler",
  .finalizer = gumjs_sampler_finalize,
};

static const JSCFunctionListEntry gumjs_sampler_functions[] =
{
  JS_CFUNC_DEF ("sample", 0, gumjs_sampler_sample),
};

static const JSClassDef gumjs_native_wallclock_stream_def =
{
  .class_name = "WallClockSampler",
};

void
_gum_quick_sampler_init (GumQuickSampler * self,
                         JSValue ns,
                         GumQuickCore * core)
{
  JSContext * ctx = core->ctx;
  JSValue proto, ctor;

  self->core = core;

  _gum_quick_core_store_module_data (core, "sampler", self);

  _gum_quick_create_class (ctx, &gumjs_file_def, core, &self->sampler_class,
      &proto);
  ctor = JS_NewCFunction2 (ctx, gumjs_sampler_construct,
      gumjs_file_def.class_name, 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor (ctx, ctor, proto);
  JS_SetPropertyFunctionList (ctx, proto, gumjs_sampler_functions,
      G_N_ELEMENTS (gumjs_sampler_functions));
  JS_DefinePropertyValueStr (ctx, ns, gumjs_file_def.class_name, ctor,
      JS_PROP_C_W_E);

  _gum_quick_create_subclass (ctx, &gumjs_native_wallclock_stream_def,
      self->sampler_class, proto, core,
      &self->wallclock_sampler_class, &proto);
  ctor = JS_NewCFunction2 (ctx, gumjs_wallclock_sampler_construct,
      gumjs_native_wallclock_stream_def.class_name, 0, JS_CFUNC_constructor, 0);
  JS_SetConstructor (ctx, ctor, proto);
  JS_DefinePropertyValueStr (ctx, ns,
      gumjs_native_wallclock_stream_def.class_name, ctor, JS_PROP_C_W_E);

  _gum_quick_object_manager_init (&self->objects, self, core);
}

void
_gum_quick_sampler_flush (GumQuickSampler * self)
{
  _gum_quick_object_manager_flush (&self->objects);
}

void
_gum_quick_sampler_dispose (GumQuickSampler * self)
{
  _gum_quick_object_manager_free (&self->objects);
}

void
_gum_quick_sampler_finalize (GumQuickSampler * self)
{
}

GUMJS_DEFINE_FINALIZER (gumjs_sampler_finalize)
{
}

GUMJS_DEFINE_CONSTRUCTOR (gumjs_sampler_construct)
{
  return _gum_quick_throw_literal (ctx, "not user-instantiable");
}

static GumQuickSampler *
gumjs_get_parent_module (GumQuickCore * core)
{
  return _gum_quick_core_load_module_data (core, "sampler");
}

static gboolean
gum_sampler_get (JSContext * ctx,
                 JSValueConst val,
                 GumQuickCore * core,
                 GumSampler ** sampler)
{
  return _gum_quick_unwrap (ctx, val,
      gumjs_get_parent_module (core)->sampler_class, core,
      (gpointer *) sampler);
}

GUMJS_DEFINE_FUNCTION (gumjs_sampler_sample)
{
  GumSampler * self;
  GumSample sample;

  if (!gum_sampler_get (ctx, this_val, core, &self))
    return JS_EXCEPTION;

  sample = gum_sampler_sample (self);

  return _gum_quick_uint64_new (ctx, sample, core);
}

GUMJS_DEFINE_CONSTRUCTOR (gumjs_wallclock_sampler_construct)
{
  GumQuickSampler * parent;
  GumSampler * sampler;

  parent = gumjs_get_parent_module (core);

  JSValue wrapper = JS_NewObjectClass (ctx, parent->sampler_class);

  sampler = gum_wallclock_sampler_new ();

  _gum_quick_object_manager_add (&parent->objects, ctx, wrapper, sampler);

  JS_SetOpaque (wrapper, sampler);

  return wrapper;
}