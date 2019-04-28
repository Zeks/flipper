
CREATE TABLE if not exists user_settings(name varchar unique, value integer unique);
INSERT INTO user_settings(name, value) values('Last Fandom Id', 0);
INSERT INTO user_settings(name, value) values('user_ffn_id', -1);

 
CREATE TABLE if not exists recent_fandoms(fandom varchar, seq_num integer);

-- needs investigating, a shit code most likely; 
INSERT INTO recent_fandoms(fandom, seq_num) SELECT 'base', 0 WHERE NOT EXISTS(SELECT 1 FROM fandoms WHERE fandom = 'base'); 
CREATE TABLE if not exists ignored_fandoms(fandom_id INTEGER PRIMARY KEY, including_crossovers integer default 0);
CREATE INDEX if not exists I_IGNORED_FANDOMS_ID ON ignored_fandoms (fandom_id ASC);  
CREATE TABLE if not exists ignored_fandoms_slash_filter(fandom_id INTEGER PRIMARY KEY);
CREATE INDEX if not exists I_IGNORED_FANDOMS_ID ON ignored_fandoms (fandom_id ASC);  

-- fictag table;
CREATE TABLE if not exists FicTags (
 fic_id integer default -1,
 ffn_id integer default -1,
 ao3_id integer default -1,
 sb_id integer default -1,
 sv_id integer default -1,
 tag varchar );
 alter table FicTags add column added datetime default 0;
CREATE INDEX if not exists I_FICTAGS_DBID ON FicTags (fic_id ASC);
CREATE INDEX if not exists I_FICTAGS_FFNID ON FicTags (ffn_id ASC);
CREATE INDEX if not exists I_FICTAGS_TAG_ADDED ON FicTags (added ASC);
update FicTags set added = date('now') where added is null;
 
 -- ficscore table;
CREATE TABLE if not exists FicScores (
 fic_id integer unique,
 score integer );
 alter table FicScores add column updated datetime;
CREATE INDEX if not exists I_FICSCORES_DBID ON FicTags (fic_id ASC);
CREATE INDEX if not exists I_FICSCORES_SCORE ON FicTags (score ASC);

CREATE TABLE if not exists FicSnoozes (
 fic_id integer unique,
 snooze_added datetime default date('now'),
 snoozed_at_chapter integer,
 snoozed_till_chapter integer,
 snoozed_until_finished integer default 1,
 snooze_expired integeger default 0);
 alter table FicSnoozes add column expired integer default 0;
CREATE INDEX if not exists I_FICSNOOZES_DBID ON FicTags (fic_id ASC);
CREATE INDEX if not exists I_FICSNOOZES_ADDED ON FicTags (snooze_added ASC);
CREATE INDEX if not exists I_FICSNOOZES_EXPIRED ON FicTags (expired ASC);

 
 -- tag table;  
CREATE TABLE if not exists Tags ( id integer default 0, tag varchar unique NOT NULL);
--INSERT INTO Tags(tag, id) values('Dead', 1);
--INSERT INTO Tags(tag, id) values('Moar_pls', 2);
--INSERT INTO Tags(tag, id) values('Hide', 3);
--INSERT INTO Tags(tag, id) values('Meh', 4);
--INSERT INTO Tags(tag, id) values('Liked', 5);
--INSERT INTO Tags(tag, id) values('Disgusting', 6);
--INSERT INTO Tags(tag, id) values('Reading', 7);
--INSERT INTO Tags(tag, id) values('Read_Queue', 8);
--INSERT INTO Tags(tag, id) values('Finished', 9);
--INSERT INTO Tags(tag, id) values('WTF', 10);


-- manually assigned tags for fics;
--CREATE TABLE if not exists FicTags (fic_id INTEGER NOT NULL, tag varchar,  PRIMARY KEY (fic_id asc, tag asc));
--CREATE INDEX if not exists  I_FIC_TAGS_PK ON FicTags (fic_id asc, tag ASC);
--CREATE INDEX if not exists  I_FIC_TAGS_TAG ON FicTags (tag ASC);
--CREATE INDEX if not exists  I_FIC_TAGS_FIC ON FicTags (fic_id ASC);
 
-- a list of all recommendaton lists with their names and statistics;
create table if not exists RecommendationLists(id INTEGER unique PRIMARY KEY AUTOINCREMENT default 1, name VARCHAR unique NOT NULL, minimum integer NOT NULL default 1, pick_ratio double not null default 1, always_pick_at integer not null default 9999, fic_count integer default 0,  created DATETIME);
alter table RecommendationLists add column sources varchar;
alter table RecommendationLists add column use_weighting integer default 1;
alter table RecommendationLists add column use_mood_adjustment integer default 1;

CREATE INDEX if not exists  I_RecommendationLists_ID ON RecommendationLists (id asc);
CREATE INDEX if not exists  I_RecommendationLists_NAME ON RecommendationLists (NAME asc);
CREATE INDEX if not exists  I_RecommendationLists_created ON RecommendationLists (created asc);

-- data for fandoms present in the list;
create table if not exists RecommendationListsFandoms(list_id INTEGER default 0, fandom_id VARCHAR default 0, is_original_fandom integer default 0, 
fic_count integer, PRIMARY KEY (list_id, fandom_id));
CREATE INDEX if not exists I_RecommendationListsFandoms_PK ON RecommendationListsFandoms (list_id ASC, fandom_id asc);
CREATE INDEX if not exists  I_RecommendationListsFandoms_LIST_ID ON RecommendationListsFandoms (list_id asc);
CREATE INDEX if not exists  I_RecommendationListsFandoms_fandom_id ON RecommendationListsFandoms (fandom_id asc);
CREATE INDEX if not exists  I_RecommendationListsFandoms_fic_count ON RecommendationListsFandoms (fic_count asc);
CREATE INDEX if not exists  I_RecommendationListsFandoms_IS_ORIGINAL_FANDOM ON RecommendationListsFandoms (is_original_fandom asc);

