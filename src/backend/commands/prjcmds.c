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

#include "catalog/ygp_prj.h"


ObjectAddress
DefineProjection(Oid relationId,
			CreateProjectionStmt *stmt,
			Oid prjRelationId,
			bool check_rights)
{
	elog(LOG, "creating projection");
}
