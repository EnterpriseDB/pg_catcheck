/*-------------------------------------------------------------------------
 *
 * compat.c
 *	  compatibility definitions for older server versions
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "pg_catcheck.h"

#if PG_VERSION_NUM < 90300

/* 9.3 and higher have this in fe_memutils.c */
void *
pg_malloc(size_t size)
{
	void	   *tmp;

	/* Avoid unportable behavior of malloc(0) */
	if (size == 0)
		size = 1;
	tmp = malloc(size);
	if (!tmp)
	{
		fprintf(stderr, _("out of memory\n"));
		exit(EXIT_FAILURE);
	}
	return tmp;
}

/* 9.3 and higher have this in fe_memutils.c */
void *
pg_malloc0(size_t size)
{
	void	   *tmp;

	tmp = pg_malloc(size);
	MemSet(tmp, 0, size);
	return tmp;
}

/* 9.3 and higher have this in fe_memutils.c */
void *
pg_realloc(void *ptr, size_t size)
{
	void	   *tmp;

	/* Avoid unportable behavior of realloc(NULL, 0) */
	if (ptr == NULL && size == 0)
		size = 1;
	tmp = realloc(ptr, size);
	if (!tmp)
	{
		fprintf(stderr, _("out of memory\n"));
		exit(EXIT_FAILURE);
	}
	return tmp;
}

/* 9.3 and higher have this in fe_memutils.c */
char *
pg_strdup(const char *in)
{
	char	   *tmp;

	if (!in)
	{
		fprintf(stderr,
				_("cannot duplicate null pointer (internal error)\n"));
		exit(EXIT_FAILURE);
	}
	tmp = strdup(in);
	if (!tmp)
	{
		fprintf(stderr, _("out of memory\n"));
		exit(EXIT_FAILURE);
	}
	return tmp;
}

/* 9.3 and higher have this in fe_memutils.c */
void
pg_free(void *ptr)
{
	if (ptr != NULL)
		free(ptr);
}

#endif
