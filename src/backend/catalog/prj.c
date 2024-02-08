
#include "postgres.h"


#include "catalog/heap.h"


/* ----------------------------------------------------------------
 *		heap_create		- Create an uncataloged heap relation
 *
 *		Note API change: the caller must now always provide the OID
 *		to use for the relation.  The relfilenode may (and, normally,
 *		should) be left unspecified.
 *
 *		rel->rd_rel is initialized by RelationBuildLocalRelation,
 *		and is mostly zeroes at return.
 * ----------------------------------------------------------------
 */
Relation
prj_create(const char *relname,
			Oid relnamespace,
			Oid reltablespace,
			Oid relid,
			Oid relfilenode,
			Oid accessmtd,
			TupleDesc tupDesc,
			char relkind,
			char relpersistence,
			bool shared_relation,
			bool mapped_relation,
			bool allow_system_table_mods,
			TransactionId *relfrozenxid,
			MultiXactId *relminmxid)
{
	bool		create_storage;
	Relation	rel;

	/* The caller must have provided an OID for the relation. */
	Assert(OidIsValid(relid));

	/*
	 * Don't allow creating relations in pg_catalog directly, even though it
	 * is allowed to move user defined relations there. Semantics with search
	 * paths including pg_catalog are too confusing for now.
	 *
	 * But allow creating indexes on relations in pg_catalog even if
	 * allow_system_table_mods = off, upper layers already guarantee it's on a
	 * user defined relation, not a system one.
	 */
	if (!allow_system_table_mods &&
		((IsCatalogNamespace(relnamespace) && relkind != RELKIND_INDEX) ||
		 IsToastNamespace(relnamespace) ||
		 IsAoSegmentNamespace(relnamespace)) &&
		IsNormalProcessingMode())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("permission denied to create \"%s.%s\"",
						get_namespace_name(relnamespace), relname),
				 errdetail("System catalog modifications are currently disallowed.")));

	*relfrozenxid = InvalidTransactionId;
	*relminmxid = InvalidMultiXactId;

	/* Handle reltablespace for specific relkinds. */
	switch (relkind)
	{
		case RELKIND_VIEW:
		case RELKIND_COMPOSITE_TYPE:
		case RELKIND_FOREIGN_TABLE:

			/*
			 * Force reltablespace to zero if the relation has no physical
			 * storage.  This is mainly just for cleanliness' sake.
			 *
			 * Partitioned tables and indexes don't have physical storage
			 * either, but we want to keep their tablespace settings so that
			 * their children can inherit it.
			 */
			reltablespace = InvalidOid;
			break;

		case RELKIND_SEQUENCE:

			/*
			 * Force reltablespace to zero for sequences, since we don't
			 * support moving them around into different tablespaces.
			 */
			reltablespace = InvalidOid;
			break;
		default:
			break;
	}

	/*
	 * Decide whether to create storage. If caller passed a valid relfilenode,
	 * storage is already created, so don't do it here.  Also don't create it
	 * for relkinds without physical storage.
	 */
	if (!RELKIND_HAS_STORAGE(relkind) || OidIsValid(relfilenode))
		create_storage = false;
	else
	{
		create_storage = true;
		/*
		 * In PostgreSQL, the relation OID is used as the relfilenode initially.
		 * In GPDB, relfilenode is assigned using a separate counter. Pass '1'
		 * to RelationBuildLocalRelation() to signal that it should assign a
		 * a new value.
		 */
		relfilenode = 1;
	}

	/*
	 * Never allow a pg_class entry to explicitly specify the database's
	 * default tablespace in reltablespace; force it to zero instead. This
	 * ensures that if the database is cloned with a different default
	 * tablespace, the pg_class entry will still match where CREATE DATABASE
	 * will put the physically copied relation.
	 *
	 * Yes, this is a bit of a hack.
	 */
	if (reltablespace == MyDatabaseTableSpace)
		reltablespace = InvalidOid;

	/*
	 * build the relcache entry.
	 */
	rel = RelationBuildLocalRelation(relname,
									 relnamespace,
									 tupDesc,
									 relid,
									 accessmtd,
									 relfilenode,
									 reltablespace,
									 shared_relation,
									 mapped_relation,
									 relpersistence,
									 relkind);

	/*
	 * Have the storage manager create the relation's disk file, if needed.
	 *
	 * For relations the callback creates both the main and the init fork, for
	 * indexes only the main fork is created. The other forks will be created
	 * on demand.
	 */
	if (create_storage)
	{
		RelationOpenSmgr(rel);

		switch (rel->rd_rel->relkind)
		{
			case RELKIND_VIEW:
			case RELKIND_COMPOSITE_TYPE:
			case RELKIND_FOREIGN_TABLE:
			case RELKIND_PARTITIONED_TABLE:
			case RELKIND_PARTITIONED_INDEX:
				Assert(false);
				break;

			case RELKIND_INDEX:
			case RELKIND_SEQUENCE:
				RelationCreateStorage(rel->rd_node, relpersistence, SMGR_MD);
				break;

			case RELKIND_RELATION:
			case RELKIND_TOASTVALUE:
			case RELKIND_MATVIEW:
				table_relation_set_new_filenode(rel, &rel->rd_node,
												relpersistence,
												relfrozenxid, relminmxid);
				break;

			case RELKIND_AOSEGMENTS:
			case RELKIND_AOVISIMAP:
			case RELKIND_AOBLOCKDIR:
				Assert(rel->rd_tableam);
				table_relation_set_new_filenode(rel, &rel->rd_node,
												relpersistence,
												relfrozenxid, relminmxid);
				break;
		}

		/*
		 * AO tables don't use the buffer manager, better to not keep the
		 * smgr open for it.
		 */
		if (RelationStorageIsAO(rel))
			RelationCloseSmgr(rel);
	}

	/*
	 * If a tablespace is specified, removal of that tablespace is normally
	 * protected by the existence of a physical file; but for relations with
	 * no files, add a pg_shdepend entry to account for that.
	 */
	if (!create_storage && reltablespace != InvalidOid)
		recordDependencyOnTablespace(RelationRelationId, relid,
									 reltablespace);

	return rel;
}




