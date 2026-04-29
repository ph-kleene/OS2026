#ifndef __ASSERT_H__
#define __ASSERT_H__

int abort(const char *, int);

/* assert: If condition is false, abort (BSOD) */
#define assert(cond) \
	((cond) ? (0) : (abort(__FILE__, __LINE__)))

#endif
