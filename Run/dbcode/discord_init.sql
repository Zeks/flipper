CREATE TABLE if not exists user_settings(name varchar unique, value varchar unique);


CREATE TABLE if not exists discord_users(
user_id varchar unique PRIMARY KEY,
user_name varchar,
ffn_id varchar,
reads_slash integer default 0,
current_list integer default 0,
banned interger default 0);
alter table discord_users add column uuid varchar;
alter table discord_users add column forced_min_matches integer default 0;
alter table discord_users add column forced_ratio integer default 0;
alter table discord_users add column use_liked_authors_only integer default 0;
alter table discord_users add column use_fresh_sorting integer default 0;
alter table discord_users add column strict_fresh_sorting integer default 0;
alter table discord_users add column show_complete_only integer default 0;
alter table discord_users add column hide_dead integer default 0;

CREATE INDEX if not exists I_discord_users ON discord_users (user_id ASC);  

CREATE TABLE if not exists user_lists(
user_id varchar,
list_id integer,
list_name varchar,
list_type integer,
at_page integer default 0,
min_match integer default 6,
match_ratio integer default 50,
always_at integer default 9999,
generated datetime,
PRIMARY KEY (user_id asc, list_id asc));
CREATE INDEX if not exists  I_USER_LISTS_PK ON user_lists (user_id asc, list_id asc);


CREATE TABLE if not exists list_sources(
user_id varchar, list_id integer, fic_id integer,PRIMARY KEY (user_id asc, list_id asc, fic_id asc));
CREATE INDEX if not exists  I_LIST_SOURCES_PK ON list_sources (list_id asc, fic_id asc);

CREATE TABLE if not exists fic_tags(
user_id integer,fic_id integer, fic_tag varchar, PRIMARY KEY (user_id asc, fic_id asc, fic_tag));
CREATE INDEX if not exists  I_fic_tags_PK ON list_sources (user_id asc, fic_id asc, fic_tag);

CREATE TABLE if not exists ignored_fandoms(user_id varchar, fandom_id INTEGER, including_crossovers integer default 0, PRIMARY KEY (user_id asc, fandom_id asc));
CREATE TABLE if not exists filtered_fandoms(user_id varchar, fandom_id INTEGER, including_crossovers integer default 0, PRIMARY KEY (user_id asc, fandom_id asc));


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


CREATE TABLE if not exists discord_servers(
server_id varchar unique PRIMARY KEY,
owner_id varchar, 
dedicated_channel_id varchar,
command_prefix varchar default '!', 
server_name varchar,
server_banned integer default 0,
server_silenced integer default 0,
bot_answers_in_pm integer default 0, 
parse_request_limit integer default 0, 
active_since DATETIME, 
last_request DATETIME, 
total_requests integer 
);
CREATE INDEX if not exists I_SERVER_ID ON discord_servers (server_id ASC);
