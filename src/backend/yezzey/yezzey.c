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

#include "access/aosegfiles.h"
#include "access/tableam.h"
#include "access/heapam.h"
#include "utils/fmgroids.h"

void YezzeyPopulateMetadataRelation(EState *estate);
void YeneidPopulateMetadataRelation(EState *estate);


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


void YeneidPopulateMetadataRelation(EState *estate) {
    MemTupleBinding * mt_bind = NULL;
    Relation yandxrel = NULL;
    CdbDispatcherState *ds = estate->dispatcherState;
	TableScanDesc aoscan;
    HeapTuple yandxtuple;
    HeapTuple tuple;

    bool nulls[Natts_pg_aoseg];
    Datum values[Natts_pg_aoseg];
    bool recordRepl[Natts_pg_aoseg];
    if (ds == NULL) {
        return;
    }
    MemSet(recordRepl, false, sizeof(recordRepl));
    recordRepl[Anum_pg_aoseg_eof - 1] = true;
    recordRepl[Anum_pg_aoseg_tupcount - 1] = true;
    recordRepl[Anum_pg_aoseg_varblockcount - 1] = true;
    recordRepl[Anum_pg_aoseg_eofuncompressed - 1] = true;
    recordRepl[Anum_pg_aoseg_modcount - 1] = true;


    Oid yrelationOid = InvalidOid;

    for (int i = 0; i < ds->primaryResults->resultCount; ++i) {
        /* take current QE result tuples and populate yezzey virtual index */
        PGconn *conn;
        conn = ds->primaryResults->resultArray[i].segdbDesc->conn;
        yeneid_tup * tupbufs = PQgetYeneidTupleBufPtr(conn);
        int nTuples = PQgetYeneidTupleCount(conn);
        for (int tindx = 0; tindx < nTuples; ++ tindx) {
            yeneid_tup tupbuf = tupbufs[tindx];
            /* form tuple */
            Oid currOid = tupbuf.relOid;

            /* Yeneid metadata relation has non-fixed OID  */
            if (yrelationOid == InvalidOid) {
                yrelationOid = currOid;
                /* INSERT INTO ... VALUES(...) */
                yandxrel = try_relation_open(yrelationOid, RowExclusiveLock, false);
                if (yandxrel == NULL) {
                    /* Noop */
                    return;
                }

                mt_bind = create_memtuple_binding(
                    RelationGetDescr(yandxrel), RelationGetNumberOfAttributes(yandxrel));

            } else {
                if (yrelationOid != currOid) {
                    elog(ERROR, "mixed metadata relation oids with yeneid %d vs %d", currOid, yrelationOid);
                }
            }

            memtuple_deform((MemTuple)(tupbuf.data), mt_bind, values,
                            nulls);
                            

            if (tupbuf.optype == 1) {
                yandxtuple = heap_form_tuple(RelationGetDescr(yandxrel), values, nulls);
                CatalogTupleInsert(yandxrel, yandxtuple);
            } else /* 2 */ {
                /* update */

                ScanKeyData scankey[1];

                ScanKeyInit(&scankey[0],
                            Anum_pg_aoseg_segno,
                            BTEqualStrategyNumber, 
                            F_INT4EQ,
                            Int32GetDatum(tupbuf.segrelid));

	            aoscan = table_beginscan_catalog(yandxrel, 1, scankey);

                if ((tuple = heap_getnext(aoscan, ForwardScanDirection)) == NULL)
                {
                    elog(ERROR, "failed to update tuple");
                    // CatalogTupleInsert(yandxrel, yandxtuple);
                } else {

                    yandxtuple = heap_modify_tuple(tuple, RelationGetDescr(yandxrel), values,
                                    nulls, recordRepl);

                    CatalogTupleUpdate(yandxrel, &tuple->t_self, yandxtuple);
                }

                table_endscan(aoscan);
            }


            heap_freetuple(yandxtuple);

            CommandCounterIncrement();
        }
    }

    if (mt_bind != NULL) {
        pfree(mt_bind);
    }
    if (yandxrel != NULL) {
        heap_close(yandxrel, RowExclusiveLock);
    }

    CommandCounterIncrement();
}
