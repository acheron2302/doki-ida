//----------------------------------------------------------------------------
// Doki Theme - logging helpers.
// All console output is prefixed with "[doki] " (cross-cutting requirement).
//----------------------------------------------------------------------------
#pragma once

// kernwin.hpp does not pull its own prerequisites; pro.h provides idaapi,
// ea_t, BADADDR and AS_PRINTF that it relies on. Include it first so this
// header is usable from any translation unit.
#include <pro.h>
#include <kernwin.hpp>

#ifndef DOKI_VERSION
#  define DOKI_VERSION "0.1.0"
#endif

namespace doki
{

// Always-on, prefixed message.
AS_PRINTF(1, 2) inline void msg_log(const char *format, ...)
{
  va_list va;
  va_start(va, format);
  ::msg("[doki] ");
  ::vmsg(format, va);
  va_end(va);
}

// Verbose log, gated behind a debug flag (toggled in a later phase).
extern bool g_verbose;

AS_PRINTF(1, 2) inline void dbg_log(const char *format, ...)
{
  if ( !g_verbose )
    return;
  va_list va;
  va_start(va, format);
  ::msg("[doki][dbg] ");
  ::vmsg(format, va);
  va_end(va);
}

} // namespace doki
