/*
 * Copyright (C) 2010-2020 Ole André Vadla Ravnås <oleavr@nowsecure.com>
 *
 * Licence: wxWindows Library Licence, Version 3.1
 */

#include "memoryaccessmonitor-fixture.c"

TESTLIST_BEGIN (memoryaccessmonitor)
  TESTENTRY (notify_on_read_access)
  TESTENTRY (notify_on_write_access)
  TESTENTRY (notify_on_execute_access)
  TESTENTRY (notify_should_include_progress)
  TESTENTRY (disable)
TESTLIST_END ()

TESTCASE (notify_on_read_access)
{
  volatile guint8 * bytes = GSIZE_TO_POINTER (fixture->range.base_address);
  guint8 val;
  volatile GumMemoryAccessDetails * d = &fixture->last_details;
  GumThreadId thread_id;

  bytes[fixture->offset_in_first_page] = 0x13;
  bytes[fixture->offset_in_second_page] = 0x37;

  ENABLE_MONITOR ();
  val = bytes[fixture->offset_in_first_page];

  thread_id = gum_process_get_current_thread_id ();
  g_assert_cmpuint (d->thread_id, ==, thread_id);

  g_assert_cmpuint (fixture->number_of_notifies, ==, 1);
  g_assert_cmpint (d->operation, ==, GUM_MEMOP_READ);
  g_assert_true (d->from != NULL && d->from != d->address);

  g_assert_true (d->address == bytes + fixture->offset_in_first_page);
  g_assert_cmpuint (val, ==, 0x13);

  val = bytes[fixture->offset_in_first_page];
  g_assert_cmpuint (fixture->number_of_notifies, ==, 1);
  g_assert_cmpuint (val, ==, 0x13);

  val = bytes[fixture->offset_in_second_page];
  g_assert_cmpuint (fixture->number_of_notifies, ==, 2);
  g_assert_cmpint (d->operation, ==, GUM_MEMOP_READ);
  g_assert_true (d->from != NULL && d->from != d->address);
  g_assert_true (d->address == bytes + fixture->offset_in_second_page);
  g_assert_cmpuint (val, ==, 0x37);

  val = bytes[fixture->offset_in_second_page];
  g_assert_cmpuint (fixture->number_of_notifies, ==, 2);
  g_assert_cmpuint (val, ==, 0x37);

  /*
   * Perform some basic architecture agnostic checks on the Cpu Context, the
   * program counter should match the `from` field and the stack pointer
   * should be non-zero.
   */
#if defined (HAVE_I386) && GLIB_SIZEOF_VOID_P == 4
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->eip));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->esp));
#elif defined (HAVE_I386) && GLIB_SIZEOF_VOID_P == 8
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->rip));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->rsp));
#else
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->pc));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->sp));
#endif
}

TESTCASE (notify_on_write_access)
{
  volatile guint8 * bytes = GSIZE_TO_POINTER (fixture->range.base_address);
  guint8 val;
  volatile GumMemoryAccessDetails * d = &fixture->last_details;
  GumThreadId thread_id;

  bytes[fixture->offset_in_first_page] = 0x13;

  ENABLE_MONITOR ();

  bytes[fixture->offset_in_first_page] = 0x14;

  thread_id = gum_process_get_current_thread_id ();
  g_assert_cmpuint (d->thread_id, ==, thread_id);

  g_assert_cmpuint (fixture->number_of_notifies, ==, 1);
  g_assert_cmpint (d->operation, ==, GUM_MEMOP_WRITE);
  g_assert_true (d->from != NULL && d->from != d->address);
  g_assert_true (d->address == bytes + fixture->offset_in_first_page);

  val = bytes[fixture->offset_in_first_page];
  g_assert_cmpuint (fixture->number_of_notifies, ==, 1);
  g_assert_cmpuint (val, ==, 0x14);

  /*
   * Perform some basic architecture agnostic checks on the Cpu Context, the
   * program counter should match the `from` field and the stack pointer
   * should be non-zero.
   */
#if defined (HAVE_I386) && GLIB_SIZEOF_VOID_P == 4
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->eip));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->esp));
#elif defined (HAVE_I386) && GLIB_SIZEOF_VOID_P == 8
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->rip));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->rsp));
#else
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->pc));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->sp));
#endif
}

TESTCASE (notify_on_execute_access)
{
  volatile GumMemoryAccessDetails * d = &fixture->last_details;
  GumThreadId thread_id;

  ENABLE_MONITOR ();

  fixture->nop_function_in_third_page ();

  thread_id = gum_process_get_current_thread_id ();
  g_assert_cmpuint (d->thread_id, ==, thread_id);

  g_assert_cmpuint (fixture->number_of_notifies, ==, 1);
  g_assert_cmpint (d->operation, ==, GUM_MEMOP_EXECUTE);
  g_assert_true (d->from != NULL && d->from == d->address);

  fixture->nop_function_in_third_page ();
  g_assert_cmpuint (fixture->number_of_notifies, ==, 1);

  /*
   * Perform some basic architecture agnostic checks on the Cpu Context, the
   * program counter should match the `from` field and the stack pointer
   * should be non-zero.
   */
#if defined (HAVE_I386) && GLIB_SIZEOF_VOID_P == 4
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->eip));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->esp));
#elif defined (HAVE_I386) && GLIB_SIZEOF_VOID_P == 8
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->rip));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->rsp));
#else
  g_assert_true (d->from == GSIZE_TO_POINTER (d->context->pc));
  g_assert_true (0 != GSIZE_TO_POINTER (d->context->sp));
#endif
}

TESTCASE (notify_should_include_progress)
{
  volatile GumMemoryAccessDetails * d = &fixture->last_details;
  volatile guint8 * bytes = GSIZE_TO_POINTER (fixture->range.base_address);

  g_assert_cmpuint (d->page_index, ==, 0);
  g_assert_cmpuint (d->pages_completed, ==, 0);
  g_assert_cmpuint (d->pages_total, ==, 0);

  ENABLE_MONITOR ();

  bytes[fixture->offset_in_second_page] = 0x37;
  g_assert_cmpuint (d->page_index, ==, 1);
  g_assert_cmpuint (d->pages_completed, ==, 1);
  g_assert_cmpuint (d->pages_total, ==, 3);

  bytes[fixture->offset_in_first_page] = 0x13;
  g_assert_cmpuint (d->page_index, ==, 0);
  g_assert_cmpuint (d->pages_completed, ==, 2);
  g_assert_cmpuint (d->pages_total, ==, 3);
}

TESTCASE (disable)
{
  volatile guint8 * bytes = GSIZE_TO_POINTER (fixture->range.base_address);
  guint8 val;

  bytes[fixture->offset_in_first_page] = 0x13;
  bytes[fixture->offset_in_second_page] = 0x37;

  ENABLE_MONITOR ();
  DISABLE_MONITOR ();

  val = bytes[fixture->offset_in_first_page];
  g_assert_cmpuint (fixture->number_of_notifies, ==, 0);
  g_assert_cmpuint (val, ==, 0x13);

  val = bytes[fixture->offset_in_second_page];
  g_assert_cmpuint (fixture->number_of_notifies, ==, 0);
  g_assert_cmpuint (val, ==, 0x37);
}
