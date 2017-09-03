create table if not exists FANFICS (FANDOM VARCHAR NOT NULL, AUTHOR VARCHAR NOT NULL,TITLE VARCHAR NOT NULL,SUMMARY VARCHAR NOT NULL,GENRES VARCHAR,CHARACTERS VARCHAR,RATED VARCHAR,PUBLISHED DATETIME NOT NULL,UPDATED DATETIME NOT NULL,URL VARCHAR NOT NULL,TAGS VARCHAR NOT NULL DEFAULT ' none ',            WORDCOUNT INTEGER NOT NULL,FAVOURITES INTEGER NOT NULL,REVIEWS INTEGER NOT NULL,CHAPTERS INTEGER NOT NULL,COMPLETE INTEGER NOT NULL DEFAULT 0,AT_CHAPTER INTEGER NOT NULL,ORIGIN VARCHAR NOT NULL, ID INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL  UNIQUE);			
create table if not exists fandoms 
                            (FANDOM VARCHAR NOT NULL,
             SECTION VARCHAR NOT NULL, 
            NORMAL_URL VARCHAR NOT NULL,
            CROSSOVER_URL VARCHAR NOT NULL);
			
create table if not exists tags (tag VARCHAR unique NOT NULL);

CREATE INDEX  if  not exists  main.I_FANFICS_IDENTITY ON FANFICS (AUTHOR ASC, TITLE ASC);
CREATE  INDEX if  not exists  main.I_FANFICS_FANDOM ON FANFICS (FANDOM ASC);
CREATE  INDEX if  not exists main.I_FANFICS_WORDCOUNT ON FANFICS (WORDCOUNT ASC);
CREATE  INDEX if  not exists main.I_FANFICS_TAGS ON FANFICS (TAGS ASC);
CREATE  INDEX if  not exists main.I_FANFICS_ID ON FANFICS (ID ASC);
CREATE  INDEX if  not exists main.I_FANFICS_GENRES ON FANFICS (GENRES ASC);
CREATE  INDEX if  not exists main.I_FANDOMS_FANDOM ON FANDOMS (FANDOM ASC);
CREATE  INDEX if  not exists main.I_FANDOMS_SECTION ON FANDOMS (SECTION ASC);


 CREATE INDEX if not exists  I_FANDOM_SECTION ON fandoms (FANDOM ASC, SECTION ASC);
 CREATE TABLE if not exists sqlite_sequence(name varchar, seq integer);
 INSERT INTO sqlite_sequence(name, seq) SELECT 'fanfics', 0 WHERE NOT EXISTS(SELECT 1 FROM sqlite_sequence WHERE name = 'fanfics');
 update sqlite_sequence set seq = (select max(id) from fanfics) where name = 'fanfics';
 --не забыть сделать авторасстановку;
 alter table tags add column id integer;
 alter table fandoms add column tracked integer default 0; 
 alter table fandoms add column tracked_crossovers integer default 0; 
 alter table fandoms add column last_update datetime; 
 alter table fandoms add column last_update_crossover datetime; 
 alter table fandoms add column id integer; 
 update fandoms set id = rowid where id is null;
 CREATE TABLE if not exists recent_fandoms(fandom varchar, seq_num integer);
 INSERT INTO recent_fandoms(fandom, seq_num) SELECT 'base', 0 WHERE NOT EXISTS(SELECT 1 FROM fandom WHERE fandom = 'base');
 CREATE TABLE if not exists Recommenders (id INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL  UNIQUE , name VARCHAR NOT NULL , url VARCHAR NOT NULL , page_data BLOB, page_updated DATETIME, author_updated DATETIME);
 CREATE TABLE if not exists Recommendations (recommender_id INTEGER NOT NULL , fic_id INTEGER NOT NULL , PRIMARY KEY (recommender_id, fic_id));
 CREATE INDEX if not exists  I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC_TAG ON Recommendations (tag ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_REC_FIC ON Recommendations (recommender_id ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_TAG ON Recommendations (tag ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC ON Recommendations (fic_id ASC);
 CREATE  INDEX if  not exists I_FIC_ID ON Recommendations (fic_id ASC);
 alter table Recommenders add column wave integer default 0;
 CREATE  INDEX if not exists I_RECOMMENDER_WAVE ON Recommenders (wave ASC);
 alter table fandoms add column fandom_multiplier integer default 1; 
 alter table fanfics add column wcr real; 
 alter table fanfics add column wcr_adjusted real; 
 alter table fanfics add column reviewstofavourites real; 
 alter table fanfics add column daysrunning integer default null; 
 alter table fanfics add column age integer default null; 
 update fanfics set wcr = wordcount*1.0/reviews where wcr is null and reviews > 0 and wordcount > 1000;
 update fanfics set wcr = 200000 where wcr is null and ( reviews = 0 or wordcount <=1000);
 update fanfics set wcr_adjusted = wcr where reviews/favourites < 2.5 and wcr_adjusted is null;
 update fanfics set wcr_adjusted = 200000 where reviews/favourites >= 2.5 and wcr_adjusted is null;
 update fanfics set reviewstofavourites = reviews*1.0/favourites where reviewstofavourites is null;
 update fanfics set age= abs(cast((strftime('%s',CURRENT_TIMESTAMP)-strftime('%s',published) ) AS real )/60/60/24) where daysrunning is null;
 update fanfics set daysrunning= abs(cast((strftime('%s',updated)-strftime('%s',published) ) AS real )/60/60/24) where daysrunning is null;
 CREATE INDEX if not exists  I_WCR ON fanfics (wcr ASC);
 CREATE INDEX if not exists  I_DAYSRUNNING ON fanfics (daysrunning ASC);
 CREATE INDEX if not exists  I_age ON fanfics (age ASC);
 CREATE INDEX if not exists  I_reviewstofavourites ON fanfics (reviewstofavourites ASC);
CREATE VIEW fandom_stats AS  select fandom, 
cast (strftime('%s',CURRENT_TIMESTAMP)-strftime('%s',min(published)) AS real )/60/60/24/365  as age,
min(published) as origin,
(select sum(wcr) from fanfics where wcr < 200000 and fandom = fs.fandom)/(select count(id) from fanfics where wcr < 200000  and fandom = fs.fandom) as averagewcr,
max(favourites) as maxfaves, min(wcr) as minwcr, count(id) as ficcount from fanfics fs group by fandom;
alter table fanfics add column fandom1 VARCHAR; 
alter table fanfics add column fandom2 VARCHAR;
CREATE INDEX if not exists  I_FANDOM1 ON fanfics (fandom1 ASC);
CREATE INDEX if not exists  I_FANDOM2 ON fanfics (fandom2 ASC);
alter table fanfics add column web_id integer default null;
alter table fanfics add column web_uid integer default null;
CREATE INDEX if not exists  I_FANFICS_WEB_ID ON fanfics (web_id ASC);
CREATE INDEX if not exists  I_FANFICS_WEB_UID ON fanfics (web_uid ASC);
alter table Recommenders add column website_type varchar default null;
alter table Recommenders add column total_fics integer default null;
update Recommenders set website_type = 'ffn' where website_type is null;
CREATE TABLE if not exists RecommendationTagStats (author_id INTEGER NOT NULL , fic_count INTEGER, match_count integer, match_ratio double, list_id integer,  PRIMARY KEY (list_id, author_id));
CREATE INDEX if not exists  I_REC_STATS_LIST_ID ON RecommendationTagStats (list_id ASC);
CREATE INDEX if not exists  I_REC_STATS_AUTHOR ON RecommendationTagStats (author_id ASC);
CREATE INDEX if not exists  I_REC_STATS_RATIO ON RecommendationTagStats (match_ratio ASC);
CREATE INDEX if not exists  I_REC_STATS_COUNT ON RecommendationTagStats (match_count ASC);
CREATE INDEX if not exists  I_REC_STATS_TOTAL_FICS ON RecommendationTagStats (fic_count ASC);
alter table Recommenders add column sumfaves integer default null;
update recommenders set sumfaves = (SELECT  Sum(favourites) as summation FROM fanfics where author = recommenders.name) where sumfaves is null;
update recommenders set sumfaves = 0 where sumfaves is null;
CREATE TABLE if not exists RecommendationFicStats (fic_id INTEGER NOT NULL , sumrecs INTEGER default 0, list_id integer,  PRIMARY KEY (fic_id, list_id));
CREATE INDEX if not exists  I_FIC_STATS_PK ON RecommendationFicStats (fic_id asc, list_id ASC);
CREATE INDEX if not exists  I_FIC_STATS_FIC ON RecommendationFicStats (fic_id asc);
CREATE INDEX if not exists  I_FIC_STATS_LIST_ID ON RecommendationFicStats (list_id asc);
CREATE TABLE if not exists FicTags (fic_id INTEGER NOT NULL, tag varchar,  PRIMARY KEY (fic_id asc, tag asc));
CREATE INDEX if not exists  I_FIC_TAGS_PK ON FicTags (fic_id asc, tag ASC);
CREATE INDEX if not exists  I_FIC_TAGS_TAG ON FicTags (tag ASC);
CREATE INDEX if not exists  I_FIC_TAGS_FIC ON FicTags (fic_id ASC);
create table if not exists RecommendationLists(id INTEGER PRIMARY KEY AUTOINCREMENT, name VARCHAR unique NOT NULL, minimum integer NOT NULL, pick_ratio double not null, always_pick_at integer not null, created DATETIME);
CREATE INDEX if not exists  I_RecommendationLists_ID ON RecommendationLists (id asc);
CREATE INDEX if not exists  I_RecommendationLists_NAME ON RecommendationLists (NAME asc);
CREATE INDEX if not exists  I_RecommendationLists_created ON RecommendationLists (created asc);
CREATE TABLE if not exists RecommendationListData(fic_id INTEGER NOT NULL, list_id integer,  PRIMARY KEY (fic_id asc, list_id asc));
CREATE INDEX if not exists  I_LIST_TAGS_PK ON RecommendationListData (list_id asc, fic_id asc);
CREATE INDEX if not exists  I_LISTDATA_ID ON RecommendationListData (list_id ASC);
CREATE INDEX if not exists  I_LISTDATA_FIC ON RecommendationListData (fic_id ASC);