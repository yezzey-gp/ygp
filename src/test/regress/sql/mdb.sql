SHOW idle_in_transaction_session_timeout;
SET idle_in_transaction_session_timeout to 500;
SHOW idle_in_transaction_session_timeout;


BEGIN;
CREATE TABLE t();
SELECT pg_sleep(1);
COMMIT;

-- success out of tx
SELECT pg_sleep(1);

SET idle_in_transaction_session_timeout to 0;

-- zero disables feature
BEGIN;
CREATE TABLE t();
SELECT pg_sleep(1);
COMMIT;


SHOW idle_session_timeout;
SET idle_session_timeout to 500;
SHOW idle_session_timeout;

-- sleep 10 sec
\! sleep 10 && echo done

-- should fail
SELECT * FROM gp_dist_random('gp_id');
