/*-------------------------------------------------------------------------
 *
 * pg_catcheck.h
 *
 * Functions and data structures for catalog integrity checking.
 *
 *-------------------------------------------------------------------------
 */

#ifndef PGCATCHECK_H
#define PGCATCHECK_H

#include "libpq-fe.h"			/* for PGresult */

/* Forward declarations. */
struct pgrhash;
typedef struct pgrhash pgrhash;
struct pg_catalog_table;
typedef struct pg_catalog_table pg_catalog_table;

/* Tri-value logic for handling table and column selection. */
enum trivalue
{
	TRI_DEFAULT,
	TRI_NO,
	TRI_YES
};

/* Defined check types. */
typedef enum checktype
{
	CHECK_ATTNUM,
	CHECK_OID_REFERENCE,
	CHECK_OID_VECTOR_REFERENCE,
	CHECK_OID_ARRAY_REFERENCE,
	CHECK_DEPENDENCY_CLASS_ID,
	CHECK_DEPENDENCY_ID,
	CHECK_DEPENDENCY_SUBID,
	CHECK_RELNATTS,
}	checktype;

/* Generic catalog check structure. */
typedef struct pg_catalog_check
{
	checktype	type;
}	pg_catalog_check;

/* Specialization of pg_catalog_check for the various types of OID checks. */
typedef struct pg_catalog_check_oid
{
	checktype	type;
	bool		zero_oid_ok;
	char	   *oid_references_table;
}	pg_catalog_check_oid;


/* Everything we need to check a catalog column. */
typedef struct pg_catalog_column
{
	/* These columns are listed in definitions.c */
	char	   *name;
	char	   *cast;
	int			minimum_version;
	int			maximum_version;
	bool		is_edb_only;
	bool		is_key_column;
	bool		is_display_column;
	void	   *check;			/* some kind of pg_catalog_check object */

	/* These columns are populated at runtime. */
	bool		available;
	enum trivalue checked;
	bool		needed;
	void	   *check_private;	/* workspace for individual checks */
	int			result_column;	/* result column number */
} pg_catalog_column;

/* Everything we need to check an entire catalog table. */
struct pg_catalog_table
{
	/* These columns are listed in definitions.c */
	char	   *table_name;
	pg_catalog_column *cols;

	/* These columns are populated at runtime. */
	bool		available;		/* OK for this version? */
	enum trivalue checked;
	bool		needs_load;		/* Still needs to be loaded? */
	bool		needs_check;	/* Still needs to be checked? */
	PGresult   *data;			/* Table data. */
	pgrhash    *ht;				/* Hash of table data. */
	int			num_needs;		/* # of tables we depend on. */
	int			num_needs_allocated;	/* Allocated slots for same. */
	pg_catalog_table **needs;	/* Array of tables we depend on. */
	int			num_needed_by;	/* # of tables depending on us. */
	int			num_needed_by_allocated;		/* Allocated slots for same. */
	pg_catalog_table **needed_by;		/* Array of tables depending on us. */
};

/* Array of tables known to this tool. */
extern struct pg_catalog_table pg_catalog_tables[];

/* Identifying characteristics of the database to be checked. */
extern int	remote_version;		/* Version number. */
extern bool remote_is_edb;		/* Is it an EDB database? */
extern char *database_oid;		/* Database OID, if known. */

/* pg_catcheck.c */
extern pg_catalog_table *find_table_by_name(char *table_name);
extern pg_catalog_column *find_column_by_name(pg_catalog_table *, char *);
extern void add_table_dependency(pg_catalog_table *needs,
					 pg_catalog_table *needed_by);

/* check_attribute.c */
extern void prepare_to_check_attnum(pg_catalog_table *tab,
						pg_catalog_column *tabcol);
extern void check_attnum(pg_catalog_table *tab, pg_catalog_column *tabcol,
			 int rownum);

/* check_class.c */
extern void prepare_to_check_relnatts(pg_catalog_table *tab,
						  pg_catalog_column *tabcol);
extern void check_relnatts(pg_catalog_table *tab, pg_catalog_column *tabcol,
			   int rownum);

/* check_depend.c */
extern void prepare_to_check_dependency_class_id(pg_catalog_table *tab,
									 pg_catalog_column *tabcol);
extern void prepare_to_check_dependency_id(pg_catalog_table *tab,
							   pg_catalog_column *tabcol);
extern void prepare_to_check_dependency_subid(pg_catalog_table *tab,
								  pg_catalog_column *tabcol);
extern void check_dependency_class_id(pg_catalog_table *tab,
						  pg_catalog_column *tabcol, int rownum);
extern void check_dependency_id(pg_catalog_table *tab,
					pg_catalog_column *tabcol, int rownum);
extern void check_dependency_subid(pg_catalog_table *tab,
					   pg_catalog_column *tabcol, int rownum);

/* check_oids.c */
extern void prepare_to_check_oid_reference(pg_catalog_table *tab,
							   pg_catalog_column *tabcol);
extern void check_oid_reference(pg_catalog_table *tab,
					pg_catalog_column *tabcol, int rownum);

/* log.c */
typedef enum pgcc_severity
{
	PGCC_DEBUG,					/* Debugging messages for developers. */
	PGCC_VERBOSE,				/* Verbose messages. */
	PGCC_PROGRESS,				/* Progress messages. */
	PGCC_NOTICE,				/* Database inconsistencies. */
	PGCC_WARNING,				/* Warnings other than inconsistencies. */
	PGCC_ERROR,					/* Serious but not fatal errors. */
	PGCC_FATAL					/* Fatal errors. */
}	pgcc_severity;

extern bool quiet;
extern int	verbose;

extern void
pgcc_log(pgcc_severity sev, char *fmt,...)
__attribute__((format(PG_PRINTF_ATTRIBUTE, 2, 3)));
extern void
pgcc_report(pg_catalog_table *tab, pg_catalog_column *tabcol,
			int rownum, char *fmt,...)
__attribute__((format(PG_PRINTF_ATTRIBUTE, 4, 5)));
extern void pgcc_log_completion(void);

/* pgrhash.c */

#define		MAX_KEY_COLS		10
extern pgrhash *pgrhash_create(PGresult *result, int nkeycols, int *keycols);
extern int	pgrhash_get(pgrhash *ht, char **keyvals);
extern int	pgrhash_insert(pgrhash *ht, int rownum);

#endif   /* PGCATCHECK_H */
