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

CREATE TABLE if not exists PageCache (URL VARCHAR PRIMARY KEY  NOT NULL , GENERATION_DATE DATETIME,
 CONTENT BLOB, NEXT VARCHAR, FANDOM VARCHAR, CROSSOVER INTEGER, PREVIOUS VARCHAR, REFERENCED_FICS BLOB, PAGE_TYPE INTEGER);
 
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
 CREATE  INDEX if  not exists I_FIC_ID ON Recommendations (fic_id ASC);