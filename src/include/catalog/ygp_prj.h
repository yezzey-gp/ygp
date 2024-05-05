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

	int2vector	prjkey;		/* column numbers of projectioned cols, or 0 */

#ifdef CATALOG_VARLEN
	/* variable-length fields start here, but we allow direct access to prjkey */
	/* currently we store projection key in gp_distr_policy */
#endif
} FormData_ypg_projection;


/* */
FOREIGN_KEY(projectionrelid REFERENCES pg_class(oid));
FOREIGN_KEY(prjrelid REFERENCES pg_class(oid));
/*   alter table ygp_prj add vector_fk indclass on pg_opclass(oid); */


extern PrjInfo *BuildPrjInfo(Relation index);

/* ----------------
 *		Form_pg_projection corresponds to a pointer to a tuple with
 *		the format of ygp_prj relation.
 * ----------------
 */
typedef FormData_ypg_projection *Form_ypg_projection;

#endif							/* PG_PROJECTION_H */
