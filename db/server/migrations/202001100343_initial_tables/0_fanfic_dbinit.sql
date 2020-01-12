-- settings table;
create table if not exists FanficData.database_settings(name varchar unique, value varchar);
INSERT INTO FanficData.database_settings(name, value) values('Last Fandom Id', '0');
INSERT INTO FanficData.database_settings(name, value) values('Last Migration Id', '0');

--fanfics;
create table if not exists FanficData.FANFICS (
ID serial PRIMARY KEY unique);	

alter table FanficData.fanfics add column lastupdate TIMESTAMP;

alter table FanficData.fanfics add column wcr real;
alter table FanficData.fanfics add column wcr_adjusted real;
alter table FanficData.fanfics add column reviewstofavourites real;

alter table FanficData.fanfics add column AUTHOR VARCHAR;
alter table FanficData.fanfics add column TITLE VARCHAR;
alter table FanficData.fanfics add column SUMMARY VARCHAR;
alter table FanficData.fanfics add column GENRES VARCHAR;
alter table FanficData.fanfics add column CHARACTERS VARCHAR;
alter table FanficData.fanfics add column RATED VARCHAR;
alter table FanficData.fanfics add column fandom VARCHAR;

alter table FanficData.fanfics add column PUBLISHED timestamp;
alter table FanficData.fanfics add column UPDATED timestamp;
alter table FanficData.fanfics add column date_added timestamp;
alter table FanficData.fanfics add column date_deactivated timestamp;

alter table FanficData.fanfics add column FAVOURITES integer default 0;
alter table FanficData.fanfics add column REVIEWS integer default 0;
alter table FanficData.fanfics add column CHAPTERS integer default 0;
alter table FanficData.fanfics add column COMPLETE boolean default false;
alter table FanficData.fanfics add column AT_CHAPTER integer default 0;
alter table FanficData.fanfics add column alive boolean default false;
alter table FanficData.fanfics add column daysrunning integer default 0;
alter table FanficData.fanfics add column age integer default 0;
alter table FanficData.fanfics add column follows integer default null;
alter table FanficData.fanfics add column ffn_id integer default null;
alter table FanficData.fanfics add column ao3_id integer default null;
alter table FanficData.fanfics add column sb_id integer default null;
alter table FanficData.fanfics add column sv_id integer default null;
alter table FanficData.fanfics add column author_id integer default null;
alter table FanficData.fanfics add column author_web_id integer default null;
alter table FanficData.fanfics add column for_fill boolean default null;
alter table FanficData.fanfics add column wordcount integer default null;
alter table FanficData.fanfics add column hidden boolean default false;
alter table FanficData.fanfics add column fandom1 integer default null;
alter table FanficData.fanfics add column fandom2 integer default null;
alter table FanficData.fanfics add column keywords_yes boolean default false;
alter table FanficData.fanfics add column keywords_no boolean default false;
alter table FanficData.fanfics add column keywords_result boolean default false;
alter table FanficData.fanfics add column filter_pass_1 boolean default false;
alter table FanficData.fanfics add column filter_pass_2 boolean default false;

alter table FanficData.fanfics add column kept_genres VARCHAR;
alter table FanficData.fanfics add column true_genre1 VARCHAR;
alter table FanficData.fanfics add column true_genre2 VARCHAR;
alter table FanficData.fanfics add column true_genre3 VARCHAR;
alter table FanficData.fanfics add column true_genre1_percent real;
alter table FanficData.fanfics add column true_genre2_percent real;
alter table FanficData.fanfics add column true_genre3_percent real;
alter table FanficData.fanfics add column max_genre_percent real;
alter table FanficData.fanfics add column queued_for_action boolean default false;
alter table FanficData.fanfics add column is_english boolean default true;

