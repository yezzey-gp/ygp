create or replace function public.annotate_query(query text)
returns text
as '$libdir/annotate_query.so','annotate_query'
language c immutable strict;
