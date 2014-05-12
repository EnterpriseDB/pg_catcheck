/*-------------------------------------------------------------------------
 *
 * check_oids.c
 *
 * Many PostgreSQL system catalogs have OID, OID array, or OID vector
 * columns where each OID identifies a row in some other catalog table.
 * Although not marked as such, these are essentially foreign key
 * relationships.  The code in this file aims to validate that every
 * object referenced in such a column actually exists.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "pg_catcheck.h"

static void do_oid_check(pg_catalog_table *tab, pg_catalog_column *tabcol,
			 int rownum, pg_catalog_check_oid * check_oid,
			 pg_catalog_table *reftab, char *value);

/*
 * Set up for an OID referential integrity check.
 *
 * We simply need to make sure that the referenced table will be available.
 */
void
prepare_to_check_oid_reference(pg_catalog_table *tab,
							   pg_catalog_column *tabcol)
{
	pg_catalog_check_oid *check_oid = tabcol->check;
	pg_catalog_table *reftab;

	reftab = find_table_by_name(check_oid->oid_references_table);
	add_table_dependency(tab, reftab);
}

/*
 * Perform an OID referential integrity check.
 */
void
check_oid_reference(pg_catalog_table *tab, pg_catalog_column *tabcol,
					int rownum)
{
	pg_catalog_check_oid *check_oid = tabcol->check;
	char	   *val = PQgetvalue(tab->data, rownum, tabcol->result_column);
	pg_catalog_table *reftab;

	/*
	 * Find the hash table we need in order to perform the check.
	 *
	 * Since find_table_by_name is O(n) in the number of catalog tables being
	 * checked, we cache the result, so that we only need to do that work
	 * once.
	 */
	if (tabcol->check_private == NULL)
	{
		reftab = find_table_by_name(check_oid->oid_references_table);
		tabcol->check_private = reftab;
	}
	else
		reftab = tabcol->check_private;

	/*
	 * The table might not be available in this server version, or we might
	 * have failed to read it.	There's actually one real case where the
	 * referenced table was adding later than referring table: pg_largeobject
	 * has existed for a long time, but pg_largeobject_metadata is newer.
	 */
	if (!reftab->ht)
		return;

	switch (check_oid->type)
	{
		case CHECK_OID_REFERENCE:

			/*
			 * Simple OID reference.  Easy!
			 *
			 * We don't use do do_oid_check() here because the error message
			 * is a little different in this case.
			 */
			{
				if (check_oid->zero_oid_ok && strcmp(val, "0") == 0)
					return;
				if (pgrhash_get(reftab->ht, &val) == -1)
					pgcc_report(tab, tabcol, rownum,
							"no matching entry in %s\n", reftab->table_name);
			}
			break;

		case CHECK_OID_VECTOR_REFERENCE:
			/* Space-separated list of values. */
			{
				char	   *s = val;
				char		buf[32];

				for (;;)
				{
					/* Find next word boundary. */
					while (*s != '\0' && *s != ' ')
						++s;

					/* If it's the last word, we're done here! */
					if (*s == '\0')
					{
						if (s > val)
						{
							/* If last entry is non-empty, check it. */
							do_oid_check(tab, tabcol, rownum, check_oid,
										 reftab, val);
						}
						break;
					}

					/* Make a copy of the data we need to check. */
					if (s - val >= sizeof buf)
					{
						/* OIDs can't be this long. */
						pgcc_report(tab, tabcol, rownum,
							"contains a token of %ld characters\n", s - val);
						return;
					}
					memcpy(buf, val, s - val);
					buf[s - val] = '\0';

					/* Check it and move ahead one character. */
					do_oid_check(tab, tabcol, rownum, check_oid, reftab, buf);
					val = ++s;
				}
			}
			break;

		case CHECK_OID_ARRAY_REFERENCE:
			/* Opening curly brace, comma-separated values, closing brace. */
			{
				char	   *s = val;
				char		buf[32];
				bool		bad = false;

				/* Allow a completely empty field. */
				if (*s == '\0')
					break;

				/* Otherwise, expect the opening delimeter. */
				if (*s == '{')
					val = ++s;
				else
					bad = true;

				while (!bad)
				{
					/* Find next delimeter. */
					while (*s != '\0' && *s != ',' && *s != '}')
						++s;

					/*
					 * If we hit '\0' before '}', that's bad; and if we hit
					 * two consecutive delimeters, that's also bad.
					 */
					if (val == s || *s == '\0')
					{
						bad = true;
						break;
					}

					/* Make a copy of the data we need to check. */
					if (s - val >= sizeof buf)
					{
						/* OIDs can't be this long. */
						pgcc_report(tab, tabcol, rownum,
							"contains a token of %ld characters\n", s - val);
						return;
					}
					memcpy(buf, val, s - val);
					buf[s - val] = '\0';

					/* Check it. */
					do_oid_check(tab, tabcol, rownum, check_oid, reftab, buf);

					/* Expect end of string if at '}'. */
					if (*s == '}')
					{
						val = ++s;
						if (*s != '\0')
							bad = true;
						break;
					}

					/* Skip comma and continue. */
					val = ++s;
				}

				if (bad)
					pgcc_report(tab, tabcol, rownum, "not a valid 1-D array");
			}
			break;

		default:
			Assert(false);
			break;
	}
}

/*
 * Check one of possibly several OIDs found in a single column.
 */
static void
do_oid_check(pg_catalog_table *tab, pg_catalog_column *tabcol, int rownum,
			 pg_catalog_check_oid * check_oid,
			 pg_catalog_table *reftab, char *value)
{
	if (check_oid->zero_oid_ok && strcmp(value, "0") == 0)
		return;
	if (pgrhash_get(reftab->ht, &value) == -1)
		pgcc_report(tab, tabcol, rownum,
					"\"%s\" not found in %s\n", value, reftab->table_name);
}
