

create table if not exists UT_Users(user_id uuid unique);

alter table UT_Users add column id serial unique;
alter table UT_Users add primary key (uuid, id);

CREATE INDEX if not exists I_UT_Users_PK ON UT_Users (user_id ASC, id ASC);  


-- 'user_ffn_id';
-- also needs password storage;
-- also probably tag list delimited by ,;
CREATE TABLE if not exists UT_UserSettings(user_id integer,
name varchar, 
value varchar);

CREATE INDEX if not exists I_UT_USERSETTINGS_UID ON UT_UserSettings (user_id ASC);  


CREATE TABLE if not exists UT_ignored_fandoms(user_id integer,
fandom_id integer,
including_crossovers boolean default false);
alter table UT_ignored_fandoms add primary key (user_id, fandom_id);

CREATE INDEX if not exists I_UT_IGNOREDFANDOMS_UID ON UT_ignored_fandoms (user_id ASC);  

-- fictag table, used to store data as a backup and synchro mechanism;
CREATE TABLE if not exists UT_FicTags (
user_id integer not null,
fic_id integer not null,
tag varchar not null,
added TIMESTAMP );

CREATE INDEX if not exists I_UT_FICTAGS_UID ON       UT_FicTags (user_id ASC); 
CREATE INDEX if not exists I_UT_FICTAGS_USER_TAGS ON UT_FicTags (user_id ASC, tag asc); 
CREATE INDEX if not exists I_UT_FICTAGS_ADDED ON     UT_FicTags (user_id ASC, added asc); 


-- ficscore table;
CREATE TABLE if not exists UT_FicScores (
user_id integer not null,
fic_id integer not null,
score integer not null,
updated TIMESTAMP );
CREATE INDEX if not exists I_UT_FICSCORES_UID          ON UT_FicScores (user_id ASC); 
CREATE INDEX if not exists I_UT_FICSCORES_USER_SCORES  ON UT_FicScores (user_id ASC, score asc); 
CREATE INDEX if not exists I_UT_FICSCORES_USER_UPDATED ON UT_FicScores (user_id ASC, updated asc); 

alter table UT_FicScores add primary key (user_id, fic_id);


CREATE TABLE if not exists UT_FicSnoozes (
user_id integer not null,
fic_id integer not null,
snooze_added timestamp not null,
snoozed_at_chapter integer,
snoozed_till_chapter integer,
snoozed_until_finished integer default 1,
snooze_expired boolean default false);

CREATE INDEX if not exists I_UT_FICSNOOZES_UID         ON UT_FicSnoozes (user_id ASC);
CREATE INDEX if not exists I_UT_FICSNOOZES_UID_FIC_ID  ON UT_FicSnoozes (user_id ASC, fic_id asc);
CREATE INDEX if not exists I_UT_FICSNOOZES_UID_EXPIRED ON UT_FicSnoozes (user_id ASC, expired ASC);

alter table UT_FicSnoozes add primary key (user_id, fic_id);

-- a list of all recommendaton lists with their names and statistics;
create table if not exists UT_RecommendationLists(user_id integer not null);

alter table UT_RecommendationLists add column list_id serial unique;
alter table UT_RecommendationLists add column name varchar not null;
alter table UT_RecommendationLists add column minimum integer NOT NULL default 1;
alter table UT_RecommendationLists add column pick_ratio real not null;
alter table UT_RecommendationLists add column always_pick_at integer not null default 9999;
alter table UT_RecommendationLists add column fic_count integer default 0;
alter table UT_RecommendationLists add column created TIMESTAMP;
alter table UT_RecommendationLists add column use_weighting boolean default true;
alter table UT_RecommendationLists add column use_mood_adjustment boolean default true;

alter table UT_RecommendationLists add primary key (user_id, list_id);

CREATE INDEX if not exists  I_UT_RecommendationLists_UID_LIST_ID ON UT_RecommendationLists (user_id asc, list_id asc);
CREATE INDEX if not exists  I_UT_RecommendationLists_UID_NAME    ON UT_RecommendationLists (user_id asc, NAME asc);


-- data for fandoms present in the list;
create table if not exists UT_RecommendationListsFandoms(list_id integer unique not null primary key);
alter table UT_RecommendationListsFandoms add column fandom_id integer unique default null;
alter table UT_RecommendationListsFandoms add column is_original_fandom boolean default false;
alter table UT_RecommendationListsFandoms add column fic_count integer default 0;

CREATE INDEX if not exists I_UT_RecommendationListsFandoms_LIST_ID                     ON UT_RecommendationListsFandoms (list_id asc);
CREATE INDEX if not exists I_UT_RecommendationListsFandoms_LIST_ID_FANDOM_ID           ON UT_RecommendationListsFandoms (list_id asc, fandom_id asc);
CREATE INDEX if not exists I_UT_RecommendationListsFandoms_LIST_ID_IS_ORIGINAL_FANDOM  ON UT_RecommendationListsFandoms (list_id asc, is_original_fandom asc);
CREATE INDEX if not exists I_UT_RecommendationListsFandoms_fandom_id                   ON UT_RecommendationListsFandoms (fandom_id asc);
CREATE INDEX if not exists I_UT_RecommendationListsFandoms_fic_count                   ON UT_RecommendationListsFandoms (fic_count asc);



-- data for recommendation lists;
CREATE TABLE if not exists UT_TemporaryRecommendationListData(user_id integer not null);

alter table UT_TemporaryRecommendationListData add column fic_id integer not null;
alter table UT_TemporaryRecommendationListData add column match_count integer not null;
alter table UT_TemporaryRecommendationListData add column score integer not null;
alter table UT_TemporaryRecommendationListData add column is_origin boolean false;

alter table UT_TemporaryRecommendationListData add primary key (user_id, fic_id);

CREATE INDEX if not exists I_UT_TemporaryRecommendationListData_UID_LIST_ID ON UT_TemporaryRecommendationListData (user_id asc, fic_id asc);
CREATE INDEX if not exists I_UT_TemporaryRecommendationListData_UID_SCORE ON UT_TemporaryRecommendationListData (user_id asc, score asc);
CREATE INDEX if not exists I_UT_TemporaryRecommendationListData_UID_MATCH_COUNT ON UT_TemporaryRecommendationListData (user_id asc, match_count asc);
CREATE INDEX if not exists I_UT_TemporaryRecommendationListData_UID_IS_ORIGIN ON UT_TemporaryRecommendationListData (user_id asc,is_origin asc);

create table if not exists UT_FicNotes (user_id integer not null);

alter table UT_FicNotes add column fic_id integer not null;
alter table UT_FicNotes add column note_content varchar;

alter table UT_FicNotes add primary key (user_id, fic_id);

CREATE INDEX if not exists I_UT_FICNOTES_UID_FIC_ID ON UT_FicNotes (user_id asc, fic_id ASC);


create table if not exists UT_FicReadingTracker (user_id integer not null);

alter table UT_FicReadingTracker add column fic_id integer not null;
alter table UT_FicReadingTracker add column at_chapter integer not null default 1;

alter table UT_FicReadingTracker add primary key (user_id, fic_id);

CREATE INDEX if not exists I_UT_FICREADINGTRACKER_UID_FIC_ID ON UT_FicReadingTracker (user_id asc, fic_id ASC);
