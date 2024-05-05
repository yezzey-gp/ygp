

/*
* ygp_prj.c
*/

#include "postgres.h"

#include "nodes/execnodes.h"
#include "access/tableam.h"
#include "executor/executor.h"

#include "access/table.h"
#include "utils/relcache.h"

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


/*

	NodeTag		type;
	int			pji_NumPrjAttrs;	/* total number of columns in projection */;

	AttrNumber	*pji_PrjAttrNumbers /* List of column attrib number */;

	List	   *pji_Predicate; /* list of Expr */

	ExprState  *pji_PredicateState;

	Oid			pji_Am;
	void	   *pji_AmCache;
	MemoryContext pji_Context;
*/

PrjInfo *
BuildPrjInfo(Relation projection)
{
	PrjInfo  *pji = makeNode(PrjInfo);
	Form_pg_class rd_rel = projection->rd_rel;
	int			i;
	int			numAtts;

	/* check the number of keys, and copy attr numbers into the IndexInfo */
	numAtts = indexStruct->indnatts;

	pji->ii_NumIndexAttrs = numAtts;

	for (i = 0; i < numAtts; i++)
		pji->pji_IndexAttrNumbers[i] = indexStruct->indkey.values[i];

	/* fetch projection predicate if any */
	pji->pji_Predicate = NULL;
	pji->pji_PredicateState = NULL;

	pji->pji_AmCache = NULL;
	pji->pji_Context = CurrentMemoryContext;

	pji->pji_Am = projection->rd_rel->relam;

	return pji;
}

