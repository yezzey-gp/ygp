## Что это?

Это небольшая библиотека, которая позволяет посмотреть синтаксическое дерево разбора запроса парсером в greenplum. Для использования нужно скомпилировать библиотеку `annotate_query.so` на мастере greenplum, распространить ее по кластеру (например, через gpscp) и создать обертку в виде sql функции для работы с библиотекой.

## Как это использовать?

Все шаги ниже производятся на мастере.

* Склонировать проект
```bash
git clone https://github.com/arenadata/gp_parser.git
cd gp_parser
```
* Скомпилировать библиотеку с помощью `make`. Должен появиться файл `annotate_query.so`

* Нужно распространить библиотеку по всем хостам кластера в каталог с библиотеками greenplum. Возможно, придется это делать из-под sudo в зависимости от прав на целевой каталог
```bash
gpscp -f all_hosts.hosts annotate_query.so =:$(pg_config --pkglibdir)
# gpscp -h mdw -h sdw1 -h sdw2 annotate_query.so =:$(pg_config --pkglibdir)
```

* После необходимо создать функцию-обертку на sql над функциями в библиотеке
```bash
psql -f annotate_query.sql your_database
```

* Проверим синтаксическое дерево разбора тестового запроса
```sql
select annotate_query($$
  select 1
$$);
-----------------------------------------------------------------------------------------------
({SELECT :distinctClause <> :intoClause <> :targetList ({RESTARGET :name <> :indirection <> :val {A_CONST 1 :typename <>} :location 10}) :fromClause <> :whereClause <> :groupClause <> :havingClause <> :w
indowClause <> :valuesLists <> :sortClause <> :scatterClause <> :withClause <> :limitOffset <> :limitCount <> :lockingClause <> :op 0 :all false :larg <> :rarg <> :distributedBy <>})
(1 row)

```

### Основано на проекте

https://github.com/pganalyze/queryparser
