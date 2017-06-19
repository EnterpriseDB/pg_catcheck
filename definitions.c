/*-------------------------------------------------------------------------
 *
 * definitions.c
 *
 * This file defines the data structures that drive our checking
 * strategies.	We define the names of each column, the versions to which
 * it applies, whether or not it forms part of the table's key, whether it
 * should be included in diagnostics regarding that table, and, if
 * applicable, the type of check that should be performed on it.
 *
 * Some columns, such as OID columns, are included even though no check
 * is defined.	This is because they're part of the key: some other table
 * might contain that OID, and we'll need to look it up in the referenced
 * table.  Note that we don't bother defining the key for all tables that
 * have one; even if a table has a unique key, there's no point in
 * building a hash table to allow lookups into that table by key unless
 * we require the ability to perform suchh lookups.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres_fe.h"

#include "pg_catcheck.h"

struct pg_catalog_check_oid check_am_oid =
{CHECK_OID_REFERENCE, false, "pg_am"};
struct pg_catalog_check_oid check_am_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_am"};
struct pg_catalog_check_oid check_attnum_value =
{CHECK_ATTNUM};
struct pg_catalog_check_oid check_authid_oid =
{CHECK_OID_REFERENCE, false, "pg_authid"};
struct pg_catalog_check_oid check_authid_oid_array_zero_ok =
{CHECK_OID_ARRAY_REFERENCE, true, "pg_authid"};
struct pg_catalog_check_oid check_authid_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_authid"};
struct pg_catalog_check_oid check_class_oid =
{CHECK_OID_REFERENCE, false, "pg_class"};
struct pg_catalog_check_oid check_class_oid_array =
{CHECK_OID_ARRAY_REFERENCE, false, "pg_class"};
struct pg_catalog_check_oid check_class_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_class"};
struct pg_catalog_check_oid check_constraint_oid =
{CHECK_OID_REFERENCE, false, "pg_constraint"};
struct pg_catalog_check_oid check_collation_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_collation"};
struct pg_catalog_check_oid check_collation_optional_oid_vector =
{CHECK_OID_VECTOR_REFERENCE, true, "pg_collation"};
struct pg_catalog_check_oid check_constraint_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_constraint"};
struct pg_catalog_check_oid check_database_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_database"};
struct pg_catalog_check check_dependency_id_value =
{CHECK_DEPENDENCY_ID};
struct pg_catalog_check check_dependency_class_id_value =
{CHECK_DEPENDENCY_CLASS_ID};
struct pg_catalog_check check_dependency_subid_value =
{CHECK_DEPENDENCY_SUBID};
struct pg_catalog_check_oid check_edb_partdef =
{CHECK_OID_REFERENCE, false, "edb_partdef"};
struct pg_catalog_check_oid check_edb_partition_optional_oid =
{CHECK_OID_REFERENCE, true, "edb_partition"};
struct pg_catalog_check_oid check_foreign_data_wrapper_oid =
{CHECK_OID_REFERENCE, false, "pg_foreign_data_wrapper"};
struct pg_catalog_check_oid check_foreign_server_oid =
{CHECK_OID_REFERENCE, false, "pg_foreign_server"};
struct pg_catalog_check_oid check_foreign_server_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_foreign_server"};
struct pg_catalog_check_oid check_index_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_index"};
struct pg_catalog_check_oid check_language_oid =
{CHECK_OID_REFERENCE, false, "pg_language"};
struct pg_catalog_check_oid check_largeobject_metadata_oid =
{CHECK_OID_REFERENCE, false, "pg_largeobject_metadata"};
struct pg_catalog_check_oid check_namespace_oid =
{CHECK_OID_REFERENCE, false, "pg_namespace"};
struct pg_catalog_check_oid check_namespace_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_namespace"};
struct pg_catalog_check_oid check_opclass_oid =
{CHECK_OID_REFERENCE, false, "pg_opclass"};
struct pg_catalog_check_oid check_opclass_oid_vector =
{CHECK_OID_VECTOR_REFERENCE, false, "pg_opclass"};
struct pg_catalog_check_oid check_operator_oid =
{CHECK_OID_REFERENCE, false, "pg_operator"};
struct pg_catalog_check_oid check_operator_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_operator"};
struct pg_catalog_check_oid check_operator_oid_array =
{CHECK_OID_ARRAY_REFERENCE, false, "pg_operator"};
struct pg_catalog_check_oid check_opfamily_oid =
{CHECK_OID_REFERENCE, false, "pg_opfamily"};
struct pg_catalog_check_oid check_opfamily_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_opfamily"};
struct pg_catalog_check_oid check_proc_oid =
{CHECK_OID_REFERENCE, false, "pg_proc"};
struct pg_catalog_check_oid check_proc_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_proc"};
struct pg_catalog_check_oid check_profile_oid =
{CHECK_OID_REFERENCE, true, "edb_profile"};
struct pg_catalog_check_oid check_relnatts_value =
{CHECK_RELNATTS};
struct pg_catalog_check_oid check_tablespace_oid =
{CHECK_OID_REFERENCE, false, "pg_tablespace"};
struct pg_catalog_check_oid check_tablespace_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_tablespace"};
struct pg_catalog_check_oid check_ts_config_oid =
{CHECK_OID_REFERENCE, true, "pg_ts_config"};
struct pg_catalog_check_oid check_ts_dict_oid =
{CHECK_OID_REFERENCE, true, "pg_ts_dict"};
struct pg_catalog_check_oid check_ts_parser_oid =
{CHECK_OID_REFERENCE, true, "pg_ts_parser"};
struct pg_catalog_check_oid check_ts_template_oid =
{CHECK_OID_REFERENCE, true, "pg_ts_template"};
struct pg_catalog_check_oid check_type_oid =
{CHECK_OID_REFERENCE, false, "pg_type"};
struct pg_catalog_check_oid check_type_oid_array =
{CHECK_OID_ARRAY_REFERENCE, false, "pg_type"};
struct pg_catalog_check_oid check_type_oid_vector =
{CHECK_OID_VECTOR_REFERENCE, false, "pg_type"};
struct pg_catalog_check_oid check_type_optional_oid =
{CHECK_OID_REFERENCE, true, "pg_type"};
struct pg_catalog_check_oid check_queue_oid =
{CHECK_OID_REFERENCE, false, "edb_queue"};
struct pg_catalog_check_oid check_collation_oid =
{CHECK_OID_REFERENCE, false, "pg_collation"};
struct pg_catalog_check_oid check_publication_oid =
{CHECK_OID_REFERENCE, false, "pg_publication"};
struct pg_catalog_check_oid check_database_oid =
{CHECK_OID_REFERENCE, false, "pg_database"};
struct pg_catalog_check_oid check_subscription_oid =
{CHECK_OID_REFERENCE, false, "pg_subscription"};

/* pg_catalog_table & pg_catalog_column */
struct pg_catalog_column pg_class_column[] =
{
	/* pg_class */
	{"oid", NULL, 0, 0, false, true, true},
	{"relowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"relnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"relname", NULL, 0, 0, false, false, true},
	{"reltype", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"reloftype", NULL, 90000, 0, false, false, false, &check_type_optional_oid},
	{"relkind", NULL, 0, 0, false, false, true},
	{"relam", NULL, 0, 0, false, false, false, &check_am_optional_oid},
	{"relnatts", NULL, 0, 0, false, false, false, &check_relnatts_value},
	{"reltablespace", NULL, 0, 0, false, false, false, &check_tablespace_optional_oid},
	{"reltoastrelid", NULL, 0, 0, false, false, false, &check_class_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_namespace_column[] =
{
	/* pg_namespace */
	{"oid", NULL, 0, 0, false, true, true},
	{"nspowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"nspparent", NULL, 0, 0, true, false, false, &check_namespace_optional_oid},
	{"nspobjecttype", NULL, 90200, 0, true, false, false, &check_type_optional_oid},
	{"nspforeignserver", NULL, 0, 0, true, false, false, &check_foreign_server_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_authid_column[] =
{
	/* pg_authid */
	{"oid", NULL, 0, 0, false, true, true},
	{"rolname", NULL, 0, 0, false, false, true},
	{"rolprofile", NULL, 90500, 0, true, false, false, &check_profile_oid},
	{NULL}
};

struct pg_catalog_column pg_tablespace_column[] =
{
	/* pg_tablespace */
	{"oid", NULL, 0, 0, false, true, true},
	{"spcname", NULL, 0, 0, false, false, false},
	{"spcowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_type_column[] =
{
	/* pg_type */
	{"oid", NULL, 0, 0, false, true, true},
	{"typname", NULL, 0, 0, false, false, false},
	{"typowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"typnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"typrelid", NULL, 0, 0, false, false, false, &check_class_optional_oid},
	{"typelem", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"typarray", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"typbasetype", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"typcollation", NULL, 90100, 0, false, false, false, &check_collation_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_am_column[] =
{
	/* pg_am */
	{"oid", NULL, 0, 0, false, true, true},
	{"amkeytype", NULL, 0, 90599, false, false, false, &check_type_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_collation_column[] =
{
	/* pg_collation */
	{"oid", NULL, 90100, 0, false, true, true},
	{"collnamespace", NULL, 90100, 0, false, false, false, &check_namespace_oid},
	{"collowner", NULL, 90100, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_proc_column[] =
{
	/* pg_proc */
	{"oid", NULL, 0, 0, false, true, true},
	{"pronamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"proowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"prolang", NULL, 0, 0, false, false, false, &check_language_oid},
	{"provariadic", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"prorettype", NULL, 0, 0, false, false, false, &check_type_oid},
	{"proargtypes", NULL, 0, 0, false, false, false, &check_type_oid_vector},
	{"proallargtypes", NULL, 0, 0, false, false, false, &check_type_oid_array},
	{NULL}
};

struct pg_catalog_column pg_language_column[] =
{
	/* pg_language */
	{"oid", NULL, 0, 0, false, true, true},
	{"lanowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"lanplcallfoid", NULL, 0, 0, false, false, false},
	{"laninline", NULL, 90000, 0, false, false, false},
	{"lanvalidator", NULL, 0, 0, false, false, false},
	{NULL}
};

struct pg_catalog_column pg_index_column[] =
{
	/* pg_index */
	{"indexrelid", NULL, 0, 0, false, true, true},
	{"indrelid", NULL, 0, 0, false, false, false, &check_class_oid},
	{"indcollation", NULL, 90100, 0, false, false, false, &check_collation_optional_oid_vector},
	{"indclass", NULL, 0, 0, false, false, false, &check_opclass_oid_vector},
	{NULL}
};

struct pg_catalog_column pg_constraint_column[] =
{
	/* pg_constraint */
	{"oid", NULL, 0, 0, false, true, true},
	{"conname", NULL, 0, 0, false, false, false},
	{"connamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"conrelid", NULL, 0, 0, false, false, false, &check_class_optional_oid},
	{"contypid", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"conindid", NULL, 90000, 0, false, false, false, &check_index_optional_oid},
	{"confrelid", NULL, 0, 0, false, false, false, &check_class_optional_oid},
	{"conpfeqop", NULL, 0, 0, false, false, false, &check_operator_oid_array},
	{"conppeqop", NULL, 0, 0, false, false, false, &check_operator_oid_array},
	{"conffeqop", NULL, 0, 0, false, false, false, &check_operator_oid_array},
	{"conexclop", NULL, 90000, 0, false, false, false, &check_operator_oid_array},
	{NULL}
};

struct pg_catalog_column pg_database_column[] =
{
	/* pg_database */
	{"oid", NULL, 0, 0, false, true, true},
	{"datname", NULL, 0, 0, false, false, false},
	{"datdba", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"dattablespace", NULL, 0, 0, false, false, false, &check_tablespace_oid},
	{NULL}
};

struct pg_catalog_column pg_cast_column[] =
{
	/* pg_cast */
	{"oid", NULL, 0, 0, false, true, true},
	{"castsource", NULL, 0, 0, false, false, false, &check_type_oid},
	{"casttarget", NULL, 0, 0, false, false, false, &check_type_oid},
	{"castfunc", NULL, 0, 0, false, false, false, &check_proc_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_conversion_column[] =
{
	/* pg_conversion */
	{"oid", NULL, 0, 0, false, true, true},
	{"connamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"conowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"conproc", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{NULL}
};

struct pg_catalog_column pg_extension_column[] =
{
	/* pg_extension */
	{"oid", NULL, 90100, 0, false, true, true},
	{"extowner", NULL, 90100, 0, false, false, false, &check_authid_oid},
	{"extnamespace", NULL, 90100, 0, false, false, false, &check_namespace_oid},
	{"extconfig", NULL, 90100, 0, false, false, false, &check_class_oid_array},
	{NULL}
};

struct pg_catalog_column pg_enum_column[] =
{
	/* pg_enum */
	{"oid", NULL, 0, 0, false, true, true},
	{"enumtypid", NULL, 0, 0, false, false, false, &check_type_oid},
	{NULL}
};

struct pg_catalog_column pg_trigger_column[] =
{
	/* pg_trigger */
	{"oid", NULL, 0, 0, false, true, true},
	{"tgrelid", NULL, 0, 0, false, false, false, &check_class_oid},
	{"tgfoid", NULL, 0, 0, false, false, false, &check_proc_oid},
	{"tgconstrrelid", NULL, 0, 0, false, false, false, &check_class_optional_oid},
	{"tgconstrindid", NULL, 90000, 0, false, false, false, &check_index_optional_oid},
	{"tgconstraint", NULL, 0, 0, false, false, false, &check_constraint_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_ts_parser_column[] =
{
	/* pg_ts_parser */
	{"oid", NULL, 0, 0, false, true, true},
	{"prsnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"prsstart", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{"prstoken", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{"prsend", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{"prsheadline", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{"prslextype", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{NULL}
};

struct pg_catalog_column pg_ts_config_column[] =
{
	/* pg_ts_config */
	{"oid", NULL, 0, 0, false, true, true},
	{"cfgowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"cfgnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"cfgparser", NULL, 0, 0, false, false, false, &check_ts_parser_oid},
	{NULL}
};

struct pg_catalog_column pg_ts_template_column[] =
{
	/* pg_ts_template */
	{"oid", NULL, 0, 0, false, true, true},
	{"tmplnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"tmplinit", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{"tmpllexize", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{NULL}
};

struct pg_catalog_column pg_ts_dict_column[] =
{
	/* pg_ts_dict */
	{"oid", NULL, 0, 0, false, true, true},
	{"dictnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"dictowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"dicttemplate", NULL, 0, 0, false, false, false, &check_ts_template_oid},
	{NULL}
};

struct pg_catalog_column pg_fdw_column[] =
{
	/* pg_foreign_data_wrapper */
	{"oid", NULL, 0, 0, false, true, true},
	{"fdwowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"fdwhandler", NULL, 90100, 0, false, false, false, &check_proc_optional_oid},
	{"fdwvalidator", NULL, 0, 0, false, false, false, &check_proc_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_foreign_server_column[] =
{
	/* pg_foreign_server */
	{"oid", NULL, 0, 0, false, true, true},
	{"srvowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"srvfdw", NULL, 0, 0, false, false, false, &check_foreign_data_wrapper_oid},
	{NULL}
};

struct pg_catalog_column pg_user_mapping_column[] =
{
	/* pg_user_mapping */
	{"oid", NULL, 0, 0, false, true, true},
	{"umuser", NULL, 0, 0, false, false, false, &check_authid_optional_oid},
	{"umserver", NULL, 0, 0, false, false, false, &check_foreign_server_oid},
	{NULL}
};

struct pg_catalog_column pg_foreign_table_column[] =
{
	/* pg_foreign_table */
	{"ftrelid", NULL, 90100, 0, false, true, true, &check_class_oid},
	{"ftserver", NULL, 90100, 0, false, false, false, &check_foreign_server_oid},
	{NULL}
};

struct pg_catalog_column pg_event_trigger_column[] =
{
	/* pg_event_trigger */
	{"oid", NULL, 90300, 0, false, true, true},
	{"evtowner", NULL, 90300, 0, false, false, false, &check_authid_oid},
	{"evtfoid", NULL, 90300, 0, false, false, false, &check_proc_oid},
	{NULL}
};

struct pg_catalog_column pg_opfamily_column[] =
{
	/* pg_opfamily */
	{"oid", NULL, 0, 0, false, true, true},
	{"opfname", NULL, 0, 0, false, false, false},
	{"opfmethod", NULL, 0, 0, false, false, false, &check_am_oid},
	{"opfnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"opfowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_opclass_column[] =
{
	/* pg_opclass */
	{"oid", NULL, 0, 0, false, true, true},
	{"opcname", NULL, 0, 0, false, false, false},
	{"opcmethod", NULL, 0, 0, false, false, false, &check_am_oid},
	{"opcnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"opcowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"opcfamily", NULL, 0, 0, false, false, false, &check_opfamily_oid},
	{"opcintype", NULL, 0, 0, false, false, false, &check_type_oid},
	{"opckeytype", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_operator_column[] =
{
	/* pg_operator */
	{"oid", NULL, 0, 0, false, true, true},
	{"oprname", NULL, 0, 0, false, false, false},
	{"oprnamespace", NULL, 0, 0, false, false, false, &check_namespace_oid},
	{"oprowner", NULL, 0, 0, false, false, false, &check_authid_oid},
	{"oprleft", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"oprright", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"oprresult", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"oprcom", NULL, 0, 0, false, false, false, &check_operator_optional_oid},
	{"oprnegate", NULL, 0, 0, false, false, false, &check_operator_optional_oid},
	{"oprcode", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_optional_oid},
	{"oprrest", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_optional_oid},
	{"oprjoin", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_amop_column[] =
{
	/* pg_amop */
	{"oid", NULL, 0, 0, false, true, true},
	{"amopfamily", NULL, 0, 0, false, false, false, &check_opfamily_oid},
	{"amoplefttype", NULL, 0, 0, false, false, false, &check_type_oid},
	{"amoprighttype", NULL, 0, 0, false, false, false, &check_type_oid},
	{"amopopr", NULL, 0, 0, false, false, false, &check_operator_oid},
	{"amopmethod", NULL, 0, 0, false, false, false, &check_am_oid},
	{"amopsortfamily", NULL, 90100, 0, false, false, false, &check_opfamily_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_amproc_column[] =
{
	/* pg_amproc */
	{"oid", NULL, 0, 0, false, true, true},
	{"amprocfamily", NULL, 0, 0, false, false, false, &check_opfamily_oid},
	{"amproclefttype", NULL, 0, 0, false, false, false, &check_type_oid},
	{"amprocrighttype", NULL, 0, 0, false, false, false, &check_type_oid},
	{"amproc", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{NULL}
};

struct pg_catalog_column pg_default_acl_column[] =
{
	/* pg_default_acl */
	{"oid", NULL, 90000, 0, false, true, true},
	{"defaclnamespace", NULL, 90000, 0, false, false, false, &check_namespace_optional_oid},
	{"defaclrole", NULL, 90000, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_rewrite_column[] =
{
	/* pg_rewrite_column */
	{"oid", NULL, 0, 0, false, true, true},
	{"rulename", NULL, 0, 0, false, false, false},
	{"ev_class", NULL, 0, 0, false, false, false, &check_class_oid},
	{NULL}
};

struct pg_catalog_column edb_variable_column[] =
{
	/* edb_variable_column */
	{"oid", NULL, 0, 0, true, true, true},
	{"varpackage", NULL, 0, 0, true, false, false, &check_namespace_oid},
	{"vartype", NULL, 0, 0, true, false, false, &check_type_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_inherits_column[] =
{
	/* pg_inherits */
	{"inhrelid", NULL, 0, 0, false, true, true, &check_class_oid},
	{"inhparent", NULL, 0, 0, false, true, true, &check_class_oid},
	{NULL}
};

struct pg_catalog_column pg_largeobject_metadata_column[] =
{
	/* pg_largeobject_metadata */
	{"oid", NULL, 90000, 0, false, true, true},
	{"lomowner", NULL, 90000, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_largeobject_column[] =
{
	/* pg_largeobject */
	{"loid", NULL, 0, 0, false, true, true, &check_largeobject_metadata_oid},
	{"pageno", NULL, 0, 0, false, true, true},
	{NULL}
};

struct pg_catalog_column pg_aggregate_column[] =
{
	/* pg_aggregate */
	{"aggfnoid", "pg_catalog.oid", 0, 0, false, true, true, &check_proc_oid},
	{"aggtransfn", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_oid},
	{"aggfinalfn", "pg_catalog.oid", 0, 0, false, false, false, &check_proc_optional_oid},
	{"aggsortop", NULL, 0, 0, false, false, false, &check_operator_optional_oid},
	{"aggtranstype", NULL, 0, 0, false, false, false, &check_type_oid},
	{NULL}
};

struct pg_catalog_column pg_ts_config_map_column[] =
{
	/* pg_ts_config_map */
	{"mapcfg", NULL, 0, 0, false, true, true, &check_ts_config_oid},
	{"maptokentype", NULL, 0, 0, false, true, true},
	{"mapseqno", NULL, 0, 0, false, true, true},
	{"mapdict", NULL, 0, 0, false, false, false, &check_ts_dict_oid},
	{NULL}
};

struct pg_catalog_column pg_range_column[] =
{
	/* pg_range */
	{"rngtypid", NULL, 90200, 0, false, true, true, &check_type_oid},
	{"rngsubtype", NULL, 90200, 0, false, false, false, &check_type_oid},
	{"rngcollation", NULL, 90200, 0, false, false, false, &check_collation_optional_oid},
	{"rngsubopc", NULL, 90200, 0, false, false, false, &check_opclass_oid},
	{"rngcanonical", "pg_catalog.oid", 90200, 0, false, false, false, &check_proc_optional_oid},
	{"rngsubdiff", "pg_catalog.oid", 90200, 0, false, false, false, &check_proc_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_attribute_column[] =
{
	/* pg_attribute */
	{"attrelid", NULL, 0, 0, false, true, true, &check_class_oid},
	{"attname", NULL, 0, 0, false, false, true},
	{"attnum", NULL, 0, 0, false, true, true, &check_attnum_value},
	{"atttypid", NULL, 0, 0, false, false, false, &check_type_optional_oid},
	{"attcollation", NULL, 90100, 0, false, false, false, &check_collation_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_statistic_column[] =
{
	/* pg_statistic */
	{"starelid", NULL, 0, 0, false, true, true, &check_class_oid},
	{"staattnum", NULL, 0, 0, false, true, true},
	{"stainherit", NULL, 90000, 0, false, true, true},
	{"staop1", NULL, 0, 0, false, false, false, &check_operator_optional_oid},
	{"staop2", NULL, 0, 0, false, false, false, &check_operator_optional_oid},
	{"staop3", NULL, 0, 0, false, false, false, &check_operator_optional_oid},
	{"staop4", NULL, 0, 0, false, false, false, &check_operator_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_db_role_setting_column[] =
{
	/* pg_db_role_setting */
	{"setdatabase", NULL, 90000, 0, false, true, true, &check_database_optional_oid},
	{"setrole", NULL, 90000, 0, false, true, true, &check_authid_optional_oid},
	{NULL}
};

struct pg_catalog_column pg_attrdef_column[] =
{
	/* pg_attrdef */
	{"oid", NULL, 0, 0, false, true, true},
	{"adrelid", NULL, 0, 0, false, false, false, &check_class_oid},
	{NULL}
};

struct pg_catalog_column edb_dir_column[] =
{
	/* edb_dir */
	{"oid", NULL, 0, 0, true, true, true},
	{"dirowner", NULL, 0, 0, true, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column edb_partdef_column[] =
{
	/* edb_partdef */
	{"oid", NULL, 90100, 0, true, true, true},
	{"pdefrel", NULL, 90100, 0, true, false, false, &check_class_oid},
	{NULL}
};

struct pg_catalog_column edb_partition_column[] =
{
	/* edb_partition */
	{"oid", NULL, 90100, 0, true, true, true},
	{"partpdefid", NULL, 90100, 0, true, false, false, &check_edb_partdef},
	{"partrelid", NULL, 90100, 0, true, false, false, &check_class_oid},
	{"partparent", NULL, 90100, 0, true, false, false, &check_edb_partition_optional_oid},
	{"partcons", NULL, 90100, 0, true, false, false, &check_constraint_oid},
	{NULL}
};

/*
 * policyobject was originally envisioned to point either to a pg_class OID
 * or a pg_synonym OID depending on policykind, but the pg_synonym support was
 * never implemented.  So for now, we can just check that it's a pg_class OID.
 */
struct pg_catalog_column edb_policy_column[] =
{
	/* edb_policy */
	{"oid", NULL, 90100, 0, true, true, true},
	{"policygroup", NULL, 90100, 0, true, false, false},
	{"policyobject", NULL, 90100, 0, true, false, false, &check_class_oid},
	{"policyproc", NULL, 90100, 0, true, false, false, &check_proc_oid},
	{NULL}
};

struct pg_catalog_column pg_synonym_column[] =
{
	/* pg_synonym */
	{"oid", NULL, 0, 0, true, true, true},
	{"synnamespace", NULL, 0, 0, true, false, false, &check_namespace_optional_oid},
	{"synowner", NULL, 0, 0, true, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_depend_column[] =
{
	/* pg_depend */
	{"classid", NULL, 0, 0, false, false, true, &check_dependency_class_id_value},
	{"objid", NULL, 0, 0, false, false, true, &check_dependency_id_value},
	{"objsubid", NULL, 0, 0, false, false, true, &check_dependency_subid_value},
	{"refclassid", NULL, 0, 0, false, false, true, &check_dependency_class_id_value},
	{"refobjid", NULL, 0, 0, false, false, true, &check_dependency_id_value},
	{"refobjsubid", NULL, 0, 0, false, false, true, &check_dependency_subid_value},
	{"deptype", NULL, 0, 0, false, false, true},
	{NULL}
};

struct pg_catalog_column pg_shdepend_column[] =
{
	/* pg_shdepend */
	{"dbid", NULL, 0, 0, false, false, true, &check_database_optional_oid},
	{"classid", NULL, 0, 0, false, false, true, &check_dependency_class_id_value},
	{"objid", NULL, 0, 0, false, false, true, &check_dependency_id_value},
	{"objsubid", NULL, 0, 0, false, false, true, &check_dependency_subid_value},
	{"refclassid", NULL, 0, 0, false, false, true, &check_dependency_class_id_value},
	{"refobjid", NULL, 0, 0, false, false, true, &check_dependency_id_value},
	{"deptype", NULL, 0, 0, false, false, true},
	{NULL}
};

struct pg_catalog_column pg_description_column[] =
{
	/* pg_description */
	{"classoid", NULL, 0, 0, false, false, true, &check_dependency_class_id_value},
	{"objoid", NULL, 0, 0, false, false, true, &check_dependency_id_value},
	{"objsubid", NULL, 0, 0, false, false, true, &check_dependency_subid_value},
	{NULL}
};

struct pg_catalog_column pg_shdescription_column[] =
{
	/* pg_shdescription */
	{"classoid", NULL, 0, 0, false, false, true, &check_dependency_class_id_value},
	{"objoid", NULL, 0, 0, false, false, true, &check_dependency_id_value},
	{NULL}
};

struct pg_catalog_column pg_seclabel_column[] =
{
	/* pg_seclabel */
	{"classoid", NULL, 90100, 0, false, false, true, &check_dependency_class_id_value},
	{"objoid", NULL, 90100, 0, false, false, true, &check_dependency_id_value},
	{"objsubid", NULL, 90100, 0, false, false, true, &check_dependency_subid_value},
	{"provider", NULL, 90100, 0, false, false, true},
	{NULL}
};

struct pg_catalog_column pg_shseclabel_column[] =
{
	/* pg_shseclabel */
	{"classoid", NULL, 90200, 0, false, false, true, &check_dependency_class_id_value},
	{"objoid", NULL, 90200, 0, false, false, true, &check_dependency_id_value},
	{"provider", NULL, 90200, 0, false, false, true, NULL},
	{NULL}
};

struct pg_catalog_column pg_auth_members_column[] =
{
	{"roleid", NULL, 0, 0, false, false, true, &check_authid_oid},
	{"member", NULL, 0, 0, false, false, true, &check_authid_oid},
	{"grantor", NULL, 0, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_policy_column[] =
{
	{"oid", NULL, 90500, 0, false, true, true},
	{"polname", NULL, 90500, 0, false, false, false},
	{"polrelid", NULL, 90500, 0, false, false, false, &check_class_oid},
	{"polroles", NULL, 90500, 0, false, false, false,
		&check_authid_oid_array_zero_ok},
	{NULL}
};

struct pg_catalog_column edb_profile_column [] =
{
	{"oid", NULL, 90500, 0, true, true, true},
	{"prfname", NULL, 90500, 0, true, false, true},
	{NULL}
};

struct pg_catalog_column edb_queue_table_column[] =
{
	{"oid", NULL, 90600, 0, true, true, true},
	{"qtname", NULL, 90600, 0, true, false, true},
	{"qtnamespace", NULL, 90600, 0, true, false, false, &check_namespace_oid},
	{"qtrelid", NULL, 90600, 0, true, false, false, &check_class_oid},
	{"qpayloadtype", NULL, 90600, 0, true, false, false, &check_type_oid},
	{NULL}
};

struct pg_catalog_column edb_queue_column [] =
{
	{"oid", NULL, 90600, 0, true, true, true},
	{"aqname", NULL, 90600, 0, true, false, true},
	{"aqrelid", NULL, 90600, 0, true, false, false, &check_class_oid},
	{NULL}
};

struct pg_catalog_column edb_password_history_column[] =
{
	/* edb_password_history */
	{"passhistroleid", NULL, 90500, 0, true, true, true, &check_authid_oid},
	{"passhistpassword", NULL, 90500, 0, true, true, true},
	{"passhistpasswordsetat", NULL, 90500, 0, true, false, true},
	{NULL}
};

struct pg_catalog_column edb_queue_callback_column[] =
{
	/* edb_queue_callback */
	{"oid", NULL, 90600, 0, true, true, true},
	{"qcbqueueid", NULL, 90600, 0, true, false, true, &check_queue_oid},
	{"qcbowner", NULL, 90600, 0, true, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column edb_resource_group_column[] =
{
	/* edb_resource_group */
	{"oid", NULL, 90400, 0, true, true, true},
	{"rgrpname", NULL, 90400, 0, true, false, true},
	{NULL}
};

struct pg_catalog_column pg_init_privs_column[] =
{
	/* pg_init_privs */
	{"objoid", NULL, 90600, 0, false, true, true},
	{"classoid", NULL, 90600, 0, false, true, true, &check_class_oid},
	{"objsubid", NULL, 90600, 0, false, true, true},
	{NULL}
};

struct pg_catalog_column pg_partitioned_table_column[] =
{
	/* pg_partitioned_table */
	{"partrelid", NULL, 100000, 0, false, true, true, &check_class_oid},
	{"partclass", NULL, 100000, 0, false, false, false, &check_opclass_oid_vector},
	{"partcollation", NULL, 100000, 0, false, false, false, &check_collation_optional_oid_vector},
	{NULL}
};

struct pg_catalog_column pg_pltemplate_column[] =
{
	/* pg_pltemplate */
	{"tmplname", NULL, 0, 0, false, true, true},
	{NULL}
};

struct pg_catalog_column pg_publication_column[] =
{
	/* pg_publication */
	{"oid", NULL, 100000, 0, false, true, true},
	{"pubowner", NULL, 100000, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_publication_rel_column[] =
{
	/* pg_publication_rel */
	{"oid", NULL, 100000, 0, false, true, true},
	{"prpubid", NULL, 100000, 0, false, false, false, &check_publication_oid},
	{"prrelid", NULL, 100000, 0, false, false, false, &check_class_oid},
	{NULL}
};

struct pg_catalog_column pg_replication_origin_column[] =
{
	/* pg_replication_origin */
	{"roident", NULL, 90500, 0, false, true, true},
	{NULL}
};

struct pg_catalog_column pg_sequence_column[] =
{
	/* pg_sequence */
	{"seqrelid", NULL, 100000, 0, false, true, true, &check_class_oid},
	{"seqtypid", NULL, 100000, 0, false, false, false, &check_type_oid},
	{NULL}
};

struct pg_catalog_column pg_statistic_ext_column[] =
{
	/* pg_statistic_ext */
	{"oid", NULL, 100000, 0, false, true, true},
	{"stxrelid", NULL, 100000, 0, false, false, false, &check_class_oid},
	{"stxnamespace", NULL, 100000, 0, false, false, false, &check_namespace_oid},
	{"stxowner", NULL, 100000, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_subscription_column[] =
{
	/* pg_subscription */
	{"oid", NULL, 100000, 0, false, true, true},
	{"subdbid", NULL, 100000, 0, false, false, false, &check_database_oid},
	{"subowner", NULL, 100000, 0, false, false, false, &check_authid_oid},
	{NULL}
};

struct pg_catalog_column pg_subscription_rel_column[] =
{
	/* pg_subscription_rel */
	{"srsubid", NULL, 100000, 0, false, true, true, &check_subscription_oid},
	{"srrelid", NULL, 100000, 0, false, true, true, &check_class_oid},
	{NULL}
};

struct pg_catalog_column pg_transform_column[] =
{
	/* pg_transform */
	{"oid", NULL, 90500, 0, false, true, true},
	{"trftype", NULL, 90500, 0, false, false, false, &check_type_oid},
	{"trflang", NULL, 90500, 0, false, false, false, &check_language_oid},
	{"trffromsql", NULL, 90500, 0, false, false, false, &check_proc_oid},
	{"trftosql", NULL, 90500, 0, false, false, false, &check_proc_oid},
	{NULL}
};

struct pg_catalog_table pg_catalog_tables[] =
{
	{"pg_class", pg_class_column},
	{"pg_namespace", pg_namespace_column},
	{"pg_authid", pg_authid_column},
	{"pg_tablespace", pg_tablespace_column},
	{"pg_type", pg_type_column},
	{"pg_am", pg_am_column},
	{"pg_collation", pg_collation_column},
	{"pg_proc", pg_proc_column},
	{"pg_language", pg_language_column},
	{"pg_index", pg_index_column},
	{"pg_constraint", pg_constraint_column},
	{"pg_database", pg_database_column},
	{"pg_cast", pg_cast_column},
	{"pg_conversion", pg_conversion_column},
	{"pg_extension", pg_extension_column},
	{"pg_enum", pg_enum_column},
	{"pg_trigger", pg_trigger_column},
	{"pg_ts_parser", pg_ts_parser_column},
	{"pg_ts_config", pg_ts_config_column},
	{"pg_ts_template", pg_ts_template_column},
	{"pg_ts_dict", pg_ts_dict_column},
	{"pg_foreign_data_wrapper", pg_fdw_column},
	{"pg_foreign_server", pg_foreign_server_column},
	{"pg_user_mapping", pg_user_mapping_column},
	{"pg_foreign_table", pg_foreign_table_column},
	{"pg_event_trigger", pg_event_trigger_column},
	{"pg_opfamily", pg_opfamily_column},
	{"pg_opclass", pg_opclass_column},
	{"pg_operator", pg_operator_column},
	{"pg_amop", pg_amop_column},
	{"pg_amproc", pg_amproc_column},
	{"pg_default_acl", pg_default_acl_column},
	{"pg_rewrite", pg_rewrite_column},
	{"pg_inherits", pg_inherits_column},
	{"pg_largeobject_metadata", pg_largeobject_metadata_column},
	{"pg_largeobject", pg_largeobject_column},
	{"pg_aggregate", pg_aggregate_column},
	{"pg_ts_config_map", pg_ts_config_map_column},
	{"pg_range", pg_range_column},
	{"pg_attrdef", pg_attrdef_column},
	{"pg_attribute", pg_attribute_column},
	{"pg_statistic", pg_statistic_column},
	{"pg_db_role_setting", pg_db_role_setting_column},
	{"pg_depend", pg_depend_column},
	{"pg_shdepend", pg_shdepend_column},
	{"edb_dir", edb_dir_column},
	{"edb_partdef", edb_partdef_column},
	{"edb_partition", edb_partition_column},
	{"edb_policy", edb_policy_column},
	{"pg_synonym", pg_synonym_column},
	{"edb_variable", edb_variable_column},
	{"pg_description", pg_description_column},
	{"pg_shdescription", pg_shdescription_column},
	{"pg_seclabel", pg_seclabel_column},
	{"pg_shseclabel", pg_shseclabel_column},
	{"pg_auth_members", pg_auth_members_column},
	{"pg_policy", pg_policy_column},
	{"edb_profile", edb_profile_column},
	{"edb_queue_table", edb_queue_table_column},
	{"edb_queue", edb_queue_column},
	{"edb_password_history", edb_password_history_column},
	{"edb_queue_callback", edb_queue_callback_column},
	{"edb_resource_group", edb_resource_group_column},
	{"pg_init_privs", pg_init_privs_column},
	{"pg_partitioned_table", pg_partitioned_table_column},
	{"pg_pltemplate", pg_pltemplate_column},
	{"pg_publication", pg_publication_column},
	{"pg_publication_rel", pg_publication_rel_column},
	{"pg_replication_origin", pg_replication_origin_column},
	{"pg_sequence", pg_sequence_column},
	{"pg_statistic_ext", pg_statistic_ext_column},
	{"pg_subscription", pg_subscription_column},
	{"pg_subscription_rel", pg_subscription_rel_column},
	{"pg_transform", pg_transform_column},
	{NULL}
};
