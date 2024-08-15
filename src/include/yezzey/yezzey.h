
/*-------------------------------------------------------------------------
 *
 * yezzey.h
 *
 * IDENTIFICATION
 *	    src/include/yezzey/yezzey.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef GP_YEZZEY_H
#define GP_YEZZEY_H


#include "nodes/execnodes.h"

void YezzeyPopulateMetadataRelation(EState *estate);
void YeneidPopulateMetadataRelation(EState *estate);

void YezzeyPopulateScanMetadata(Relation relation, Scan *scan);

#endif /* GP_YEZZEY_H */