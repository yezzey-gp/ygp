--  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
--  Gpperfmon Schema

-- Note: In 4.x, this file was run as part of upgrade (in single user mode).
-- Therefore, we could not make use of psql escape sequences such as
-- "\c gpperfmon" and every statement had to be on a single line.
--
-- Violating the above _would_ break 4.x upgrades.
--

--  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
--  system
--

alter database gpperfmon set gp_default_storage_options to 'appendonly=false,blocksize=32768,compresstype=none,checksum=true,orientation=row';

\c gpperfmon;

drop table if exists public.system_history;
create table public.system_history
(
    ctime timestamp(0) not null, -- record creation time
    hostname varchar(64) not null, -- hostname of system this metric belongs to
    mem_total bigint not null, mem_used bigint not null, -- total system memory
    mem_actual_used bigint not null, mem_actual_free bigint not null, -- memory used
    swap_total bigint not null, swap_used bigint not null, -- total swap space
    swap_page_in bigint not null, swap_page_out bigint not null, -- swap pages in
    cpu_user float not null, cpu_sys float not null, cpu_idle float not null, -- cpu usage
    load0 float not null, load1 float not null, load2 float not null, -- cpu load avgs
    quantum int not null, -- interval between metric collection for this entry
    disk_ro_rate bigint not null, -- system disk read ops per second
    disk_wo_rate bigint not null, -- system disk write ops per second
    disk_rb_rate bigint not null, -- system disk read bytes per second
    disk_wb_rate bigint not null, -- system disk write bytes per second
    net_rp_rate bigint not null,  -- system net read packets per second
    net_wp_rate bigint not null,  -- system net write packets per second
    net_rb_rate bigint not null,  -- system net read bytes per second
    net_wb_rate bigint not null   -- system net write bytes per second
)
    with (fillfactor=100)
    distributed by (ctime)
    partition by range (ctime)(start (date '2010-01-01') end (date '2010-02-01') EVERY (interval '1 month'));


--  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
--  queries
--

drop table if exists public.queries_history;
create table public.queries_history
(
    ctime            timestamp(0),                     -- record creation time
    tmid             int          not null,            -- time id
    ssid             int          not null,            -- session id
    ccnt             int          not null,            -- command count in session
    username         varchar(64)  not null,            -- username that issued the query
    db               varchar(64)  not null,            -- database name for the query
    cost             int          not null,            -- query cost (not implemented)
    tsubmit          timestamp(0) not null,            -- query submit time
    tstart           timestamp(0),                     -- query start time
    tfinish          timestamp(0),                     -- query end time
    status           varchar(64)  not null,            -- query status (start, end, abort)
    rows_out         bigint       not null,            -- rows out for query
    cpu_elapsed      bigint       not null,            -- cpu usage for query across all segments
    cpu_currpct      float        not null,            -- current cpu percent avg for all processes executing query
    skew_cpu         float        not null,            -- coefficient of variance for cpu_elapsed of iterators across segments for query
    skew_rows        float        not null,            -- coefficient of variance for rows_in of iterators across segments for query
    query_hash       bigint       not null,            -- (not implemented)
    query_text       text         not null default '', -- query text
    query_plan       text         not null default '', -- query plan (not implemented)
    application_name varchar(64),                      -- from 4.2 onwards
    rsqname          varchar(64),                      -- from 4.2 onwards
    rqppriority      varchar(16),                      -- from 4.2 onwards

    CONSTRAINT pk_queries_history PRIMARY KEY (ctime, tmid, ssid, ccnt)
)
    with (fillfactor=100)
    distributed by (ctime)
    partition by range (ctime)(start (date '2010-01-01') end (date '2010-02-01') EVERY (interval '1 month'));

drop index if exists i_queries_history_tmid_ssid_ccnt;
drop index if exists i_queries_history_ctime;
CREATE INDEX i_queries_history_tmid_ssid_ccnt ON public.queries_history (tmid, ssid, ccnt);
CREATE INDEX i_queries_history_ctime ON public.queries_history (ctime);

