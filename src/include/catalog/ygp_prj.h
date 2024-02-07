/*-------------------------------------------------------------------------
 *
 * ygp_prj.h
 *	  definition of the "projection" system catalog (ygp_prj)
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/ygp_prj.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_PROJECTION_H
#define PG_PROJECTION_H

#include "catalog/genbki.h"
#include "catalog/ygp_prj_d.h"

/* ----------------
 *		ygp_prj definition.  cpp turns this into
 *		typedef struct FormData_pg_projection.
 * ----------------
 */
CATALOG(ygp_prj,4189,ProjectionRelationId) BKI_SCHEMA_MACRO
{
	Oid			prjrelid;		/* OID of the projection */
	Oid			projectionrelid;		/* OID of the relation it projectiones */
	int16		prjkeyatts;	   /* number of key columns in projection */
	bool		prjisvalid;		/* is this projection valid for use by queries? */
	bool		prjcheckxmin;	/* must we wait for xmin to be old? */
	bool		prjisready;		/* is this projection ready for inserts? */
	bool		prjislive;		/* is this projection alive at all? */

	/* variable-length fields start here, but we allow direct access to indkey */
	int2vector	prjkey;			/* column numbers of projectioned cols, or 0 */

#ifdef CATALOG_VARLEN
	pg_node_tree projectionprs;		/* expression trees for projection attributes that
								 * are not simple column references; one for
								 * each zero entry in indkey[] */
	pg_node_tree prjpred;		/* expression tree for predicate, if a partial
								 * projection; else NULL */
#endif
} FormData_pg_projection;


/* GPDB added foreign key definitions for gpcheckcat. */
FOREIGN_KEY(projectionrelid REFERENCES pg_class(oid));
FOREIGN_KEY(prjrelid REFERENCES pg_class(oid));
/*   alter table ygp_prj add vector_fk indclass on pg_opclass(oid); */

/* ----------------
 *		Form_pg_projection corresponds to a pointer to a tuple with
 *		the format of ygp_prj relation.
 * ----------------
 */
typedef FormData_pg_projection *Form_pg_projection;

#ifdef EXPOSE_TO_CLIENT_CODE

/*
 * projection AMs that support ordered scans must support these two indoption
 * bits.  Otherwise, the content of the per-column indoption fields is
 * open for future definition.
 */
#define INDOPTION_DESC			0x0001	/* values are in reverse order */
#define INDOPTION_NULLS_FIRST	0x0002	/* NULLs are first instead of last */

#endif							/* EXPOSE_TO_CLIENT_CODE */

#endif							/* PG_PROJECTION_H */
