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

#include "nodes/makefuncs.h"

#include "utils/builtins.h"
#include "utils/syscache.h"

#include "catalog/oid_dispatch.h"
#include "access/xact.h"
#include "access/tableam.h"

#include "cdb/cdbvars.h"
#include "cdb/cdbdisp_query.h"

static void UpdateProjectionRelation(Oid prjoid,
					Oid heapoid, PrjInfo* info);


static void
UpdateProjectionRelation(Oid prjoid,
					Oid heapoid, PrjInfo* info)
{
	Datum		values[Natts_ygp_prj];
	bool		nulls[Natts_ygp_prj];
	Relation	ygp_prj;
	HeapTuple	tuple;
	int			i;
	int2vector *prjkey;

	char *exprsString;
	Datum exprsDatum;
	char *predString;
	Datum predDatum;

	/*
	 * Copy the index key, opclass, and indoption info into arrays (should we
	 * make the caller pass them like this to start with?)
	 */
	prjkey = buildint2vector(NULL, info->pji_NumPrjAttrs);
	for (i = 0; i < info->pji_NumPrjAttrs; i++)
		prjkey->values[i] = info->pji_PrjAttrNumbers[i];
	

	/*
	 * Convert the projection expressions (if any) to a text datum
	 */
	if (info->pji_Expressions != NIL)
	{
		char	   *exprsString;

		exprsString = nodeToString(info->pji_Expressions);
		exprsDatum = CStringGetTextDatum(exprsString);
		pfree(exprsString);
	}
	else
		exprsDatum = (Datum) 0;

	/*
	 * Convert the projection predicate (if any) to a text datum.  Note we convert
	 * implicit-AND format to normal explicit-AND for storage.
	 */
	if (info->pji_Predicate != NIL)
	{
		char	   *predString;

		predString = nodeToString(make_ands_explicit(info->pji_Predicate));
		predDatum = CStringGetTextDatum(predString);
		pfree(predString);
	}
	else
		predDatum = (Datum) 0;

	/*
	 * open the system catalog index relation
	 */
	ygp_prj = table_open(ProjectionRelationId, RowExclusiveLock);

	/*
	 * Build a ygp_prj tuple
	 */
	MemSet(nulls, false, sizeof(nulls));

	values[Anum_ygp_prj_projectionrelid - 1] = ObjectIdGetDatum(prjoid);
	values[Anum_ygp_prj_prjrelid - 1] = ObjectIdGetDatum(heapoid);
	values[Anum_ygp_prj_prjnatts - 1] = Int16GetDatum(info->pji_NumPrjAttrs);
	values[Anum_ygp_prj_prjkey - 1] = PointerGetDatum(prjkey);

	values[Anum_ygp_prj_projectionxprs - 1] = exprsDatum;
	if (exprsDatum == (Datum) 0)
		nulls[Anum_ygp_prj_projectionxprs - 1] = true;
	values[Anum_ygp_prj_prjpred - 1] = predDatum;
	if (predDatum == (Datum) 0)
		nulls[Anum_ygp_prj_prjpred - 1] = true;

	tuple = heap_form_tuple(RelationGetDescr(ygp_prj), values, nulls);

	/*
	 * insert the tuple into the ygp_prj catalog
	 */
	CatalogTupleInsert(ygp_prj, tuple);

	/*
	 * close the relation and free the tuple
	 */
	table_close(ygp_prj, RowExclusiveLock);
	heap_freetuple(tuple);
}


/*
 *		ConstructPrjTupleDescriptor
 *
 * Build an projection tuple descriptor for a new projection
 */
static TupleDesc
ConstructPrjTupleDescriptor(Relation heapRelation,
						 PrjInfo *prjInfo,
						 List *projectionColNames,
						 Oid *collationObjectId)
{
	int			numatts = prjInfo->pji_NumPrjAttrs;
	ListCell   *colnames_item = list_head(projectionColNames);
	TupleDesc	heapTupDesc;
	TupleDesc	prjTupDesc;
	int			natts;			/* #atts in heap rel --- for error checks */
	int			i;

	/* ... and to the table's tuple descriptor */
	heapTupDesc = RelationGetDescr(heapRelation);
	natts = RelationGetForm(heapRelation)->relnatts;

	/*
	 * allocate the new tuple descriptor
	 */
	prjTupDesc = CreateTemplateTupleDesc(numatts);

	/*
	 * Fill in the pg_attribute row.
	 */
	for (i = 0; i < numatts; i++)
	{
		AttrNumber	atnum = prjInfo->pji_PrjAttrNumbers[i];
		Form_pg_attribute to = TupleDescAttr(prjTupDesc, i);
		HeapTuple	tuple;
		Form_pg_type typeTup;
		Oid			keyType;

		/* Fill fixed part with errors */
		MemSet(to, 0, ATTRIBUTE_FIXED_PART_SIZE);
		to->attnum = i + 1;
		to->attstattarget = -1;
		to->attcacheoff = -1;
		to->attislocal = true;
		to->attcollation = collationObjectId[i];

		/*
		 * Set the attribute name as specified by caller.
		 */
		if (colnames_item == NULL)	/* shouldn't happen */
			elog(ERROR, "too few entries in colnames list");
		namestrcpy(&to->attname, (const char *) lfirst(colnames_item));
		colnames_item = lnext(colnames_item);

		/*
		 * For simple index columns, we copy some pg_attribute fields from the
		 * parent relation.  For expressions we have to look at the expression
		 * result.
		 */
		if (atnum != 0)
		{
			/* Simple index column */
			const FormData_pg_attribute *from;

			Assert(atnum > 0);	/* should've been caught above */

			if (atnum > natts)	/* safety check */
				elog(ERROR, "invalid column number %d", atnum);
			from = TupleDescAttr(heapTupDesc,
								 AttrNumberGetAttrOffset(atnum));

			to->atttypid = from->atttypid;
			to->attlen = from->attlen;
			to->attndims = from->attndims;
			to->atttypmod = from->atttypmod;
			to->attbyval = from->attbyval;
			to->attstorage = from->attstorage;
			to->attalign = from->attalign;
		}
		else
		{
			/* Expressional index */
			/* TODO: support */
		}

		/*
		 * We do not yet have the correct relation OID for the projection, so just
		 * set it invalid for now.  InitializeAttributeOids() will fix it
		 * later.
		 */
		to->attrelid = InvalidOid;
	}

	return prjTupDesc;
}


