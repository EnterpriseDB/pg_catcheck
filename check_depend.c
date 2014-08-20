/*-------------------------------------------------------------------------
 *
 * check_depend.c
 *
 * A number of PostgreSQL system catalogs store references to SQL objects
 * of arbitrary type by recording a class ID (the OID of the system
 * catalog that contains the referenced object) and an object ID (the OID
 * of the referenced object within that catalog).  In cases where the
 * referenced object may be a table column, there is also a sub-ID;
 * when the referenced object is a table column, (class ID, sub-ID) should
 * match the pg_attribute row's (attrelid, attnum).  In all other cases,
 * the sub-ID should be zero.
 *
 * The code in this file aims to validate the class ID, object ID, and
 * sub-ID.	There is some duplication in the code structure, because to
 * check the object ID, we must validate the class ID and look up the
 * corresponding table.  However, we try hard not to complain about what
 * is in essence the same problem more than once, and to complain about
 * it with respect to the correct column.
 *
 * The name of this file comes from the fact that the classic example of
 * the class ID/object ID/sub-ID notation is in the pg_depend catalog,
 * but we actually use this code to validate other tables that use a
 * similar convention, such as pg_description.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"
#include "pg_catcheck.h"

typedef enum
{
	DEPEND_COLUMN_STYLE_OBJID,	/* pg_(sh)depend, referring side */
	DEPEND_COLUMN_STYLE_REFOBJID,		/* pg_(sh)depend, referenced side */
	DEPEND_COLUMN_STYLE_OBJOID, /* pg_(sh)description, pg_(sh)seclabel */
} depend_column_style;

typedef struct check_depend_cache
{
	depend_column_style style;
	bool		is_broken;
	int			database_result_column;
	int			class_result_column;
	int			object_result_column;
	int			deptype_result_column;
	pgrhash	   *duplicate_owner_ht;
} check_depend_cache;

typedef struct class_id_mapping_type
{
	char	   *oid;
	pg_catalog_table *tab;
} class_id_mapping_type;

/*
 * EnterpriseDB versions prior to 9.4 are expected to have a number of
 * dangling dependency entries, unless initialized with --no-redwood-compat.
 * We avoid complaining about these because (1) they're known and basically
 * harmless and (2) we don't want to give the misimpression of real
 * corruption.
 */

typedef struct exception_list
{
	char	   *table_name;
	char	   *class;
	char	   *object;
} exception_list;

exception_list edb84_exception_list[] = {
	{"pg_depend", "1255", "877"},
	{"pg_depend", "1255", "883"},
	{"pg_depend", "1255", "1777"},
	{"pg_depend", "1255", "1780"},
	{"pg_depend", "1255", "2049"},
	{"pg_depend", "2617", "2779"},
	{"pg_depend", "2617", "2780"},
	{NULL}
};

exception_list edb90_exception_list[] = {
	{"pg_depend", "1255", "877"},
	{"pg_depend", "1255", "883"},
	{"pg_depend", "1255", "1777"},
	{"pg_depend", "1255", "1780"},
	{"pg_depend", "1255", "2049"},
	{"pg_depend", "2617", "2779"},
	{"pg_depend", "2617", "2780"},
	{NULL}
};

exception_list edb91_92_exception_list[] = {
	{"pg_depend", "1255", "877"},
	{"pg_depend", "1255", "883"},
	{"pg_depend", "1255", "1777"},
	{"pg_depend", "1255", "1780"},
	{"pg_depend", "1255", "2049"},
	{"pg_depend", "2617", "2779"},
	{"pg_depend", "2617", "2780"},
	{"pg_description", "2617", "2779"},
	{"pg_description", "2617", "2780"},
	{NULL}
};

exception_list edb93_exception_list[] = {
	{"pg_depend", "1255", "877"},
	{"pg_depend", "1255", "883"},
	{"pg_depend", "1255", "1777"},
	{"pg_depend", "1255", "1780"},
	{"pg_depend", "1255", "2049"},
	{NULL}
};

static bool class_id_mappings_attempted;
static int	num_class_id_mapping;
static class_id_mapping_type *class_id_mapping;
static char *pg_class_oid;
static pg_catalog_table *pg_attribute_table;
static pg_catalog_table *pg_type_table;

static pg_catalog_table *lookup_class_id(char *oid);
static void build_class_id_mappings(void);
static bool table_key_is_oid(pg_catalog_table *tab);
static check_depend_cache *build_depend_cache(pg_catalog_table *tab,
				   pg_catalog_column *tabcol);
