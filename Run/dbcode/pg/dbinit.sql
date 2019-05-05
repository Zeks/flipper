--fanfics;
create table if not exists FANFICS (
ID serial PRIMARY KEY unique);	

alter table fanfics add column wcr real;
alter table fanfics add column wcr_adjusted real;
alter table fanfics add column reviewstofavourites real;

alter table fanfics add column AUTHOR VARCHAR;
alter table fanfics add column TITLE VARCHAR;
alter table fanfics add column SUMMARY VARCHAR;
alter table fanfics add column GENRES VARCHAR;
alter table fanfics add column CHARACTERS VARCHAR;
alter table fanfics add column RATED VARCHAR;
alter table fanfics add column fandom VARCHAR;

alter table fanfics add column PUBLISHED timestamp;
alter table fanfics add column UPDATED timestamp;
alter table fanfics add column date_added timestamp;
alter table fanfics add column date_deactivated timestamp;

alter table fanfics add column FAVOURITES integer default 0;
alter table fanfics add column REVIEWS integer default 0;
alter table fanfics add column CHAPTERS integer default 0;
alter table fanfics add column COMPLETE boolean default false;
alter table fanfics add column AT_CHAPTER integer default 0;
alter table fanfics add column alive boolean default false;
alter table fanfics add column daysrunning integer default 0;
alter table fanfics add column age integer default 0;
alter table fanfics add column follows integer default null;
alter table fanfics add column ffn_id integer default null;
alter table fanfics add column ao3_id integer default null;
alter table fanfics add column sb_id integer default null;
alter table fanfics add column sv_id integer default null;
alter table fanfics add column author_id integer default null;
alter table fanfics add column author_web_id integer default null;
alter table fanfics add column for_fill boolean default null;
alter table fanfics add column wordcount integer default null;
alter table fanfics add column hidden boolean default false;
alter table fanfics add column lastupdate TIMESTAMP;
alter table fanfics add column fandom1 integer default null;
alter table fanfics add column fandom2 integer default null;
alter table fanfics add column keywords_yes boolean default false;
alter table fanfics add column keywords_no boolean default false;
alter table fanfics add column keywords_result boolean default false;
alter table fanfics add column filter_pass_1 boolean default false;
alter table fanfics add column filter_pass_2 boolean default false;

alter table fanfics add column kept_genres VARCHAR;
alter table fanfics add column true_genre1 VARCHAR;
alter table fanfics add column true_genre2 VARCHAR;
alter table fanfics add column true_genre3 VARCHAR;
alter table fanfics add column true_genre1_percent real;
alter table fanfics add column true_genre2_percent real;
alter table fanfics add column true_genre3_percent real;
alter table fanfics add column max_genre_percent real;
alter table fanfics add column queued_for_action boolean default false;
alter table fanfics add column is_english boolean default true;

CREATE INDEX if not exists  I_FANFICS_ID ON FANFICS (ID ASC);
CREATE INDEX if not exists  I_FANFICS_FANDOM_1 ON fanfics (fandom1 ASC);
CREATE INDEX if not exists  I_FANFICS_FANDOM_2 ON fanfics (fandom2 ASC);
CREATE INDEX if not exists  I_FANFICS_KW_YES ON fanfics (keywords_yes ASC);
CREATE INDEX if not exists  I_FANFICS_KW_NO ON fanfics (keywords_no ASC);
CREATE INDEX if not exists  I_FANFICS_KW_RESULT ON fanfics (keywords_result ASC);
CREATE INDEX if not exists  I_FANFICS_FIRST ON fanfics (filter_pass_1 ASC);
CREATE INDEX if not exists  I_FANFICS_SECOND  ON fanfics (filter_pass_2 ASC);
CREATE INDEX if not exists  I_FANFICS_QFA  ON fanfics (queued_for_action ASC);
CREATE INDEX if not exists  I_FANFICS_AUTHOR_ID  ON fanfics (author_id ASC);
CREATE INDEX if not exists  I_FANFICS_IS_ENGLISH  ON fanfics (is_english ASC);
CREATE INDEX if not exists  I_FANFICS_RATED  ON fanfics (rated ASC);

CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_TG1 ON FANFICS (true_genre1 asc);
CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_TG2 ON FANFICS (true_genre2 asc);
CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_TG3 ON FANFICS (true_genre3 asc);
 
