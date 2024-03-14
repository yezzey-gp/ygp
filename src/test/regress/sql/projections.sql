create table aott(i int, j int);
create projection p1 on aott distributed by (j);

explain analyze insert into aott values(4,5), (5,6), (7,8), (1, 2), (2,3);

select * from aott;
select * from p1;

