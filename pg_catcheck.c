/*-------------------------------------------------------------------------
 *
 * pg_catcheck.c
 *
 * Driver code for the system catalog integrity checker.  Options parsing
 * logic as well as code to connect to the database and build and execute
 * SQL queries live here, as does other management code that is used to
 * plan and drive the flow of the checks.  The checks themselves, however,
 * are not defined here.
 *
 * This tool only attempts to detect logical errors (like a dependency in
 * pg_depend that points to a non-existent logic), not lower-level
 * corruption scenarios (like an index that doesn't match the table).
 * Nevertheless, we attempt to be resilient against the possible presence
 * of such scenarios by issuing just one query per table fetching only
 * the columns we need, and continuing on so far as possible even if some
 * queries fail.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "getopt_long.h"
#include "pqexpbuffer.h"
#include "libpq-fe.h"

#include "pg_catcheck.h"
#include <ctype.h>

extern char *optarg;
extern int	optind;

/* variable definitions */
static char *pghost = "";
static char *pgport = "";
static char *login = NULL;
static char *dbName;
const char *progname;
int			remote_version;
bool		remote_is_edb;
char	   *database_oid;

#define MINIMUM_SUPPORTED_VERSION				80400

/* Static functions */
static int	parse_target_version(char *version);
static void select_column(char *column_name, enum trivalue whether);
static void select_table(char *table_name, enum trivalue whether);
static PGconn *do_connect(void);
static void decide_what_to_check(bool selected_columns);
static void perform_checks(PGconn *conn);
static void load_table(PGconn *conn, pg_catalog_table *tab);
static void check_table(PGconn *conn, pg_catalog_table *tab);
static PQExpBuffer build_query_for_table(pg_catalog_table *tab);
static void build_hash_from_query_results(pg_catalog_table *tab);
static void usage(void);
static char *get_database_oid(PGconn *conn);

/*
 * Main program.
 */