CREATE INDEX if not exists  I_FANFICS_GENRES ON FANFICS (GENRES ASC);
CREATE INDEX if not exists  I_FANFICS_WCR ON fanfics (wcr ASC);
CREATE INDEX if not exists  I_FANFICS_DAYSRUNNING ON fanfics (daysrunning ASC);
CREATE INDEX if not exists  I_FANFICS_age ON fanfics (age ASC);
CREATE INDEX if not exists  I_FANFICS_favourites ON fanfics (favourites ASC);
CREATE INDEX if not exists  I_FANFICS_reviews ON fanfics (reviews ASC);
CREATE INDEX if not exists  I_FANFICS_complete ON fanfics (complete ASC);
CREATE INDEX if not exists  I_FANFICS_reviewstofavourites ON fanfics (reviewstofavourites ASC);
CREATE INDEX if not exists  I_FANFICS_UPDATED ON fanfics (UPDATED ASC);
CREATE INDEX if not exists  I_FANFICS_PUBLISHED ON fanfics (PUBLISHED ASC);
CREATE INDEX if not exists  I_FANFICS_FFN_ID ON fanfics (ffn_id ASC);
CREATE INDEX if not exists  I_FANFICS_ALIVE ON fanfics (alive ASC);
CREATE INDEX if not exists  I_FANFICS_DATE_ADDED ON fanfics (date_added ASC);
CREATE INDEX if not exists  I_FANFICS_FOR_FILL ON fanfics (for_fill ASC);
CREATE INDEX if not exists  I_FANFICS_HIDDEN on fanfics (hidden ASC);
CREATE INDEX if not exists  I_FANFICS_SUMMARY on fanfics (summary ASC);
CREATE INDEX if not exists  I_FANFICS_TITLE on fanfics (title ASC);

CREATE INDEX if not exists  I_FANFICS_IDENTITY ON FANFICS (AUTHOR ASC, TITLE ASC);
CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_FP1 ON FANFICS (WORDCOUNT ASC , filter_pass_1 desc);
CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_FP2 ON FANFICS (WORDCOUNT ASC , filter_pass_2 desc);
CREATE INDEX if not exists  I_FANFICS_FAVOURITES_FP1 ON FANFICS (favourites ASC , filter_pass_1 desc);
CREATE INDEX if not exists  I_FANFICS_FAVOURITES_FP2 ON FANFICS (favourites ASC , filter_pass_2 desc);



-- slash ffn db;
 create table if not exists slash_data_ffn (ffn_id integer primary key);
 alter table slash_data_ffn add column keywords_yes boolean default false;
 alter table slash_data_ffn add column keywords_no boolean default false;
 alter table slash_data_ffn add column keywords_result boolean default false;
 alter table slash_data_ffn add column filter_pass_1 boolean default false;
 alter table slash_data_ffn add column filter_pass_2 boolean default false;
 
CREATE INDEX if not exists I_SDFFN_KEY ON slash_data_ffn (ffn_id ASC);
CREATE INDEX if not exists I_SDFFN_KW_YES ON slash_data_ffn (keywords_yes ASC);
CREATE INDEX if not exists I_SDFFN_KW_NO ON slash_data_ffn (keywords_no ASC);
CREATE INDEX if not exists I_SDFFN_KW_RESULT ON slash_data_ffn (keywords_result ASC);
CREATE INDEX if not exists I_SDFFN_FIRST ON slash_data_ffn (filter_pass_1 ASC);
CREATE INDEX if not exists I_SDFFN_SECOND  ON slash_data_ffn (filter_pass_2 ASC);
 
 
create table if not exists FIC_GENRE_ITERATIONS ( fic_id integer primary key );
alter table FIC_GENRE_ITERATIONS add column true_genre1 VARCHAR;
alter table FIC_GENRE_ITERATIONS add column true_genre2 VARCHAR;
alter table FIC_GENRE_ITERATIONS add column true_genre3 VARCHAR;
alter table FIC_GENRE_ITERATIONS add column kept_genres VARCHAR;
alter table FIC_GENRE_ITERATIONS add column true_genre1_percent real;
alter table FIC_GENRE_ITERATIONS add column true_genre2_percent real;
alter table FIC_GENRE_ITERATIONS add column true_genre3_percent real;
alter table FIC_GENRE_ITERATIONS add column max_genre_percent real;
CREATE INDEX if not exists  I_FGI_PK  ON FIC_GENRE_ITERATIONS (fic_id ASC);
 
