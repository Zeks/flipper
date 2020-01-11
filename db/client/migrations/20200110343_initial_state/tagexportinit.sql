-- fictag table;
CREATE TABLE if not exists UserFicTags (
 ffn_id integer default -1,
 ao3_id integer default -1,
 sb_id integer default -1,
 sv_id integer default -1,
 tag varchar );
-- tag table; 
 CREATE TABLE if not exists UserTags (
 id integer default 0,
 tag varchar );