int
main(int argc, char **argv)
{
	static struct option long_options[] = {
		/* systematic long/short named options */
		{"host", required_argument, NULL, 'h'},
		{"port", required_argument, NULL, 'p'},
		{"username", required_argument, NULL, 'U'},
		{"table", required_argument, NULL, 't'},
		{"column", required_argument, NULL, 'c'},
		{"quiet", no_argument, NULL, 'q'},
		{"verbose", no_argument, NULL, 'v'},
		{"target-version", required_argument, NULL, 101},
		{"enterprisedb", no_argument, NULL, 102},
		{"postgresql", no_argument, NULL, 103},
		{NULL, 0, NULL, 0}
	};

	int			c;
	int			optindex;
	char	   *env;
	PGconn	   *conn;
	int			target_version = 0;
	bool		detect_edb = true;
	bool		selected_columns = false;

	progname = get_progname(argv[0]);

	if (argc > 1)
	{
		if (strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-?") == 0)
		{
			usage();
			exit(0);
		}
		if (strcmp(argv[1], "--version") == 0 || strcmp(argv[1], "-V") == 0)
		{
			puts("pg_catcheck (EnterpriseDB) " PG_VERSION);
			exit(0);
		}
	}

#ifdef WIN32
	/* stderr is buffered on Win32. */
	setvbuf(stderr, NULL, _IONBF, 0);
#endif

	if ((env = getenv("PGHOST")) != NULL && *env != '\0')
		pghost = env;
	if ((env = getenv("PGPORT")) != NULL && *env != '\0')
		pgport = env;
	else if ((env = getenv("PGUSER")) != NULL && *env != '\0')
		login = env;

	while ((c = getopt_long(argc, argv, "h:p:U:t:T:g:c:C:qv", long_options, &optindex)) != -1)
	{
		switch (c)
		{
			case 'h':
				pghost = pg_strdup(optarg);
				break;
			case 'p':
				pgport = pg_strdup(optarg);
				break;
			case 'c':
				select_column(optarg, TRI_YES);
				selected_columns = true;
				break;
			case 'C':
				select_column(optarg, TRI_NO);
				break;
			case 't':
				select_table(optarg, TRI_YES);
				selected_columns = true;
				break;
			case 'T':
				select_table(optarg, TRI_NO);
				break;
			case 'U':
				login = pg_strdup(optarg);
				break;
			case 'q':
				quiet = true;
				break;
			case 'v':
				++verbose;
				break;
			case 101:
				target_version = parse_target_version(optarg);
				break;
			case 102:
				remote_is_edb = true;
				detect_edb = false;
				break;
			case 103:
				remote_is_edb = false;
				detect_edb = false;
				break;
			default:
				fprintf(stderr, _("Try \"%s --help\" for more information.\n"), progname);
				exit(1);
				break;
		}
	}

	if (argc > optind)
		dbName = argv[optind];
	else
	{
		if ((env = getenv("PGDATABASE")) != NULL && *env != '\0')
			dbName = env;
		else if (login != NULL && *login != '\0')
			dbName = login;
		else
			dbName = "";
	}

	/* opening connection... */
	conn = do_connect();
	if (conn == NULL)
		exit(1);

	if (PQstatus(conn) == CONNECTION_BAD)
		pgcc_log(PGCC_FATAL, "could not connect to server: %s",
				 PQerrorMessage(conn));

	/* Detect remote version, or user user-specified value. */
	if (target_version == 0)
	{
		remote_version = PQserverVersion(conn);
		pgcc_log(PGCC_VERBOSE, "detected server version %d\n",
				 remote_version);
	}
	else
	{
		remote_version = target_version;
		pgcc_log(PGCC_VERBOSE, "assuming server version %d\n",
				 remote_version);
	}

	/* Warn that we don't support checking really old versions. */
	if (remote_version < MINIMUM_SUPPORTED_VERSION)
		pgcc_log(PGCC_WARNING, "server version (%d) is older than the minimum version supported by this tool (%d)\n",
				 remote_version, MINIMUM_SUPPORTED_VERSION);

	/*
	 * If neither --enterprisedb nor --postgresql was specified, attempt to
	 * detect which type of database we're accessing.
	 */
	if (detect_edb)
	{
		PGresult   *res;

		res = PQexec(conn, "select strpos(version(), 'EnterpriseDB')");
		if (PQresultStatus(res) != PGRES_TUPLES_OK)
		{
			pgcc_log(PGCC_ERROR, "query failed: %s", PQerrorMessage(conn));
			PQclear(res);
			pgcc_log(PGCC_FATAL,
					 "Please use --enterprisedb or --postgresql to specify the database type.\n");
		}
		remote_is_edb = atol(PQgetvalue(res, 0, 0));
		if (remote_is_edb)
			pgcc_log(PGCC_VERBOSE, "detected EnterpriseDB server\n");
		else
			pgcc_log(PGCC_VERBOSE, "detected PostgreSQL server\n");
		PQclear(res);
	}
	else
	{
		if (remote_is_edb)
			pgcc_log(PGCC_VERBOSE, "assuming EnterpriseDB server\n");
		else
			pgcc_log(PGCC_VERBOSE, "assuming PostgreSQL server\n");
	}

	/*
	 * At this point, we know the database version and flavor that we'll be
	 * checking and can fix the list of columns to be checked.
	 */
	decide_what_to_check(selected_columns);

	/* Cache the OID of the current database, if possible. */
	database_oid = get_database_oid(conn);

	/* Run the checks. */
	perform_checks(conn);

	/* Cleanup */
	PQfinish(conn);
	pgcc_log_completion();

	return 0;
}

/*
 * Parse the target version string.
 *
 * We expect either something of the form MAJOR.MINOR or else a single number
 * in the format used by PQremoteVersion().
 */
static int
parse_target_version(char *version)
{
	int			major = 0;
	int			minor = 0;
	char	   *s;

	/* Try to parse major version number. */
	for (s = version; *s >= '0' && *s <= '9'; ++s)
		major = (major * 10) + (*s - '0');

	/*
	 * If we found a 5+ digit number, assume it's in PQremoteVersion() format
	 * and call it good.
	 */
	if (*s == '\0' && major >= 10000)
		return major;

	/* Expecting a period and then a minor version number. */
	if (*s != '.')
		goto bad;
	for (++s; *s >= '0' && *s <= '9'; ++s)
		minor = (minor * 10) + (*s - '0');

	/* And now we're expecting end of string. */
	if (*s != '\0')
		goto bad;
	return (major * 10000) + (minor * 100);

bad:
	fprintf(stderr, _("%s: invalid argument for option --target-version\n"),
			progname);
	fprintf(stderr, _("Target version should be formatted as MAJOR.MINOR.\n"));
	exit(1);
}

/*
 * Select or deselect the named table.
 */
static void
select_table(char *table_name, enum trivalue whether)
{
	pg_catalog_table *tab;
	int			nmatched = 0;

	for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
	{
		if (strcmp(table_name, tab->table_name) != 0)
			continue;
		++nmatched;
		tab->checked = whether;
	}

	if (nmatched == 0)
		pgcc_log(PGCC_FATAL, "table name \"%s\" not recognized\n",
				 table_name);
}

/*
 * Select or deselect the named column.
 *
 * FIXME: Allow table_name.column_name syntax here.
 */
static void
select_column(char *column_name, enum trivalue whether)
{
	pg_catalog_table *tab;
	int			nmatched = 0;

	for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
	{
		pg_catalog_column *tabcol;

		for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
		{
			if (strcmp(column_name, tabcol->name) == 0)
			{
				tabcol->checked = whether;
				++nmatched;
			}
		}
	}

	if (nmatched == 0)
		pgcc_log(PGCC_FATAL, "column name \"%s\" not recognized\n",
				 column_name);
}

/*
 * Given a table name, find the corresponding pg_catalog_table structure.
 */
pg_catalog_table *
find_table_by_name(char *table_name)
{
	pg_catalog_table *tab;

	for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
		if (strcmp(tab->table_name, table_name) == 0)
			return tab;

	pgcc_log(PGCC_FATAL, "no metadata found for table %s\n", table_name);
	return NULL;				/* placate compiler */
}

/*
 * Given a table and a column name, find the pg_catalog_column structure.
 */
pg_catalog_column *
find_column_by_name(pg_catalog_table *tab, char *name)
{
	pg_catalog_column *tabcol;

	for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
		if (strcmp(tabcol->name, name) == 0)
			return tabcol;

	pgcc_log(PGCC_FATAL, "no metadata found for column %s.%s\n",
			 tab->table_name, name);
	return NULL;				/* placate compiler */
}

/*
 * Connect to the database.
 */
static PGconn *
do_connect(void)
{
	PGconn	   *conn;
	static char *password = NULL;
	bool		new_pass;

	/*
	 * Start the connection.  Loop until we have a password if requested by
	 * backend.
	 */
	do
	{
#define PARAMS_ARRAY_SIZE	7

		const char *keywords[PARAMS_ARRAY_SIZE];
		const char *values[PARAMS_ARRAY_SIZE];

		keywords[0] = "host";
		values[0] = pghost;
		keywords[1] = "port";
		values[1] = pgport;
		keywords[2] = "user";
		values[2] = login;
		keywords[3] = "password";
		values[3] = password;
		keywords[4] = "dbname";
		values[4] = dbName;
		keywords[5] = "fallback_application_name";
		values[5] = progname;
		keywords[6] = NULL;
		values[6] = NULL;

		new_pass = false;

		conn = PQconnectdbParams(keywords, values, true);

		if (!conn)
		{
			pgcc_log(PGCC_FATAL, "could not connect to server: %s",
					 PQerrorMessage(conn));
			return NULL;
		}

		if (PQstatus(conn) == CONNECTION_BAD &&
			PQconnectionNeedsPassword(conn) &&
			password == NULL)
		{
			PQfinish(conn);
			password = simple_prompt("Password: ", 100, false);
			new_pass = true;
		}
	} while (new_pass);

	/* check to see that the backend connection was successfully made */
	if (PQstatus(conn) == CONNECTION_BAD)
		pgcc_log(PGCC_FATAL, "could not connect to server: %s",
				 PQerrorMessage(conn));

	return conn;
}

/*
 * Attempt to obtain the OID of the database being checked.
 */
char *
get_database_oid(PGconn *conn)
{
	PGresult   *res;
	char	   *val;

	res = PQexec(conn,
			 "SELECT oid FROM pg_database WHERE datname = current_database()");
	if (PQresultStatus(res) != PGRES_TUPLES_OK)
	{
		char	   *message = PQresultErrorMessage(res);

		if (message != NULL && message[0] != '\0')
			pgcc_log(PGCC_ERROR, "could not determine database OID: %s",
					 message);
		else
			pgcc_log(PGCC_ERROR,
				  "could not determine database OID: unexpected status %s\n",
					 PQresStatus(PQresultStatus(res)));
		return NULL;
	}
	if (PQntuples(res) != 1)
	{
		pgcc_log(PGCC_ERROR, "query for database OID returned %d values\n",
				 PQntuples(res));
		return NULL;
	}
	val = pg_strdup(PQgetvalue(res, 0, 0));
	PQclear(res);

	pgcc_log(PGCC_DEBUG, "database OID is %s\n", val);

	return val;
}

/*
 * Decide which columns to check.
 */
static void
decide_what_to_check(bool selected_columns)
{
	pg_catalog_table *tab;

	/*
	 * First pass: set "checked" flags, and tentatively set "needed" flags.
	 */
	for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
	{
		pg_catalog_column *tabcol;

		tab->available = false;

		for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
		{
			/*
			 * If the user explicitly asked us to check a column we don't know
			 * how to check, that's a usage error, so bail out.
			 */
			if (tabcol->checked == TRI_YES && tabcol->check == NULL)
				pgcc_log(PGCC_FATAL,
						 "no check defined for column %s.%s\n",
						 tab->table_name, tabcol->name);

			/* Decide whether this column is available. */
			if (tabcol->is_edb_only && !remote_is_edb)
				tabcol->available = false;		/* EDB column on non-EDB
												 * database */
			else if (tabcol->minimum_version &&
					 remote_version < tabcol->minimum_version)
				tabcol->available = false;		/* DB version too old */
			else if (tabcol->maximum_version &&
					 remote_version < tabcol->maximum_version)
				tabcol->available = false;		/* DB version too new */
			else
				tabcol->available = true;
			if (tabcol->available)
				tab->available = true;

			/*
			 * If the column looks like it is not available in this version
			 * but the user asked explicitly for that particular column, warn
			 * them that things might not work out well.
			 */
			if (tabcol->available == false && tabcol->checked == TRI_YES)
			{
				tabcol->available = true;
				pgcc_log(PGCC_WARNING,
					"column %s.%s is not supported by this server version\n",
						 tab->table_name, tabcol->name);
			}

			/*
			 * If the user didn't specify whether to check the column, decide
			 * whether or not to do so.
			 */
			if (tabcol->checked == TRI_DEFAULT)
			{
				if (tabcol->check == NULL)
					tabcol->checked = TRI_NO;	/* no check defined */
				else if (!tabcol->available)
					tabcol->checked = TRI_NO;	/* not in this version */
				else if (tab->checked != TRI_DEFAULT)
					tabcol->checked = tab->checked;	/* use table setting */
				else if (selected_columns)
					tabcol->checked = TRI_NO;	/* only specified columns */
				else
					tabcol->checked = TRI_YES;	/* otherwise, check it */
			}

			/*
			 * Decide whether the column is needed, indicating whether it will
			 * be selected when we retrieve data from the table.  We exclude
			 * columns not available in this server version, but include other
			 * columns if they are to be checked, if they are part of the key,
			 * or if we display them for purposes of row identification.
			 */
			if (!tabcol->available)
				tabcol->needed = false;
			else
				tabcol->needed = (tabcol->checked == TRI_YES)
					|| tabcol->is_key_column || tabcol->is_display_column;
		}
	}

	/*
	 * Second pass: allow individual checks to mark additional columns as
	 * needed, and set ordering dependencies.
	 */
	for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
	{
		pg_catalog_column *tabcol;

		for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
		{
			pg_catalog_check *check;

			check = tabcol->check;
			if (check == NULL || tabcol->checked != TRI_YES)
				continue;

			switch (check->type)
			{
				case CHECK_ATTNUM:
					prepare_to_check_relnatts(tab, tabcol);
					break;
				case CHECK_OID_REFERENCE:
				case CHECK_OID_VECTOR_REFERENCE:
				case CHECK_OID_ARRAY_REFERENCE:
					prepare_to_check_oid_reference(tab, tabcol);
					break;
				case CHECK_DEPENDENCY_CLASS_ID:
					prepare_to_check_dependency_class_id(tab, tabcol);
					break;
				case CHECK_DEPENDENCY_ID:
					prepare_to_check_dependency_id(tab, tabcol);
					break;
				case CHECK_DEPENDENCY_SUBID:
					prepare_to_check_dependency_subid(tab, tabcol);
					break;
				case CHECK_RELNATTS:
					prepare_to_check_relnatts(tab, tabcol);
					break;
			}
		}
	}

}

/*
 * Set up metadata that will be needed to choose an order in which to check
 * the tables.
 */
static void
perform_checks(PGconn *conn)
{
	pg_catalog_table *tab;

	/* Initialize the table check states. */
	for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
	{
		pg_catalog_column *tabcol;

		if (tab->num_needed_by != 0)
			tab->needs_load = true;

		for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
		{
			if (tabcol->needed)
				tab->needs_load = true;
			if (tabcol->checked == TRI_YES)
			{
				Assert(tab->needs_load);
				tab->needs_check = true;
				break;
			}
		}
	}

	/* Loop until all checks are complete. */
	for (;;)
	{
		pg_catalog_table *best = NULL;
		pg_catalog_table *tab;
		int			remaining = 0;

		/*
		 * Search for tables that can be checked without loading any more data
		 * from the database.  If we find any, check them.	Along the way,
		 * keep a count of the number of tables remaining to be checked.
		 */
		for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
		{
			if (tab->needs_check && !tab->needs_load && tab->num_needs == 0)
				check_table(conn, tab);
			if (tab->needs_check)
				++remaining;
		}

		/* If no tables remain to be checked, we're done. */
		if (remaining == 0)
			break;

		/*
		 * There are tables that remain to be checked, but none of them can be
		 * checked without reading data from the database.	Choose one which
		 * requires preloading the fewest tables; in case of a tie, prefer the
		 * one required by the most yet-to-be-checked tables, in the hopes of
		 * unblocking as many other checks as possible.
		 */
		for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
		{
			if (!tab->needs_check)
				continue;
			if (best == NULL || tab->num_needs < best->num_needs ||
				(tab->num_needs == best->num_needs
				 && tab->num_needed_by > best->num_needed_by))
				best = tab;
		}
		Assert(best != NULL);

		/* If the selected candidate needs other tables preloaded, do that. */
		while (best->num_needs > 0)
		{
			pg_catalog_table *reftab = best->needs[best->num_needs - 1];
			int			old_num_needs = best->num_needs;

			if (!reftab->needs_load)
				continue;
			pgcc_log(PGCC_VERBOSE,
					 "preloading table %s because it is required in order to check %s\n",
					 reftab->table_name, best->table_name);
			load_table(conn, reftab);
			Assert(old_num_needs > best->num_needs);
		}

		/* Load the table itself, if it isn't already. */
		if (best->needs_load)
		{
			pgcc_log(PGCC_VERBOSE, "loading table %s\n",
					 best->table_name);
			load_table(conn, best);
		}

		/* Check the table. */
		check_table(conn, best);
	}
}

/*
 * Load a table into memory.
 */
static void
load_table(PGconn *conn, pg_catalog_table *tab)
{
	PQExpBuffer query;
	int			i;

	Assert(tab->needs_load);

	/* Load the table data. */
	query = build_query_for_table(tab);
	pgcc_log(PGCC_DEBUG, "executing query: %s\n", query->data);
	tab->data = PQexec(conn, query->data);
	if (PQresultStatus(tab->data) != PGRES_TUPLES_OK)
	{
		char	   *message = PQresultErrorMessage(tab->data);

		if (message != NULL && message[0] != '\0')
			pgcc_log(PGCC_ERROR, "could not load table %s: %s",
					 tab->table_name, message);
		else
			pgcc_log(PGCC_ERROR,
					 "could not load table %s: unexpected status %s\n",
					 tab->table_name, PQresStatus(PQresultStatus(tab->data)));
	}
	else
		build_hash_from_query_results(tab);
	destroyPQExpBuffer(query);

	/* This table is now loaded. */
	tab->needs_load = false;

	/* Any other tables that neeed this table no longer do. */
	for (i = 0; i < tab->num_needed_by; ++i)
	{
		pg_catalog_table *reftab = tab->needed_by[i];
		int			j,
					k;

		for (j = 0, k = 0; j < reftab->num_needs; ++j)
		{
			reftab->needs[k] = reftab->needs[j];
			if (tab != reftab->needs[j])
				++k;
		}
		Assert(k == reftab->num_needs - 1);
		reftab->num_needs = k;
	}

	/* We can throw away our side of the dependency information as well. */
	tab->num_needed_by = 0;
	if (tab->num_needed_by_allocated > 0)
	{
		pg_free(tab->needed_by);
		tab->num_needed_by_allocated = 0;
	}
}

/*
 * Perform integrity checks on a table.
 */
static void
check_table(PGconn *conn, pg_catalog_table *tab)
{
	int			i;
	int			ntups;

	/* Once we've tried to check the table, we shouldn't try again. */
	tab->needs_check = false;
	Assert(tab->data != NULL);

	/*
	 * If we weren't able to retrieve the table data, then we can't check the
	 * table.  But there's no real need to log the error message, becaues
	 * load_table() will have already done so.
	 */
	if (PQresultStatus(tab->data) != PGRES_TUPLES_OK)
		return;

	/* Log a message, if verbose mode is enabled. */
	ntups = PQntuples(tab->data);
	pgcc_log(PGCC_VERBOSE, "checking table %s (%d rows)\n", tab->table_name,
			 ntups);

	/* Loop over the rows and check them. */
	for (i = 0; i < ntups; ++i)
	{
		pg_catalog_column *tabcol;

		for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
		{
			pg_catalog_check *check;

			if (tabcol->checked != TRI_YES || tabcol->check == NULL)
				continue;
			check = tabcol->check;

			switch (check->type)
			{
				case CHECK_ATTNUM:
					check_attnum(tab, tabcol, i);
					break;
				case CHECK_OID_REFERENCE:
				case CHECK_OID_VECTOR_REFERENCE:
				case CHECK_OID_ARRAY_REFERENCE:
					check_oid_reference(tab, tabcol, i);
					break;
				case CHECK_DEPENDENCY_CLASS_ID:
					check_dependency_class_id(tab, tabcol, i);
					break;
				case CHECK_DEPENDENCY_ID:
					check_dependency_id(tab, tabcol, i);
					break;
				case CHECK_DEPENDENCY_SUBID:
					check_dependency_subid(tab, tabcol, i);
					break;
				case CHECK_RELNATTS:
					check_relnatts(tab, tabcol, i);
					break;
			}
		}
	}
}

/*
 * Build a hash table on the key columns of the catalog table contents.
 */
static void
build_hash_from_query_results(pg_catalog_table *tab)
{
	int			i;
	pg_catalog_column *tabcol;
	pgrhash    *ht;
	int			keycols[MAX_KEY_COLS];
	int			nkeycols = 0;
	int			ntups = PQntuples(tab->data);

	for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
		if (tabcol->available && tabcol->is_key_column)
			keycols[nkeycols++] = PQfnumber(tab->data, tabcol->name);

	/*
	 * Tables like pg_depend get loaded so that we can check them, but they
	 * don't have a primary key, so we don't build a hash table.
	 */
	if (nkeycols == 0)
		return;

	/* Create the hash table. */
	ht = tab->ht = pgrhash_create(tab->data, nkeycols, keycols);

	for (i = 0; i < ntups; i++)
		if (pgrhash_insert(ht, i) != -1)
			pgcc_report(tab, NULL, i, "%s row duplicates existing key\n",
						tab->table_name);
}

/*
 * Build a query to read the needed columns from a table.
 */
static PQExpBuffer
build_query_for_table(pg_catalog_table *tab)
{
	PQExpBuffer query;
	pg_catalog_column *tabcol;
	int			index = 0;

	query = createPQExpBuffer();

	appendPQExpBuffer(query, "SELECT");
	for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
	{
		if (!tabcol->needed)
			continue;
		if (index == 0)
			appendPQExpBuffer(query, " %s", tabcol->name);
		else
			appendPQExpBuffer(query, ", %s", tabcol->name);
		if (tabcol->cast)
			appendPQExpBuffer(query, "::%s", tabcol->cast);

		/* Remember where this column is supposed to be in the output. */
		tabcol->result_column = index;
		index++;
	}

	Assert(index > 0);

	appendPQExpBuffer(query, " FROM pg_catalog.%s", tab->table_name);

	return query;
}

/*
 * Indicate that one table ("needs") requires that another table ("needed_by")
 * be loaded before it is checked.
 */
void
add_table_dependency(pg_catalog_table *needs, pg_catalog_table *needed_by)
{
	int			i;

	if (!needs->available || !needed_by->available)
		return;

	/*
	 * We necessarily load tables before checking them, so there's no point in
	 * a circular dependency.
	 */
	if (needs == needed_by)
		return;

	pgcc_log(PGCC_DEBUG, "table %s depends on table %s\n",
			 needs->table_name, needed_by->table_name);

	/* Check whether the dependency is already present; if so, do nothing. */
	for (i = 0; i < needs->num_needs; ++i)
		if (needs->needs[i] == needed_by)
			return;

	/* Make sure there's enough space to store the new dependency. */
	if (needs->num_needs >= needs->num_needs_allocated)
	{
		if (needs->num_needs_allocated == 0)
		{
			needs->num_needs_allocated = 4;
			needs->needs =
				pg_malloc(needs->num_needs_allocated * sizeof(pg_catalog_table *));
		}
		else
		{
			needs->num_needs_allocated *= 2;
			needs->needs = pg_realloc(needs->needs,
					needs->num_needs_allocated * sizeof(pg_catalog_table *));
		}
	}
	if (needed_by->num_needed_by >= needed_by->num_needed_by_allocated)
	{
		if (needed_by->num_needed_by_allocated == 0)
		{
			needed_by->num_needed_by_allocated = 4;
			needed_by->needed_by =
				pg_malloc(needed_by->num_needed_by_allocated *
						  sizeof(pg_catalog_table *));
		}
		else
		{
			needed_by->num_needed_by_allocated *= 2;
			needed_by->needed_by = pg_realloc(needed_by->needed_by,
			needed_by->num_needed_by_allocated * sizeof(pg_catalog_table *));
		}
	}

	/* Add the dependency. */
	needs->needs[needs->num_needs] = needed_by;
	++needs->num_needs;
	needed_by->needed_by[needed_by->num_needed_by] = needs;
	++needed_by->num_needed_by;
}

/*
 * Print a usage message and exit.
 */
static void
usage(void)
{
	printf("%s is catalog table validation tool for PostgreSQL.\n\n", progname);
	printf("Usage:\n  %s [OPTION]... [DBNAME]\n\n", progname);
	printf("Options:\n");
	printf("  -c, --column             check only the named columns\n");
	printf("  -t, --table              check only columns in the named tables\n");
	printf("  -T, --exclude-table      do NOT check the named tables\n");
	printf("  -C, --exclude-column     do NOT check the named columns\n");
	printf("  --target-version=VERSION assume specified target version\n");
	printf("  --enterprisedb           assume EnterpriseDB database\n");
	printf("  --postgresql             assume PostgreSQL database\n");
	printf("  -h, --host=HOSTNAME      database server host or socket directory\n");
	printf("  -p, --port=PORT          database server port number\n");
	printf("  -q, --quiet              do not display progress messages\n");
	printf("  -U, --username=USERNAME  connect as specified database user\n");
	printf("  -v, --verbose            enable verbose internal logging\n");
	printf("  -V, --version            output version information, then exit\n");
	printf("  -?, --help               show this help, then exit\n");
	printf("\nReport bugs to <support@enterprisedb.com>.\n");
}