-- algo passes;
 create table if not exists algopasses (fic_id integer primary key);
 alter table algopasses add column keywords_yes boolean default false;
 alter table algopasses add column keywords_no boolean default false;
 alter table algopasses add column keywords_pass_result boolean default false;
 alter table algopasses add column pass_1 boolean default false;
 alter table algopasses add column pass_2 boolean default false;
 alter table algopasses add column pass_3 boolean default false;
 alter table algopasses add column pass_4 boolean default false;
 alter table algopasses add column pass_5 boolean default false;
 --alter table slashpasses add column pass_6 integer default 0;
 --alter table slashpasses add column pass_7 integer default 0;
 --alter table slashpasses add column pass_8 integer default 0;
 --alter table slashpasses add column pass_9 integer default 0;
 --alter table slashpasses add column pass_10 integer default 0;
-- alter table slashpasses add column pass_XD integer default 0;
 
 CREATE INDEX if not exists I_SP_FID ON algopasses (fic_id ASC);
 CREATE INDEX if not exists I_SP_YES_KEY ON algopasses (keywords_yes ASC);
 CREATE INDEX if not exists I_SP_NO_KEY ON algopasses (keywords_no ASC);
 CREATE INDEX if not exists I_SP_KEY_PASS_RESULT ON algopasses (keywords_pass_result ASC);
 CREATE INDEX if not exists I_SP_PASS_1 ON algopasses (pass_1 ASC);
 CREATE INDEX if not exists I_SP_PASS_2 ON algopasses (pass_2 ASC);
 CREATE INDEX if not exists I_SP_PASS_3 ON algopasses (pass_3 ASC);
 CREATE INDEX if not exists I_SP_PASS_4 ON algopasses (pass_4 ASC);
 CREATE INDEX if not exists I_SP_PASS_5 ON algopasses (pass_5 ASC);
 
create table if not exists fandomindex (id serial unique);
alter table fandomindex add column updated timestamp;
alter table fandomindex add column name varchar;
alter table fandomindex add primary key (id, name);


CREATE INDEX if not exists I_FANDOMINDEX_PK ON fandomindex (id ASC, name asc);
CREATE INDEX if not exists I_FANDOMINDEX_UPDATED ON fandomindex (updated ASC);
CREATE INDEX if not exists I_FANDOMINDEX_ID ON fandomindex (id ASC);
CREATE INDEX if not exists I_FANDOMINDEX_NAME ON fandomindex (name ASC);

create table if not exists fandomurls (global_id integer);
alter table fandomurls add column url varchar;
alter table fandomurls add column website varchar not null default 'ffn';
alter table fandomurls add column custom varchar;
alter table fandomurls add primary key (global_id, url);
 
CREATE INDEX if not exists I_FURL_ID ON fandomurls (global_id ASC);
CREATE INDEX if not exists I_FURL_URL ON fandomurls (url ASC);
CREATE INDEX if not exists I_FURL_CUSTOM ON fandomurls (custom ASC);
CREATE INDEX if not exists I_FURL_WEBSITE ON fandomurls (website ASC);

create table if not exists fandomsources (global_id integer);
 
 alter table fandomsources add column website varchar not null default 'ffn';
 alter table fandomsources add column last_update TIMESTAMP;
 alter table fandomsources add column last_parse_limit TIMESTAMP;
 alter table fandomsources add column second_last_parse_limit TIMESTAMP;
 alter table fandomsources add column date_of_first_fic TIMESTAMP;
 alter table fandomsources add column date_of_creation TIMESTAMP;
 alter table fandomsources add column fandom_multiplier integer default 1;
 alter table fandomsources add column fic_count integer default 0;
 alter table fandomsources add column average_faves_top_3 real default 0;
 