static bool not_for_this_database(check_depend_cache *cache,
					  pg_catalog_table *tab, pg_catalog_column *tabcol,
					  int rownum);
static depend_column_style get_style(char *table_name, char *column_name);
static bool check_for_exception(char *table_name, char *classval,
					char *objval);

/*
 * Set up to check a class ID.
 */
void
prepare_to_check_dependency_class_id(pg_catalog_table *tab,
									 pg_catalog_column *tabcol)
{
	pg_catalog_table *pg_class = find_table_by_name("pg_class");
	pg_catalog_column *pg_class_relname;
	pg_catalog_column *pg_class_relnamespace;

	/*
	 * We need pg_class to figure out system catalog table OIDs.
	 */
	add_table_dependency(tab, pg_class);
	pg_class_relname = find_column_by_name(pg_class, "relname");
	pg_class_relname->needed = true;
	pg_class_relnamespace = find_column_by_name(pg_class, "relnamespace");
	pg_class_relnamespace->needed = true;

	/* We need this to determine whether the class ID can legally zero. */
	if (get_style(tab->table_name, tabcol->name) == DEPEND_COLUMN_STYLE_OBJID)
	{
		pg_catalog_column *deptype = find_column_by_name(tab, "deptype");

		deptype->needed = true;
	}
}

/*
 * Set up to check an object ID.
 */
void
prepare_to_check_dependency_id(pg_catalog_table *tab, pg_catalog_column *tabcol)
{
	pg_catalog_table *cattab;
	pg_catalog_column *classid;

	/*
	 * Just as when checking a class ID, we need pg_class to map class IDs to
	 * catalog tables.
	 */
	prepare_to_check_dependency_class_id(tab, tabcol);

	/*
	 * All catalog tables that have an OID column must be loaded before we can
	 * check dependency IDs.
	 */
	for (cattab = pg_catalog_tables; cattab->table_name != NULL; ++cattab)
		if (table_key_is_oid(cattab))
			add_table_dependency(tab, cattab);

	/* Force the necessary classid column to be selected. */
	switch (get_style(tab->table_name, tabcol->name))
	{
		case DEPEND_COLUMN_STYLE_OBJID:
			classid = find_column_by_name(tab, "classid");
			break;
		case DEPEND_COLUMN_STYLE_REFOBJID:
			classid = find_column_by_name(tab, "refclassid");
			break;
		case DEPEND_COLUMN_STYLE_OBJOID:
			classid = find_column_by_name(tab, "classoid");
			break;
		default:
			pgcc_log(PGCC_FATAL, "unexpected depend column style");
			return;			/* placate compiler */
	}
	classid->needed = true;
}

/*
 * Set up to check a sub-ID.
 */
void
prepare_to_check_dependency_subid(pg_catalog_table *tab,
								  pg_catalog_column *tabcol)
{
	pg_catalog_column *classid;
	pg_catalog_column *objectid;

	/*
	 * Just as when checking a class ID, we need pg_class to map class IDs to
	 * catalog tables.	Specifically, we've got to be able to identify the OID
	 * of pg_class itself, so that we know whether a non-zero sub-ID is legal.
	 * Currently, that OID is the same in all server versions we support, so
	 * we could hard-code it, but there's little harm in doing it this way: if
	 * pg_class is too botched to interpret, the chances of anything else
	 * making much sense are slim to none.
	 */
	prepare_to_check_dependency_class_id(tab, tabcol);

	/* We need the pg_attribute table to check sub-IDs. */
	add_table_dependency(tab, find_table_by_name("pg_attribute"));

	/* Make sure we have the class and object IDs. */
	switch (get_style(tab->table_name, tabcol->name))
	{
		case DEPEND_COLUMN_STYLE_OBJID:
			classid = find_column_by_name(tab, "classid");
			objectid = find_column_by_name(tab, "objid");
			break;
		case DEPEND_COLUMN_STYLE_REFOBJID:
			classid = find_column_by_name(tab, "refclassid");
			objectid = find_column_by_name(tab, "refobjid");
			break;
		case DEPEND_COLUMN_STYLE_OBJOID:
			classid = find_column_by_name(tab, "classoid");
			objectid = find_column_by_name(tab, "objoid");
			break;
		default:
			pgcc_log(PGCC_FATAL, "unexpected depend column style");
			return;		/* placate compiler */
	}
	classid->needed = true;
	objectid->needed = true;
}

