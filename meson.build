pg_catcheck_sources = files(
'pg_catcheck.c',
'check_attribute.c',
'check_class.c',
'check_depend.c',
'check_oids.c',
'compat.c',
'definitions.c',
'log.c',
'pgrhash.c',
'select_from_relations.c',
)

if host_system == 'windows'
  pg_catcheck_sources += rc_lib_gen.process(win32ver_rc, extra_args: [
    '--NAME', 'pg_catcheck',
    '--FILEDESC', 'pg_catcheck - diagnosing system catalog corruption',])
endif

pg_catcheck = executable('pg_catcheck',
  pg_catcheck_sources,
  dependencies: [frontend_code, libpq],
  kwargs: default_bin_args,
)

contrib_targets += pg_catcheck