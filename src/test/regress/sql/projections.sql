create table aott(i int, j int);
create projection p1 on aott distributed by (j);

explain analyze insert into aott values(4,5), (5,6), (7,8), (1, 2), (2,3);

select * from aott;
select * from p1;


select count(1) from p1;
delete from p1 where i = 1;
update p1 set i = 1999 where i  =7;
select count(1) from p1;