/*
 * Check a class ID.
 *
 * This is basically just testing that the class ID is a system catalog
 * table that we know about and that's supposed to exists in this server
 * version, or else 0 if that's a legal value in this context.
 */
void
check_dependency_class_id(pg_catalog_table *tab, pg_catalog_column *tabcol,
						  int rownum)
{
	char	   *val;
	check_depend_cache *cache;

	cache = build_depend_cache(tab, tabcol);
	if (cache->is_broken)
		return;
	if (not_for_this_database(cache, tab, tabcol, rownum))
		return;

	val = PQgetvalue(tab->data, rownum, tabcol->result_column);

	/*
	 * We normally expect that the class ID is non-zero, but "pin" depedencies
	 * are an exception.
	 */
	if (strcmp(val, "0") == 0)
	{
		bool		complain = true;

		if (cache->style == DEPEND_COLUMN_STYLE_OBJID)
		{
			char	   *deptype;

			deptype = PQgetvalue(tab->data, rownum,
								 cache->deptype_result_column);

			if (strcmp(deptype, "p") == 0)
				complain = false;
		}

		if (complain)
			pgcc_report(tab, tabcol, rownum, "unexpected zero value\n");
		return;
	}

	if (lookup_class_id(val) == NULL)
	{
		/*
		 * Workaround for an old EnterpriseDB bug: 8.4 installed a bogus
		 * dependency with reclassid 16722.
		 */
		if (remote_is_edb && remote_version <= 90000 &&
			strcmp(val, "16722") == 0)
		{
			pgcc_log(PGCC_DEBUG, "ignoring reference to class ID 16722\n");
			return;
		}
		pgcc_report(tab, tabcol, rownum, "not a system catalog OID\n");
	}
}

/*
 * Check a dependency ID.
 *
 * We have to examine the class ID to figure out which table ought to
 * contain the indicated object.  We then look up that table and check
 * whether the value appears in its OID column.
 */
void
check_dependency_id(pg_catalog_table *tab, pg_catalog_column *tabcol,
					int rownum)
{
	char	   *classval;
	char	   *val;
	pg_catalog_table *object_tab;
	check_depend_cache *cache;

	cache = build_depend_cache(tab, tabcol);
	if (cache->is_broken)
		return;
	if (not_for_this_database(cache, tab, tabcol, rownum))
		return;

	/*
	 * Check for multiple owner dependencies for the same object.
	 *
	 * FIXME: Current pg_catcheck design don't support table-level checks,
	 * all checks are column-level. We might want to re-architect this at
	 * some point in the future.  For now, we check this here.
	 */
	if (cache->duplicate_owner_ht != NULL &&
		strcmp(PQgetvalue(tab->data, rownum,
						  PQfnumber(tab->data, "deptype")), "o") == 0 &&
		pgrhash_insert(cache->duplicate_owner_ht, rownum) != -1)
		pgcc_report(tab, NULL, rownum, "duplicate owner dependency\n");

	/* Fetch the class ID and object ID. */
	classval = PQgetvalue(tab->data, rownum, cache->class_result_column);
	val = PQgetvalue(tab->data, rownum, tabcol->result_column);

	/* If the class ID is zero, the object ID should be zero as well. */
	if (strcmp(classval, "0") == 0)
	{
		if (strcmp(val, "0") != 0)
			pgcc_report(tab, tabcol, rownum,
						"class ID is zero, but object ID is non-zero\n");
		return;
	}

	/* Find the correct table. */
	object_tab = lookup_class_id(classval);
	if (object_tab == NULL || object_tab->ht == NULL)
		return;

	/*
	 * Workaround for EnterpriseDB bug: EnterpriseDB versions prior to 9.4
	 * would sometimes create bogus dependencies on type ID 0.	Since we'll
	 * never create a real type with that OID, this was (as far as we know)
	 * harmless, so just ignore them.
	 */
	if (pg_type_table == NULL)
		pg_type_table = find_table_by_name("pg_type");
	if (remote_version < 90400 && remote_is_edb && object_tab == pg_type_table
		&& strcmp(val, "0") == 0)
	{
		pgcc_log(PGCC_DEBUG,
				 "ignoring reference to pg_type OID 0\n");
		return;
	}

	/*
	 * lookup_class_id() will only return tables where the only key column is
	 * the OID column.	So this is safe.
	 */
	if (pgrhash_get(object_tab->ht, &val) == -1 &&
		!check_for_exception(tab->table_name, classval, val))
		pgcc_report(tab, tabcol, rownum, "no matching entry in %s\n",
					object_tab->table_name);
}

