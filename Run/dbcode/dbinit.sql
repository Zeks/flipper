--fanfics;
create table if not exists FANFICS (
AUTHOR VARCHAR,
TITLE VARCHAR,
SUMMARY VARCHAR,
GENRES VARCHAR,
CHARACTERS VARCHAR,
RATED VARCHAR,
PUBLISHED DATETIME,
UPDATED DATETIME,
WORDCOUNT INTEGER,
FAVOURITES INTEGER,
REVIEWS INTEGER,
CHAPTERS INTEGER DEFAULT 0,
COMPLETE INTEGER DEFAULT 0,
AT_CHAPTER INTEGER default 0, 
alive integer default 1,
wcr real,
fandom VARCHAR,
wcr_adjusted real,
reviewstofavourites real,
daysrunning integer default null,
age integer default null,
follows integer default 0,
ffn_id integer default -1,
ao3_id integer default -1,
sb_id integer default -1,
sv_id integer default -1,
author_id integer default -1,
author_web_id integer default -1,
date_added datetime default null,
date_deactivated datetime default null,
for_fill integer default 0,
ID INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL  UNIQUE);	
 
 CREATE VIEW vFanfics AS select id, author, title, summary, characters, genres, characters, rated, published, updated, reviews,
wordcount, favourites, chapters, complete, at_chapter, ffn_id, author_id,
wcr, wcr_adjusted, reviewstofavourites,daysrunning,age,alive, date_deactivated, follows
 from fanfics;

CREATE INDEX  if  not exists  main.I_FANFICS_IDENTITY ON FANFICS (AUTHOR ASC, TITLE ASC);
CREATE  INDEX if  not exists main.I_FANFICS_WORDCOUNT ON FANFICS (WORDCOUNT ASC);
CREATE  INDEX if  not exists main.I_FANFICS_TAGS ON FANFICS (TAGS ASC);
CREATE  INDEX if  not exists main.I_FANFICS_ID ON FANFICS (ID ASC);
CREATE  INDEX if  not exists main.I_FANFICS_GENRES ON FANFICS (GENRES ASC);
CREATE INDEX if not exists  I_WCR ON fanfics (wcr ASC);
CREATE INDEX if not exists  I_DAYSRUNNING ON fanfics (daysrunning ASC);
CREATE INDEX if not exists  I_age ON fanfics (age ASC);
CREATE INDEX if not exists  I_reviewstofavourites ON fanfics (reviewstofavourites ASC);
CREATE INDEX if not exists  I_FANFIC_UPDATED ON fanfics (UPDATED ASC);
CREATE INDEX if not exists  I_FANFIC_PUBLISHED ON fanfics (PUBLISHED ASC);
CREATE INDEX if not exists  I_AUTHOR_WEB_ID ON fanfics (author_web_id ASC);
CREATE INDEX if not exists  I_FANFICS_FFN_ID ON fanfics (ffn_id ASC);
CREATE INDEX if not exists  I_ALIVE ON fanfics (alive ASC);
CREATE INDEX if not exists  I_DATE_ADDED ON fanfics (date_added ASC);
CREATE INDEX if not exists  I_FOR_FILL ON fanfics (for_fill ASC);

-- fanfics sequence;
 CREATE TABLE if not exists sqlite_sequence(name varchar, seq integer);
 INSERT INTO sqlite_sequence(name, seq) SELECT 'fanfics', 0 WHERE NOT EXISTS(SELECT 1 FROM sqlite_sequence WHERE name = 'fanfics');
 update sqlite_sequence set seq = (select max(id) from fanfics) where name = 'fanfics';
 
create table if not exists fandomindex (id integer, name VARCHAR NOT NULL, tracked integer default 0, primary key(id, name));
alter table fandomindex add column updated datetime;
CREATE INDEX if not exists I_FANDOMINDEX_PK ON fandomindex (id ASC, name asc);
CREATE INDEX if not exists I_FANDOMINDEX_UPDATED ON fandomindex (updated ASC);
CREATE INDEX if not exists I_FANDOMINDEX_ID ON fandomindex (id ASC);
CREATE INDEX if not exists I_FANDOMINDEX_NAME ON fandomindex (name ASC);

