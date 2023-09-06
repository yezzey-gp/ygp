CREATE ROLE regress_mdb_admin_fdw_user;

GRANT mdb_admin TO regress_mdb_admin_fdw_user;
GRANT CREATE ON DATABASE regression TO regress_mdb_admin_fdw_user;

SET ROLE regress_mdb_admin_fdw_user;

CREATE EXTENSION postgres_fdw;

CREATE SERVER regress_test_fdw FOREIGN DATA WRAPPER postgres_fdw;

RESET SESSION AUTHORIZATION;
--
REVOKE CREATE ON DATABASE regression FROM regress_mdb_admin_fdw_user;

DROP SERVER regress_test_fdw;
DROP EXTENSION postgres_fdw;
DROP ROLE regress_mdb_admin_fdw_user;