CREATE INDEX if not exists  I_FANFICS_LASTUPDATE ON FanficData.FANFICS (lastupdate ASC);
CREATE INDEX if not exists  I_FANFICS_ID ON FanficData.FANFICS (ID ASC);
CREATE INDEX if not exists  I_FANFICS_FANDOM_1 ON FanficData.fanfics (fandom1 ASC);
CREATE INDEX if not exists  I_FANFICS_FANDOM_2 ON FanficData.fanfics (fandom2 ASC);
CREATE INDEX if not exists  I_FANFICS_KW_YES ON FanficData.fanfics (keywords_yes ASC);
CREATE INDEX if not exists  I_FANFICS_KW_NO ON FanficData.fanfics (keywords_no ASC);
CREATE INDEX if not exists  I_FANFICS_KW_RESULT ON FanficData.fanfics (keywords_result ASC);
CREATE INDEX if not exists  I_FANFICS_FIRST ON FanficData.fanfics (filter_pass_1 ASC);
CREATE INDEX if not exists  I_FANFICS_SECOND  ON FanficData.fanfics (filter_pass_2 ASC);
CREATE INDEX if not exists  I_FANFICS_QFA  ON FanficData.fanfics (queued_for_action ASC);
CREATE INDEX if not exists  I_FANFICS_AUTHOR_ID  ON FanficData.fanfics (author_id ASC);
CREATE INDEX if not exists  I_FANFICS_IS_ENGLISH  ON FanficData.fanfics (is_english ASC);
CREATE INDEX if not exists  I_FANFICS_RATED  ON FanficData.fanfics (rated ASC);

CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_TG1 ON FanficData.FANFICS (true_genre1 asc);
CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_TG2 ON FanficData.FANFICS (true_genre2 asc);
CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_TG3 ON FanficData.FANFICS (true_genre3 asc);
 
CREATE INDEX if not exists  I_FANFICS_GENRES ON FanficData.FANFICS (GENRES ASC);
CREATE INDEX if not exists  I_FANFICS_WCR ON FanficData.fanfics (wcr ASC);
CREATE INDEX if not exists  I_FANFICS_DAYSRUNNING ON FanficData.fanfics (daysrunning ASC);
CREATE INDEX if not exists  I_FANFICS_age ON FanficData.fanfics (age ASC);
CREATE INDEX if not exists  I_FANFICS_favourites ON FanficData.fanfics (favourites ASC);
CREATE INDEX if not exists  I_FANFICS_reviews ON FanficData.fanfics (reviews ASC);
CREATE INDEX if not exists  I_FANFICS_complete ON FanficData.fanfics (complete ASC);
CREATE INDEX if not exists  I_FANFICS_reviewstofavourites ON FanficData.fanfics (reviewstofavourites ASC);
CREATE INDEX if not exists  I_FANFICS_UPDATED ON FanficData.fanfics (UPDATED ASC);
CREATE INDEX if not exists  I_FANFICS_PUBLISHED ON FanficData.fanfics (PUBLISHED ASC);
CREATE INDEX if not exists  I_FANFICS_FFN_ID ON FanficData.fanfics (ffn_id ASC);
CREATE INDEX if not exists  I_FANFICS_ALIVE ON FanficData.fanfics (alive ASC);
CREATE INDEX if not exists  I_FANFICS_DATE_ADDED ON FanficData.fanfics (date_added ASC);
CREATE INDEX if not exists  I_FANFICS_FOR_FILL ON FanficData.fanfics (for_fill ASC);
CREATE INDEX if not exists  I_FANFICS_HIDDEN on FanficData.fanfics (hidden ASC);
CREATE INDEX if not exists  I_FANFICS_SUMMARY on FanficData.fanfics (summary ASC);
CREATE INDEX if not exists  I_FANFICS_TITLE on FanficData.fanfics (title ASC);

