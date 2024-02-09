/*-------------------------------------------------------------------------
 *
 * ygp_projection.h
 *	  definition of the "type" system catalog (ygp_projection)
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/ygp_projection.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */

#ifndef PG_EXTPROTOCOL_H
#define PG_EXTPROTOCOL_H

#include "catalog/genbki.h"
#include "catalog/pg_extprotocol_d.h"
#include "nodes/pg_list.h"
#include "utils/acl.h"

/* ----------------
 *		ygp_projection definition.  cpp turns this into
 *		typedef struct FormData_ygp_projection
 *
 * ----------------
 */

CATALOG(ygp_projection,7175,YGPPRojectionRelationId)
{
	Oid			projectionrelid;		/* OID of the index */
	Oid			prjrelid;		/* OID of the relation it indexes */
	int16		prjnatts;		/* total number of columns in index */
	int16		indnkeyatts;	/* number of key columns in index */
	bool		indisunique;	/* is this a unique index? */
	bool		indisprimary;	/* is this index for primary key? */
	bool		indisexclusion; /* is this index for exclusion constraint? */
	bool		indimmediate;	/* is uniqueness enforced immediately? */
	bool		indisclustered; /* is this the index last clustered by? */
	bool		indisvalid;		/* is this index valid for use by queries? */
	bool		indcheckxmin;	/* must we wait for xmin to be old? */
	bool		indisready;		/* is this index ready for inserts? */
	bool		indislive;		/* is this index alive at all? */
	bool		indisreplident; /* is this index the identity for replication? */

	/* variable-length fields start here, but we allow direct access to indkey */
	int2vector	indkey;			/* column numbers of indexed cols, or 0 */

#ifdef CATALOG_VARLEN
	oidvector	indcollation;	/* collation identifiers */
	oidvector	indclass;		/* opclass identifiers */
	int2vector	indoption;		/* per-column flags (AM-specific meanings) */
	pg_node_tree indexprs;		/* expression trees for index attributes that
								 * are not simple column references; one for
								 * each zero entry in indkey[] */
	pg_node_tree indpred;		/* expression tree for predicate, if a partial
								 * index; else NULL */
#endif

} FormData_ypg_projection;


/* ----------------
 *		FormData_ypg_projection corresponds to a pointer to a tuple with
 *		the format of ygp_projection relation.
 * ----------------
 */
typedef FormData_ygp_projection *FormData_ypg_projection;

extern Oid
ProjectionCreate(const char *projectionName);

extern Oid get_prjection_oid(const char *prj_name, bool error_if_missing);

extern char *
ProjectionGetNameByOid(Oid	protOid);

#endif /* PG_EXTPROTOCOL_H */
