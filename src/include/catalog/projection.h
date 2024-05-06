/*-------------------------------------------------------------------------
 *
 * projection.h
 *	  prototypes for catalog/ygp_prj.c.
 *
 *
 * Portions Copyright (c) 1996-2019, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/projection.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PROJECTION_H
#define PROJECTION_H

#include "access/relscan.h"     /* Relation, Snapshot */
#include "catalog/objectaddress.h"
#include "executor/tuptable.h"  /* TupTableSlot */
#include "nodes/execnodes.h"

struct EState;                  /* #include "nodes/execnodes.h" */


extern PrjInfo *BuildPrjInfo(Relation index);


#endif							/* PROJECTION_H */


