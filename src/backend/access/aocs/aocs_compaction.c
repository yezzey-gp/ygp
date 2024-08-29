/*------------------------------------------------------------------------------
 *
 * Code dealing with the compaction of append-only tables.
 *
 * Copyright (c) 2013-Present VMware, Inc. or its affiliates.
 *
 *
 * IDENTIFICATION
 *	    src/backend/access/aocs/aocs_compaction.c
 *
 *------------------------------------------------------------------------------
 */

#include "postgres.h"

#include <limits.h>

#include "access/amapi.h"
#include "access/genam.h"
#include "access/heapam.h"
#include "access/transam.h"
#include "access/aosegfiles.h"
#include "access/aomd.h"
#include "access/aocs_compaction.h"
#include "access/appendonly_compaction.h"
#include "access/appendonlywriter.h"
#include "catalog/catalog.h"
#include "catalog/indexing.h"
#include "catalog/pg_appendonly.h"
#include "cdb/cdbaocsam.h"
#include "cdb/cdbvars.h"
#include "commands/vacuum.h"
#include "executor/executor.h"
#include "nodes/execnodes.h"
#include "storage/lmgr.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/relcache.h"
#include "utils/snapmgr.h"
#include "utils/guc.h"
#include "miscadmin.h"

/* 
 * Hook for plugins to get control after move or throw away tuple in 
 * AOCSSegmentFileFullCompaction().
 * 
 * For example, zombodb will delete the corresponding docs after the tuple
 * is moved or thrown away.
 */
aocs_compaction_delete_hook_type aocs_compaction_delete_hook = NULL;

/*
 * Drops a segment file.
 *
 * Actually, we just truncate the segfile to 0 bytes, to reclaim the space.
 * Before GPDB 6, we used to remove the file, but with WAL replication, we
 * no longer have a convenient function to remove a single segment of a
 * relation. An empty file is as almost as good as a non-existent file. If
 * the relation is dropped later, the code in mdunlink() will remove all
 * segments, including any empty ones we've left behind.
 */
void
AOCSCompaction_DropSegmentFile(Relation aorel, int segno)
{
	int			col;

	Assert(RelationIsAoCols(aorel));

	for (col = 0; col < RelationGetNumberOfAttributes(aorel); col++)
	{
		char		filenamepath[MAXPGPATH];
		int			pseudoSegNo;
		File		fd;
		char * relname;
		char *nspname;

		/* Filenum for the col */
		FileNumber  filenum = GetFilenumForAttribute(RelationGetRelid(aorel), col + 1);

		/* Open and truncate the relation segfile */
		MakeAOSegmentFileName(aorel, segno, col, &pseudoSegNo, filenamepath);

		elogif(Debug_appendonly_print_compaction, LOG,
			   "Drop segment file: "
			   "segno %d",
			   pseudoSegNo);

		relname = RelationGetRelationName(aorel);
		nspname = get_namespace_name(RelationGetNamespace(aorel));

		RelationOpenSmgr(aorel);

		fd = OpenAOSegmentFile(aorel, nspname, filenamepath, 0, -1);
		pfree(nspname);
		if (fd >= 0)
		{
			TruncateAOSegmentFile(fd, aorel, pseudoSegNo, 0, vacrelstats);
			CloseAOSegmentFile(aorel, fd);
		}
		else
		{
			/*
			 * The file we were about to drop/truncate didn't exist. That's normal,
			 * for example, if a column is added with ALTER TABLE ADD COLUMN.
			 */
			elog(DEBUG1, "could not truncate segfile %s, because it does not exist", filenamepath);
		}
		RelationCloseSmgr(aorel);
	}
}

/*
 * AOCSSegmentFileTruncateToEOF()
 *
 * Truncates the files for all columns of logical segfile 'segno' to
 * the EOF values from 'vpinfo'. The caller is responsible for locking so
 * that a concurrent backend doesn't write to the segfile while we truncate
 * it.
 *
 * This is used to clean up space left behind by aborted or crashed
 * transactions.
 */