CREATE INDEX if not exists I_FSOURCE_ID ON fandomsources (global_id ASC);
CREATE INDEX if not exists I_FSOURCE_WEBSITE ON fandomsources (website ASC);
CREATE INDEX if not exists I_FSOURCE_LASTUPDATE ON fandomsources (last_update ASC);
CREATE INDEX if not exists I_FSOURCE_AVGF3 ON fandomsources (average_faves_top_3 ASC);

 CREATE TABLE if not exists Recommenders (id serial PRIMARY KEY UNIQUE);
 
 alter table Recommenders add column name varchar not null;
 alter table Recommenders add column url varchar not null;
 alter table Recommenders add column page_data bytea;
 alter table Recommenders add column page_updated TIMESTAMP;
 alter table Recommenders add column author_updated TIMESTAMP;
  
 alter table Recommenders add column favourites integer default null;
 alter table Recommenders add column fics integer default null;
 alter table Recommenders add column ffn_id integer default null;
 alter table Recommenders add column ao3_id integer default null;
 alter table Recommenders add column sb_id integer default null;
 alter table Recommenders add column sv_id integer default null;
 
 alter table Recommenders add column page_creation_date TIMESTAMP;
 alter table Recommenders add column info_updated TIMESTAMP;
 alter table Recommenders add column info_wordcount integer default null;
 alter table Recommenders add column last_published_fic_date TIMESTAMP;
 alter table Recommenders add column first_published_fic_date TIMESTAMP;
 alter table Recommenders add column last_favourites_update TIMESTAMP;
 alter table Recommenders add column last_favourites_checked TIMESTAMP;
 alter table Recommenders add column own_wordcount integer default null;              -- author's own wordcount;
 alter table Recommenders add column own_favourites integer default null;             -- author's own favourites on fics (pretty much useless);
 alter table Recommenders add column own_finished_ratio real;                       -- ratio of finished fics;
 alter table Recommenders add column most_written_size integer default null;          -- small/medium/huge;
 alter table Recommenders add column website_type varchar default null;
 alter table Recommenders add column sumfaves integer default null;
 alter table Recommenders add column total_fics integer default null;
 alter table Recommenders add column wave integer default 0;
 
 CREATE  INDEX if not exists I_RECOMMENDERS_NAME ON Recommenders (name ASC);
 CREATE  INDEX if not exists I_RECOMMENDERS_URL ON Recommenders (URL ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_FAVOURITES ON Recommenders (favourites ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_FFN_ID ON Recommenders (ffn_id ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_AO3_ID ON Recommenders (ao3_id ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_SB_ID ON Recommenders (sb_id ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_SV_ID ON Recommenders (sv_id ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_LAST_FAV_UPDATE ON Recommenders (last_favourites_update ASC);
 CREATE  INDEX if not exists I_RECOMMENDER_LAST_FAV_CHECK ON Recommenders (last_favourites_checked ASC);
 
 update recommenders set sumfaves = (SELECT  Sum(favourites) as summation FROM fanfics where author = recommenders.name) where sumfaves is null;
 update recommenders set sumfaves = 0 where sumfaves is null;
 update Recommenders set website_type = 'ffn' where website_type is null;
 
 
 --- +++++
  
 -- holds various stats for the author's page;
 -- to be used for fics clustering;
 CREATE TABLE if not exists AuthorFavouritesStatistics (author_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
 alter table AuthorFavouritesStatistics add column favourites integer default 0;                  -- how much stuff did the author favourite;
 alter table AuthorFavouritesStatistics add column favourites_wordcount integer default 0;        -- how much stuff did the author favourite;
 alter table AuthorFavouritesStatistics add column average_words_per_chapter real;
 
 alter table AuthorFavouritesStatistics add column esrb_type integer default -1;                          -- agnostic/kiddy/mature;
 alter table AuthorFavouritesStatistics add column prevalent_mood integer default -1;             -- sad/neutral/positive as categorized by genres;
 
 alter table AuthorFavouritesStatistics add column most_favourited_size integer default -1;       -- small/medium/large/huge;
 alter table AuthorFavouritesStatistics add column favourites_type integer default -1;            -- tiny(<50)/medium(50-500)/large(500-2000)/bullshit(2k+);
 
 alter table AuthorFavouritesStatistics add column average_favourited_length real default null;  -- excluding the biggest outliers if there are not enough of them;
 alter table AuthorFavouritesStatistics add column favourite_fandoms_diversity real;              -- how uniform are his favourites relative to most popular fandom;
 alter table AuthorFavouritesStatistics add column explorer_factor real;                          -- deals with how likely is the author to explore otherwise unpopular fics;
 alter table AuthorFavouritesStatistics add column mega_explorer_factor real;                     -- deals with how likely is the author to explore otherwise unpopular fics;
 
 alter table AuthorFavouritesStatistics add column crossover_factor real;                         -- how willing is he to read crossovers;
 alter table AuthorFavouritesStatistics add column unfinished_factor real;                        -- how willing is he to read stuff that isn't finished;
 alter table AuthorFavouritesStatistics add column esrb_uniformity_factor real;                   -- how faithfully the author sticks to the same ESRB when favouriting. Only  M/everything else taken into account;
 alter table AuthorFavouritesStatistics add column esrb_kiddy real;
 alter table AuthorFavouritesStatistics add column esrb_mature real;
 
 alter table AuthorFavouritesStatistics add column genre_diversity_factor real;                  -- how likely is the author to stick to a single genre;
 alter table AuthorFavouritesStatistics add column mood_uniformity_factor real;
 alter table AuthorFavouritesStatistics add column mood_sad real;
 alter table AuthorFavouritesStatistics add column mood_neutral real;
 alter table AuthorFavouritesStatistics add column mood_happy real;
 
 alter table AuthorFavouritesStatistics add column crack_factor real;
 alter table AuthorFavouritesStatistics add column slash_factor real;
 alter table AuthorFavouritesStatistics add column not_slash_factor real;
 alter table AuthorFavouritesStatistics add column smut_factor real;
 
 alter table AuthorFavouritesStatistics add column prevalent_genre varchar default null;

 alter table AuthorFavouritesStatistics add column size_tiny real;
 alter table AuthorFavouritesStatistics add column size_medium real;
 alter table AuthorFavouritesStatistics add column size_large real;
 alter table AuthorFavouritesStatistics add column size_huge real;
 
 alter table AuthorFavouritesStatistics add column first_published TIMESTAMP;
 alter table AuthorFavouritesStatistics add column last_published TIMESTAMP;
  
 
-- genre and fandom individual ratios in the separate tables;
-- which type of fandoms they are in : anime, games, books....  ;

 
 -- contains percentage per genre for favourite lists;
 CREATE TABLE if not exists AuthorFavouritesGenreStatistics (author_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
 alter table AuthorFavouritesGenreStatistics add column General_ real default 0;
 alter table AuthorFavouritesGenreStatistics add column Humor real default 0;
 alter table AuthorFavouritesGenreStatistics add column Poetry real default 0;
 alter table AuthorFavouritesGenreStatistics add column Adventure real default 0;
 alter table AuthorFavouritesGenreStatistics add column Mystery real default 0;
 alter table AuthorFavouritesGenreStatistics add column Horror real default 0;
 alter table AuthorFavouritesGenreStatistics add column Drama real default 0;
 alter table AuthorFavouritesGenreStatistics add column Parody real default 0;
 alter table AuthorFavouritesGenreStatistics add column Angst real default 0;
 alter table AuthorFavouritesGenreStatistics add column Supernatural real default 0;
 alter table AuthorFavouritesGenreStatistics add column Suspense real default 0;
 alter table AuthorFavouritesGenreStatistics add column Romance real default 0;
 alter table AuthorFavouritesGenreStatistics add column NoGenre real default 0;
 alter table AuthorFavouritesGenreStatistics add column SciFi real default 0;
 alter table AuthorFavouritesGenreStatistics add column Fantasy real default 0;
 alter table AuthorFavouritesGenreStatistics add column Spiritual real default 0;
 alter table AuthorFavouritesGenreStatistics add column Tragedy real default 0;
 alter table AuthorFavouritesGenreStatistics add column Western real default 0;
 alter table AuthorFavouritesGenreStatistics add column Crime real default 0;
 alter table AuthorFavouritesGenreStatistics add column Family real default 0;
 alter table AuthorFavouritesGenreStatistics add column HurtComfort real default 0;
 alter table AuthorFavouritesGenreStatistics add column Friendship real default 0;
 
 
  -- contains percentage per genre for favourite lists;
 CREATE TABLE if not exists AuthorFavouritesGenreStatisticsIter2 (author_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
 alter table AuthorFavouritesGenreStatisticsIter2 add column General_ real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Humor real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Poetry real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Adventure real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Mystery real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Horror real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Drama real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Parody real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Angst real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Supernatural real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Suspense real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Romance real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column NoGenre real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column SciFi real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Fantasy real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Spiritual real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Tragedy real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Western real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Crime real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Family real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column HurtComfort real default 0;
 alter table AuthorFavouritesGenreStatisticsIter2 add column Friendship real default 0;
 
  -- contains averaged percentage per genre for fics;
 CREATE TABLE if not exists FicGenreStatistics (fic_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
 alter table FicGenreStatistics add column General_ real default 0;
 alter table FicGenreStatistics add column Humor real default 0;
 alter table FicGenreStatistics add column Poetry real default 0;
 alter table FicGenreStatistics add column Adventure real default 0;
 alter table FicGenreStatistics add column Mystery real default 0;
 alter table FicGenreStatistics add column Horror real default 0;
 alter table FicGenreStatistics add column Drama real default 0;
 alter table FicGenreStatistics add column Parody real default 0;
 alter table FicGenreStatistics add column Angst real default 0;
 alter table FicGenreStatistics add column Supernatural real default 0;
 alter table FicGenreStatistics add column Suspense real default 0;
 alter table FicGenreStatistics add column Romance real default 0;
 alter table FicGenreStatistics add column NoGenre real default 0;
 alter table FicGenreStatistics add column SciFi real default 0;
 alter table FicGenreStatistics add column Fantasy real default 0;
 alter table FicGenreStatistics add column Spiritual real default 0;
 alter table FicGenreStatistics add column Tragedy real default 0;
 alter table FicGenreStatistics add column Western real default 0;
 alter table FicGenreStatistics add column Crime real default 0;
 alter table FicGenreStatistics add column Family real default 0;
 alter table FicGenreStatistics add column HurtComfort real default 0;
 alter table FicGenreStatistics add column Friendship real default 0;
 
 CREATE  INDEX if not exists I_FGS_FID ON FicGenreStatistics (fic_id ASC);
  
  -- need cluster relations in separate table;
 CREATE TABLE if not exists AuthorFavouritesFandomRatioStatistics (author_id INTEGER);
 
 alter table AuthorFavouritesFandomRatioStatistics add column fandom_id integer default null;
 alter table AuthorFavouritesFandomRatioStatistics add column fandom_ratio real default null;
 alter table AuthorFavouritesFandomRatioStatistics add column fic_count integer default 0;
 
 alter table AuthorFavouritesFandomRatioStatistics add primary key (author_id, fandom_id);
 
 CREATE  INDEX if not exists I_AFRS_AID_FAID ON AuthorFavouritesFandomRatioStatistics (author_id ASC, fandom_id asc);
  

CREATE TABLE if not exists Genres (id serial primary key);
 alter table Genres add column genre varchar;
 alter table Genres add column website varchar default 'ffn';
 
CREATE INDEX if not exists  I_GENRES_PK ON Genres (genre asc, website ASC);
CREATE INDEX if not exists  I_GENRES_GENRE ON Genres (genre ASC);
CREATE INDEX if not exists  I_GENRES_WEBSITE ON Genres (website ASC);


-- fandoms for fics;
CREATE TABLE if not exists FicFandoms (fic_id INTEGER NOT NULL);
 
 alter table FicFandoms add column fandom_id integer;
 alter table FicFandoms add primary key (fic_id, fandom_id);
 
 
CREATE INDEX if not exists  I_FIC_FANDOMS_PK ON FicFandoms (fic_id asc, fandom_id ASC);
CREATE INDEX if not exists  I_FIC_FANDOMS_FANDOM ON FicFandoms (fandom_id ASC);
CREATE INDEX if not exists  I_FIC_FANDOMS_FIC ON FicFandoms (fic_id ASC);

 
  CREATE TABLE if not exists Recommendations (recommender_id INTEGER NOT NULL);
 alter table Recommendations add column fic_id integer not null;
 alter table Recommendations add primary key (recommender_id, fic_id);
 
 CREATE INDEX if not exists I_RECOMMENDATIONS ON Recommendations (recommender_id ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_REC_FIC ON Recommendations (recommender_id ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC ON Recommendations (fic_id ASC);
  
 
 -- linked authors table;
 CREATE TABLE if not exists LinkedAuthors (recommender_id INTEGER NOT NULL);
 
 alter table LinkedAuthors add column url varchar;
 alter table LinkedAuthors add column ffn_id integer default null;
 alter table LinkedAuthors add column ao3_id integer default null;
 alter table LinkedAuthors add column sb_id integer default null;
 alter table LinkedAuthors add column sv_id integer default null;
 
 CREATE INDEX if not exists I_LINKED_AUTHORS_PK ON LinkedAuthors (recommender_id ASC);


-- data for fic relationships;
CREATE TABLE if not exists FicRelations(fic_id serial NOT NULL);
 
alter table FicRelations add column fic1 integer; 
alter table FicRelations add column fic1_list_count integer; 
alter table FicRelations add column fic2_list_count integer; 
alter table FicRelations add column meeting_list_count integer; 
alter table FicRelations add column fic2 integer; 
alter table FicRelations add column same_fandom boolean; 
alter table FicRelations add column attraction real default 0; 
alter table FicRelations add column repulsion real default 0; 
alter table FicRelations add column final_attraction real default 0; 
alter table FicRelations add primary key (fic1, fic2);
CREATE INDEX if not exists  I_FR_PK ON FicRelations (fic1 asc, fic2 asc);



CREATE TABLE if not exists user_settings(name varchar unique, value varchar);
INSERT INTO user_settings(name, value) values('Last Fandom Id', '0');


create or replace view AuthorMoodStatistics AS
select 
author_id as author_id,
NoGenre as None,
Adventure+Scifi+Spiritual+Supernatural+Suspense+Mystery+Crime+Fantasy+Western as Neutral,
Romance+ Family + Friendship + Humor + Parody + Drama+Tragedy+Angst +Horror+ HurtComfort+General_ as NonNeutral,
Romance as Flirty, 
Scifi+Spiritual+Supernatural+Suspense+Mystery+Crime+Fantasy+Western+Drama+
Family + Friendship + Humor + Parody + Tragedy+Angst +Horror+ HurtComfort+General_ as NonFlirty,
Family + Friendship as Bondy,
Scifi+Spiritual+Supernatural+Suspense+Mystery+Crime+Fantasy+Western+
Romance+ Humor + Parody + Drama+Tragedy+Angst +Horror+ HurtComfort+General_
as NonBondy,
Humor + Parody as Funny,
Scifi+Spiritual+Supernatural+Suspense+Mystery+Crime+Fantasy+Western+
Romance + Family + Friendship + Drama+Tragedy+Angst +Horror+ HurtComfort+General_
as NonFunny,
Drama+Tragedy+Angst as Dramatic,
Scifi+Spiritual+Supernatural+Suspense+Mystery+Crime+Fantasy+Western+
Humor + Parody + Romance + Family + Friendship  +Horror+ HurtComfort+General_
as NonDramatic,
Horror as Shocky,
Scifi+Spiritual+Supernatural+Suspense+Mystery+Crime+Fantasy+Western+
Romance + Family + Friendship + Drama+Tragedy+Angst + HurtComfort+General_ + Humor + Parody
as NonShocky,
HurtComfort as Hurty, 
Scifi+Spiritual+Supernatural+Suspense+Mystery+Crime+Fantasy+Western+
Romance + Family + Friendship + Drama+Tragedy+Angst + Horror +General_ + Humor + Parody
as NonHurty,
General_ + Poetry as Other
from AuthorFavouritesGenreStatistics;

DROP VIEW IF EXISTS vFanfics;
create or replace view vFanfics AS select id, author, author_id, title, summary, characters, genres, rated, published, updated, reviews,
wordcount, favourites, chapters, complete, at_chapter, ffn_id,
wcr, wcr_adjusted, reviewstofavourites,daysrunning,age,alive, date_deactivated, follows, hidden, keywords_yes, keywords_no, keywords_result,
filter_pass_1,filter_pass_2, fandom1, fandom2,
true_genre1,true_genre2,true_genre3,
true_genre1_percent,true_genre2_percent,true_genre3_percent, kept_genres, max_genre_percent
 from fanfics;

 
  
CREATE OR REPLACE VIEW  fandom_stats AS  select fandom, 
cast (EXTRACT(EPOCH FROM (now() - min(fs.published))) as real)/60/60/24/365  as age,
min(published) as origin,
(select sum(wcr) from fanfics where wcr < 200000 and fandom = fs.fandom)/(select count(id) from fanfics where wcr < 200000  and fandom = fs.fandom) as averagewcr,
max(favourites) as maxfaves, min(wcr) as minwcr, count(id) as ficcount from fanfics fs group by fandom;



alter table recommenders add column in_favourites integer;
alter table recommenders add column latest_favourited_fic_date timestamp;
alter table recommenders add column earliest_favourited_fic_date timestamp;
alter table fandomindex  add column  tracked integer;