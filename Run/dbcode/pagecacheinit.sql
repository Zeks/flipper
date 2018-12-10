CREATE TABLE if not exists PageCache (URL VARCHAR PRIMARY KEY  NOT NULL , GENERATION_DATE DATETIME,CONTENT BLOB, NEXT VARCHAR, PAGE_TYPE INTEGER, compressed integer default 0);
alter table PageCache add column regeneration_period integer default 60; 
alter table PageCache add column task_id interger default -1; 
CREATE INDEX if not exists I_PC_URL ON PageCache (URL ASC);
CREATE INDEX if not exists I_PC_GENERATED ON PageCache (GENERATION_DATE ASC);

CREATE TABLE if not exists user_settings(name varchar unique, value integer unique);
INSERT INTO user_settings(name, value) values('Last Fandom Id', 0);