create table if not exists fandomurls (global_id integer, url VARCHAR NOT NULL, website varchar not null, custom VARCHAR, primary key(global_id, url));
CREATE INDEX if not exists I_FURL_ID ON fandomurls (global_id ASC);
CREATE INDEX if not exists I_FURL_URL ON fandomurls (url ASC);
CREATE INDEX if not exists I_FURL_CUSTOM ON fandomurls (custom ASC);
CREATE INDEX if not exists I_FURL_WEBSITE ON fandomurls (website ASC);

create table if not exists fandomsources (global_id integer,
 website VARCHAR NOT NULL,
 last_update datetime,
 last_parse_limit datetime,
 second_last_parse_limit datetime, 
 date_of_first_fic datetime,
 date_of_creation datetime,
 fandom_multiplier integer default 1,
 fic_count integer default 0,
 average_faves_top_3 real default 0
 );
CREATE INDEX if not exists I_FSOURCE_ID ON fandomsources (global_id ASC);
CREATE INDEX if not exists I_FSOURCE_WEBSITE ON fandomsources (website ASC);
CREATE INDEX if not exists I_FSOURCE_LASTUPDATE ON fandomsources (last_update ASC);
CREATE INDEX if not exists I_FSOURCE_AVGF3 ON fandomsources (average_faves_top_3 ASC);

 
 update fandoms set id = rowid where id is null;
 
 CREATE TABLE if not exists recent_fandoms(fandom varchar, seq_num integer);
 INSERT INTO recent_fandoms(fandom, seq_num) SELECT 'base', 0 WHERE NOT EXISTS(SELECT 1 FROM fandom WHERE fandom = 'base');
 
 CREATE TABLE if not exists Recommenders (id INTEGER PRIMARY KEY  AUTOINCREMENT  NOT NULL  UNIQUE , name VARCHAR NOT NULL , url VARCHAR NOT NULL , page_data BLOB, page_updated DATETIME, author_updated DATETIME);
 
 alter table Recommenders add column wave integer default 0;
 alter table Recommenders add column favourites integer default -1;
  alter table Recommenders add column in_favourites integer default 0;
 alter table Recommenders add column fics integer default -1;
 alter table Recommenders add column ffn_id integer default -1;
 alter table Recommenders add column ao3_id integer default -1;
 alter table Recommenders add column sb_id integer default -1;
 alter table Recommenders add column sv_id integer default -1;
 
 alter table Recommenders add column page_creation_date datetime;
 alter table Recommenders add column info_updated datetime;
 alter table Recommenders add column last_published_fic_date datetime;
 alter table Recommenders add column first_published_fic_date datetime;
 alter table Recommenders add column latest_favourited_fic_date datetime;
 alter table Recommenders add column earliest_favourited_fic_date datetime;
 
 -- holds various stats for the author's page;
 -- to be used for fics clustering;
 CREATE TABLE if not exists AuthorPageStatistics (author_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
  
 alter table AuthorPageStatistics add column favourites integer default 0;                  -- how much stuff did the author favourite;
 alter table AuthorPageStatistics add column favourites_type integer default -1;            -- tiny(<50)/medium(50-500)/large(500-2000)/bullshit(2k+);

 alter table AuthorPageStatistics add column favourite_fandoms_diversity real;              -- how uniform are his favourites relative to most popular fandom;
 alter table AuthorPageStatistics add column explorer_factor real;                          -- deals with how likely is the author to explore otherwise unpopular fics;
 alter table AuthorPageStatistics add column crossover_factor real;                         -- how willing is he to read crossovers;
 alter table AuthorPageStatistics add column unfinished_factor real;                        -- how willing is he to read stuff that isn't finished;
 alter table AuthorPageStatistics add column just_started_factor real;                      -- how willing is he to read stuff that has just started and is still very small (but the fic is still active);
 alter table AuthorPageStatistics add column esrb_uniformity_factor real;                   -- how faithfully the author sticks to the same ESRB when favouriting. Only  M/everything else taken into account;
 alter table AuthorPageStatistics add column esrb_type integer -1;                          -- agnostic/kiddy/mature;
 alter table AuthorPageStatistics add column genre_uniformity_factor real;                  -- how likely is the author to stick to a single genre;
 alter table AuthorPageStatistics add column prevalent_mood integer default -1;             -- sad/neutral/positive as categorized by genres;
 alter table AuthorPageStatistics add column prevalent_genre varchar default null;
 alter table AuthorPageStatistics add column average_favourited_length integer default -1;  -- excluding the biggest outliers if there are not enough of them;
 alter table AuthorPageStatistics add column most_favourited_size integer default -1;       -- small/medium/large/huge;
 
 alter table AuthorPageStatistics add column bio_wordcount integer default -1;              -- how big is author's bio section;
 alter table AuthorPageStatistics add column own_wordcount integer default -1;              -- author's own wordcount;
 alter table AuthorPageStatistics add column own_favourites integer default -1;             -- author's own favourites on fics (pretty much useless);
 alter table AuthorPageStatistics add column own_finished_ratio real;                       -- ratio of finished fics
 alter table AuthorPageStatistics add column most_written_size integer default -1;          -- small/medium/huge;
  
 alter table AuthorPageStatistics add column size_tiny real;
 alter table AuthorPageStatistics add column size_medium real;
 alter table AuthorPageStatistics add column size_large real;
 alter table AuthorPageStatistics add column size_huge real;
 
 alter table AuthorPageStatistics add column esrb_kiddy real;
 alter table AuthorPageStatistics add column esrb_mature real;
 
 alter table AuthorPageStatistics add column fandom_size_dominant real;
 alter table AuthorPageStatistics add column fandom_size_minor real;
 
 alter table AuthorPageStatistics add column mood_sad real;
 alter table AuthorPageStatistics add column mood_neutral real;
 alter table AuthorPageStatistics add column mood_happy real;
 
 alter table AuthorPageStatistics add column explorer_popular real;
 alter table AuthorPageStatistics add column explorer_minor real;

 alter table AuthorPageStatistics add column unfinished_yes real;
 alter table AuthorPageStatistics add column unfinished_no real;
 
 alter table AuthorPageStatistics add column juststarted_no real;
 alter table AuthorPageStatistics add column juststarted_yes real;
 
  
 -- need genre coefficients in a separate table probably;
 -- need cluster relations in separate table;
 
 CREATE  INDEX if not exists I_RECOMMENDERS_NAME ON Recommenders (name ASC);
 CREATE  INDEX if not exists I_RECOMMENDERS_URL ON Recommenders (URL ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_WAVE ON Recommenders (wave ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_FAVOURITES ON Recommenders (favourites ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_FFN_ID ON Recommenders (ffn_id ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_AO3_ID ON Recommenders (ao3_id ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_SB_ID ON Recommenders (sb_id ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_SV_ID ON Recommenders (sv_id ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_IN_FAVOURITES ON Recommenders (in_favourites ASC);
 
 CREATE TABLE if not exists Recommendations (recommender_id INTEGER NOT NULL , fic_id INTEGER NOT NULL , PRIMARY KEY (recommender_id, fic_id));
 CREATE INDEX if not exists  I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC_TAG ON Recommendations (tag ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_REC_FIC ON Recommendations (recommender_id ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_TAG ON Recommendations (tag ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC ON Recommendations (fic_id ASC);
 CREATE  INDEX if not exists I_FIC_ID ON Recommendations (fic_id ASC);
 
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
create table if not exists tags (tag VARCHAR unique NOT NULL, id integer);


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
CREATE INDEX if not exists  I_FIC_FANDOMS_FIC ON FicFandoms (fic_id ASC);


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
 
 -- linked authors table;
 CREATE TABLE if not exists LinkedAuthors (recommender_id INTEGER NOT NULL, url VARCHAR);
 alter table LinkedAuthors add column ffn_id integer default -1;
 alter table LinkedAuthors add column ao3_id integer default -1;
 alter table LinkedAuthors add column sb_id integer default -1;
 alter table LinkedAuthors add column sv_id integer default -1;
 CREATE INDEX if not exists I_LINKED_AUTHORS_PK ON LinkedAuthors (recommender_id ASC);

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

-- data for fandoms present in the list;
create table if not exists RecommendationListsFandoms(list_id INTEGER default 0, fandom_id VARCHAR default 0, is_original_fandom integer deault 0, fic_count integer, PRIMARY KEY (list_id, fandom_id))
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