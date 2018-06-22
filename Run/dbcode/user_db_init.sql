
CREATE TABLE if not exists user_settings(name varchar unique, value integer);
INSERT INTO user_settings(name, name) values('Last Fandom Id', 0);

 
CREATE TABLE if not exists recent_fandoms(fandom varchar, seq_num integer);

-- needs investigating, a shit code most likely; 
INSERT INTO recent_fandoms(fandom, seq_num) SELECT 'base', 0 WHERE NOT EXISTS(SELECT 1 FROM fandoms WHERE fandom = 'base'); 
CREATE TABLE if not exists ignored_fandoms(fandom_id INTEGER PRIMARY KEY, including_crossovers integer default 0);
CREATE INDEX if not exists I_IGNORED_FANDOMS_ID ON ignored_fandoms (fandom_id ASC);  
CREATE TABLE if not exists ignored_fandoms_slash_filter(fandom_id INTEGER PRIMARY KEY);
CREATE INDEX if not exists I_IGNORED_FANDOMS_ID ON ignored_fandoms (fandom_id ASC);  

-- user tags;
create table if not exists tags (tag VARCHAR unique NOT NULL, id integer);
INSERT INTO tags(tag, id) values('Dead', 1);
INSERT INTO tags(tag, id) values('Moar_pls', 2);
INSERT INTO tags(tag, id) values('Hide', 3);
INSERT INTO tags(tag, id) values('Meh', 4);
INSERT INTO tags(tag, id) values('Liked', 5);
INSERT INTO tags(tag, id) values('Disgusting', 6);
INSERT INTO tags(tag, id) values('Reading', 7);
INSERT INTO tags(tag, id) values('Read_Queue', 8);
INSERT INTO tags(tag, id) values('Finished', 9);
INSERT INTO tags(tag, id) values('WTF', 10);


-- manually assigned tags for fics;
CREATE TABLE if not exists FicTags (fic_id INTEGER NOT NULL, tag varchar,  PRIMARY KEY (fic_id asc, tag asc));
CREATE INDEX if not exists  I_FIC_TAGS_PK ON FicTags (fic_id asc, tag ASC);
CREATE INDEX if not exists  I_FIC_TAGS_TAG ON FicTags (tag ASC);
CREATE INDEX if not exists  I_FIC_TAGS_FIC ON FicTags (fic_id ASC);
 
-- a list of all recommendaton lists with their names and statistics;
create table if not exists RecommendationLists(id INTEGER unique PRIMARY KEY AUTOINCREMENT default 1, name VARCHAR unique NOT NULL, minimum integer NOT NULL default 1, pick_ratio double not null default 1, always_pick_at integer not null default 9999, fic_count integer default 0,  created DATETIME);
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
CREATE TABLE if not exists RecommendationListData(fic_id INTEGER NOT NULL, list_id integer, is_origin integer default 0, match_count integer default 0, PRIMARY KEY (fic_id asc, list_id asc));
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

-- per author stats for specific recommendation list, only in this file to support queries;
CREATE TABLE if not exists RecommendationListAuthorStats (author_id INTEGER NOT NULL , fic_count INTEGER, match_count integer, match_ratio double, list_id integer,  PRIMARY KEY (list_id, author_id));
CREATE INDEX if not exists  I_REC_STATS_LIST_ID ON RecommendationListAuthorStats (list_id ASC);
CREATE INDEX if not exists  I_REC_STATS_AUTHOR ON RecommendationListAuthorStats (author_id ASC);
CREATE INDEX if not exists  I_REC_STATS_RATIO ON RecommendationListAuthorStats (match_ratio ASC);
CREATE INDEX if not exists  I_REC_STATS_COUNT ON RecommendationListAuthorStats (match_count ASC);
CREATE INDEX if not exists  I_REC_STATS_TOTAL_FICS ON RecommendationListAuthorStats (fic_count ASC);