ObjectAddress
DefineProjection(Oid relationId,
			CreateProjectionStmt *stmt,
			Oid prjRelationId,
			bool check_rights,
			bool dispatch)
{
	GpPolicy   *policy;
	TupleDesc	descriptor;
	Oid			namespaceId;
	Relation rel;
	Oid prjOid;
	ObjectAddress address;
	HeapTuple	amtuple;
	Form_pg_am	accessMethodForm;
	PrjInfo *newInfo;
	Oid accessMethodId;

	char * accessMethodName;
	bool		shouldDispatch = dispatch &&
								 Gp_role == GP_ROLE_DISPATCH &&
                                 IsNormalProcessingMode();

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

	int numberOfAttributes;

	numberOfAttributes = list_length(stmt->prjParams);

	/*
	 * look up the access method, verify it can handle the requested features
	 */
	if (stmt->accessMethod != NULL) {
		accessMethodName = stmt->accessMethod;
	} else {
		accessMethodName = default_table_access_method;
	}


	amtuple = SearchSysCache1(AMNAME, PointerGetDatum(accessMethodName));
	if (!HeapTupleIsValid(amtuple))
	{
		/* invalid access method */ 
		elog(ERROR, "invalid access method %s", accessMethodName);
	}
	accessMethodForm = (Form_pg_am) GETSTRUCT(amtuple);
	accessMethodId = accessMethodForm->oid;

	newInfo = makePrjInfo(numberOfAttributes, accessMethodId, 
							  NIL,	/* expressions, NIL for now */
							  make_ands_implicit((Expr *) stmt->whereClause));

		/*
	 * Extract the list of column names and the column numbers for the new
	 * index information.  All this information will be used for the index
	 * creation.
	 */
    ListCell *cell;
	List *prjColNames;
	Oid *collationObjectId;
	
	prjColNames = NULL;

	collationObjectId = (Oid *) palloc(numberOfAttributes * sizeof(Oid));
	int ind;

	ind = 0;

	foreach(cell, stmt->prjParams) {		
		/* Simple index attribute */
		HeapTuple	atttuple;
		Form_pg_attribute attform;

		ProjectionElem    *pelem = (ProjectionElem *) lfirst(cell);
		prjColNames = lappend(prjColNames, pelem->name);

		if (pelem->collation != NULL) {
			collationObjectId[ind] = get_collation_oid(pelem->collation, false /*missing not ok*/);
		} else {
			collationObjectId[ind] = InvalidOid;
		}

		atttuple = SearchSysCacheAttName(relationId, pelem->name);
		if (!HeapTupleIsValid(atttuple))
		{
			ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_COLUMN),
							errmsg("column \"%s\" does not exist",
								pelem->name)));
		}
		attform = (Form_pg_attribute) GETSTRUCT(atttuple);
		newInfo->pji_PrjAttrNumbers[ind] = attform->attnum;


		ReleaseSysCache(atttuple);

		++ind;
	}


	descriptor = ConstructPrjTupleDescriptor(rel, newInfo, prjColNames, collationObjectId);

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
		policy /* GP Policy */, 
		(Datum)0,
		false /* use_user_acl */,
    	true,
		true,
		InvalidOid /*relrewrite*/,
		NULL, 
		false /* valid_opts */
	);


  	// /* Make this prj visible */
	// CommandCounterIncrement();

	ReleaseSysCache(amtuple);


	UpdateProjectionRelation(
		prjOid,
		relationId,
		newInfo
	);

  	/* Make this changes visible */
	CommandCounterIncrement();

	
	/* It is now safe to dispatch */
	if (shouldDispatch)
	{
		/*
		 * Dispatch the statement tree to all primary and mirror segdbs.
		 * Doesn't wait for the QEs to finish execution.
		 *
		 * The OIDs are carried out-of-band.
		 */

		CdbDispatchUtilityStatement((Node *) stmt,
									DF_CANCEL_ON_ERROR |
									DF_NEED_TWO_PHASE |
									DF_WITH_SNAPSHOT,
									GetAssignedOidsForDispatch(),
									NULL);
	}

	ObjectAddressSet(address, ProjectionRelationId, prjOid);

	elog(LOG, "created projection %s", stmt->prjname);

	table_close(rel, ShareLock);


	return address;
}
