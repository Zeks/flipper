--fanfics;
create table if not exists FANFICS (FANDOM VARCHAR NOT NULL, AUTHOR VARCHAR NOT NULL,TITLE VARCHAR NOT NULL,SUMMARY VARCHAR NOT NULL,GENRES VARCHAR,CHARACTERS VARCHAR,RATED VARCHAR,PUBLISHED DATETIME NOT NULL,UPDATED DATETIME NOT NULL, WORDCOUNT INTEGER NOT NULL,FAVOURITES INTEGER NOT NULL,REVIEWS INTEGER NOT NULL,CHAPTERS INTEGER NOT NULL,COMPLETE INTEGER NOT NULL DEFAULT 0,AT_CHAPTER INTEGER NOT NULL, ID INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL  UNIQUE);			
 alter table fanfics add column alive integer default 1;
 alter table fanfics add column wcr real; 
 alter table fanfics add column wcr_adjusted real; 
 alter table fanfics add column reviewstofavourites real; 
 alter table fanfics add column daysrunning integer default null; 
 alter table fanfics add column author_id integer default null; 
 alter table fanfics add column age integer default null; 
 alter table fanfics add column follows integer default 0; 
 alter table fanfics add column fandom1 VARCHAR; 
 alter table fanfics add column fandom2 VARCHAR;
 alter table fanfics add column ffn_id integer default null;
 alter table fanfics add column ao3_id integer default null;
 alter table fanfics add column sb_id integer default null;
 alter table fanfics add column sv_id integer default null;
 alter table fanfics add column author_id integer default null;
 alter table fanfics add column date_added datetime default;
 alter table fanfics add column date_deactivated datetime default null;

 update fanfics set wcr = wordcount*1.0/reviews where wcr is null and reviews > 0 and wordcount > 1000;
 update fanfics set wcr = 200000 where wcr is null and ( reviews = 0 or wordcount <=1000);
 update fanfics set wcr_adjusted = wcr where reviews/favourites < 2.5 and wcr_adjusted is null;
 update fanfics set wcr_adjusted = 200000 where reviews/favourites >= 2.5 and wcr_adjusted is null;
 update fanfics set reviewstofavourites = reviews*1.0/favourites where reviewstofavourites is null;
 update fanfics set age= abs(cast((strftime('%s',CURRENT_TIMESTAMP)-strftime('%s',published) ) AS real )/60/60/24) where daysrunning is null;
 update fanfics set daysrunning= abs(cast((strftime('%s',updated)-strftime('%s',published) ) AS real )/60/60/24) where daysrunning is null;

 CREATE INDEX  if  not exists  main.I_FANFICS_IDENTITY ON FANFICS (AUTHOR ASC, TITLE ASC);
CREATE  INDEX if  not exists  main.I_FANFICS_FANDOM ON FANFICS (FANDOM ASC);
CREATE  INDEX if  not exists main.I_FANFICS_WORDCOUNT ON FANFICS (WORDCOUNT ASC);
CREATE  INDEX if  not exists main.I_FANFICS_TAGS ON FANFICS (TAGS ASC);
CREATE  INDEX if  not exists main.I_FANFICS_ID ON FANFICS (ID ASC);
CREATE  INDEX if  not exists main.I_FANFICS_GENRES ON FANFICS (GENRES ASC);
CREATE INDEX if not exists  I_WCR ON fanfics (wcr ASC);
CREATE INDEX if not exists  I_DAYSRUNNING ON fanfics (daysrunning ASC);
CREATE INDEX if not exists  I_age ON fanfics (age ASC);
CREATE INDEX if not exists  I_reviewstofavourites ON fanfics (reviewstofavourites ASC);
CREATE INDEX if not exists  I_FANDOM1 ON fanfics (fandom1 ASC);
CREATE INDEX if not exists  I_FANDOM2 ON fanfics (fandom2 ASC);
CREATE INDEX if not exists  I_FANFICS_FFN_ID ON fanfics (ffn_id ASC);
CREATE INDEX if not exists  I_ALIVE ON fanfics (alive ASC);
CREATE INDEX if not exists  I_DATE_ADDED ON fanfics (date_added ASC);

-- fanfics sequence;
 CREATE TABLE if not exists sqlite_sequence(name varchar, seq integer);
 INSERT INTO sqlite_sequence(name, seq) SELECT 'fanfics', 0 WHERE NOT EXISTS(SELECT 1 FROM sqlite_sequence WHERE name = 'fanfics');
 update sqlite_sequence set seq = (select max(id) from fanfics) where name = 'fanfics';

