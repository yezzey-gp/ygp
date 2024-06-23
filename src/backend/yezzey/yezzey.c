#include "postgres.h"
#include "libpq-fe.h"
#include "libpq-int.h"
#include "cdb/cdbconn.h"
#include "cdb/cdbdispatchresult.h"
#include "yezzey/yezzey.h"
#include "nodes/execnodes.h"
#include "executor/execUtils.h"
#include "cdb/cdbdisp.h"
#include "access/xact.h"
#include "catalog/indexing.h"
#include "utils/rel.h"
#include "access/table.h"
#include "access/relation.h"

void YezzeyPopulateMetadataRelation(EState *estate);


/* TODO: move somewhere */
#define Natts_yezzey_virtual_index 10
#define YEZZEY_TEMP_INDEX_RELATION 8500

void YezzeyPopulateMetadataRelation(EState *estate) {
    MemTupleBinding * mt_bind;
    Relation yandxrel;
    CdbDispatcherState *ds = estate->dispatcherState;

    bool nulls[Natts_yezzey_virtual_index];
    Datum values[Natts_yezzey_virtual_index];
    if (ds == NULL) {
        return;
    }
    
    /* Yezzey metadata relation has fixed OID  */
    /* INSERT INTO  yezzey.yezzey_virtual_index_<oid> VALUES(...) */
    yandxrel = try_relation_open(YEZZEY_TEMP_INDEX_RELATION, RowExclusiveLock, false);
    if (yandxrel == NULL) {
        /* Noop */
        return;
    }

    mt_bind = create_memtuple_binding(
        RelationGetDescr(yandxrel), RelationGetNumberOfAttributes(yandxrel));


    for (int i = 0; i < ds->primaryResults->resultCount; ++i) {
        /* take current QE result tuples and populate yezzey virtual index */
        pg_res_tuple * tupbufs = PQgetYezzeyTupleBufPtr(ds->primaryResults->resultArray[i].segdbDesc->conn);
        int nTuples = PQgetYezzeyTupleCount(ds->primaryResults->resultArray[i].segdbDesc->conn);
        for (int tindx = 0; tindx < nTuples; ++ tindx) {
            pg_res_tuple tupbuf = tupbufs[tindx];
            /* form tuple */

            memtuple_deform((MemTuple)(tupbuf.data), mt_bind, values,
                            nulls);
                            
            HeapTuple yandxtuple = heap_form_tuple(RelationGetDescr(yandxrel), values, nulls);

            CatalogTupleInsert(yandxrel, yandxtuple);

            heap_freetuple(yandxtuple);
        }
    }

    pfree(mt_bind);
    heap_close(yandxrel, RowExclusiveLock);

    CommandCounterIncrement();
}