/*
 * Check a dependency sub-ID.
 *
 * This should always be zero except in the where the class ID points to
 * pg_class.  In that case, should be able to find <object ID, sub-ID> in
 * pg_attribute. The object ID will appar in attrelid and the sub-ID in attnum.
 */
void
check_dependency_subid(pg_catalog_table *tab, pg_catalog_column *tabcol,
					   int rownum)
{
	char	   *classval;
	char	   *vals[2];
	check_depend_cache *cache;

	cache = build_depend_cache(tab, tabcol);
	if (cache->is_broken)
		return;

	/*
	 * We find pg_attribute on our first trip through this function and avoid
	 * repeating the lookup thereafter using a global variable.
	 */
	if (pg_attribute_table == NULL)
		pg_attribute_table = find_table_by_name("pg_attribute");

	/* Fetch the class ID, object ID, and sub-ID. */
	classval = PQgetvalue(tab->data, rownum, cache->class_result_column);
	vals[0] = PQgetvalue(tab->data, rownum, cache->object_result_column);
	vals[1] = PQgetvalue(tab->data, rownum, tabcol->result_column);

	/* Sub-ID is always permitted to be zero. */
	if (strcmp(vals[1], "0") == 0)
		return;

	/*
	 * If we get here, the sub-ID is non-zero.	Therefore, the class ID should
	 * definitely point to pg_class; if it does not, that's an inconsistency.
	 * If it does point to pg_class, then a matching pg_attribute row should
	 * exist.
	 */
	if (strcmp(classval, pg_class_oid) != 0)
		pgcc_report(tab, tabcol, rownum,
					"class ID %s is not pg_class, but sub-ID is non-zero\n",
					classval);
	else if (pg_attribute_table->ht)	/* We might have failed to read it. */
	{
		if (pgrhash_get(pg_attribute_table->ht, vals) == -1)
			pgcc_report(tab, tabcol, rownum, "no matching entry in %s\n",
						pg_attribute_table->table_name);
	}
}

/*
 * Given a text-form OID found in an objid or refobjid table, search for a
 * corresponding catalog table.
 *
 * NB: We could make this more efficient by teaching build_class_id_mappings()
 * to sort the array, and then using binary search.
 */
static pg_catalog_table *
lookup_class_id(char *oid)
{
	int			i;

	Assert(class_id_mappings_attempted && class_id_mapping != NULL);

	for (i = 0; i < num_class_id_mapping; ++i)
		if (strcmp(class_id_mapping[i].oid, oid) == 0)
			return class_id_mapping[i].tab;

	return NULL;
}

/*
 * Build a set of mappings from text-form OIDs which PQgetvalue might hand
 * back for an objid or refobjid column to pg_catalog_table objects.
 */
