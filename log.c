/*-------------------------------------------------------------------------
 *
 * log.c
 *
 * Logging support for the system catalog integrity checker.  This vaguely
 * parallel's the backend and pg_upgrade, but with some foibles specific
 * to our particular needs.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "pg_catcheck.h"

bool		quiet = false;		/* Don't display progress messages. */
int			verbose = 0;		/* 1 = verbose messages; 2+ = debug messages */

static int	notices = 0;
static int	warnings = 0;
static int	errors = 0;
static pgcc_severity highest_message_severity = PGCC_DEBUG;

static bool pgcc_log_severity(pgcc_severity sev);

/*
 * Log a message.  We write messages of level NOTICE and below to standard
 * output; anything higher goes to standard error.
 */
void
pgcc_log(pgcc_severity sev, char *fmt,...)
{
	va_list		args;

	if (!pgcc_log_severity(sev))
		return;

	va_start(args, fmt);
	if (sev <= PGCC_NOTICE)
		vfprintf(stdout, fmt, args);
	else
		vfprintf(stderr, fmt, args);
	va_end(args);

	if (sev >= PGCC_FATAL)
		exit(2);
}

/*
 * Report a catalog inconsistency; this always uses level PGCC_NOTICE.
 */
void
pgcc_report(pg_catalog_table *tab, pg_catalog_column *tabcol,
			int rownum, char *fmt,...)
{
	va_list		args;
	pg_catalog_column *displaytabcol;
	bool		first = true;

	if (!pgcc_log_severity(PGCC_NOTICE))
		return;
	if (tabcol != NULL)
		printf("%s row has invalid %s \"%s\": ", tab->table_name, tabcol->name,
			   PQgetvalue(tab->data, rownum, tabcol->result_column));

	va_start(args, fmt);
	vfprintf(stdout, fmt, args);
	va_end(args);

	for (displaytabcol = tab->cols;
		 displaytabcol->name != NULL;
		 ++displaytabcol)
	{
		if (displaytabcol->is_display_column)
		{
			printf("%s%s=\"%s\"", first ?
				   "row identity: " : " ", displaytabcol->name,
				PQgetvalue(tab->data, rownum, displaytabcol->result_column));
			first = false;
		}
	}
	if (!first)
		printf("\n");
}

/*
 * Report that we have completed our checks, and exit with an appropriate
 * status code.
 */
void
pgcc_log_completion(void)
{
	pgcc_log(PGCC_PROGRESS,
			 "done (%d inconsistencies, %d warnings, %d errors)\n",
			 notices, warnings, errors);
	if (highest_message_severity > PGCC_NOTICE)
		exit(2);
	else if (highest_message_severity == PGCC_NOTICE)
		exit(1);
	exit(0);
}

/*
 * Common code for pgcc_log() and pgcc_report().
 *
 * Determine whether a message of the indicated severity should be logged
 * given the command-line options specified by the user, and print out the
 * severity level on standard output or standard error as appropriate, so that
 * it prefixes the message.
 *
 * Also updates statistics on what messages have been logged so that
 * pgcc_log_completion() can make use of that information.
 */
static bool
pgcc_log_severity(pgcc_severity sev)
{
	switch (sev)
	{
		case PGCC_DEBUG:
			if (verbose < 2)
				return false;
			printf("debug: ");
			break;
		case PGCC_VERBOSE:
			if (verbose < 1)
				return false;
			printf("verbose: ");
			break;
		case PGCC_PROGRESS:
			if (quiet)
				return false;
			printf("progress: ");
			break;
		case PGCC_NOTICE:
			printf("notice: ");
			break;
		case PGCC_WARNING:
			fprintf(stderr, "warning: ");
			break;
		case PGCC_ERROR:
			fprintf(stderr, "error: ");
			break;
		case PGCC_FATAL:
			fprintf(stderr, "fatal: ");
			break;
	}

	if (sev == PGCC_NOTICE)
		++notices;
	if (sev == PGCC_WARNING)
		++warnings;
	if (sev == PGCC_ERROR)
		++errors;

	if (sev >= highest_message_severity)
		highest_message_severity = sev;

	return true;
}
