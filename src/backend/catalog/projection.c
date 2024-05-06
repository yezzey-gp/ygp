/*
* ygp_prj.c
*/

#include "postgres.h"

#include "nodes/execnodes.h"
#include "access/tableam.h"
#include "executor/executor.h"

#include "access/table.h"
#include "utils/relcache.h"
#include "utils/syscache.h"

#include "catalog/ygp_prj.h"
#include "catalog/projection.h"

/* ----------------------------------------------------------------
 *						projection_build support
 * ----------------------------------------------------------------
 */

/* ----------------
 *		BuildProjectionInfo
 *			Construct an ProjectionInfo record for an open projection
 *
 * ProjectionInfo stores the information about the projection that's needed by
 * FormProjectionDatum, which is used for both projection_build() and later insertion
 * of individual projection tuples.  Normally we build an ProjectionInfo for an projection
 * just once per command, and then use it for (potentially) many tuples.
 * ----------------
 */


PrjInfo *
BuildPrjInfo(Relation projection)
{
	PrjInfo  *pji = makeNode(PrjInfo);
	Form_pg_class rd_rel = projection->rd_rel;
	int			i;
	int			numAtts;

	Form_ygp_projection prj;
	HeapTuple projectionTuple;

	projectionTuple = SearchSysCache1(PROJECTIONOID, ObjectIdGetDatum(RelationGetRelid(projection)));
	if (!HeapTupleIsValid(projectionTuple))	/* should not happen */
		elog(ERROR, "cache lookup failed for projection %u", RelationGetRelid(projection));
	prj = (Form_ygp_projection) GETSTRUCT(projectionTuple);

	/* check the number of keys, and copy attr numbers into the PrjInfo */
	numAtts = prj->prjnatts;

	pji->pji_NumPrjAttrs = numAtts;

	pji->pji_PrjAttrNumbers = (Oid *) palloc(numAtts * sizeof(Oid));

	for (i = 0; i < numAtts; i++)
		pji->pji_PrjAttrNumbers[i] = prj->prjkey.values[i];

	/* fetch any expressions needed for expressional projections */
	pji->pji_Expressions = RelationGetProjectionExpressions(projection);
	pji->pji_ExpressionsState = NIL;

	/* fetch projection predicate if any */
	pji->pji_Predicate = RelationGetProjectionPredicate(projection);
	pji->pji_PredicateState = NULL;


	pji->pji_AmCache = NULL;
	pji->pji_Context = CurrentMemoryContext;

	pji->pji_Am = projection->rd_rel->relam;

	ReleaseSysCache(projectionTuple);

	return pji;
}

