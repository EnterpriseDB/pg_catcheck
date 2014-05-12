/*-------------------------------------------------------------------------
 *
 * pgrhash.c
 *
 * Simple hash table implementation for text data stored in a PGresult.
 * The user can specify which columns are to serve as keys.  The code
 * is loosely based on the backend's dynahash.c, but is dramatically
 * simpler since we need only a small subset of the functionality offered
 * by that module.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "pg_catcheck.h"

typedef struct pgrhash_entry
{
	struct pgrhash_entry *next; /* link to next entry in same bucket */
	uint32		hashvalue;		/* hash function result for this entry */
	int			rownum;			/* row number of data in PGresult */
} pgrhash_entry;

struct pgrhash
{
	PGresult   *res;			/* pointer to PGresult data */
	int			nkeycols;		/* number of key columns */
	int			keycols[MAX_KEY_COLS];	/* array of key column indices */
	unsigned	nbuckets;		/* number of buckets */
	pgrhash_entry **bucket;		/* pointer to hash entries */
};

static uint32 string_hash_sdbm(const char *key);
static bool pgrhash_compare(pgrhash *ht, int rownum, char **keyvals);

/*
 * Create a new hash table for given result set, keyed by the indicate
 * column indexes, but do not populate it.	pgrhash_insert() should
 * be called separately for each row of the result set to actually
 * insert the rows.
 */
pgrhash *
pgrhash_create(PGresult *result, int nkeycols, int *keycols)
{
	unsigned	bucket_shift;
	pgrhash    *ht;

	Assert(nkeycols >= 1 && nkeycols <= MAX_KEY_COLS);

	bucket_shift = fls(PQntuples(result));
	if (bucket_shift >= sizeof(unsigned) * BITS_PER_BYTE)
		pgcc_log(PGCC_FATAL, "too many tuples");

	ht = (pgrhash *) pg_malloc(sizeof(pgrhash));
	ht->res = result;
	ht->nbuckets = ((unsigned) 1) << bucket_shift;
	ht->bucket = (pgrhash_entry **)
		pg_malloc0(ht->nbuckets * sizeof(pgrhash_entry *));
	ht->nkeycols = nkeycols;
	memcpy(ht->keycols, keycols, sizeof(int) * nkeycols);

	return ht;
}

/*
 * Search a result-set hash table for a row matching a given set of key values.
 *
 * The return value is the matching row number, or -1 if none.
 */
int
pgrhash_get(pgrhash *ht, char **keyvals)
{
	int			i;
	uint32		hashvalue = 0;
	pgrhash_entry *bucket;

	for (i = 0; i < ht->nkeycols; i++)
		hashvalue ^= string_hash_sdbm(keyvals[i]);

	for (bucket = ht->bucket[hashvalue & (ht->nbuckets - 1)];
		 bucket != NULL; bucket = bucket->next)
		if (pgrhash_compare(ht, bucket->rownum, keyvals))
			return bucket->rownum;

	return -1;
}

/*
 * Insert a row into a result-set hash table, provided no such row is already
 * present.
 *
 * The return value is -1 on success, or the row number of an existing row
 * with the same key.
 *
 * The only reason we expose this as a separate function, rather than making
 * it part of pgrhash_create, is that it allows callers to insert rows one
 * at a time and detect unexpected duplicate key violations.
 */
int
pgrhash_insert(pgrhash *ht, int rownum)
{
	unsigned	bucket_number;
	int			i;
	unsigned	hashvalue = 0;
	char	   *keyvals[MAX_KEY_COLS];
	pgrhash_entry *bucket;
	pgrhash_entry *entry;

	for (i = 0; i < ht->nkeycols; i++)
	{
		keyvals[i] = PQgetvalue(ht->res, rownum, ht->keycols[i]);
		hashvalue ^= string_hash_sdbm(keyvals[i]);
	}

	/* Check for a conflicting entry already present in the table. */
	bucket_number = hashvalue & (ht->nbuckets - 1);
	for (bucket = ht->bucket[bucket_number];
		 bucket != NULL; bucket = bucket->next)
		if (pgrhash_compare(ht, bucket->rownum, keyvals))
			return bucket->rownum;

	/* Insert the new entry. */
	entry = pg_malloc(sizeof(pgrhash_entry));
	entry->next = ht->bucket[bucket_number];
	entry->rownum = rownum;
	ht->bucket[bucket_number] = entry;

	return -1;
}

/*
 * Simple string hash function from http://www.cse.yorku.ca/~oz/hash.html
 *
 * The backend uses a more sophisticated function for hashing strings,
 * but we don't really need that complexity here.  Most of the values
 * that we're hashing are short integers formatted as text, so there
 * shouldn't be much room for pathological input.
 */
static uint32
string_hash_sdbm(const char *key)
{
	uint32		hash = 0;
	int			c;

	while ((c = *key++))
		hash = c + (hash << 6) + (hash << 16) - hash;

	return hash;
}

/*
 * Test whether the given row number is match for the supplied keys.
 */
static bool
pgrhash_compare(pgrhash *ht, int rownum, char **keyvals)
{
	int			i;
	char	   *keycol;
	char	   *keyval;

	for (i = 0; i < ht->nkeycols; i++)
	{
		keycol = PQgetvalue(ht->res, rownum, ht->keycols[i]);
		keyval = keyvals[i];

		if (strcmp(keycol, keyval) != 0)
			return false;
	}

	return true;
}
