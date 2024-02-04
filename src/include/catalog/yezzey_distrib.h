/*-------------------------------------------------------------------------
 *
 * yezzey_distrib.h
 *	  definitions for the yezzey_distrib catalog table
 *
 * Portions Copyright (c) 2005-2011, Greenplum inc
 * Portions Copyright (c) 2012-Present VMware, Inc. or its affiliates.
 *
 *
 * IDENTIFICATION
 *	    src/include/catalog/yezzey_distrib.h
 *
 * NOTES
 *
 *-------------------------------------------------------------------------
 */

#ifndef GP_YEZZEY_DISTRIB_H
#define GP_YEZZEY_DISTRIB_H

#include "access/attnum.h"
#include "access/tupdesc.h"
#include "catalog/genbki.h"
#include "catalog/yezzey_distrib_d.h"
#include "nodes/pg_list.h"

/*
 * Defines for yezzey_distrib
 */
CATALOG(yezzey_distrib,8777,YezzeyDistribRelationId)
{
	Oid			reloid;
#ifdef CATALOG_VARLEN			/* variable-length fields start here */
	int2vector	distkey;		/* column numbers of distribution key cols */
#endif
} FormData_yezzey_distrib;

/* GPDB added foreign key definitions for gpcheckcat. */
FOREIGN_KEY(reloid REFERENCES pg_class(oid));

/* ----------------
 *		Form_gp_distribution_policy corresponds to a pointer to a tuple with
 *		the format of gp_distribution_policy relation.
 * ----------------
 */
typedef FormData_yezzey_distrib *Form_yezzey_distrib;


#endif			/* GP_YEZZEY_DISTRIB_H */