drop table if exists public.queries_now;
create table public.queries_now
(
    ctime            timestamp(0),                     -- record creation time
    tmid             int          not null,            -- time id
    ssid             int          not null,            -- session id
    ccnt             int          not null,            -- command count in session
    username         varchar(64)  not null,            -- username that issued the query
    db               varchar(64)  not null,            -- database name for the query
    cost             int          not null,            -- query cost (not implemented)
    tsubmit          timestamp(0) not null,            -- query submit time
    tstart           timestamp(0),                     -- query start time
    tfinish          timestamp(0),                     -- query end time
    status           varchar(64)  not null,            -- query status (start, end, abort)
    rows_out         bigint       not null,            -- rows out for query
    cpu_elapsed      bigint       not null,            -- cpu usage for query across all segments
    cpu_currpct      float        not null,            -- current cpu percent avg for all processes executing query
    skew_cpu         float        not null,            -- coefficient of variance for cpu_elapsed of iterators across segments for query
    skew_rows        float        not null,            -- coefficient of variance for rows_in of iterators across segments for query
    query_hash       bigint       not null,            -- (not implemented)
    query_text       text         not null default '', -- query text
    query_plan       text         not null default '', -- query plan (not implemented)
    application_name varchar(64),                      -- from 4.2 onwards
    rsqname          varchar(64),                      -- from 4.2 onwards
    rqppriority      varchar(16),                      -- from 4.2 onwards

    CONSTRAINT pk_queries_now PRIMARY KEY (ctime, tmid, ssid, ccnt)
)
    with (fillfactor=100)
    distributed by (ctime);

drop index if exists i_queries_now_tmid_ssid_ccnt;
drop index if exists i_queries_now_ctime;
CREATE INDEX i_queries_now_tmid_ssid_ccnt ON public.queries_now (tmid, ssid, ccnt);
CREATE INDEX i_queries_now_ctime ON public.queries_now (ctime);

--  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
--  database
--

drop table if exists public.database_history;
create table public.database_history
(
    ctime timestamp(0) not null, -- record creation time
    queries_total int not null, -- total number of queries
    queries_running int not null, -- number of running queries
    queries_queued int not null -- number of queued queries
)
    with (fillfactor=100)
    distributed by (ctime)
    partition by range (ctime)(start (date '2010-01-01') end (date '2010-02-01') EVERY (interval '1 month'));

--  !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
--  Web API views
--

-- TABLE: segment_history
--   ctime                      record creation time
--   dbid                       segment database id
--   hostname                   hostname of system this metric belongs to
--   dynamic_memory_used        bytes of dynamic memory used by the segment
--   dynamic_memory_available   bytes of dynamic memory available for use by the segment

drop table if exists public.segment_history;
create table public.segment_history
(
    ctime timestamp(0) not null,
    dbid int not null,
    hostname varchar(64) not null,
    dynamic_memory_used bigint not null,
    dynamic_memory_available bigint not null
)
    with (fillfactor=100)
    distributed by (ctime)
    partition by range (ctime)(start (date '2010-01-01') end (date '2010-02-01') EVERY (interval '1 month'));

DROP VIEW IF EXISTS public.memory_info;
DROP VIEW IF EXISTS public.dynamic_memory_info;

-- VIEW: dynamic_memory_info
CREATE VIEW public.dynamic_memory_info
as select public.segment_history.ctime,
          public.segment_history.hostname,
          round(sum(public.segment_history.dynamic_memory_used)/1024/1024, 2) AS dynamic_memory_used_mb,
          round(sum(public.segment_history.dynamic_memory_available)/1024/1024, 2) AS dynamic_memory_available_mb
   FROM public.segment_history
   GROUP BY public.segment_history.ctime,
            public.segment_history.hostname;

-- VIEW: memory_info
CREATE VIEW public.memory_info
as select public.system_history.ctime,
          public.system_history.hostname,
          round(public.system_history.mem_total/1024/1024, 2) as mem_total_mb,
          round(public.system_history.mem_used/1024/1024, 2) as mem_used_mb,
          round(public.system_history.mem_actual_used/1024/1024, 2) as mem_actual_used_mb,
          round(public.system_history.mem_actual_free/1024/1024, 2) as mem_actual_free_mb,
          round(public.system_history.swap_total/1024/1024, 2) as swap_total_mb,
          round(public.system_history.swap_used/1024/1024, 2) as swap_used_mb,
          dynamic_memory_info.dynamic_memory_used_mb as dynamic_memory_used_mb,
          dynamic_memory_info.dynamic_memory_available_mb as dynamic_memory_available_mb
   FROM public.system_history, dynamic_memory_info
   WHERE public.system_history.hostname = dynamic_memory_info.hostname
     AND public.system_history.ctime = public.dynamic_memory_info.ctime;


