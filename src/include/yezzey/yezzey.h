
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


#ifdef __cplusplus
#define EXTERNC extern "C"
#else
#define EXTERNC
#endif

EXTERNC void YezzeyPopulateMetadataRelation(EState *estate);


#endif /* GP_YEZZEY_H */