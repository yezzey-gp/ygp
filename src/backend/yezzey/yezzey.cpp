extern "C" {
    #include "postgres.h"
    #include "yezzey/yezzey.h"
    #include "nodes/execnodes.h"
    #include "executor/execUtils.h"
    #include "cdb/cdbdisp.h"
}

EXTERNC void YezzeyPopulateMetadataRelation(EState *estate);


EXTERNC void YezzeyPopulateMetadataRelation(EState *estate) {
    CdbDispatchResults *pr = NULL;
    CdbDispatcherState *ds = estate->dispatcherState;
    DispatchWaitMode waitMode = DISPATCH_WAIT_NONE;


	int			primaryWriterSliceIndex;

	/* caller must have switched into per-query memory context already */

	auto currentSlice = getCurrentSlice(estate, LocallyExecutingSliceIndex(estate));
	primaryWriterSliceIndex = PrimaryWriterSliceIndex(estate);

    /* Yezzey metadata relation has fixed OID  */

}
