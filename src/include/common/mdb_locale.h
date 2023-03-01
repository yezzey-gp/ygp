/*-------------------------------------------------------------------------
 *
 * locale_mdb.h
 *	  Generic headers for custom MDB-locales patch.
 *
 * IDENTIFICATION
 *		  src/include/common/mdb_locale.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef PG_MDB_LOCALE_H
#define PG_MDB_LOCALE_H

#ifdef USE_MDBLOCALES
#include <mdblocales.h>
#define SETLOCALE(category, locale) mdb_setlocale(category, locale)
#define NEWLOCALE(category, locale, base) mdb_newlocale(category, locale, base)
#else
#define SETLOCALE(category, locale) setlocale(category, locale)
#define NEWLOCALE(category, locale, base) newlocale(category, locale, base)
#endif

#endif							/* PG_MDB_LOCALE_H */