/*-------------------------------------------------------------------------
 *
 * pg_namespace.h
 *	  definition of the "namespace" system catalog (pg_namespace)
 *
 *
 * Portions Copyright (c) 1996-2021, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/pg_namespace.h
 *
 * NOTES
 *	  The Catalog.pm module reads this file and derives schema
 *	  information.
 *
 *-------------------------------------------------------------------------
 */
#ifndef PG_NAMESPACE_H
#define PG_NAMESPACE_H

#include "catalog/genbki.h"
#include "catalog/pg_namespace_d.h"
#include "utils/acl.h"

/* ----------------------------------------------------------------
 *		pg_namespace definition.
 *
 *		cpp turns this into typedef struct FormData_pg_namespace
 *
 *	nspname				name of the namespace
 *	nspowner			owner (creator) of the namespace
 *	nspacl				access privilege list
 * ----------------------------------------------------------------
 */
CATALOG(pg_namespace,2615,NamespaceRelationId)
{
	Oid			oid;			/* oid */

	NameData	nspname;
	Oid			nspowner BKI_DEFAULT(POSTGRES) BKI_LOOKUP(pg_authid);

#ifdef CATALOG_VARLEN			/* variable-length fields start here */
	aclitem		nspacl[1];
#endif
} FormData_pg_namespace;

/* GPDB added foreign key definitions for gpcheckcat. */
FOREIGN_KEY(nspowner REFERENCES pg_authid(oid));

/* ----------------
 *		Form_pg_namespace corresponds to a pointer to a tuple with
 *		the format of pg_namespace relation.
 * ----------------
 */
typedef FormData_pg_namespace *Form_pg_namespace;

#define YEZZEY_AUX_NAMESPACE 8001

#define IsBuiltInNameSpace(namespaceId) \
	(namespaceId == PG_CATALOG_NAMESPACE || \
	 namespaceId == PG_TOAST_NAMESPACE || \
	 namespaceId == PG_PUBLIC_NAMESPACE || \
	 namespaceId == PG_EXTAUX_NAMESPACE || \
	 namespaceId == PG_BITMAPINDEX_NAMESPACE || \
	 namespaceId == PG_AOSEGMENT_NAMESPACE || \
	 namespaceId == YEZZEY_AUX_NAMESPACE)

DECLARE_TOAST(pg_namespace, 4163, 4164);

DECLARE_UNIQUE_INDEX(pg_namespace_nspname_index, 2684, on pg_namespace using btree(nspname name_ops));
#define NamespaceNameIndexId  2684
DECLARE_UNIQUE_INDEX_PKEY(pg_namespace_oid_index, 2685, on pg_namespace using btree(oid oid_ops));
#define NamespaceOidIndexId  2685

/*
 * prototypes for functions in pg_namespace.c
 */
extern Oid	NamespaceCreate(const char *nspName, Oid ownerId, bool isTemp);

#endif							/* PG_NAMESPACE_H */