void
AOCSSegmentFileTruncateToEOF(Relation aorel, int segno, AOCSVPInfo *vpinfo)
{
	const char *relname = RelationGetRelationName(aorel);
	int			j;

	Assert(RelationIsAoCols(aorel));

	relname = RelationGetRelationName(aorel);

	for (j = 0; j < vpinfo->nEntry; ++j)
	{
		int64		segeof;
		char		filenamepath[MAXPGPATH];
		AOCSVPInfoEntry *entry;
		File		fd;
		int32		fileSegNo;
		char 		*nspname;

		/* Filenum for the column */
		FileNumber  filenum = GetFilenumForAttribute(RelationGetRelid(aorel), j + 1);
 
		entry = &vpinfo->entry[j];
		segeof = entry->eof;

		nspname = get_namespace_name(RelationGetNamespace(aorel));
		/* Open and truncate the relation segfile to its eof */
		MakeAOSegmentFileName(aorel, segno, j, &fileSegNo, filenamepath);

		elogif(Debug_appendonly_print_compaction, LOG,
			   "Opening AO COL relation \"%s.%s\", relation id %u, relfilenode %lu column #%d, logical segment #%d (physical segment file #%d, logical EOF " INT64_FORMAT ")",
			   get_namespace_name(RelationGetNamespace(aorel)),
			   relname,
			   aorel->rd_id,
			   aorel->rd_node.relNode,
			   j,
			   segno,
			   fileSegNo,
			   segeof);

		relname = RelationGetRelationName(aorel);

		RelationOpenSmgr(aorel);

		fd = OpenAOSegmentFile(aorel, nspname, filenamepath, segeof, -1);
		if (fd >= 0)
		{
			TruncateAOSegmentFile(fd, aorel, fileSegNo, segeof, vacrelstats);
			CloseAOSegmentFile(aorel, fd);

			elogif(Debug_appendonly_print_compaction, LOG,
				   "Successfully truncated AO COL relation \"%s.%s\", relation id %u, relfilenode %u column #%d, logical segment #%d (physical segment file #%d, logical EOF " INT64_FORMAT ")",
				   nspname,
				   relname,
				   aorel->rd_id,
				   aorel->rd_node.relNode,
				   j,
				   segno,
				   fileSegNo,
				   segeof);
		}
		else
		{
			elogif(Debug_appendonly_print_compaction, LOG,
				   "No gp_relation_node entry for AO COL relation \"%s.%s\", relation id %u, relfilenode %lu column #%d, logical segment #%d (physical segment file #%d, logical EOF " INT64_FORMAT ")",
				   get_namespace_name(RelationGetNamespace(aorel)),
				   relname,
				   aorel->rd_id,
				   aorel->rd_node.relNode,
				   j,
				   segno,
				   fileSegNo,
				   segeof);
		}
		RelationCloseSmgr(aorel);
		pfree(nspname);
	}
}

static void
AOCSMoveTuple(TupleTableSlot *slot,
			  AOCSInsertDesc insertDesc,
			  ResultRelInfo *resultRelInfo,
			  EState *estate)
{
	AOTupleId  *oldAoTupleId;
	AOTupleId	newAoTupleId;

	Assert(resultRelInfo);
	Assert(slot);
	Assert(estate);

	oldAoTupleId = (AOTupleId *) &slot->tts_tid;
	/* Extract all the values of the tuple */
	slot_getallattrs(slot);

	(void) aocs_insert_values(insertDesc,
							  slot->tts_values,
							  slot->tts_isnull,
							  &newAoTupleId);
	memcpy(&slot->tts_tid, &newAoTupleId, sizeof(ItemPointerData));

	/* insert index' tuples if needed */
	if (resultRelInfo->ri_NumIndices > 0)
	{
		ExecInsertIndexTuples(resultRelInfo,
							  slot, estate, false, false,
							  NULL, NIL);
		ResetPerTupleExprContext(estate);
	}

	elogif(Debug_appendonly_print_compaction, DEBUG5,
		   "Compaction: Moved tuple (%d," INT64_FORMAT ") -> (%d," INT64_FORMAT ")",
		   AOTupleIdGet_segmentFileNum(oldAoTupleId), AOTupleIdGet_rowNum(oldAoTupleId),
		   AOTupleIdGet_segmentFileNum(&newAoTupleId), AOTupleIdGet_rowNum(&newAoTupleId));
}

