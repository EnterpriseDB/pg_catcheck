/*-------------------------------------------------------------------------
 *
 * check_attribute.c
 *
 * Custom checks for pg_attribute fields.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "pg_catcheck.h"
#include "catalog/pg_attribute.h"

typedef struct
{
	pg_catalog_table *pg_class;
	int			attrelid_result_column;
	int			relnatts_result_column;
} attnum_cache;

/*
 * Set up to check attnum.
 */
void
prepare_to_check_attnum(pg_catalog_table *tab, pg_catalog_column *tabcol)
{
	add_table_dependency(tab, find_table_by_name("pg_class"));
}

/*
 * Sanity-check the relnatts field.
 */
void
check_attnum(pg_catalog_table *tab, pg_catalog_column *tabcol, int rownum)
{
	char	   *val = PQgetvalue(tab->data, rownum, tabcol->result_column);
	char	   *attrelid_val;
	char	   *relnatts_val;
	char	   *endptr;
	attnum_cache *cache;
	int			class_rownum;
	long		attnum;
	long		relnatts;
	long		min_attno;

	/* Convert the value to a number. */
	attnum = strtol(val, &endptr, 10);
	if (*endptr != '\0')
	{
		pgcc_report(tab, tabcol, rownum, "must be an integer\n");
		return;
	}

	/* Our attribute number should not be zero. */
	if (attnum == 0)
	{
		pgcc_report(tab, tabcol, rownum, "must not be zero\n");
		return;
	}

	/* And it should be at least -7 for PostgreSQL, -8 for EnterpriseDB. */
	min_attno = remote_is_edb ? -8 : -7;
	if (attnum < min_attno)
	{
		pgcc_report(tab, tabcol, rownum, "must be at least %ld\n",
					min_attno);
		return;
	}

	/* Find the pg_attribute table; cache result in check_private. */
	if (tabcol->check_private == NULL)
	{
		cache = pg_malloc(sizeof(attnum_cache));
		cache->pg_class = find_table_by_name("pg_class");
		cache->attrelid_result_column = PQfnumber(tab->data, "attrelid");
		cache->relnatts_result_column = PQfnumber(cache->pg_class->data,
												  "relnatts");
		tabcol->check_private = cache;
	}
	else
		cache = tabcol->check_private;

	/*
	 * Skip max-bound checking if the pg_class data is not available, or if
	 * the pg_class.relnatts or pg_attribute.attrelid column is not available.
	 */
	if (cache->pg_class->ht == NULL || cache->relnatts_result_column == -1 ||
		cache->attrelid_result_column == -1)
		return;

	/* Find row number of this table in pg_class. */
	attrelid_val = PQgetvalue(tab->data, rownum,
							  cache->attrelid_result_column);
	class_rownum = pgrhash_get(cache->pg_class->ht, &attrelid_val);
	if (class_rownum == -1)
		return;					/* It's not our job to complain about
								 * attrelid. */

	/* Get relnatts, as a number. */
	relnatts_val = PQgetvalue(cache->pg_class->data, class_rownum,
							  cache->relnatts_result_column);
	relnatts = strtol(relnatts_val, &endptr, 10);
	if (*endptr != '\0' || relnatts < 0)
		return;					/* It's not our job to complain about
								 * relnatts. */

	/* Our attribute number should be less than relnatts. */
	if (attnum > relnatts)
		pgcc_report(tab, tabcol, rownum,
					"exceeds relnatts value of %ld\n",
					relnatts);
}