CREATE INDEX if not exists  I_FANFICS_IDENTITY ON FanficData.FANFICS (AUTHOR ASC, TITLE ASC);
CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_FP1 ON FanficData.FANFICS (WORDCOUNT ASC , filter_pass_1 desc);
CREATE INDEX if not exists  I_FANFICS_WORDCOUNT_FP2 ON FanficData.FANFICS (WORDCOUNT ASC , filter_pass_2 desc);
CREATE INDEX if not exists  I_FANFICS_FAVOURITES_FP1 ON FanficData.FANFICS (favourites ASC , filter_pass_1 desc);
CREATE INDEX if not exists  I_FANFICS_FAVOURITES_FP2 ON FanficData.FANFICS (favourites ASC , filter_pass_2 desc);

 
create table if not exists FanficData.fandomindex (id serial unique);
alter table FanficData.fandomindex add column lastupdate timestamp;
alter table FanficData.fandomindex add column name varchar;
alter table FanficData.fandomindex add primary key (id, name);
alter table FanficData.fandomindex  add column tracked integer default 0;

CREATE INDEX if not exists I_FANDOMINDEX_UPDATED ON FanficData.fandomindex (lastupdate ASC);
CREATE INDEX if not exists I_FANDOMINDEX_PK ON FanficData.fandomindex (id ASC, name asc);
CREATE INDEX if not exists I_FANDOMINDEX_ID ON FanficData.fandomindex (id ASC);
CREATE INDEX if not exists I_FANDOMINDEX_NAME ON FanficData.fandomindex (name ASC);

create table if not exists FanficData.fandomurls (global_id integer);
alter table FanficData.fandomurls add column lastupdate TIMESTAMP;
alter table FanficData.fandomurls add column url varchar;
alter table FanficData.fandomurls add column website varchar not null default 'ffn';
alter table FanficData.fandomurls add column custom varchar;
alter table FanficData.fandomurls add primary key (global_id, url);
 
CREATE INDEX if not exists I_FURL_ID ON FanficData.fandomurls (lastupdate ASC);
CREATE INDEX if not exists I_FURL_LASTUPDATE ON FanficData.fandomurls (global_id ASC);
CREATE INDEX if not exists I_FURL_URL ON FanficData.fandomurls (url ASC);
CREATE INDEX if not exists I_FURL_CUSTOM ON FanficData.fandomurls (custom ASC);
CREATE INDEX if not exists I_FURL_WEBSITE ON FanficData.fandomurls (website ASC);


 

 create table if not exists FanficData.Recommenders (id serial PRIMARY KEY UNIQUE);
 
 alter table FanficData.Recommenders add column name varchar not null;
 alter table FanficData.Recommenders add column url varchar not null;
 alter table FanficData.Recommenders add column page_data bytea;
 alter table FanficData.Recommenders add column page_updated TIMESTAMP;
 alter table FanficData.Recommenders add column author_updated TIMESTAMP;
  
 alter table FanficData.Recommenders add column favourites integer default null;
 alter table FanficData.Recommenders add column fics integer default null;
 alter table FanficData.Recommenders add column ffn_id integer default null;
 alter table FanficData.Recommenders add column ao3_id integer default null;
 alter table FanficData.Recommenders add column sb_id integer default null;
 alter table FanficData.Recommenders add column sv_id integer default null;
 
 alter table FanficData.Recommenders add column page_creation_date TIMESTAMP;
 alter table FanficData.Recommenders add column info_updated TIMESTAMP;
 alter table FanficData.Recommenders add column info_wordcount integer default null;
 alter table FanficData.Recommenders add column last_published_fic_date TIMESTAMP;
 alter table FanficData.Recommenders add column first_published_fic_date TIMESTAMP;
 alter table FanficData.Recommenders add column last_favourites_update TIMESTAMP;
 alter table FanficData.Recommenders add column last_favourites_checked TIMESTAMP;
 alter table FanficData.Recommenders add column own_wordcount integer default null;              -- author's own wordcount;
 alter table FanficData.Recommenders add column own_favourites integer default null;             -- author's own favourites on fics (pretty much useless);
 alter table FanficData.Recommenders add column own_finished_ratio real;                       -- ratio of finished fics;
 alter table FanficData.Recommenders add column most_written_size integer default null;          -- small/medium/huge;
 alter table FanficData.Recommenders add column website_type varchar default null;
 alter table FanficData.Recommenders add column sumfaves integer default null;
 alter table FanficData.Recommenders add column total_fics integer default null;
 alter table FanficData.Recommenders add column wave integer default 0;
 alter table FanficData.recommenders add column in_favourites integer;
 alter table FanficData.recommenders add column latest_favourited_fic_date timestamp;
 alter table FanficData.recommenders add column earliest_favourited_fic_date timestamp;
 
 
 CREATE INDEX if not exists I_RECOMMENDERS_NAME ON FanficData.Recommenders (name ASC);
 CREATE INDEX if not exists I_RECOMMENDERS_URL ON FanficData.Recommenders (URL ASC);
 CREATE INDEX if not exists I_RECOMMENDER_FAVOURITES ON FanficData.Recommenders (favourites ASC);
 CREATE INDEX if not exists I_RECOMMENDER_FFN_ID ON FanficData.Recommenders (ffn_id ASC);
 CREATE INDEX if not exists I_RECOMMENDER_AO3_ID ON FanficData.Recommenders (ao3_id ASC);
 CREATE INDEX if not exists I_RECOMMENDER_SB_ID ON FanficData.Recommenders (sb_id ASC);
 CREATE INDEX if not exists I_RECOMMENDER_SV_ID ON FanficData.Recommenders (sv_id ASC);
 CREATE INDEX if not exists I_RECOMMENDER_LAST_FAV_UPDATE ON FanficData.Recommenders (last_favourites_update ASC);
 CREATE INDEX if not exists I_RECOMMENDER_LAST_FAV_CHECK ON FanficData.Recommenders (last_favourites_checked ASC);
 
