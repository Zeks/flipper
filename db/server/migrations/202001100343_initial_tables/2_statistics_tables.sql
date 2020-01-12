 --- +++++
 -- holds various stats for the author's page;
 -- to be used for fics clustering;
 create table if not exists StatisticsData.AuthorFavouritesStatistics (author_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
 alter table StatisticsData.AuthorFavouritesStatistics add column favourites integer default 0;                  -- how much stuff did the author favourite;
 alter table StatisticsData.AuthorFavouritesStatistics add column favourites_wordcount integer default 0;        -- how much stuff did the author favourite;
 alter table StatisticsData.AuthorFavouritesStatistics add column average_words_per_chapter real;
             
 alter table StatisticsData.AuthorFavouritesStatistics add column esrb_type integer default -1;                          -- agnostic/kiddy/mature;
 alter table StatisticsData.AuthorFavouritesStatistics add column prevalent_mood integer default -1;             -- sad/neutral/positive as categorized by genres;
             
 alter table StatisticsData.AuthorFavouritesStatistics add column most_favourited_size integer default -1;       -- small/medium/large/huge;
 alter table StatisticsData.AuthorFavouritesStatistics add column favourites_type integer default -1;            -- tiny(<50)/medium(50-500)/large(500-2000)/bullshit(2k+);
             
 alter table StatisticsData.AuthorFavouritesStatistics add column average_favourited_length real default null;  -- excluding the biggest outliers if there are not enough of them;
 alter table StatisticsData.AuthorFavouritesStatistics add column favourite_fandoms_diversity real;              -- how uniform are his favourites relative to most popular fandom;
 alter table StatisticsData.AuthorFavouritesStatistics add column explorer_factor real;                          -- deals with how likely is the author to explore otherwise unpopular fics;
 alter table StatisticsData.AuthorFavouritesStatistics add column mega_explorer_factor real;                     -- deals with how likely is the author to explore otherwise unpopular fics;
             
 alter table StatisticsData.AuthorFavouritesStatistics add column crossover_factor real;                         -- how willing is he to read crossovers;
 alter table StatisticsData.AuthorFavouritesStatistics add column unfinished_factor real;                        -- how willing is he to read stuff that isn't finished;
 alter table StatisticsData.AuthorFavouritesStatistics add column esrb_uniformity_factor real;                   -- how faithfully the author sticks to the same ESRB when favouriting. Only  M/everything else taken into account;
 alter table StatisticsData.AuthorFavouritesStatistics add column esrb_kiddy real;
 alter table StatisticsData.AuthorFavouritesStatistics add column esrb_mature real;
             
 alter table StatisticsData.AuthorFavouritesStatistics add column genre_diversity_factor real;                  -- how likely is the author to stick to a single genre;
 alter table StatisticsData.AuthorFavouritesStatistics add column mood_uniformity_factor real;
 alter table StatisticsData.AuthorFavouritesStatistics add column mood_sad real;
 alter table StatisticsData.AuthorFavouritesStatistics add column mood_neutral real;
 alter table StatisticsData.AuthorFavouritesStatistics add column mood_happy real;
             
 alter table StatisticsData.AuthorFavouritesStatistics add column crack_factor real;
 alter table StatisticsData.AuthorFavouritesStatistics add column slash_factor real;
 alter table StatisticsData.AuthorFavouritesStatistics add column not_slash_factor real;
 alter table StatisticsData.AuthorFavouritesStatistics add column smut_factor real;
             
 alter table StatisticsData.AuthorFavouritesStatistics add column prevalent_genre varchar default null;
             
 alter table StatisticsData.AuthorFavouritesStatistics add column size_tiny real;
 alter table StatisticsData.AuthorFavouritesStatistics add column size_medium real;
 alter table StatisticsData.AuthorFavouritesStatistics add column size_large real;
 alter table StatisticsData.AuthorFavouritesStatistics add column size_huge real;
             
 alter table StatisticsData.AuthorFavouritesStatistics add column first_published TIMESTAMP;
 alter table StatisticsData.AuthorFavouritesStatistics add column last_published TIMESTAMP;
  
 
-- genre and fandom individual ratios in the separate tables;
-- which type of fandoms they are in : anime, games, books....  ;

 
 -- contains percentage per genre for favourite lists;
 create table if not exists StatisticsData.AuthorFavouritesGenreStatistics (author_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column General_ real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Humor real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Poetry real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Adventure real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Mystery real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Horror real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Drama real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Parody real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Angst real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Supernatural real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Suspense real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Romance real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column NoGenre real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column SciFi real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Fantasy real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Spiritual real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Tragedy real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Western real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Crime real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Family real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column HurtComfort real default 0;
 alter table StatisticsData.AuthorFavouritesGenreStatistics add column Friendship real default 0;
 
 

 
  -- contains averaged percentage per genre for fics;
 create table if not exists StatisticsData.FicGenreStatistics (fic_id INTEGER PRIMARY KEY NOT NULL UNIQUE);
 alter table StatisticsData.FicGenreStatistics add column General_ real default 0;
 alter table StatisticsData.FicGenreStatistics add column Humor real default 0;
 alter table StatisticsData.FicGenreStatistics add column Poetry real default 0;
 alter table StatisticsData.FicGenreStatistics add column Adventure real default 0;
 alter table StatisticsData.FicGenreStatistics add column Mystery real default 0;
 alter table StatisticsData.FicGenreStatistics add column Horror real default 0;
 alter table StatisticsData.FicGenreStatistics add column Drama real default 0;
 alter table StatisticsData.FicGenreStatistics add column Parody real default 0;
 alter table StatisticsData.FicGenreStatistics add column Angst real default 0;
 alter table StatisticsData.FicGenreStatistics add column Supernatural real default 0;
 alter table StatisticsData.FicGenreStatistics add column Suspense real default 0;
 alter table StatisticsData.FicGenreStatistics add column Romance real default 0;
 alter table StatisticsData.FicGenreStatistics add column NoGenre real default 0;
 alter table StatisticsData.FicGenreStatistics add column SciFi real default 0;
 alter table StatisticsData.FicGenreStatistics add column Fantasy real default 0;
 alter table StatisticsData.FicGenreStatistics add column Spiritual real default 0;
 alter table StatisticsData.FicGenreStatistics add column Tragedy real default 0;
 alter table StatisticsData.FicGenreStatistics add column Western real default 0;
 alter table StatisticsData.FicGenreStatistics add column Crime real default 0;
 alter table StatisticsData.FicGenreStatistics add column Family real default 0;
 alter table StatisticsData.FicGenreStatistics add column HurtComfort real default 0;
 alter table StatisticsData.FicGenreStatistics add column Friendship real default 0;
 
 CREATE INDEX if not exists I_FGS_FID ON StatisticsData.FicGenreStatistics (fic_id ASC);
  
  -- need cluster relations in separate table;
 create table if not exists StatisticsData.AuthorFavouritesFandomRatioStatistics (author_id INTEGER);
 
 alter table StatisticsData.AuthorFavouritesFandomRatioStatistics add column fandom_id integer default null;
 alter table StatisticsData.AuthorFavouritesFandomRatioStatistics add column fandom_ratio real default null;
 alter table StatisticsData.AuthorFavouritesFandomRatioStatistics add column fic_count integer default 0;
 
 alter table StatisticsData.AuthorFavouritesFandomRatioStatistics add primary key (author_id, fandom_id);
 
 CREATE INDEX if not exists I_AFRS_AID_FAID ON StatisticsData.AuthorFavouritesFandomRatioStatistics (author_id ASC, fandom_id asc);
  
  
  
create or replace view StatisticsData.AuthorMoodStatistics AS
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
from StatisticsData.AuthorFavouritesGenreStatistics;
