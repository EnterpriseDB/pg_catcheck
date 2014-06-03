What is pg_catcheck?
====================

pg_catcheck is a simple tool for diagnosing system catalog corruption.
If you suspect that your system catalogs are corrupted, this tool may
help you figure out exactly what problems you have and how serious they
are.  If you are paranoid, you can run it routinely to search for system
catalog corruption that might otherwise go undetected.  However, pg_catcheck
is not a general corruption detector.  For that, you should use PostgreSQL's
checksum feature (`initdb -k`).

PostgreSQL stores the metadata for SQL objects such as tables and functions
using special tables called system catalog tables.  Users do not normally
modify these tables directly, but instead modify them using SQL commands
such as CREATE, ALTER, and DROP.  If the system catalog tables become
corrupted, you may experience errors when attempting to access your data.
Sometimes, it can be impossible to back up your data using pg_dump without
correcting these errors.  pg_catcheck won't tell you how to your database
got corrupted in the first place, and it won't tell you how to fix it.
But it will usually be able to give you detailed information about what
is broken, which may make it easier for you (or your PostgreSQL support
provider) to understand what has gone wrong and explain the options for
recovery.

How Do I Run pg_catcheck?
=========================

pg_catcheck takes the same arguments as most other PostgreSQL utilites,
such as -h for the host or -p for the port.  You can also pass it a
connection string or URL, just like psql.  For a full list of options,
run `pg_catcheck --help`.  If pg_catcheck isn't already installed, you might
need to build it first.  If no pre-compiled binary package is available for
you to install, see the instructions below for "Building on UNIX/Linux" and
"Building on Windows".

When you run pg_catcheck, it will normally print out a line that looks like
this:

	progress: done (0 inconsistencies, 0 warnings, 0 errors)

If you see that line, it means pg_catcheck didn't find any problems.
Otherwise, pg_catcheck will generally print two lines of output for each
problem it finds, like this:

	notice: pg_class row has invalid relnamespace "24580": no matching entry in pg_namespace
	row identity: oid="24581" relname="foo" relkind="r"
	notice: pg_type row has invalid typnamespace "24580": no matching entry in pg_namespace
	row identity: oid="24583"
	notice: pg_type row has invalid typnamespace "24580": no matching entry in pg_namespace
	row identity: oid="24582"
	notice: pg_depend row has invalid refobjid "24580": no matching entry in pg_namespace
	row identity: classid="1259" objid="24581" objsubid="0" refclassid="2615" refobjid="24580" refobjsubid="0" deptype="n"
	progress: done (4 inconsistencies, 0 warnings, 0 errors)

If the final output line mentions inconsistencies, that means that it found
problems with the logical structure of your system catalogs.  Warnings or
errors indicate more serious problems, like not being able to read the system
catalogs at all.  In this particular example, there are four errors: one
pg_class row, two pg_type rows, and one pg_depend row all reference an OID
24580 which they expect to find in pg_namespace.  In reality, no such row
exists.

There are several ways to recover from an error of this type.  You could
modify the OIDs in the referring rows so that they refer to a pg_namespace
entry that does exist.  This might enable you to recover access to the
underlying data.  Note that in this case all four references pertain to the
same table (pg_class OID 24581, which has pg_type OIDs 24582 and 24583 for
its record and array-of-record types) so you would probably want to make
all of those references point to the same namespace.  Alternatively, if the
dangling references are objects you don't care about (e.g. if the backing
file for the pg_class entry doesn't even exist on disk), you could simply
delete the referring rows also.  This is often enough to make pg_dump run
successfully, which is often the main goal.

Unless you are sure you understand what pg_catcheck is telling you, you
may wish to consult with a PostgreSQL expert.  Changing the system catalogs
manually can make a bad situation worse and lead to data loss, and should
not be attempted unless you are knowledgeable about how PostgreSQL uses these
catalogs.

What is the license for pg_catcheck?  Can I contribute?
=======================================================

pg_catcheck was initially developed by EnterpriseDB and is released under
the same license as PostgreSQL itself.  Patches are welcome.  Please subscribe
to our mailing list, pg-catcheck@enterprisedb.com, by visiting:

https://groups.google.com/a/enterprisedb.com/forum/#!forum/pg-catcheck

How do I get support for pg_catcheck?
=====================================

As with any open source project, you may be able to obtain support via the
public mailing list, which is pg-catcheck@enterprisedb.com; to subscribe,
visit:

https://groups.google.com/a/enterprisedb.com/forum/#!forum/pg-catcheck

If you need commercial support, please contact the EnterpriseDB sales
team, or check whether your existing PostgreSQL support provider can also
support pg_catcheck.

What versions of PostgreSQL does pg_catcheck support?
=====================================================

pg_catcheck should work when run against a server running PostgreSQL 8.4
or higher.  It also should also work when run against a server running
EnterpriseDB's Advanced Server product, version 8.4 or higher.  To
compile pg_catcheck, you will need to build against a server source tree
version 9.0 or higher, because it relies on the function PQconnectdbParams(),
which did not exist in 8.4.

Building on UNIX/Linux
======================

* Make sure that you have a working pg_config executable in your path.
  (If you are using a binary installation of PostgreSQL, you might need
  to install additional packages, such as postgresql-devel or libpq-dev.)
* Run "make" and, if desired, "make install".
* To remove generated files, run "make clean".

Building on Windows
===================

* Build PostgreSQL with MSVC as described in
  http://www.postgresql.org/docs/devel/static/install-windows-full.html
* Start the Visual Studio Command Prompt. If you wish to build a 64-bit
  version, you must use the 64-bit version of the command.
* "cd" to the source directory (e.g. cd c:\pg_catcheck)
* Use a command like "msbuild /p:PGPATH=C:\postgresql-9.4.0 /p:DEBUG=0
  /p:ARCH=x64" to perform the actual build.  PGPATH should be set to the
  location of the PostgreSQL source code, and DEBUG should be set to 1 for
  a debug build.
* If you wish to remove the generated files, use "msbuild /target:clean".
