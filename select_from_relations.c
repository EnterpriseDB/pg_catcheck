/*-------------------------------------------------------------------------
 *
 * select_from_relations.c
 *
 * Try to select from relations with storage. This will fail if the
 * underlying files are absent or inaccessible. This is a little outside
 * the general remit of this tool, which is to check the integrity of
 * the system catalogs, but it seems like a useful addition.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "pg_catcheck.h"
#include "pqexpbuffer.h"

/*
 * Set up to check SELECT from relations.
 */
void
prepare_to_select_from_relations(void)
{
	pg_catalog_table *pg_class = find_table_by_name("pg_class");
	pg_catalog_table *pg_namespace = find_table_by_name("pg_namespace");

	/* Flag tables that must be loaded for this check. */
	pg_class->needs_check = true;
	pg_class->needs_load = true;
	pg_namespace->needs_load = true;
	pg_namespace->needs_check = true;

	/* Flag columns that must be loaded for this check. */
	find_column_by_name(pg_namespace, "nspname")->needed = true;
	find_column_by_name(pg_class, "relname")->needed = true;
	find_column_by_name(pg_class, "relnamespace")->needed = true;
	find_column_by_name(pg_class, "relkind")->needed = true;
}

/*
 * Try a SELECT from each relation.
 *
 * We use SELECT 0 here to make it fast; we're just trying to verify
 * that selecting data from the relation doesn't fail outright.
 */
void
perform_select_from_relations(PGconn *conn)
{
	PQExpBuffer query;
	char	   *tablename,
			   *nspname,
			   *nspoid;
	int			rownum;
	int			ntups;
	int			oid_result_column;
	int			relname_result_column;
	int			relnamespace_result_column;
	int			relkind_result_column;
	int			nspname_result_column;
	pg_catalog_table *pg_class = find_table_by_name("pg_class");
	pg_catalog_table *pg_namespace = find_table_by_name("pg_namespace");

	/*
	 * If we weren't able to retrieve the table data for either table, then
	 * we can't run these checks.
	 */
	if (PQresultStatus(pg_class->data) != PGRES_TUPLES_OK ||
		PQresultStatus(pg_namespace->data) != PGRES_TUPLES_OK)
		return;

	/* Locate the data we need. */
	ntups = PQntuples(pg_class->data);
	oid_result_column = PQfnumber(pg_class->data, "oid");
	relname_result_column = PQfnumber(pg_class->data, "relname");
	relnamespace_result_column = PQfnumber(pg_class->data, "relnamespace");
	relkind_result_column = PQfnumber(pg_class->data, "relkind");
	nspname_result_column = PQfnumber(pg_namespace->data, "nspname");

	query = createPQExpBuffer();

	/* Loop over the rows and check them. */
	for (rownum = 0; rownum < ntups; ++rownum)
	{
		char	relkind;
		int		nsp_rownum;
		PGresult	   *qryres;

		/* Check plain tables, toast tables, and materialized views. */
		relkind = *(PQgetvalue(pg_class->data, rownum, relkind_result_column));
		if (relkind != 'r' && relkind != 't' && relkind != 'm')
			continue;

		/* Get the table name and namespace OID from the pg_class */
		tablename = PQgetvalue(pg_class->data, rownum, relname_result_column);
		nspoid = PQgetvalue(pg_class->data, rownum, relnamespace_result_column);

		/*
		 * Get the namespace name for the given namespace OID. Any errors here
		 * have already been reported, so we just emit a debug message here.
		 */
		nsp_rownum = pgrhash_get(pg_namespace->ht, &nspoid);
		if (nsp_rownum == -1)
		{
			pgcc_log(PGCC_DEBUG,
					 "can't find schema name for select query for table with OID %s\n",
					 PQgetvalue(pg_class->data, rownum, oid_result_column));
			continue;
		}
		nspname = PQgetvalue(pg_namespace->data, nsp_rownum, nspname_result_column);

		/* Debug message. */
		pgcc_log(PGCC_DEBUG, "selecting from \"%s\".\"%s\"\n", nspname, tablename);

		/* Build up a query. */
		resetPQExpBuffer(query);
		appendPQExpBuffer(query, "SELECT 1 FROM %s.%s LIMIT 0",
						  PQescapeIdentifier(conn, nspname, strlen(nspname)),
						  PQescapeIdentifier(conn, tablename, strlen(tablename)));

		/* Run the query. */
		qryres = PQexec(conn, query->data);
		if (PQresultStatus(qryres) != PGRES_TUPLES_OK)
			pgcc_log(PGCC_NOTICE,
					 "unable to query relation \"%s\".\"%s\": %s",
					 nspname, tablename, PQerrorMessage(conn));

		/* Clean up. */
		PQclear(qryres);
	}
	destroyPQExpBuffer(query);
}
