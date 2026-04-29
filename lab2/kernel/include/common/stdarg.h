#ifndef __STDARG_H__
#define __STDARG_H__

typedef char * va_list;

#define _INTSIZEOF(n)   ( (sizeof(n) + sizeof(int) - 1) & ~(sizeof(int) - 1) )
#define va_start(ap, format) ( ap = (va_list)&format + _INTSIZEOF(format) )
#define va_arg(ap, type) ( *(type*)((ap += _INTSIZEOF(type)) - _INTSIZEOF(type)) )
#define va_end(ap)  ( ap = (va_list)0 )

#endif