static void
build_class_id_mappings(void)
{
	int			map_used = 0;
	int			map_size = 10;
	class_id_mapping_type *map;
	pg_catalog_table *pg_class_tab;
	int			oid_column;
	int			relnamespace_column;
	int			relname_column;
	int			ntups = 0;
	int			i;

	/*
	 * If we fall out of this function due to some kind of unexpected error,
	 * we do not want to retry, as we'll just hit the same problem the second
	 * time.  Set a flag so that callers can detect this case.
	 */
	class_id_mappings_attempted = true;

	/* Find the pg_class table. */
	pg_class_tab = find_table_by_name("pg_class");
	if (pg_class_tab->data != NULL)
		ntups = PQntuples(pg_class_tab->data);
	if (ntups == 0)
	{
		pgcc_log(PGCC_WARNING,
				 "can't identify class IDs: no pg_class data\n");
		return;
	}

	/* Find the pg_class columns we need. */
	oid_column = PQfnumber(pg_class_tab->data, "oid");
	relnamespace_column = PQfnumber(pg_class_tab->data, "relnamespace");
	relname_column = PQfnumber(pg_class_tab->data, "relname");
	if (oid_column == -1 || relname_column == -1 || relnamespace_column == -1)
	{
		pgcc_log(PGCC_WARNING,
				 "can't identify class IDs: missing pg_class columns\n");
		return;
	}

	/* Initialize map data structure. */
	map = pg_malloc(sizeof(class_id_mapping_type) * map_size);

	/* Scan pg_class rows to construct mapping table. */
	for (i = 0; i < ntups; ++i)
	{
		pg_catalog_table *tab;
		char	   *relnamespace;
		char	   *relname;

		/* Skip tables that are not part of the pg_catalog namespace. */
		relnamespace = PQgetvalue(pg_class_tab->data, i, relnamespace_column);
		if (strcmp(relnamespace, "11") != 0)
			continue;

		/* See if it's a catalog table we know about. */
		relname = PQgetvalue(pg_class_tab->data, i, relname_column);
		for (tab = pg_catalog_tables; tab->table_name != NULL; ++tab)
		{
			/* Skip table if name does not match. */
			if (strcmp(tab->table_name, relname) != 0)
				continue;

			/* Ignore matching table if it's not available. */
			if (!tab->available)
				break;

			/* Ignore matching table if not keyed by OID. */
			if (!table_key_is_oid(tab))
				break;

			/* Increase map space if required. */
			if (map_used >= map_size)
			{
				map_size *= 2;
				map = pg_realloc(map,
								 sizeof(class_id_mapping_type) * map_size);
			}

			/* Create map entry. */
			map[map_used].oid = PQgetvalue(pg_class_tab->data, i, oid_column);
			map[map_used].tab = tab;

			/*
			 * Special bookkeeping for pg_class itself, due to its role in
			 * checking sub-IDs.
			 */
			if (pg_class_tab == tab)
				pg_class_oid = map[map_used].oid;

			++map_used;
			break;
		}
	}

	/* Avoid installing a bogus empty mapping. */
	if (map_used == 0)
	{
		pgcc_log(PGCC_WARNING,
		  "can't identify class IDs: no catalog tables found in pg_class\n");
		return;
	}

	/*
	 * Avoid installing mapping that doesn't include pg_class itself.
	 *
	 * This is probably a good sanity check on general principal, but what
	 * truly makes it necessary is that sub-ID verification needs
	 * pg_class_oid.
	 */
	if (pg_class_oid == NULL)
	{
		pgcc_log(PGCC_WARNING,
			   "can't identify class IDs: pg_class not found in pg_class\n");
		return;
	}

	/* Install new mapping table. */
	class_id_mapping = map;
	num_class_id_mapping = map_used;
}

/*
 * Is oid the only key column for this table?
 */
static bool
table_key_is_oid(pg_catalog_table *tab)
{
	bool		has_oid_key = false;
	bool		has_other_key = false;
	pg_catalog_column *tabcol;

	for (tabcol = tab->cols; tabcol->name != NULL; ++tabcol)
	{
		if (!tabcol->is_key_column)
			continue;
		if (strcmp(tabcol->name, "oid") == 0)
			has_oid_key = true;
		else
			has_other_key = true;
	}

	return has_oid_key && !has_other_key;
}

/*
 * Cache per-column dependency checking information, basically column indexes
 * into the PGresult structure so that we can quickly find the class ID for
 * an object ID and the class and object ID for a sub-ID.
 */
static check_depend_cache *
build_depend_cache(pg_catalog_table *tab, pg_catalog_column *tabcol)
{
	check_depend_cache *cache;
	bool		columns_missing = false;

	/* If we've already built the cache, just return the existing data. */
	if (tabcol->check_private != NULL)
		return tabcol->check_private;

	/* Create and initialize the cache object. */
	cache = pg_malloc0(sizeof(check_depend_cache));
	cache->style = get_style(tab->table_name, tabcol->name);
	cache->is_broken = false;
	switch (cache->style)
	{
		case DEPEND_COLUMN_STYLE_OBJID:
			/* special case for pg_shdepend */
			cache->database_result_column = PQfnumber(tab->data, "dbid");
			cache->class_result_column = PQfnumber(tab->data, "classid");
			cache->object_result_column = PQfnumber(tab->data, "objid");
			break;
		case DEPEND_COLUMN_STYLE_REFOBJID:
			cache->database_result_column = -1;
			cache->class_result_column = PQfnumber(tab->data, "refclassid");
			cache->object_result_column = PQfnumber(tab->data, "refobjid");
			break;
		case DEPEND_COLUMN_STYLE_OBJOID:
			cache->database_result_column = -1;
			cache->class_result_column = PQfnumber(tab->data, "classoid");
			cache->object_result_column = PQfnumber(tab->data, "objoid");
			break;
		default:
			pgcc_log(PGCC_FATAL, "unexpected depend column style");
			break;
	}

	/* Verify that PQfnumber() worked as expected. */
	if (cache->class_result_column == -1)
		columns_missing = true;
	if (cache->object_result_column == -1)
		columns_missing = true;

	/* Look up the column number for the deptype column, if expected. */
	if (cache->style == DEPEND_COLUMN_STYLE_OBJID)
	{
		cache->deptype_result_column = PQfnumber(tab->data, "deptype");
		if (cache->deptype_result_column == -1)
			columns_missing = true;
	}

	/*
	 * If we failed to find the relevant columns, then mark the cache as
	 * broken, which will cause the individual rows not to be checked.
	 * Otherwise, we'd end up having to emit a message for every row, which
	 * would be too much chatter.
	 */
	if (columns_missing)
	{
		pgcc_log(PGCC_WARNING,
				 "can't identify class IDs: columns missing from %s\n",
				 tab->table_name);
		cache->is_broken = true;
	}

	/*
	 * For the convenience of our callers, we also try to build the global
	 * cache data here, namely the mappings from class IDs to pg_catalog_table
	 * objects.  Unlike the data stored in the cache object, these mappings
	 * can be reused across all columns where we check dependencies.
	 */
	if (!class_id_mappings_attempted)
		build_class_id_mappings();

	/*
	 * If we failed to build the mappings, either now or previously, mark the
	 * cache object as broken, to avoid log chatter for each separate row.
	 */
	if (class_id_mapping == NULL)
		cache->is_broken = true;

	/*
	 * If needed, create hash table for duplicate-owner-dependency cheecking.
	 */
	if (!cache->is_broken && cache->database_result_column != -1 &&
		cache->deptype_result_column != -1)
	{
		int					keycols[4];

		keycols[0] = cache->database_result_column;
		keycols[1] = cache->class_result_column;
		keycols[2] = cache->object_result_column;
		cache->duplicate_owner_ht = pgrhash_create(tab->data, 4, keycols);
	}

	/* We're done. */
	tabcol->check_private = cache;
	return cache;
}

