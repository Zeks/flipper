CREATE TABLE if not exists user_settings(name varchar unique, value varchar unique);


CREATE TABLE if not exists discord_users(
user_id varchar unique PRIMARY KEY,
user_name varchar,
ffn_id varchar,
reads_slash integer default 0,
current_list integer default 0,
banned interger default 0);

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
