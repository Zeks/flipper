alter table discord_users add column year_published varchar;
alter table discord_users add column year_finished varchar;
alter table discord_servers add column explain_allowed integer default 0;