create table if not exists FanficData.Genres (id serial primary key);
 alter table FanficData.Genres add column genre varchar;
 alter table FanficData.Genres add column website varchar default 'ffn';
 
CREATE INDEX if not exists I_GENRES_PK ON FanficData.Genres (genre asc, website ASC);
CREATE INDEX if not exists I_GENRES_GENRE ON FanficData.Genres (genre ASC);
CREATE INDEX if not exists I_GENRES_WEBSITE ON FanficData.Genres (website ASC);


-- fandoms for fics;
create table if not exists FanficData.FicFandoms (fic_id INTEGER NOT NULL);
 
 alter table FanficData.FicFandoms add column fandom_id integer;
 alter table FanficData.FicFandoms add primary key (fic_id, fandom_id);
 
 
CREATE INDEX if not exists I_FIC_FANDOMS_PK ON FanficData.FicFandoms (fic_id asc, fandom_id ASC);
CREATE INDEX if not exists I_FIC_FANDOMS_FANDOM ON FanficData.FicFandoms (fandom_id ASC);
CREATE INDEX if not exists I_FIC_FANDOMS_FIC ON FanficData.FicFandoms (fic_id ASC);

 
 create table if not exists FanficData.Recommendations (recommender_id INTEGER NOT NULL);
 alter table FanficData.Recommendations add column fic_id integer not null;
 alter table FanficData.Recommendations add primary key (recommender_id, fic_id);
 
 CREATE INDEX if not exists I_RECOMMENDATIONS ON FanficData.Recommendations (recommender_id ASC);
 CREATE INDEX if not exists I_RECOMMENDATIONS_REC_FIC ON FanficData.Recommendations (recommender_id ASC, fic_id asc);
 CREATE INDEX if not exists I_RECOMMENDATIONS_FIC ON FanficData.Recommendations (fic_id ASC);
  
 
 -- linked authors table;
 create table if not exists FanficData.LinkedAuthors (recommender_id INTEGER NOT NULL);
 
 alter table FanficData.LinkedAuthors add column url varchar;
 alter table FanficData.LinkedAuthors add column ffn_id integer default null;
 alter table FanficData.LinkedAuthors add column ao3_id integer default null;
 alter table FanficData.LinkedAuthors add column sb_id integer default null;
 alter table FanficData.LinkedAuthors add column sv_id integer default null;
 
 CREATE INDEX if not exists I_LINKED_AUTHORS_PK ON FanficData.LinkedAuthors (recommender_id ASC);