/*
 * Subroutine of AOCSCompact().
 */
static bool
AOCSSegmentFileFullCompaction(Relation aorel,
							  AOCSInsertDesc insertDesc,
							  AOCSFileSegInfo *fsinfo,
							  Snapshot snapshot)
{
	const char *relname;
	AppendOnlyVisimap visiMap;
	AOCSScanDesc scanDesc;
	TupleDesc	tupDesc;
	TupleTableSlot *slot;
	int			compact_segno;
	int64		movedTupleCount = 0;
	ResultRelInfo *resultRelInfo;
	MemTupleBinding *mt_bind;
	EState	   *estate;
	AOTupleId  *aoTupleId;
	ItemPointerData otid;
	int64		tupleCount = 0;
	int64		tuplePerPage = INT_MAX;

	Assert(Gp_role == GP_ROLE_EXECUTE || Gp_role == GP_ROLE_UTILITY);
	Assert(RelationIsAoCols(aorel));
	Assert(insertDesc);

	compact_segno = fsinfo->segno;
	if (fsinfo->varblockcount > 0)
	{
		tuplePerPage = fsinfo->total_tupcount / fsinfo->varblockcount;
	}
	relname = RelationGetRelationName(aorel);

	AppendOnlyVisimap_Init(&visiMap,
						   insertDesc->visimaprelid,
						   insertDesc->visimapidxid,
						   ShareLock,
						   snapshot);

	elogif(Debug_appendonly_print_compaction,
		   LOG, "Compact AO segfile %d, relation %sd",
		   compact_segno, relname);

	scanDesc = aocs_beginrangescan(aorel,
								   snapshot, snapshot,
								   &compact_segno, 1);

	tupDesc = RelationGetDescr(aorel);
	slot = MakeSingleTupleTableSlot(tupDesc, &TTSOpsVirtual);
	slot->tts_tableOid = RelationGetRelid(aorel);

	mt_bind = create_memtuple_binding(tupDesc);

	/*
	 * We need a ResultRelInfo and an EState so we can use the regular
	 * executor's index-entry-making machinery.
	 */
	estate = CreateExecutorState();
	resultRelInfo = makeNode(ResultRelInfo);
	resultRelInfo->ri_RangeTableIndex = 1;	/* dummy */
	resultRelInfo->ri_RelationDesc = aorel;
	resultRelInfo->ri_TrigDesc = NULL;	/* we don't fire triggers */
	ExecOpenIndices(resultRelInfo, false);
	estate->es_opened_result_relations =
			lappend(estate->es_opened_result_relations, resultRelInfo);

	/*
	 * We don't want uniqueness checks to be performed while "insert"ing tuples
	 * to a destination segfile during AOCSMoveTuple(). This is to ensure that
	 * we can avoid spurious conflicts between the moved tuple and the original
	 * tuple.
	 */
	estate->gp_bypass_unique_check = true;

	while (aocs_getnext(scanDesc, ForwardScanDirection, slot))
	{
		CHECK_FOR_INTERRUPTS();

		aoTupleId = (AOTupleId *) &slot->tts_tid;
		otid = slot->tts_tid;
		if (AppendOnlyVisimap_IsVisible(&scanDesc->visibilityMap, aoTupleId))
		{
			AOCSMoveTuple(slot,
						  insertDesc,
						  resultRelInfo,
						  estate);
			movedTupleCount++;
		}
		else
		{
			/* Tuple is invisible and needs to be dropped */
			AppendOnlyThrowAwayTuple(aorel, slot, mt_bind);
		}

		if (aocs_compaction_delete_hook)
			(*aocs_compaction_delete_hook) (aorel, &otid);

		/*
		 * Check for vacuum delay point after approximatly a var block
		 */
		tupleCount++;
		if (VacuumCostActive && tupleCount % tuplePerPage == 0)
		{
			vacuum_delay_point();
		}
	}

	MarkAOCSFileSegInfoAwaitingDrop(aorel, compact_segno);

	AppendOnlyVisimap_DeleteSegmentFile(&visiMap,
										compact_segno);

	/* Delete all mini pages of the segment files if block directory exists */
	if (OidIsValid(insertDesc->blkdirrelid))
	{
		AppendOnlyBlockDirectory_DeleteSegmentFile(aorel,
												   snapshot,
												   compact_segno,
												   0);
	}

	elogif(Debug_appendonly_print_compaction, LOG,
		   "Finished compaction: "
		   "AO segfile %d, relation %s, moved tuple count " INT64_FORMAT,
		   compact_segno, relname, movedTupleCount);

	AppendOnlyVisimap_Finish(&visiMap, NoLock);

	ExecCloseIndices(resultRelInfo);
	FreeExecutorState(estate);

	ExecDropSingleTupleTableSlot(slot);
	destroy_memtuple_binding(mt_bind);

	aocs_endscan(scanDesc);

	return true;
}

