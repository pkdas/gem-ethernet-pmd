#ifndef __GEMU_LOG_H
#define __GEMU_LOG_H

#include <stdarg.h>

static inline void gemu_err(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
}

static inline void gemu_log(const char *format, ...)
{
#if 0
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
#endif
}

static inline void gemu_mm_err(const char *format, ...)
{
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
}

static inline void gemu_mm_log(const char *format, ...)
{
#if 0
  va_list ap;
  va_start(ap, format);
  vprintf(format, ap);
  va_end(ap);
#endif
}

#endif // __GEMU_LOG_H
