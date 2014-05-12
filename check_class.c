/*-------------------------------------------------------------------------
 *
 * check_class.c
 *
 * Custom checks for pg_class fields.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "pg_catcheck.h"

typedef struct
{
	pg_catalog_table *pg_attribute;
	int			oid_result_column;
} relnatts_cache;

/*
 * Set up to check relnatts.
 */
void
prepare_to_check_relnatts(pg_catalog_table *tab, pg_catalog_column *tabcol)
{
	add_table_dependency(tab, find_table_by_name("pg_attribute"));
}

/*
 * Sanity-check the relnatts field.
 */
void
check_relnatts(pg_catalog_table *tab, pg_catalog_column *tabcol, int rownum)
{
	char	   *val = PQgetvalue(tab->data, rownum, tabcol->result_column);
	char	   *endptr;
	relnatts_cache *cache;
	long		relnatts;
	int			attno;
	char		buf[512];
	char	   *keys[2];

	/* Convert the value to a number. */
	relnatts = strtol(val, &endptr, 10);
	if (*endptr != '\0' || val < 0)
		pgcc_report(tab, tabcol, rownum, "must be a non-negative integer\n");

	/* Find the pg_attribute table; cache result in check_private. */
	if (tabcol->check_private == NULL)
	{
		cache = pg_malloc(sizeof(relnatts_cache));
		cache->pg_attribute = find_table_by_name("pg_attribute");
		cache->oid_result_column = PQfnumber(tab->data, "oid");
		tabcol->check_private = cache;
	}
	else
		cache = tabcol->check_private;

	/*
	 * Skip detailed checking if pg_attribute data is not available, or if the
	 * oid column of pg_class is not available.
	 */
	if (cache->pg_attribute->ht == NULL || cache->oid_result_column == -1)
		return;

	/* Set up for pg_attribute hash table probes. */
	keys[0] = PQgetvalue(tab->data, rownum, cache->oid_result_column);
	keys[1] = buf;

	/*
	 * Check that all positive-numbered attributes we expect to find are in
	 * fact present.
	 *
	 * TODO: We could check for negative-numbered attributes as well, but
	 * whether or not those are present will depend on relkind inter alia.
	 */
	for (attno = 1; attno <= relnatts; ++attno)
	{
		snprintf(buf, sizeof buf, "%d", attno);
		if (pgrhash_get(cache->pg_attribute->ht, keys) == -1)
			pgcc_report(tab, tabcol, rownum,
						"attribute %d does not exist in pg_attribute\n",
						attno);
	}
}