Oid
prj_create_with_catalog(const char *relname,
						 Oid relnamespace,
						 Oid reltablespace,
						 Oid relid,
						 Oid reltypeid,
						 Oid reloftypeid,
						 Oid ownerid,
						 Oid accessmtd,
						 TupleDesc tupdesc,
						 List *cooked_constraints,
						 char relkind,
						 char relpersistence,
						 bool shared_relation,
						 bool mapped_relation,
						 OnCommitAction oncommit,
                         const struct GpPolicy *policy,
						 Datum reloptions,
						 bool use_user_acl,
						 bool allow_system_table_mods,
						 bool is_internal,
						 Oid relrewrite,
						 ObjectAddress *typaddress,
						 bool valid_opts)
{
	Relation	pg_class_desc;
	Relation	ypg_prj_class_desc;
	Relation	new_rel_desc;
	Acl		   *relacl;
	Oid			existing_relid;
	Oid			old_type_oid;
	Oid			new_type_oid;
	TransactionId relfrozenxid;
	MultiXactId relminmxid;
	char	   *relarrayname = NULL;

	pg_class_desc = table_open(RelationRelationId, RowExclusiveLock);

	/*
	 * sanity checks
	 */
	Assert(IsNormalProcessingMode() || IsBootstrapProcessingMode());


	/*
	 * Validate proposed tupdesc for the desired relkind.  If
	 * allow_system_table_mods is on, allow ANYARRAY to be used; this is a
	 * hack to allow creating pg_statistic and cloning it during VACUUM FULL.
	 */
	CheckAttributeNamesTypes(tupdesc, relkind,
							 allow_system_table_mods ? CHKATYPE_ANYARRAY : 0);

	/*
	 * This would fail later on anyway, if the relation already exists.  But
	 * by catching it here we can emit a nicer error message.
	 */
	existing_relid = get_relname_relid(relname, relnamespace);
	if (existing_relid != InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_TABLE),
				 errmsg("relation \"%s\" already exists", relname)));

	/*
	 * Since we are going to create a rowtype as well, also check for
	 * collision with an existing type name.  If there is one and it's an
	 * autogenerated array, we can rename it out of the way; otherwise we can
	 * at least give a good error message.
	 */
	old_type_oid = GetSysCacheOid2(TYPENAMENSP, Anum_pg_type_oid,
								   CStringGetDatum(relname),
								   ObjectIdGetDatum(relnamespace));
	if (OidIsValid(old_type_oid))
	{
		if (!moveArrayTypeName(old_type_oid, relname, relnamespace))
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("type \"%s\" already exists", relname),
					 errhint("A relation has an associated type of the same name, "
							 "so you must use a name that doesn't conflict "
							 "with any existing type.")));
	}

	/*
	 * Shared relations must be in pg_global (last-ditch check)
	 */
	if (shared_relation && reltablespace != GLOBALTABLESPACE_OID)
		elog(ERROR, "shared relations must be placed in pg_global tablespace");

	/*
	 * Allocate an OID for the relation, unless we were told what to use.
	 *
	 * In PostgreSQL, the OID will be the relfilenode as well, but in GPDB
	 * that is assigned separately.
	 */
	if (!OidIsValid(relid))
	{
		relid = GetNewOidForRelation(pg_class_desc, ClassOidIndexId, Anum_pg_class_oid,
									 pstrdup(relname), relnamespace);
	}

	/*
	 * Determine the relation's initial permissions.
	 */
	if (use_user_acl)
	{
		switch (relkind)
		{
			case RELKIND_RELATION:
			case RELKIND_VIEW:
			case RELKIND_MATVIEW:
			case RELKIND_FOREIGN_TABLE:
			case RELKIND_PARTITIONED_TABLE:
				relacl = get_user_default_acl(OBJECT_TABLE, ownerid,
											  relnamespace);
				break;
			case RELKIND_SEQUENCE:
				relacl = get_user_default_acl(OBJECT_SEQUENCE, ownerid,
											  relnamespace);
				break;
			default:
				relacl = NULL;
				break;
		}
	}
	else
		relacl = NULL;

	/*
	 * Create the relcache entry (mostly dummy at this point) and the physical
	 * disk file.  (If we fail further down, it's the smgr's responsibility to
	 * remove the disk file again.)
	 */
	new_rel_desc = heap_create(relname,
							   relnamespace,
							   reltablespace,
							   relid,
							   InvalidOid,
							   accessmtd,
							   tupdesc,
							   relkind,
							   relpersistence,
							   shared_relation,
							   mapped_relation,
							   allow_system_table_mods,
							   &relfrozenxid,
							   &relminmxid);

	Assert(relid == RelationGetRelid(new_rel_desc));

	new_rel_desc->rd_rel->relrewrite = relrewrite;

	/*
	 * Decide whether to create a pg_type entry for the relation's rowtype.
	 * These types are made except where the use of a relation as such is an
	 * implementation detail: toast tables, sequences and indexes.
	 *
	 * Also not for the auxiliary heaps created for bitmap indexes or append-
	 * only tables.
	 *
	 * GPDB:
	 * In Greenplum, if user using the GPDB's create partition table syntax,
	 * it may failed with typename collision since the child partition table
	 * name is generated from user input, which may cause relarrayname exceed
	 * NAMEDATALEN and gets truncated. Then the name may same with other child
	 * table's.
	 *
	 * The below code is different from upstream since we preassign type
	 * OID first on QD and use the name as key to retrieve the pre-assigned
	 * OID from QE.
	 */
	if (!(relkind == RELKIND_SEQUENCE ||
		  relkind == RELKIND_TOASTVALUE ||
		  relkind == RELKIND_INDEX ||
		  relkind == RELKIND_PARTITIONED_INDEX ||
		  relkind == RELKIND_AOSEGMENTS ||
		  relkind == RELKIND_AOBLOCKDIR ||
		  relkind == RELKIND_AOVISIMAP) &&
		  relnamespace != PG_BITMAPINDEX_NAMESPACE)
	{
		Oid		   new_array_oid = InvalidOid;
		ObjectAddress new_type_addr;

		/*
		 * We'll make an array over the composite type, too.  For largely
		 * historical reasons, the array type's OID is assigned first.
		 *
		 * GPDB: Avoid creating array type for AO relation types, it's not
		 * useful for anything and only grows the catalog for no use.
		 */
		if (!RelationIsAppendOptimized(new_rel_desc))
		{
			relarrayname = makeArrayTypeName(relname, relnamespace);
			new_array_oid = AssignTypeArrayOid(relarrayname, relnamespace);
		}

		/*
		 * Make the pg_type entry for the composite type.  The OID of the
		 * composite type can be preselected by the caller, but if reltypeid
		 * is InvalidOid, we'll generate a new OID for it.
		 *
		 * NOTE: we could get a unique-index failure here, in case someone
		 * else is creating the same type name in parallel but hadn't
		 * committed yet when we checked for a duplicate name above.
		 */
		new_type_addr = AddNewRelationType(relname,
										   relnamespace,
										   relid,
										   relkind,
										   ownerid,
										   reltypeid,
										   new_array_oid);
		new_type_oid = new_type_addr.objectId;
		if (typaddress)
			*typaddress = new_type_addr;

		/* GPDB: Avoid creating array type for AO relation types. */
		if (!RelationIsAppendOptimized(new_rel_desc))
		{
            elog(ERROR, "unsupp");
			pfree(relarrayname);
		}
	}
	else
	{
		/* Caller should not be expecting a type to be created. */
		Assert(reltypeid == InvalidOid);
		Assert(typaddress == NULL);

		new_type_oid = InvalidOid;
	}

	/*
	 * If this is an append-only relation, add an entry in pg_appendonly.
	 */
	if (RelationStorageIsAO(new_rel_desc))
	{
		InsertAppendOnlyEntry(relid,
							  InvalidOid,
							  InvalidOid,
							  InvalidOid,
							  AORelationVersion_GetLatest());
	}

	/*
	 * now create an entry in pg_class for the relation.
	 *
	 * NOTE: we could get a unique-index failure here, in case someone else is
	 * creating the same relation name in parallel but hadn't committed yet
	 * when we checked for a duplicate name above.
	 */
	AddNewRelationTuple(pg_class_desc,
						new_rel_desc,
						relid,
						new_type_oid,
						reloftypeid,
						ownerid,
						relkind,
						relfrozenxid,
						relminmxid,
						PointerGetDatum(relacl),
						reloptions);
	/*
	 * now add tuples to pg_attribute for the attributes in our new relation.
	 */
	AddNewAttributeTuples(relid, new_rel_desc->rd_att, relkind, RelationIsAppendOptimized(new_rel_desc));

	/*
	 * Make a dependency link to force the relation to be deleted if its
	 * namespace is.  Also make a dependency link to its owner, as well as
	 * dependencies for any roles mentioned in the default ACL.
	 *
	 * For composite types, these dependencies are tracked for the pg_type
	 * entry, so we needn't record them here.  Likewise, TOAST tables don't
	 * need a namespace dependency (they live in a pinned namespace) nor an
	 * owner dependency (they depend indirectly through the parent table), nor
	 * should they have any ACL entries.  The same applies for extension
	 * dependencies.
	 *
	 * Also, skip this in bootstrap mode, since we don't make dependencies
	 * while bootstrapping.
	 */
	if (relkind != RELKIND_COMPOSITE_TYPE &&
		relkind != RELKIND_TOASTVALUE &&
		!IsBootstrapProcessingMode())
	{
		ObjectAddress myself,
					referenced;

		myself.classId = RelationRelationId;
		myself.objectId = relid;
		myself.objectSubId = 0;

		referenced.classId = NamespaceRelationId;
		referenced.objectId = relnamespace;
		referenced.objectSubId = 0;
		recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

		recordDependencyOnOwner(RelationRelationId, relid, ownerid);

		recordDependencyOnNewAcl(RelationRelationId, relid, 0, ownerid, relacl);

		recordDependencyOnCurrentExtension(&myself, false);

		if (reloftypeid)
		{
			referenced.classId = TypeRelationId;
			referenced.objectId = reloftypeid;
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
		}

		/*
		 * Make a dependency link to force the relation to be deleted if its
		 * access method is. Do this only for relation, materialized views and
		 * partitioned tables.
		 *
		 * No need to add an explicit dependency for the toast table, as the
		 * main table depends on it.
		 */
		if (relkind == RELKIND_RELATION ||
			relkind == RELKIND_MATVIEW ||
			relkind == RELKIND_PARTITIONED_TABLE)
		{
			referenced.classId = AccessMethodRelationId;
			referenced.objectId = accessmtd;
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
		}
	}

	/* Post creation hook for new relation */
	InvokeObjectPostCreateHookArg(RelationRelationId, relid, 0, is_internal);

	/*
	 * Store any supplied constraints and defaults.
	 *
	 * NB: this may do a CommandCounterIncrement and rebuild the relcache
	 * entry, so the relation must be valid and self-consistent at this point.
	 * In particular, there are not yet constraints and defaults anywhere.
	 */
	StoreConstraints(new_rel_desc, cooked_constraints, is_internal);

	/*
	 * If there's a special on-commit action, remember it
	 */
	if (oncommit != ONCOMMIT_NOOP)
		register_on_commit_action(relid, oncommit);

	/*
	 * CDB: If caller gave us a distribution policy, store the distribution
	 * key column list in the gp_distribution_policy catalog and attach a
	 * copy to the relcache entry.
	 */
	if (policy &&
			(Gp_role == GP_ROLE_DISPATCH ||
			 Gp_role == GP_ROLE_EXECUTE ||
			 IsBinaryUpgrade))
	{
		MemoryContext oldcontext;

		Assert(relkind == RELKIND_RELATION ||
			   relkind == RELKIND_PARTITIONED_TABLE ||
			   relkind == RELKIND_MATVIEW ||
			   relkind == RELKIND_FOREIGN_TABLE);

		oldcontext = MemoryContextSwitchTo(GetMemoryChunkContext(new_rel_desc));
		new_rel_desc->rd_cdbpolicy = GpPolicyCopy(policy);
		MemoryContextSwitchTo(oldcontext);
		GpPolicyStore(relid, policy);
	}

	if (Gp_role == GP_ROLE_DISPATCH) /* MPP-11313: */
	{
		bool doIt = true;
		char *subtyp = "TABLE";

		switch (relkind)
		{
			case RELKIND_PARTITIONED_TABLE:
			case RELKIND_RELATION:
			case RELKIND_PROJECTION:
				break;
			case RELKIND_INDEX:
				subtyp = "INDEX";
				break;
			case RELKIND_SEQUENCE:
				subtyp = "SEQUENCE";
				break;
			case RELKIND_VIEW:
				subtyp = "VIEW";
				break;
			case RELKIND_MATVIEW:
				subtyp = "MATVIEW";
				break;
			default:
				doIt = false;
		}

		/* MPP-7576: don't track internal namespace tables */
		switch (relnamespace) 
		{
			case PG_CATALOG_NAMESPACE:
				/* MPP-7773: don't track objects in system namespace
				 * if modifying system tables (eg during upgrade)  
				 */
				if (allowSystemTableMods)
					doIt = false;
				break;

			case PG_TOAST_NAMESPACE:
			case PG_BITMAPINDEX_NAMESPACE:
			case PG_AOSEGMENT_NAMESPACE:
				doIt = false;
				break;
			default:
				break;
		}

		/* MPP-7572: not valid if in any temporary namespace */
		if (doIt)
			doIt = (!(isAnyTempNamespace(relnamespace)));

		/* MPP-6929: metadata tracking */
		if (doIt)
			MetaTrackAddObject(RelationRelationId,
							   relid, GetUserId(), /* not ownerid */
							   "CREATE", subtyp
					);
	}

	/*
	 * ok, the relation has been cataloged, so close our relations and return
	 * the OID of the newly created relation.
	 */
	table_close(new_rel_desc, NoLock);	/* do not unlock till end of xact */
	table_close(pg_class_desc, RowExclusiveLock);

	return relid;
}