--fandoms;
create table if not exists fandoms (FANDOM VARCHAR NOT NULL, SECTION VARCHAR NOT NULL, NORMAL_URL VARCHAR NOT NULL, CROSSOVER_URL VARCHAR NOT NULL);
 alter table fandoms add column id integer AUTOINCREMENT default 0; 
 alter table fandoms add column tracked integer default 0; 
 alter table fandoms add column last_update datetime; 
 alter table fandoms add column date_of_first_fic datetime; 
 alter table fandoms add column date_of_last_fic datetime; 
 alter table fandoms add column date_of_creation datetime; 
 alter table fandoms add column fandom_multiplier integer default 1; 
 alter table fandoms add column fic_count integer default 0; 
 alter table fandoms add column average_faves_top_3 real default 0; 
 alter table fandoms add column source varchar default 'ffn'; 
 
 
 update fandoms set id = rowid where id is null;
 CREATE TABLE if not exists recent_fandoms(fandom varchar, seq_num integer);
 INSERT INTO recent_fandoms(fandom, seq_num) SELECT 'base', 0 WHERE NOT EXISTS(SELECT 1 FROM fandom WHERE fandom = 'base');
 CREATE TABLE if not exists Recommenders (id INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL  UNIQUE , name VARCHAR NOT NULL , url VARCHAR NOT NULL , page_data BLOB, page_updated DATETIME, author_updated DATETIME);
 CREATE INDEX if not exists I_RECOMMENDERS_NAME ON Recommenders (name ASC);
 CREATE INDEX if not exists I_RECOMMENDERS_URL ON Recommenders (URL ASC);
 CREATE TABLE if not exists Recommendations (recommender_id INTEGER NOT NULL , fic_id INTEGER NOT NULL , PRIMARY KEY (recommender_id, fic_id));
 CREATE INDEX if not exists  I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC_TAG ON Recommendations (tag ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_REC_FIC ON Recommendations (recommender_id ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_TAG ON Recommendations (tag ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC ON Recommendations (fic_id ASC);
 CREATE  INDEX if not exists I_FIC_ID ON Recommendations (fic_id ASC);
 alter table Recommenders add column wave integer default 0;
 CREATE  INDEX if not exists I_RECOMMENDER_WAVE ON Recommenders (wave ASC);
 
 
CREATE  INDEX if  not exists main.I_FANDOMS_FANDOM ON FANDOMS (FANDOM ASC);
CREATE  INDEX if  not exists main.I_FANDOMS_SECTION ON FANDOMS (SECTION ASC);
CREATE INDEX if not exists  I_FANDOM_SECTION ON fandoms (FANDOM ASC, SECTION ASC);
update fandoms set id = rowid where id is null;


 CREATE INDEX if not exists  I_FICS_FAVOURITES ON fanfics (favourites ASC);
CREATE VIEW fandom_stats AS  select fandom, 
cast (strftime('%s',CURRENT_TIMESTAMP)-strftime('%s',min(published)) AS real )/60/60/24/365  as age,
min(published) as origin,
(select sum(wcr) from fanfics where wcr < 200000 and fandom = fs.fandom)/(select count(id) from fanfics where wcr < 200000  and fandom = fs.fandom) as averagewcr,
max(favourites) as maxfaves, min(wcr) as minwcr, count(id) as ficcount from fanfics fs group by fandom;

-- user tags;
create table if not exists tags (tag VARCHAR unique NOT NULL);
alter table tags add column id integer;

-- manually assigned tags for fics;
CREATE TABLE if not exists FicTags (fic_id INTEGER NOT NULL, tag varchar,  PRIMARY KEY (fic_id asc, tag asc));
CREATE INDEX if not exists  I_FIC_TAGS_PK ON FicTags (fic_id asc, tag ASC);
CREATE INDEX if not exists  I_FIC_TAGS_TAG ON FicTags (tag ASC);
CREATE INDEX if not exists  I_FIC_TAGS_FIC ON FicTags (fic_id ASC);

CREATE TABLE if not exists Genres (id INTEGER PRIMARY KEY autoincrement NOT NULL default 0, genre varchar, website varchar);
CREATE INDEX if not exists  I_GENRES_PK ON Genres (genre asc, website ASC);
CREATE INDEX if not exists  I_GENRES_GENRE ON Genres (genre ASC);
CREATE INDEX if not exists  I_GENRES_WEBSITE ON Genres (website ASC);

-- fandoms for fics;
CREATE TABLE if not exists FicFandoms (fic_id INTEGER NOT NULL, fandom_id integer,  PRIMARY KEY (fic_id asc, fandom_id asc));
CREATE INDEX if not exists  I_FIC_FANDOMS_PK ON FicFandoms (fic_id asc, fandom_id ASC);
CREATE INDEX if not exists  I_FIC_FANDOMS_FANDOM ON FicFandoms (fandom_id ASC);
CREATE INDEX if not exists  I_FIC_TAGS_FIC ON FicFandoms (fic_id ASC);

 -- recent fandoms;
 CREATE TABLE if not exists recent_fandoms(fandom varchar, seq_num integer);
 INSERT INTO recent_fandoms(fandom, seq_num) SELECT 'base', 0 WHERE NOT EXISTS(SELECT 1 FROM fandoms WHERE fandom = 'base');
  
 --authors table;
 CREATE TABLE if not exists Recommenders (id INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL  UNIQUE , name VARCHAR NOT NULL , url VARCHAR NOT NULL , page_data BLOB, page_updated DATETIME, author_updated DATETIME);
 alter table Recommenders add column wave integer default 0;
 alter table Recommenders add column website_type varchar default null;
 alter table Recommenders add column total_fics integer default null;
 alter table Recommenders add column sumfaves integer default null;
  
 CREATE INDEX if not exists I_AUTHORS_NAME ON Recommenders (name ASC);
 CREATE INDEX if not exists I_AUTHORS_ID ON Recommenders (id ASC);
 CREATE INDEX if not exists I_AUTHORS_URL ON Recommenders (url ASC);
 CREATE INDEX if not exists I_AUTHORS_UPDATED ON Recommenders (author_updated ASC);
 
 update recommenders set sumfaves = (SELECT  Sum(favourites) as summation FROM fanfics where author = recommenders.name) where sumfaves is null;
 update recommenders set sumfaves = 0 where sumfaves is null;
 update Recommenders set website_type = 'ffn' where website_type is null;

 --author fics table;
 CREATE TABLE if not exists Recommendations (recommender_id INTEGER NOT NULL , fic_id INTEGER NOT NULL , PRIMARY KEY (recommender_id, fic_id));
 CREATE INDEX if not exists I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_REC_FIC ON Recommendations (recommender_id ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC ON Recommendations (fic_id ASC);

-- per author stats for specific recommendation list;
CREATE TABLE if not exists RecommendationListAuthorStats (author_id INTEGER NOT NULL , fic_count INTEGER, match_count integer, match_ratio double, list_id integer,  PRIMARY KEY (list_id, author_id));
CREATE INDEX if not exists  I_REC_STATS_LIST_ID ON RecommendationListAuthorStats (list_id ASC);
CREATE INDEX if not exists  I_REC_STATS_AUTHOR ON RecommendationListAuthorStats (author_id ASC);
CREATE INDEX if not exists  I_REC_STATS_RATIO ON RecommendationListAuthorStats (match_ratio ASC);
CREATE INDEX if not exists  I_REC_STATS_COUNT ON RecommendationListAuthorStats (match_count ASC);
CREATE INDEX if not exists  I_REC_STATS_TOTAL_FICS ON RecommendationListAuthorStats (fic_count ASC);

-- a list of all recommendaton lists with their names and statistics;
create table if not exists RecommendationLists(id INTEGER unique PRIMARY KEY AUTOINCREMENT default 1, name VARCHAR unique NOT NULL, minimum integer NOT NULL default 1, pick_ratio double not null default 1, always_pick_at integer not null default 9999, fic_count integer default 0,  created DATETIME);
CREATE INDEX if not exists  I_RecommendationLists_ID ON RecommendationLists (id asc);
CREATE INDEX if not exists  I_RecommendationLists_NAME ON RecommendationLists (NAME asc);
CREATE INDEX if not exists  I_RecommendationLists_created ON RecommendationLists (created asc);

-- data for fandoms present in the list
create table if not exists RecommendationListsFandoms(list_id INTEGER default 0, fandom_id VARCHAR default 0, is_original_fandom integer deault 0, fic_count integer, PRIMARY KEY (list_id, fandom_id))
CREATE INDEX if not exists I_RecommendationListsFandoms_PK ON RecommendationListsFandoms (list_id ASC, fandom_id asc);
CREATE INDEX if not exists  I_RecommendationListsFandoms_LIST_ID ON RecommendationListsFandoms (list_id asc);
CREATE INDEX if not exists  I_RecommendationListsFandoms_fandom_id ON RecommendationListsFandoms (fandom_id asc);
CREATE INDEX if not exists  I_RecommendationListsFandoms_fic_count ON RecommendationListsFandoms (fic_count asc);
CREATE INDEX if not exists  I_RecommendationListsFandoms_IS_ORIGINAL_FANDOM ON RecommendationListsFandoms (is_original_fandom asc);

-- data for recommendation lists;
CREATE TABLE if not exists RecommendationListData(fic_id INTEGER NOT NULL, list_id integer, match_count integer default 0, PRIMARY KEY (fic_id asc, list_id asc));
CREATE INDEX if not exists  I_LIST_TAGS_PK ON RecommendationListData (list_id asc, fic_id asc, match_count asc);
CREATE INDEX if not exists  I_LISTDATA_ID ON RecommendationListData (list_id ASC);
CREATE INDEX if not exists  I_LISTDATA_FIC ON RecommendationListData (fic_id ASC);
CREATE INDEX if not exists  I_LISTDATA_MATCHCOUNT ON RecommendationListData (match_count ASC);