create or replace view FanficData.vFanfics AS select id, author, author_id, title, summary, characters, genres, rated, published, updated, reviews,
wordcount, favourites, chapters, complete, at_chapter, ffn_id,
wcr, wcr_adjusted, reviewstofavourites,daysrunning,age,alive, date_deactivated, follows, hidden, keywords_yes, keywords_no, keywords_result,
filter_pass_1,filter_pass_2, fandom1, fandom2,
true_genre1,true_genre2,true_genre3,
true_genre1_percent,true_genre2_percent,true_genre3_percent, kept_genres, max_genre_percent
 from FanficData.fanfics;



--create table if not exists FanficData.fandomsources (global_id integer);
 
--alter table FanficData.fandomsources add column website varchar not null default 'ffn';
--alter table FanficData.fandomsources add column last_update TIMESTAMP;
--alter table FanficData.fandomsources add column last_parse_limit TIMESTAMP;
--alter table FanficData.fandomsources add column second_last_parse_limit TIMESTAMP;
--alter table FanficData.fandomsources add column date_of_first_fic TIMESTAMP;
--alter table FanficData.fandomsources add column date_of_creation TIMESTAMP;
--alter table FanficData.fandomsources add column fandom_multiplier integer default 1;
--alter table FanficData.fandomsources add column fic_count integer default 0;
--alter table FanficData.fandomsources add column average_faves_top_3 real default 0;
 
--CREATE INDEX if not exists I_FSOURCE_ID ON FanficData.fandomsources (global_id ASC);
--CREATE INDEX if not exists I_FSOURCE_WEBSITE ON FanficData.fandomsources (website ASC);
--CREATE INDEX if not exists I_FSOURCE_LASTUPDATE ON FanficData.fandomsources (last_update ASC);
--CREATE INDEX if not exists I_FSOURCE_AVGF3 ON FanficData.fandomsources (average_faves_top_3 ASC);


