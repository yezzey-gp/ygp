/*-------------------------------------------------------------------------
 *
 * yezzey_base.h
 *
 * IDENTIFICATION
 *	    src/include/yezzey/yezzey_base.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef GP_YEZZEY_BASE_H
#define GP_YEZZEY_BASE_H


typedef struct yezzeyScanTupleDB {
  Oid reloid;            /* relation oid */
  Oid relfileoid;        /* relation filenode oid */
  int blkno;             /* AO relation block file no */
  uint64_t start_offset;  /* start_offset of block file chunk */
  uint64_t finish_offset; /* finish_offset of block file chunk */
  int32_t encrypted;     /* Is chunk in external storage encrypted */
  int32_t reused;        /* Is chunk writted by yezzey or reused from backup */
  uint64_t modcount;      /* modcount of block file chunk */
  uint64 lsn;        /* Chunk lsn */
  text x_path;           /* external path */
} yezzeyScanTupleDB;

typedef struct yezzeyScanTuple {
  Oid reloid;            /* relation oid */
  Oid relfileoid;        /* relation filenode oid */
  int blkno;             /* AO relation block file no */
  uint64_t start_offset;  /* start_offset of block file chunk */
  uint64_t finish_offset; /* finish_offset of block file chunk */
  int32_t encrypted;     /* Is chunk in external storage encrypted */
  int32_t reused;        /* Is chunk writted by yezzey or reused from backup */
  uint64_t modcount;      /* modcount of block file chunk */
  uint64 lsn;        /* Chunk lsn */
  char* x_path;           /* external path */
} yezzeyScanTuple;


#endif /* GP_YEZZEY_BASE_H */