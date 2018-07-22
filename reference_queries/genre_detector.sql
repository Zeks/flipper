with count_per_fic(fic_id,title, genres, total, humor, romance, drama, adventure, hurtcomfort) as (
with
fic_ids as (select fic_id as fid from fictags where tag ='Rec' union all select 34634 as fid),

total as (
select fic_id, count(distinct recommender_id) as total from recommendations, fic_ids 
on fic_ids.fid = recommendations.fic_id 
and recommender_id in (select author_id from AuthorFavouritesStatistics where favourites > 30)
group by fic_id
)
select fanfics.id as id, fanfics.title as title, fanfics.genres as genres, total.total, 
(select count(distinct author_id) from  AuthorFavouritesGenreStatistics where  ((humor+parody) - romance)  > 0.2 and author_id in 
(select distinct recommender_id from recommendations where fanfics.id = fic_id)) as humor,
(select count(distinct author_id) from  AuthorFavouritesGenreStatistics where  (romance - (humor+parody)) > 0.3 and author_id in 
(select distinct recommender_id from recommendations where fanfics.id = fic_id)) as romance,
(select count(distinct author_id) from  AuthorFavouritesGenreStatistics where  drama > 0.3 and author_id in 
(select distinct recommender_id from recommendations where fanfics.id = fic_id)) as drama,
(select count(distinct author_id) from  AuthorFavouritesGenreStatistics where  adventure > 0.3 and author_id in 
(select distinct recommender_id from recommendations where fanfics.id = fic_id)) as adventure,
(select count(distinct author_id) from  AuthorFavouritesGenreStatistics where  hurtcomfort > 0.15 and author_id in 
(select distinct recommender_id from recommendations where fanfics.id = fic_id)) as hurtcomfort




   from fanfics, total on total.fic_id = fanfics.id 
)
select  fic_id, total, title, genres,
cast(humor as float)/cast(total as float) as humor,
cast(romance as float)/cast(total as float) as div_romance,
cast(drama as float)/cast(total as float) as drama,
cast(adventure as float)/cast(total as float) as div_adventure,
cast(hurtcomfort as float)/cast(total as float) as hurtcomfort

 from count_per_fic
where total > 50
