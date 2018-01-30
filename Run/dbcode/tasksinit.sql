-- task table;
CREATE TABLE if not exists PageTasks (
 id integer NOT NULL primary key autoincrement default 0,
 type integer not null, 
 parts integer default 0, 
 entities integer default 0, 
 created_at DATETIME, 
 scheduled_to DATETIME, 
 started_at DATETIME, 
 finished_at DATETIME, 
 finished integer default 0,
 results varchar, 
 allowed_retry_count integer default 2,
 allowed_subtask_retry_count integer default 2,
 retries integer default 0, 
 cache_mode integer default 0,
 refresh_if_needed integer default 0,
 task_comment varchar,
 task_size integer default 0,
 success integer default 0);
  
CREATE INDEX if not exists I_PT_ID ON PageTasks (id asc);
CREATE INDEX if not exists I_PT_TYPE ON PageTasks (type asc);
CREATE INDEX if not exists I_PT_FINISHED ON PageTasks (finished asc);
CREATE INDEX if not exists I_PT_CREATED ON PageTasks (created_at asc);
CREATE INDEX if not exists I_PT_SCHEDULED ON PageTasks (scheduled_to asc);
CREATE INDEX if not exists I_PT_SUCCESS ON PageTasks (success asc);


-- subtask table;
CREATE TABLE if not exists PageTaskParts (
 task_id integer NOT NULL,
 sub_id integer not null default 0, 
 content varchar, 
 created_at DATETIME, 
 attempted boolean default false,
 scheduled_to DATETIME, 
 started_at DATETIME, 
 finished_at DATETIME, 
 task_size integer default 0,
 success integer default 0,
 finished integer default 0,
 retries integer default 0, 
 PRIMARY KEY (task_id, sub_id));
 alter table PageTaskParts add column subtask_comment varchar;
 alter table PageTaskParts add column custom_data1 varchar;
 alter table PageTaskParts add column parse_up_to datetime;
CREATE INDEX if not exists I_PTP_PK ON PageTaskParts (task_id asc, sub_id asc);
CREATE INDEX if not exists I_PTP_STARTED ON PageTaskParts (started_at asc);
CREATE INDEX if not exists I_PTP_ERROR_CODE ON PageTaskParts (success asc);

-- subtask action table;
CREATE TABLE if not exists PageTaskActions (
 action_uuid integer NOT NULL,
 task_id integer NOT NULL,
 sub_id integer,
 started_at DATETIME, 
 finished_at DATETIME, 
 success integer default 0,
 PRIMARY KEY (action_uuid));
 
CREATE INDEX if not exists I_PTA_PK ON PageTaskActions (action_uuid asc);
CREATE INDEX if not exists I_PTA_TASK_KEY ON PageTaskActions (task_id asc, sub_id asc);
CREATE INDEX if not exists I_PTA_TASK_ID ON PageTaskActions (task_id asc);
CREATE INDEX if not exists I_PTA_SUB_ID ON PageTaskActions (sub_id asc);
CREATE INDEX if not exists I_PTA_STARTED ON PageTaskActions (started_at asc);
CREATE INDEX if not exists I_PTA_FINISHED ON PageTaskActions (finished_at asc);


-- failed pages for subtasks;
CREATE TABLE if not exists PageWarnings (
 id integer NOT NULL PRIMARY KEY autoincrement default 0,
 action_uuid varchar,
 task_id integer NOT NULL,
 sub_id integer NOT NULL,
 URL VARCHAR,
 attempted_at datetime,
 last_seen_at datetime,
 error_code integer default 0,
 error_level integer default 0,
 error varchar);
CREATE INDEX if not exists I_PW_ID ON PageWarnings (id asc);
CREATE INDEX if not exists I_PW_ACTION_ID ON PageWarnings (action_id asc);
CREATE INDEX if not exists I_PW_TASK_ID ON PageWarnings (task_id asc);
CREATE INDEX if not exists I_PW_SUBTASK_ID ON PageWarnings (subtask_id asc);
CREATE INDEX if not exists I_PW_ERROR_CODE ON PageWarnings (error_code asc);
CREATE INDEX if not exists I_PW_URL ON PageWarnings (URL asc);
CREATE INDEX if not exists I_PW_ERROR_LEVEL ON PageWarnings (error_level asc);
