
# Copyright (c) 2021-2023, PostgreSQL Global Development Group

use strict;
use warnings;

use PostgreSQL::Test::Cluster;
use PostgreSQL::Test::Utils;
use Test::More;

# Initialize primary node. Force UTF-8 encoding, so that we can use non-ASCII
# characters in the passwords below.
my $node = PostgreSQL::Test::Cluster->new('primary');
$node->init(extra => [ '--locale=C', '--encoding=UTF8' ]);
$node->start;


# Create test roles.
$node->safe_psql(
	'postgres',
	"SET password_encryption='scram-sha-256';
SET client_encoding='utf8';
CREATE USER mdbsar_test_role LOGIN PASSWORD 'test';

CREATE ROLE mdb_service_auth LOGIN PASSWORD 'serv';
CREATE ROLE sup LOGIN SUPERUSER PASSWORD '123';
");


# Delete pg_hba.conf from the given node, add a new entry to it
# and then execute a reload to refresh it.
sub reset_pg_hba
{
	my $node       = shift;
	my $hba_method = shift;

	unlink($node->data_dir . '/pg_hba.conf');
	$node->append_conf('pg_hba.conf', "local all all $hba_method");
	$node->reload;
	return;
}

# Require password from now on.
reset_pg_hba($node, 'scram-sha-256');



# Test access for a single role, useful to wrap all tests into one.
sub test_login
{
	local $Test::Builder::Level = $Test::Builder::Level + 1;

	my $node          = shift;
	my $role          = shift;
	my $password      = shift;
	my $expected_res  = shift;
	my $add_serv_role = shift;
	my $status_string = 'failed';

	$status_string = 'success' if ($expected_res eq 0);

	my $connstr = "user=$role";
	my $testname =
	  "authentication $status_string for role $role with password $password";

	$ENV{"PGPASSWORD"} = $password;
	if ($add_serv_role eq 1)
	{
		$ENV{"PGSERVICEAUTHROLE"} = 'mdb_service_auth';
	}

	if ($expected_res eq 0)
	{
		$node->connect_ok($connstr, $testname);
	}
	else
	{
		# No checks of the error message, only the status code.
		$node->connect_fails($connstr, $testname);
	}
}


test_login($node, 'mdb_service_auth', "serv", 0, 0);
test_login($node, 'mdbsar_test_role', "serv", 2, 0);
test_login($node, 'mdbsar_test_role', "test", 0, 0);

test_login($node, 'sup', "123", 0, 0);

test_login($node, 'mdb_service_auth', "serv", 0, 1);
test_login($node, 'mdbsar_test_role', "serv", 0, 1);
test_login($node, 'mdbsar_test_role', "test", 2, 1);

test_login($node, 'sup', "serv", 2, 1);

ok(1);
done_testing();