-- data for recommendation lists;
CREATE TABLE if not exists RecommendationListData(
fic_id INTEGER NOT NULL, 
list_id integer,
 is_origin integer default 0, 
 match_count integer default 0,
 PRIMARY KEY (fic_id asc, list_id asc));
  alter table RecommendationListData add column votes_common integer default 0;
  alter table RecommendationListData add column votes_uncommon integer default 0;
  alter table RecommendationListData add column votes_rare integer default 0;
  alter table RecommendationListData add column votes_unique integer default 0;
  
  alter table RecommendationListData add column value_common integer default 0;
  alter table RecommendationListData add column value_uncommon integer default 0;
  alter table RecommendationListData add column value_rare integer default 0;
  alter table RecommendationListData add column value_unique integer default 0;
  alter table RecommendationListData add column purged integer default 0;
  
  alter table RecommendationListData add column breakdown_available integer default 0;

CREATE INDEX if not exists  I_LIST_TAGS_PK ON RecommendationListData (list_id asc, fic_id asc, match_count asc);
CREATE INDEX if not exists  I_LISTDATA_ID ON RecommendationListData (list_id ASC);
CREATE INDEX if not exists  I_IS_ORIGIN ON RecommendationListData (is_origin ASC);
CREATE INDEX if not exists  I_LISTDATA_FIC ON RecommendationListData (fic_id ASC);
CREATE INDEX if not exists  I_LISTDATA_MATCHCOUNT ON RecommendationListData (match_count ASC);


create table if not exists fandomindex (id integer, name VARCHAR NOT NULL, tracked integer default 0, primary key(id, name));
alter table fandomindex add column updated datetime;
CREATE INDEX if not exists I_FANDOMINDEX_PK ON fandomindex (id ASC, name asc);
CREATE INDEX if not exists I_FANDOMINDEX_ID ON fandomindex (id ASC);
CREATE INDEX if not exists I_FANDOMINDEX_NAME ON fandomindex (name ASC);

create table if not exists fandomurls (global_id integer, url VARCHAR NOT NULL, website varchar not null, custom VARCHAR, primary key(global_id, url));
CREATE INDEX if not exists I_FURL_ID ON fandomurls (global_id ASC);
CREATE INDEX if not exists I_FURL_URL ON fandomurls (url ASC);
CREATE INDEX if not exists I_FURL_CUSTOM ON fandomurls (custom ASC);
CREATE INDEX if not exists I_FURL_WEBSITE ON fandomurls (website ASC);


create table if not exists ficnotes (
fic_id integer PRIMARY KEY not null, 
note_content varchar);

CREATE INDEX if not exists I_FNQ_FIC_ID ON ficnotesquotes (fic_id ASC);


-- per author stats for specific recommendation list, only in this file to support queries;
CREATE TABLE if not exists RecommendationListAuthorStats (author_id INTEGER NOT NULL , fic_count INTEGER, match_count integer, match_ratio double, list_id integer,  PRIMARY KEY (list_id, author_id));
CREATE INDEX if not exists  I_REC_STATS_LIST_ID ON RecommendationListAuthorStats (list_id ASC);
CREATE INDEX if not exists  I_REC_STATS_AUTHOR ON RecommendationListAuthorStats (author_id ASC);
CREATE INDEX if not exists  I_REC_STATS_RATIO ON RecommendationListAuthorStats (match_ratio ASC);
CREATE INDEX if not exists  I_REC_STATS_COUNT ON RecommendationListAuthorStats (match_count ASC);
CREATE INDEX if not exists  I_REC_STATS_TOTAL_FICS ON RecommendationListAuthorStats (fic_count ASC);

--CREATE TABLE if not exists authorstats (author_id INTEGER NOT NULL PRIMARY KEY, minor_liked INTEGER, minor_disliked integer major_liked double, major_disliked integer);
--CREATE INDEX if not exists  I_AUTHOR_STATS_AUTHOR_ID ON authorstats (author_id ASC);
--CREATE INDEX if not exists  I_AUTHOR_STATS_AUTHOR_ID_MINOR_LIKED ON authorstats (author_id  ASC, minor_liked asc);
--CREATE INDEX if not exists  I_AUTHOR_STATS_AUTHOR_ID_MINOR_DISLIKED ON authorstats (author_id  ASC, minor_disliked asc);
--CREATE INDEX if not exists  I_AUTHOR_STATS_AUTHOR_ID_MAJOR_LIKED ON authorstats (author_id  ASC, major_liked asc);
--CREATE INDEX if not exists  I_AUTHOR_STATS_AUTHOR_ID_MAJOR_DISLIKED ON authorstats (author_id  ASC, major_disliked asc);

-- fictag table;
CREATE TABLE if not exists FicAuthors (
 fic_id integer default -1,
 author_id integer default -1,  PRIMARY KEY (fic_id, author_id)
 );
CREATE INDEX if not exists I_FICAUTHORS_FIC_ID ON FicAuthors (fic_id ASC);
CREATE INDEX if not exists I_FICAUTHORS_AUTHOR_ID ON FicAuthors (author_id ASC);

