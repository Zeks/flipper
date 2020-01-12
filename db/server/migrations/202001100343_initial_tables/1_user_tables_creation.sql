

create table if not exists UserData.Users(user_id uuid unique);

alter table UserData.Users add column id serial unique;
alter table UserData.Users add primary key (user_id, id);
alter table UserData.Users add column name varchar;

CREATE INDEX if not exists I_Users_PK ON UserData.Users (user_id ASC, id ASC);  
CREATE INDEX if not exists I_Users_NAME ON UserData.Users (name ASC, id ASC);  


-- 'user_ffn_id';
-- also needs password storage;
-- also probably tag list delimited by ,;
CREATE TABLE if not exists UserData.UserSettings(user_id integer,
name varchar, 
value varchar);

CREATE INDEX if not exists I_USERSETTINGS_UID ON UserData.UserSettings (user_id ASC);  


CREATE TABLE if not exists UserData.ignored_fandoms(user_id integer,
fandom_id integer,
including_crossovers boolean default false);
alter table UserData.ignored_fandoms add primary key (user_id, fandom_id);

CREATE INDEX if not exists I_IGNOREDFANDOMS_UID ON UserData.ignored_fandoms (user_id ASC);  

-- fictag table, used to store data as a backup and synchro mechanism;
CREATE TABLE if not exists UserData.FicTags (
user_id integer not null,
fic_id integer not null,
tag varchar not null,
added TIMESTAMP );

CREATE INDEX if not exists I_FICTAGS_UID ON       UserData.FicTags (user_id ASC); 
CREATE INDEX if not exists I_FICTAGS_USER_TAGS ON UserData.FicTags (user_id ASC, tag asc); 
CREATE INDEX if not exists I_FICTAGS_ADDED ON     UserData.FicTags (user_id ASC, added asc); 


-- ficscore table;
CREATE TABLE if not exists UserData.FicScores (
user_id integer not null,
fic_id integer not null,
score integer not null,
updated TIMESTAMP );
CREATE INDEX if not exists I_FICSCORES_UID          ON UserData.FicScores (user_id ASC); 
CREATE INDEX if not exists I_FICSCORES_USER_SCORES  ON UserData.FicScores (user_id ASC, score asc); 
CREATE INDEX if not exists I_FICSCORES_USER_UPDATED ON UserData.FicScores (user_id ASC, updated asc); 

alter table UserData.FicScores add primary key (user_id, fic_id);


CREATE TABLE if not exists UserData.FicSnoozes (
user_id integer not null,
fic_id integer not null,
snooze_added timestamp not null,
snoozed_at_chapter integer,
snoozed_till_chapter integer,
snoozed_until_finished integer default 1,
snooze_expired boolean default false);

CREATE INDEX if not exists I_FICSNOOZES_UID         ON UserData.FicSnoozes (user_id ASC);
CREATE INDEX if not exists I_FICSNOOZES_UID_FIC_ID  ON UserData.FicSnoozes (user_id ASC, fic_id asc);
CREATE INDEX if not exists I_FICSNOOZES_UID_EXPIRED ON UserData.FicSnoozes (user_id ASC, snooze_expired ASC);

alter table UserData.FicSnoozes add primary key (user_id, fic_id);

-- a list of all recommendaton lists with their names and statistics;
create table if not exists UserData.RecommendationLists(user_id integer not null);

alter table UserData.RecommendationLists add column list_id serial unique;
alter table UserData.RecommendationLists add column name varchar not null;
alter table UserData.RecommendationLists add column minimum integer NOT NULL default 1;
alter table UserData.RecommendationLists add column pick_ratio real not null;
alter table UserData.RecommendationLists add column always_pick_at integer not null default 9999;
alter table UserData.RecommendationLists add column fic_count integer default 0;
alter table UserData.RecommendationLists add column created TIMESTAMP;
alter table UserData.RecommendationLists add column use_weighting boolean default true;
alter table UserData.RecommendationLists add column use_mood_adjustment boolean default true;

alter table UserData.RecommendationLists add primary key (user_id, list_id);

CREATE INDEX if not exists  I_RecommendationLists_UID_LIST_ID ON UserData.RecommendationLists (user_id asc, list_id asc);
CREATE INDEX if not exists  I_RecommendationLists_UID_NAME    ON UserData.RecommendationLists (user_id asc, NAME asc);


-- data for fandoms present in the list;
create table if not exists UserData.RecommendationListsFandoms(list_id integer unique not null primary key);
alter table UserData.RecommendationListsFandoms add column fandom_id integer unique default null;
alter table UserData.RecommendationListsFandoms add column is_original_fandom boolean default false;
alter table UserData.RecommendationListsFandoms add column fic_count integer default 0;

CREATE INDEX if not exists I_RecommendationListsFandoms_LIST_ID                     ON UserData.RecommendationListsFandoms (list_id asc);
CREATE INDEX if not exists I_RecommendationListsFandoms_LIST_ID_FANDOM_ID           ON UserData.RecommendationListsFandoms (list_id asc, fandom_id asc);
CREATE INDEX if not exists I_RecommendationListsFandoms_LIST_ID_IS_ORIGINAL_FANDOM  ON UserData.RecommendationListsFandoms (list_id asc, is_original_fandom asc);
CREATE INDEX if not exists I_RecommendationListsFandoms_fandom_id                   ON UserData.RecommendationListsFandoms (fandom_id asc);
CREATE INDEX if not exists I_RecommendationListsFandoms_fic_count                   ON UserData.RecommendationListsFandoms (fic_count asc);



-- data for recommendation lists;
CREATE TABLE if not exists UserData.TemporaryRecommendationListData(user_id integer not null);

alter table UserData.TemporaryRecommendationListData add column fic_id integer not null;
alter table UserData.TemporaryRecommendationListData add column match_count integer not null;
alter table UserData.TemporaryRecommendationListData add column score integer not null;
alter table UserData.TemporaryRecommendationListData add column is_origin boolean default false;

alter table UserData.TemporaryRecommendationListData add primary key (user_id, fic_id);

CREATE INDEX if not exists I_TemporaryRecommendationListData_UID_LIST_ID ON UserData.TemporaryRecommendationListData (user_id asc, fic_id asc);
CREATE INDEX if not exists I_TemporaryRecommendationListData_UID_SCORE ON UserData.TemporaryRecommendationListData (user_id asc, score asc);
CREATE INDEX if not exists I_TemporaryRecommendationListData_UID_MATCH_COUNT ON UserData.TemporaryRecommendationListData (user_id asc, match_count asc);
CREATE INDEX if not exists I_TemporaryRecommendationListData_UID_IS_ORIGIN ON UserData.TemporaryRecommendationListData (user_id asc,is_origin asc);

create table if not exists UserData.FicNotes (user_id integer not null);

alter table UserData.FicNotes add column fic_id integer not null;
alter table UserData.FicNotes add column note_content varchar;

alter table UserData.FicNotes add primary key (user_id, fic_id);

CREATE INDEX if not exists I_FICNOTES_UID_FIC_ID ON UserData.FicNotes (user_id asc, fic_id ASC);


create table if not exists UserData.FicReadingTracker (user_id integer not null);

alter table UserData.FicReadingTracker add column fic_id integer not null;
alter table UserData.FicReadingTracker add column at_chapter integer not null default 1;

alter table UserData.FicReadingTracker add primary key (user_id, fic_id);

CREATE INDEX if not exists I_FICREADINGTRACKER_UID_FIC_ID ON UserData.FicReadingTracker (user_id asc, fic_id ASC);
