

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

/* ----------------------------------------------------------------
 *						index_build support
 * ----------------------------------------------------------------
 */

/* ----------------
 *		BuildIndexInfo
 *			Construct an IndexInfo record for an open index
 *
 * IndexInfo stores the information about the index that's needed by
 * FormIndexDatum, which is used for both index_build() and later insertion
 * of individual index tuples.  Normally we build an IndexInfo for an index
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
		elog(ERROR, "cache lookup failed for index %u", RelationGetRelid(projection));
	prj = (Form_ygp_projection) GETSTRUCT(projectionTuple);

	/* check the number of keys, and copy attr numbers into the IndexInfo */
	numAtts = prj->prjnatts;

	pji->pji_NumPrjAttrs = numAtts;

	for (i = 0; i < numAtts; i++)
		pji->pji_PrjAttrNumbers[i] = prj->prjkey.values[i];

	/* fetch projection predicate if any */
	pji->pji_Predicate = NULL;
	pji->pji_PredicateState = NULL;

	pji->pji_AmCache = NULL;
	pji->pji_Context = CurrentMemoryContext;

	pji->pji_Am = projection->rd_rel->relam;

	return pji;
}

