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

	/*
	 * Copy the index key, opclass, and indoption info into arrays (should we
	 * make the caller pass them like this to start with?)
	 */
	prjkey = buildint2vector(NULL, info->pji_NumPrjAttrs);
	for (i = 0; i < info->pji_NumPrjAttrs; i++)
		prjkey->values[i] = info->pji_PrjAttrNumbers[i];
	
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


// IndexTuple
// prj_form_tuple(TupleDesc tupleDescriptor,
// 				 Datum *values,
// 				 bool *isnull)
// {
// 	char	   *tp;				/* tuple pointer */
// 	IndexTuple	tuple;			/* return tuple */
// 	Size		size,
// 				data_size,
// 				hoff;
// 	int			i;
// 	unsigned short infomask = 0;
// 	bool		hasnull = false;
// 	uint16		tupmask = 0;
// 	int			numberOfAttributes = tupleDescriptor->natts;

// 	Datum		untoasted_values[INDEX_MAX_KEYS];
// 	bool		untoasted_free[INDEX_MAX_KEYS];

// 	if (numberOfAttributes > INDEX_MAX_KEYS)
// 		ereport(ERROR,
// 				(errcode(ERRCODE_TOO_MANY_COLUMNS),
// 				 errmsg("number of index columns (%d) exceeds limit (%d)",
// 						numberOfAttributes, INDEX_MAX_KEYS)));

// #ifdef TOAST_INDEX_HACK
// 	for (i = 0; i < numberOfAttributes; i++)
// 	{
// 		Form_pg_attribute att = TupleDescAttr(tupleDescriptor, i);

// 		untoasted_values[i] = values[i];
// 		untoasted_free[i] = false;

// 		/* Do nothing if value is NULL or not of varlena type */
// 		if (isnull[i] || att->attlen != -1)
// 			continue;

// 		/*
// 		 * If value is stored EXTERNAL, must fetch it so we are not depending
// 		 * on outside storage.  This should be improved someday.
// 		 */
// 		if (VARATT_IS_EXTERNAL(DatumGetPointer(values[i])))
// 		{
// 			untoasted_values[i] =
// 				PointerGetDatum(heap_tuple_fetch_attr((struct varlena *)
// 													  DatumGetPointer(values[i])));
// 			untoasted_free[i] = true;
// 		}

// 		/*
// 		 * If value is above size target, and is of a compressible datatype,
// 		 * try to compress it in-line.
// 		 */
// 		if (!VARATT_IS_EXTENDED(DatumGetPointer(untoasted_values[i])) &&
// 			VARSIZE(DatumGetPointer(untoasted_values[i])) > TOAST_INDEX_TARGET &&
// 			(att->attstorage == 'x' || att->attstorage == 'm'))
// 		{
// 			Datum		cvalue = toast_compress_datum(untoasted_values[i]);

// 			if (DatumGetPointer(cvalue) != NULL)
// 			{
// 				/* successful compression */
// 				if (untoasted_free[i])
// 					pfree(DatumGetPointer(untoasted_values[i]));
// 				untoasted_values[i] = cvalue;
// 				untoasted_free[i] = true;
// 			}
// 		}
// 	}
// #endif

// 	for (i = 0; i < numberOfAttributes; i++)
// 	{
// 		if (isnull[i])
// 		{
// 			hasnull = true;
// 			break;
// 		}
// 	}

// 	if (hasnull)
// 		infomask |= INDEX_NULL_MASK;

// 	hoff = IndexInfoFindDataOffset(infomask);
// #ifdef TOAST_INDEX_HACK
// 	data_size = heap_compute_data_size(tupleDescriptor,
// 									   untoasted_values, isnull);
// #else
// 	data_size = heap_compute_data_size(tupleDescriptor,
// 									   values, isnull);
// #endif
// 	size = hoff + data_size;
// 	size = MAXALIGN(size);		/* be conservative */

// 	tp = (char *) palloc0(size);
// 	tuple = (IndexTuple) tp;

// 	heap_fill_tuple(tupleDescriptor,
// #ifdef TOAST_INDEX_HACK
// 					untoasted_values,
// #else
// 					values,
// #endif
// 					isnull,
// 					(char *) tp + hoff,
// 					data_size,
// 					&tupmask,
// 					(hasnull ? (bits8 *) tp + sizeof(IndexTupleData) : NULL));

// #ifdef TOAST_INDEX_HACK
// 	for (i = 0; i < numberOfAttributes; i++)
// 	{
// 		if (untoasted_free[i])
// 			pfree(DatumGetPointer(untoasted_values[i]));
// 	}
// #endif

// 	/*
// 	 * We do this because heap_fill_tuple wants to initialize a "tupmask"
// 	 * which is used for HeapTuples, but we want an indextuple infomask. The
// 	 * only relevant info is the "has variable attributes" field. We have
// 	 * already set the hasnull bit above.
// 	 */
// 	if (tupmask & HEAP_HASVARWIDTH)
// 		infomask |= INDEX_VAR_MASK;

// 	/* Also assert we got rid of external attributes */
// #ifdef TOAST_INDEX_HACK
// 	Assert((tupmask & HEAP_HASEXTERNAL) == 0);
// #endif

// 	/*
// 	 * Here we make sure that the size will fit in the field reserved for it
// 	 * in t_info.
// 	 */
// 	if ((size & INDEX_SIZE_MASK) != size)
// 		ereport(ERROR,
// 				(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
// 				 errmsg("index row requires %zu bytes, maximum size is %zu",
// 						size, (Size) INDEX_SIZE_MASK)));

// 	infomask |= size;

// 	/*
// 	 * initialize metadata
// 	 */
// 	tuple->t_info = infomask;
// 	return tuple;
// }


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

	newInfo = makePrjInfo(numberOfAttributes, accessMethodId, stmt->prjParams);

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
