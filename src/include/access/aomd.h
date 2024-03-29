/*-------------------------------------------------------------------------
 *
 * aomd.h
 *	  Declarations and functions for supporting aomd.c
 *
 * Portions Copyright (c) 2008, Greenplum Inc.
 * Portions Copyright (c) 2012-Present Pivotal Software, Inc.
 *
 *
 * IDENTIFICATION
 *	    src/include/access/aomd.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef AOMD_H
#define AOMD_H

#include "htup_details.h"
#include "storage/fd.h"
#include "storage/smgr.h"
#include "utils/rel.h"

extern int AOSegmentFilePathNameLen(Relation rel);

extern void FormatAOSegmentFileName(
						char *basepath,
						int segno,
						int col,
						int32 *fileSegNo,
						char *filepathname);

extern void MakeAOSegmentFileName(
					  Relation rel,
					  int segno,
					  int col,
					  int32 *fileSegNo,
					  char *filepathname);

extern File OpenAOSegmentFile(Relation aorel, const char *nspname, char *filepathname, int64 logicalEof, int64 modcount);

extern void CloseAOSegmentFile(Relation aorel, File fd);

extern void
TruncateAOSegmentFile(File fd,
					  Relation rel,
					  int32 segmentFileNum,
					  int64 offset);

extern void ao_truncate_one_rel(Relation rel);

extern void
mdunlink_ao(const char *path, ForkNumber forkNumber);

extern void
copy_append_only_data(RelFileNode src, RelFileNode dst, BackendId backendid, char relpersistence);

/*
 * return value should be true if the callback was able to find the given
 * segment number on disk and false otherwise. Failures during operation should
 * be handled out of band, either with a PG_THROW/elog/etc., or through the
 * passed user context.
 */
typedef bool (*ao_extent_callback)(int segno, void *ctx);

extern void ao_foreach_extent_file(ao_extent_callback callback, void *ctx);

extern void register_dirty_segment_ao(RelFileNode rnode, int segno, File vfd, struct f_smgr_ao *smgrao);

#endif							/* AOMD_H */
