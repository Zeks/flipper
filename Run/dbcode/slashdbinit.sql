-- slash db;
 create table if not exists slash_data_ffn (ffn_id integer primary key);
 alter table slash_data_ffn add column keywords_yes integer default 0;
 alter table slash_data_ffn add column keywords_no integer default 0;
 alter table slash_data_ffn add column keywords_result integer default 0;
 alter table slash_data_ffn add column filter_pass_1 integer default 0;
 alter table slash_data_ffn add column filter_pass_2 integer default 0;