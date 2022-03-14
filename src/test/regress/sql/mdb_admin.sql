CREATE ROLE regress_mdb_admin_user1;
CREATE ROLE regress_mdb_admin_user2;
CREATE ROLE regress_mdb_admin_user3;

GRANT mdb_admin TO regress_mdb_admin_user1;
GRANT CREATE ON DATABASE regression TO regress_mdb_admin_user2;
GRANT CREATE ON DATABASE regression TO regress_mdb_admin_user3;

SET ROLE regress_mdb_admin_user2;
CREATE FUNCTION regress_mdb_admin_add(integer, integer) RETURNS integer
    AS 'SELECT $1 + $2;'
    LANGUAGE SQL
    IMMUTABLE
    RETURNS NULL ON NULL INPUT;

CREATE SCHEMA regress_mdb_admin_schema;
GRANT CREATE ON SCHEMA regress_mdb_admin_schema TO regress_mdb_admin_user3;
CREATE TABLE regress_mdb_admin_schema.regress_mdb_admin_table();
CREATE TABLE regress_mdb_admin_table();
CREATE VIEW regress_mdb_admin_view as SELECT 1;
SET ROLE regress_mdb_admin_user1;

ALTER FUNCTION regress_mdb_admin_add (integer, integer) OWNER TO regress_mdb_admin_user3;
ALTER VIEW regress_mdb_admin_view OWNER TO regress_mdb_admin_user3;
ALTER TABLE regress_mdb_admin_schema.regress_mdb_admin_table OWNER TO regress_mdb_admin_user3;
ALTER TABLE regress_mdb_admin_table OWNER TO regress_mdb_admin_user3;
ALTER SCHEMA regress_mdb_admin_schema OWNER TO regress_mdb_admin_user3;

RESET SESSION AUTHORIZATION;
--
REVOKE CREATE ON DATABASE regression FROM regress_mdb_admin_user2;
REVOKE CREATE ON DATABASE regression FROM regress_mdb_admin_user3;

DROP VIEW regress_mdb_admin_view;
DROP FUNCTION regress_mdb_admin_add(integer, integer);
DROP TABLE regress_mdb_admin_schema.regress_mdb_admin_table;
DROP TABLE regress_mdb_admin_table;
DROP SCHEMA regress_mdb_admin_schema;
DROP ROLE regress_mdb_admin_user1;
DROP ROLE regress_mdb_admin_user2;
DROP ROLE regress_mdb_admin_user3;