-- TABLE: diskspace_history
--   ctime                      time of measurement
--   hostname                   hostname of measurement
--   filesytem                  name of filesystem for measurement
--   total_bytes                bytes total in filesystem
--   bytes_used                 bytes used in the filesystem
--   bytes_available            bytes available in the filesystem

drop table if exists public.diskspace_history;
create table public.diskspace_history
(
    ctime timestamp(0) not null,
    hostname varchar(64) not null,
    filesystem text not null,
    total_bytes bigint not null,
    bytes_used bigint not null,
    bytes_available bigint not null
)
    with (fillfactor=100)
    distributed by (ctime)
    partition by range (ctime) (start (date '2010-01-01') end (date '2010-02-01') EVERY (interval '1 month'));

-- TABLE: network_interface_history -------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- ctime timestamp(0) not null,
-- hostname varchar(64) not null,
-- interface_name varchar(64) not null,
-- bytes_received bigint,
-- packets_received bigint,
-- receive_errors bigint,
-- receive_drops bigint,
-- receive_fifo_errors bigint,
-- receive_frame_errors bigint,
-- receive_compressed_packets int,
-- receive_multicast_packets int,
-- bytes_transmitted bigint,
-- packets_transmitted bigint,
-- transmit_errors bigint,
-- transmit_drops bigint,
-- transmit_fifo_errors bigint,
-- transmit_collision_errors bigint,
-- transmit_carrier_errors bigint,
-- transmit_compressed_packets int

drop table if exists public.network_interface_history;
create table public.network_interface_history
(
    ctime timestamp(0) not null,
    hostname varchar(64) not null,
    interface_name varchar(64) not null,
    bytes_received bigint,
    packets_received bigint,
    receive_errors bigint,
    receive_drops bigint,
    receive_fifo_errors bigint,
    receive_frame_errors bigint,
    receive_compressed_packets int,
    receive_multicast_packets int,
    bytes_transmitted bigint,
    packets_transmitted bigint,
    transmit_errors bigint,
    transmit_drops bigint,
    transmit_fifo_errors bigint,
    transmit_collision_errors bigint,
    transmit_carrier_errors bigint,
    transmit_compressed_packets int
)
    with (fillfactor=100)
    distributed by (ctime)
    partition by range (ctime) (start (date '2010-01-01') end (date '2010-02-01') EVERY (interval '1 month'));

-- TABLE: sockethistory --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
-- ctime timestamp(0) not null,
-- hostname varchar(64) not null,
-- total_sockets_used int,
-- tcp_sockets_inuse int,
-- tcp_sockets_orphan int,
-- tcp_sockets_timewait int,
-- tcp_sockets_alloc int,
-- tcp_sockets_memusage_inbytes int,
-- udp_sockets_inuse int,
-- udp_sockets_memusage_inbytes int,
-- raw_sockets_inuse int,
-- frag_sockets_inuse int,
-- frag_sockets_memusage_inbytes int

drop table if exists public.socket_history;
create table public.socket_history
(
    ctime timestamp(0) not null,
    hostname varchar(64) not null,
    total_sockets_used int,
    tcp_sockets_inuse int,
    tcp_sockets_orphan int,
    tcp_sockets_timewait int,
    tcp_sockets_alloc int,
    tcp_sockets_memusage_inbytes int,
    udp_sockets_inuse int,
    udp_sockets_memusage_inbytes int,
    raw_sockets_inuse int,
    frag_sockets_inuse int,
    frag_sockets_memusage_inbytes int
)
    with (fillfactor=100)
    distributed by (ctime)
    partition by range (ctime) (start (date '2010-01-01') end (date '2010-02-01') EVERY (interval '1 month'));

-- schema changes for gpperfmon needed to complete the creation of the schema

revoke all on database gpperfmon from public;

-- for web ui auth everyone needs connect permissions
grant connect on database gpperfmon to public;

-- END
