/*-------------------------------------------------------------------------
 *
 * prjcmds.c
 *	  projections
 *
 *
 * Copyright (c) 2024-, Mother Russia
 *
 * IDENTIFICATION
 *	  src/backend/commands/prjcmds.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "nodes/parsenodes.h"
#include "commands/defrem.h"
#include "catalog/heap.h"
#include "catalog/namespace.h"
#include "catalog/pg_am_d.h"
#include "catalog/pg_class_d.h"
#include "catalog/ygp_prj.h"
#include "parser/parse_utilcmd.h"
#include "access/table.h"
#include "utils/rel.h"

#include "catalog/oid_dispatch.h"
#include "access/xact.h"

#include "cdb/cdbvars.h"

ObjectAddress
DefineProjection(Oid relationId,
			CreateProjectionStmt *stmt,
			Oid prjRelationId,
			bool check_rights)
{
	GpPolicy   *policy;
	TupleDesc	descriptor;
	Oid			namespaceId;
	Relation rel;
	Oid prjOid;
	ObjectAddress address;

	rel = table_open(relationId, ShareLock);
	/*
	 * Look up the namespace in which we are supposed to create the prjection,
	 * check we have permission to create there, lock it against concurrent
	 * drop, and mark stmt->relation as RELPERSISTENCE_TEMP if a temporary
	 * namespace is selected.
	 */
	namespaceId =
		RangeVarGetAndCheckCreationNamespace(stmt->relation, NoLock, NULL);

	/*
	 * Create a tuple descriptor from the relation schema.  Note that this
	 * deals with column names, types, and NOT NULL constraints, but not
	 * default values or CHECK constraints; we handle those below.
	 */
	/* todo: get schem a from query */
	// descriptor = BuildDescForRelation(schema);
	descriptor = RelationGetDescr(rel);

	policy = getPolicyForDistributedBy(stmt->distributedBy, descriptor);


	/* prj_create_with_catalog */
	prjOid = heap_create_with_catalog(
		stmt->prjname,
		namespaceId,
		InvalidOid /* relation tablespace */,
		prjRelationId /* prj relation oid */,
		InvalidOid /* relation type oid */,
		InvalidOid /* rel of type oid */,
 		GetUserId() /* owner */,
		HEAP_TABLE_AM_OID /* access method -- fixx !! learn it from define stmt */,
		descriptor /* tuple desc */, 
		NIL /* cooked contrains */,
		RELKIND_PROJECTION /*relation kind*/,
		RELPERSISTENCE_PERMANENT /* persistance */,
		false /*shared*/, false /*mapped*/,
      	ONCOMMIT_NOOP, 
		 NULL /* GP Policy */, 
		(Datum)0,
		false /* use_user_acl */,
    	true,
		true,
		InvalidOid /*relrewrite*/,
		NULL, 
		false /* valid_opts */
	);


  	/* Make this prj visible */
	CommandCounterIncrement();


  if (Gp_role != GP_ROLE_DISPATCH) {
    /*  */

  } else {
    /* clear all pre-assigned oids */
    GetAssignedOidsForDispatch();
  }

	table_close(rel, ShareLock);

	ObjectAddressSet(address, ProjectionRelationId, prjOid);
	
	elog(LOG, "creating projection");

	return address;
}
