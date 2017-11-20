CREATE TABLE if not exists PageCache (URL VARCHAR PRIMARY KEY  NOT NULL , GENERATION_DATE DATETIME,CONTENT BLOB, NEXT VARCHAR, PAGE_TYPE INTEGER, compressed integer default 0);
alter table PageCache add column regeneration_period integer default 60; 
CREATE INDEX if not exists I_PC_URL ON PageCache (URL ASC);
CREATE INDEX if not exists I_PC_GENERATED ON PageCache (GENERATION_DATE ASC);

-- task table;
CREATE TABLE if not exists PageTask (
 id integer NOT NULL primary key autoincrement,
 type integer not null, 
 parts integer default 0, 
 entities integer default 0, 
 created DATETIME, 
 scheduled_to DATETIME, 
 started DATETIME, 
 finished DATETIME, 
 results varchar, 
 error_code integer default 0, 
 auto_retry_count integer default 2,
 part_retries integer default 0, 
 ignore_cache integer default 0,
 refresh_if_needed integer default 0,
 task_comment varchar,
 success integer deault 0,
 PRIMARY KEY (id));
 CREATE INDEX if not exists I_PT_ID ON PageTask (id asc);
CREATE INDEX if not exists I_PT_TYPE ON PageTask (type asc);
CREATE INDEX if not exists I_PT_CREATED ON PageTask (created asc);
CREATE INDEX if not exists I_PT_SCHEDULED ON PageTask (scheduled_to asc);
CREATE INDEX if not exists I_PT_SUCCESS ON PageTask (success asc);

-- subtask table;
CREATE TABLE if not exists PageTaskParts (
 task_id integer NOT NULL,
 sub_id integer not null, 
 content varchar, 
 created DATETIME, 
 scheduled_to DATETIME, 
 started DATETIME, 
 finished DATETIME, 
 results varchar, 
 error_code integer default 0, 
 operation_id integer not null, 
 retries integer default 0, 
 PRIMARY KEY (task_id, sub_id));
CREATE INDEX if not exists I_PTP_PK ON PageTaskParts (task_id asc, sub_id asc);
CREATE INDEX if not exists I_PTP_STARTED ON PageTaskParts (started asc);
CREATE INDEX if not exists I_PTP_ERROR_CODE ON PageTaskParts (error_code asc);
CREATE INDEX if not exists I_PTP_OPERATION_ID ON PageTaskParts (operation_id asc);

-- task failed pages;
CREATE TABLE if not exists PageTaskFailedPages (
 operation_id integer NOT NULL primary key autoincrement,
 task_id integer NOT NULL ,
 URL VARCHAR,
 process_attempt datetime,
 last_seen datetime,
 PRIMARY KEY (operation_id)
);
CREATE INDEX if not exists I_PTF_OPERATION_ID ON PageTaskFailedPages (operation_id asc);
CREATE INDEX if not exists I_PTF_TASK_ID ON PageTaskFailedPages (task_id asc);

-- task warnings;
CREATE TABLE if not exists PageTaskWarnings(
 operation_id integer NOT NULL primary key autoincrement,
 task_id integer not null,
 process_attempt datetime,
 warning VARCHAR,
 PRIMARY KEY (warning_id)
);
CREATE INDEX if not exists I_PTW_operation_id ON PageTaskWarnings (operation_id asc);
CREATE INDEX if not exists I_PTW_TASK_ID ON PageTaskWarnings (task_id asc);