--create table if not exists FanficData.FIC_GENRE_ITERATIONS ( fic_id integer primary key );
--alter table FanficData.FIC_GENRE_ITERATIONS add column true_genre1 VARCHAR;
--alter table FanficData.FIC_GENRE_ITERATIONS add column true_genre2 VARCHAR;
--alter table FanficData.FIC_GENRE_ITERATIONS add column true_genre3 VARCHAR;
--alter table FanficData.FIC_GENRE_ITERATIONS add column kept_genres VARCHAR;
--alter table FanficData.FIC_GENRE_ITERATIONS add column true_genre1_percent real;
--alter table FanficData.FIC_GENRE_ITERATIONS add column true_genre2_percent real;
--alter table FanficData.FIC_GENRE_ITERATIONS add column true_genre3_percent real;
--alter table FanficData.FIC_GENRE_ITERATIONS add column max_genre_percent real;
--CREATE INDEX if not exists  I_FGI_PK  ON FanficData.FIC_GENRE_ITERATIONS (fic_id ASC);

  -- contains percentage per genre for favourite lists;
 -- create table if not exists FanficData.AuthorFavouritesGenreStatisticsIter2 (author_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column General_ real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Humor real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Poetry real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Adventure real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Mystery real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Horror real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Drama real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Parody real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Angst real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Supernatural real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Suspense real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Romance real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column NoGenre real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column SciFi real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Fantasy real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Spiritual real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Tragedy real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Western real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Crime real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Family real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column HurtComfort real default 0;
 -- alter table FanficData.AuthorFavouritesGenreStatisticsIter2 add column Friendship real default 0;
 
 
 -- data for fic relationships;
--create table if not exists FanficData.FicRelations(fic_id serial NOT NULL);
-- 
--alter table FanficData.FicRelations add column fic1 integer; 
--alter table FanficData.FicRelations add column fic1_list_count integer; 
--alter table FanficData.FicRelations add column fic2_list_count integer; 
--alter table FanficData.FicRelations add column meeting_list_count integer; 
--alter table FanficData.FicRelations add column fic2 integer; 
--alter table FanficData.FicRelations add column same_fandom boolean; 
--alter table FanficData.FicRelations add column attraction real default 0; 
--alter table FanficData.FicRelations add column repulsion real default 0; 
--alter table FanficData.FicRelations add column final_attraction real default 0; 
--alter table FanficData.FicRelations add primary key (fic1, fic2);
--CREATE INDEX if not exists I_FR_PK ON FanficData.FicRelations (fic1 asc, fic2 asc);


-- temporary tables for calculations;
-- slash ffn db;
 --create table if not exists FanficData.slash_data_ffn (ffn_id integer primary key);
 --alter table FanficData.slash_data_ffn add column keywords_yes boolean default false;
 --alter table FanficData.slash_data_ffn add column keywords_no boolean default false;
 --alter table FanficData.slash_data_ffn add column keywords_result boolean default false;
 --alter table FanficData.slash_data_ffn add column filter_pass_1 boolean default false;
 --alter table FanficData.slash_data_ffn add column filter_pass_2 boolean default false;
 
--CREATE INDEX if not exists I_SDFFN_KEY ON FanficData.slash_data_ffn (ffn_id ASC);
--CREATE INDEX if not exists I_SDFFN_KW_YES ON FanficData.slash_data_ffn (keywords_yes ASC);
--CREATE INDEX if not exists I_SDFFN_KW_NO ON FanficData.slash_data_ffn (keywords_no ASC);
--CREATE INDEX if not exists I_SDFFN_KW_RESULT ON FanficData.slash_data_ffn (keywords_result ASC);
--CREATE INDEX if not exists I_SDFFN_FIRST ON FanficData.slash_data_ffn (filter_pass_1 ASC);
--CREATE INDEX if not exists I_SDFFN_SECOND  ON FanficData.slash_data_ffn (filter_pass_2 ASC);


-- algo passes;
-- create table if not exists FanficData.algopasses (fic_id integer primary key);
-- alter table FanficData.algopasses add column keywords_yes boolean default false;
-- alter table FanficData.algopasses add column keywords_no boolean default false;
-- alter table FanficData.algopasses add column keywords_pass_result boolean default false;
-- alter table FanficData.algopasses add column pass_1 boolean default false;
-- alter table FanficData.algopasses add column pass_2 boolean default false;
-- alter table FanficData.algopasses add column pass_3 boolean default false;
-- alter table FanficData.algopasses add column pass_4 boolean default false;
-- alter table FanficData.algopasses add column pass_5 boolean default false;
-- 
-- 
--
-- CREATE INDEX if not exists I_SP_FID ON FanficData.algopasses (fic_id ASC);
-- CREATE INDEX if not exists I_SP_YES_KEY ON FanficData.algopasses (keywords_yes ASC);
-- CREATE INDEX if not exists I_SP_NO_KEY ON FanficData.algopasses (keywords_no ASC);
-- CREATE INDEX if not exists I_SP_KEY_PASS_RESULT ON FanficData.algopasses (keywords_pass_result ASC);
-- CREATE INDEX if not exists I_SP_PASS_1 ON FanficData.algopasses (pass_1 ASC);
-- CREATE INDEX if not exists I_SP_PASS_2 ON FanficData.algopasses (pass_2 ASC);
-- CREATE INDEX if not exists I_SP_PASS_3 ON FanficData.algopasses (pass_3 ASC);
-- CREATE INDEX if not exists I_SP_PASS_4 ON FanficData.algopasses (pass_4 ASC);
-- CREATE INDEX if not exists I_SP_PASS_5 ON FanficData.algopasses (pass_5 ASC);

-- update FanficData.recommenders set sumfaves = (SELECT  Sum(favourites) as summation FROM FanficData.fanfics where author = recommenders.name) where sumfaves is null;
-- update FanficData.recommenders set sumfaves = 0 where sumfaves is null;
-- update FanficData.Recommenders set website_type = 'ffn' where website_type is null;