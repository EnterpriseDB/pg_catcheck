/*-------------------------------------------------------------------------
 *
 * compat.h
 *
 * Compatibility with older PostgreSQL source trees.
 *
 *-------------------------------------------------------------------------
 */

#ifndef COMPAT_H
#define COMPAT_H

#if PG_VERSION_NUM < 90300

/* Just disable assertions for builds against older source trees. */
#define Assert(p)

/* Convenience routines for frontend memory allocation were added in 9.3. */
extern char *pg_strdup(const char *in);
extern void *pg_malloc(size_t size);
extern void *pg_malloc0(size_t size);
extern void *pg_realloc(void *pointer, size_t size);
extern void pg_free(void *pointer);

#endif /* PG_VERSION_NUM < 90300 */

#if PG_VERSION_NUM < 90100

/*
 * PG_PRINTF_ATTRIBUTE is essential for suppressing compiler warning from
 * printf-like functions, but it wasn't added until 9.1.
 */
#ifdef WIN32
#define PG_PRINTF_ATTRIBUTE gnu_printf
#else
#define PG_PRINTF_ATTRIBUTE printf
#endif

#endif

#endif /* COMPAT_H */