/*
 * Determine whether this dependency should be ignored because it's not
 * part of this database.  This will only ever return true when we're checking
 * a table that has a dbid column, which currently means just pg_shdepend.
 */
static bool
not_for_this_database(check_depend_cache *cache, pg_catalog_table *tab,
					  pg_catalog_column *tabcol, int rownum)
{
	char	   *dbval;

	/* If there's no dbid column, then it's part of this database. */
	if (cache->database_result_column == -1)
		return false;

	/* Look up the value in that column. */
	dbval = PQgetvalue(tab->data, rownum, cache->database_result_column);

	/* 0 means it's a global object, so it's fine to check it here. */
	if (strcmp(dbval, "0") == 0)
		return false;

	/*
	 * If we don't know the database OID, skip the check, to avoid bogus
	 * complaints.
	 */
	if (database_oid == NULL)
		return true;

	/* Straightforward comparison. */
	return strcmp(database_oid, dbval) != 0;
}

/*
 * Determine which naming style applies to this table and column.
 *
 * There are three naming conventions that are used for references to objects
 * in arbitrary catalogs.  pg_depend and pg_shdepend use classid/objid/objsubid
 * for one side of the dependency and refclassid/refobjid/refobjsubid for the
 * other.  Other tables that contain similar information, such as
 * pg_description, pg_shdescription, pg_seclabel, and pg_shseclabel, use
 * objoid/classoid/objsubid.
 */
static depend_column_style
get_style(char *table_name, char *column_name)
{
	if (strncmp(column_name, "ref", 3) == 0)
		return DEPEND_COLUMN_STYLE_REFOBJID;
	else if (strstr(table_name, "depend"))
		return DEPEND_COLUMN_STYLE_OBJID;
	else
		return DEPEND_COLUMN_STYLE_OBJOID;
}

/*
 * Check whether a detected inconsistency is one that we were expecting.
 */
static bool
check_for_exception(char *table_name, char *classval, char *objval)
{
	exception_list *exc;

	if (!remote_is_edb || remote_version >= 90400)
		return false;

	if (remote_version >= 90300)
		exc = edb93_exception_list;
	else if (remote_version >= 90100)
		exc = edb91_92_exception_list;
	else if (remote_version >= 90000)
		exc = edb90_exception_list;
	else
		exc = edb84_exception_list;

	while (exc->table_name != NULL)
	{
		if (strcmp(exc->table_name, table_name) == 0 &&
			strcmp(exc->class, classval) == 0 &&
			strcmp(exc->object, objval) == 0)
		{
			pgcc_log(PGCC_DEBUG,
					 "ignoring reference to class ID %s object ID %s in %s\n",
					 exc->class, exc->object, exc->table_name);
			return true;
		}
		++exc;
	}

	return false;
}