/*
 * Performs a compaction of an append-only relation in column-orientation.
 *
 * The compaction segment file should be locked for this transaction in
 * the appendonlywriter.c code.
 *
 * On exit, *insert_segno will be set to the the segment that was used as the
 * insertion target. The segfiles listed in 'avoid_segnos' will not be used
 * for insertion.
 *
 * The caller is required to hold either an AccessExclusiveLock (vacuum full)
 * or a ShareLock on the relation.
 */
void
AOCSCompact(Relation aorel,
			int compaction_segno,
			int *insert_segno,
			bool isFull,
			List *avoid_segnos)
{
	const char *relname;
	AOCSInsertDesc insertDesc = NULL;
	AOCSFileSegInfo *fsinfo;
	Snapshot	appendOnlyMetaDataSnapshot = RegisterSnapshot(GetCatalogSnapshot(InvalidOid));

	Assert(RelationIsAoCols(aorel));
	Assert(Gp_role == GP_ROLE_EXECUTE || Gp_role == GP_ROLE_UTILITY);

	relname = RelationGetRelationName(aorel);
	elogif(Debug_appendonly_print_compaction, LOG,
		   "Compact AO relation %s", relname);

	/* Fetch under the write lock to get latest committed eof. */
	fsinfo = GetAOCSFileSegInfo(aorel, appendOnlyMetaDataSnapshot, compaction_segno, true);

	if (AppendOnlyCompaction_ShouldCompact(aorel,
										   compaction_segno, fsinfo->total_tupcount, isFull,
										   appendOnlyMetaDataSnapshot))
	{
		if (*insert_segno == -1)
		{
			/* get the insertion segment on first call. */
			*insert_segno = ChooseSegnoForCompactionWrite(aorel, avoid_segnos);
		}

		if (*insert_segno != -1)
		{
			insertDesc = aocs_insert_init(aorel, *insert_segno);

			AOCSSegmentFileFullCompaction(aorel,
										  insertDesc,
										  fsinfo,
										  appendOnlyMetaDataSnapshot);

			insertDesc->skipModCountIncrement = true;
			aocs_insert_finish(insertDesc, NULL);
		}
		else
		{
			/* FIXME: Could not find a target segment. What now? */
		}
	}

	pfree(fsinfo);

	UnregisterSnapshot(appendOnlyMetaDataSnapshot);
}
