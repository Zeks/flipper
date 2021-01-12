--
-- PostgreSQL database dump
--

-- Dumped from database version 13.1 (Debian 13.1-1.pgdg100+1)
-- Dumped by pg_dump version 13.1 (Ubuntu 13.1-1.pgdg20.04+1)

-- Started on 2021-01-03 09:24:38 MSK

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- TOC entry 200 (class 1259 OID 16385)
-- Name: discord_servers; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.discord_servers (
    server_id text NOT NULL,
    owner_id text,
    dedicated_channel_id text,
    command_prefix text DEFAULT '!'::text,
    server_name text,
    server_banned bigint DEFAULT 0,
    server_silenced bigint DEFAULT 0,
    bot_answers_in_pm bigint DEFAULT 0,
    parse_request_limit bigint DEFAULT 0,
    active_since timestamp without time zone,
    last_request timestamp without time zone,
    total_requests bigint
);


ALTER TABLE public.discord_servers OWNER TO socrates;

--
-- TOC entry 201 (class 1259 OID 16398)
-- Name: discord_users; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.discord_users (
    user_id text NOT NULL,
    user_name text,
    ffn_id text,
    reads_slash bigint DEFAULT 0,
    current_list bigint DEFAULT 0,
    banned text DEFAULT 0,
    uuid text,
    forced_min_matches bigint DEFAULT 0,
    forced_ratio bigint DEFAULT 0,
    use_liked_authors_only bigint DEFAULT 0,
    use_fresh_sorting bigint DEFAULT 0,
    strict_fresh_sorting bigint DEFAULT 0,
    show_complete_only bigint DEFAULT 0,
    hide_dead bigint DEFAULT 0,
    similar_fics_id bigint DEFAULT 0,
    words_filter_range_begin bigint DEFAULT 0,
    words_filter_range_end bigint DEFAULT 0,
    dead_fic_days_range bigint DEFAULT 365,
    favourites_size bigint DEFAULT 0,
    last_large_list_generated text,
    last_large_list_counter bigint,
    words_filter_type bigint DEFAULT 0,
    sorting_mode bigint DEFAULT 0
);


ALTER TABLE public.discord_users OWNER TO socrates;

--
-- TOC entry 202 (class 1259 OID 16423)
-- Name: fandomindex; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.fandomindex (
    id bigint NOT NULL,
    name text NOT NULL,
    tracked bigint DEFAULT 0,
    updated timestamp without time zone
);


ALTER TABLE public.fandomindex OWNER TO socrates;

--
-- TOC entry 203 (class 1259 OID 16432)
-- Name: fandomurls; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.fandomurls (
    global_id bigint NOT NULL,
    url text NOT NULL,
    website text NOT NULL,
    custom text
);


ALTER TABLE public.fandomurls OWNER TO socrates;

--
-- TOC entry 204 (class 1259 OID 16440)
-- Name: ffn_pages; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.ffn_pages (
    page_id text NOT NULL,
    last_parse text,
    daily_parse_counter bigint DEFAULT 0,
    total_parse_counter bigint DEFAULT 0,
    favourites bigint DEFAULT 0
);


ALTER TABLE public.ffn_pages OWNER TO socrates;

--
-- TOC entry 205 (class 1259 OID 16451)
-- Name: fic_tags; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.fic_tags (
    user_id bigint NOT NULL,
    fic_id bigint NOT NULL,
    fic_tag text NOT NULL
);


ALTER TABLE public.fic_tags OWNER TO socrates;

--
-- TOC entry 206 (class 1259 OID 16459)
-- Name: filtered_fandoms; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.filtered_fandoms (
    user_id text NOT NULL,
    fandom_id bigint NOT NULL,
    including_crossovers bigint DEFAULT 0
);


ALTER TABLE public.filtered_fandoms OWNER TO socrates;

--
-- TOC entry 207 (class 1259 OID 16468)
-- Name: ignored_fandoms; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.ignored_fandoms (
    user_id text NOT NULL,
    fandom_id bigint NOT NULL,
    including_crossovers bigint DEFAULT 0
);


ALTER TABLE public.ignored_fandoms OWNER TO socrates;

--
-- TOC entry 208 (class 1259 OID 16477)
-- Name: list_sources; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.list_sources (
    user_id text NOT NULL,
    list_id bigint NOT NULL,
    fic_id bigint NOT NULL
);


ALTER TABLE public.list_sources OWNER TO socrates;

--
-- TOC entry 209 (class 1259 OID 16485)
-- Name: user_lists; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.user_lists (
    user_id text NOT NULL,
    list_id bigint NOT NULL,
    list_name text,
    list_type bigint,
    at_page bigint DEFAULT 0,
    min_match bigint DEFAULT 6,
    match_ratio bigint DEFAULT 50,
    always_at bigint DEFAULT 9999,
    generated timestamp without time zone
);


ALTER TABLE public.user_lists OWNER TO socrates;

--
-- TOC entry 210 (class 1259 OID 16497)
-- Name: user_settings; Type: TABLE; Schema: public; Owner: socrates
--

CREATE TABLE public.user_settings (
    name text,
    value text
);


ALTER TABLE public.user_settings OWNER TO socrates;

--
-- TOC entry 2886 (class 2606 OID 16397)
-- Name: discord_servers discord_servers_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.discord_servers
    ADD CONSTRAINT discord_servers_pkey PRIMARY KEY (server_id);


--
-- TOC entry 2889 (class 2606 OID 16422)
-- Name: discord_users discord_users_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.discord_users
    ADD CONSTRAINT discord_users_pkey PRIMARY KEY (user_id);


--
-- TOC entry 2895 (class 2606 OID 16431)
-- Name: fandomindex fandomindex_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.fandomindex
    ADD CONSTRAINT fandomindex_pkey PRIMARY KEY (id, name);


--
-- TOC entry 2901 (class 2606 OID 16439)
-- Name: fandomurls fandomurls_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.fandomurls
    ADD CONSTRAINT fandomurls_pkey PRIMARY KEY (global_id, url);


--
-- TOC entry 2904 (class 2606 OID 16450)
-- Name: ffn_pages ffn_pages_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.ffn_pages
    ADD CONSTRAINT ffn_pages_pkey PRIMARY KEY (page_id);


--
-- TOC entry 2906 (class 2606 OID 16458)
-- Name: fic_tags fic_tags_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.fic_tags
    ADD CONSTRAINT fic_tags_pkey PRIMARY KEY (user_id, fic_id, fic_tag);


--
-- TOC entry 2908 (class 2606 OID 16467)
-- Name: filtered_fandoms filtered_fandoms_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.filtered_fandoms
    ADD CONSTRAINT filtered_fandoms_pkey PRIMARY KEY (user_id, fandom_id);


--
-- TOC entry 2910 (class 2606 OID 16476)
-- Name: ignored_fandoms ignored_fandoms_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.ignored_fandoms
    ADD CONSTRAINT ignored_fandoms_pkey PRIMARY KEY (user_id, fandom_id);


--
-- TOC entry 2913 (class 2606 OID 16484)
-- Name: list_sources list_sources_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.list_sources
    ADD CONSTRAINT list_sources_pkey PRIMARY KEY (user_id, list_id, fic_id);


--
-- TOC entry 2916 (class 2606 OID 16496)
-- Name: user_lists user_lists_pkey; Type: CONSTRAINT; Schema: public; Owner: socrates
--

ALTER TABLE ONLY public.user_lists
    ADD CONSTRAINT user_lists_pkey PRIMARY KEY (user_id, list_id);


--
-- TOC entry 2890 (class 1259 OID 16505)
-- Name: I_FANDOMINDEX_ID; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_FANDOMINDEX_ID" ON public.fandomindex USING btree (id);


--
-- TOC entry 2891 (class 1259 OID 16506)
-- Name: I_FANDOMINDEX_NAME; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_FANDOMINDEX_NAME" ON public.fandomindex USING btree (name);


--
-- TOC entry 2892 (class 1259 OID 16507)
-- Name: I_FANDOMINDEX_PK; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_FANDOMINDEX_PK" ON public.fandomindex USING btree (id, name);


--
-- TOC entry 2893 (class 1259 OID 16508)
-- Name: I_FANDOMINDEX_UPDATED; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_FANDOMINDEX_UPDATED" ON public.fandomindex USING btree (updated);


--
-- TOC entry 2896 (class 1259 OID 16509)
-- Name: I_FURL_CUSTOM; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_FURL_CUSTOM" ON public.fandomurls USING btree (custom);


--
-- TOC entry 2897 (class 1259 OID 16510)
-- Name: I_FURL_ID; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_FURL_ID" ON public.fandomurls USING btree (global_id);


--
-- TOC entry 2898 (class 1259 OID 16511)
-- Name: I_FURL_URL; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_FURL_URL" ON public.fandomurls USING btree (url);


--
-- TOC entry 2899 (class 1259 OID 16512)
-- Name: I_FURL_WEBSITE; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_FURL_WEBSITE" ON public.fandomurls USING btree (website);


--
-- TOC entry 2911 (class 1259 OID 16514)
-- Name: I_LIST_SOURCES_PK; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_LIST_SOURCES_PK" ON public.list_sources USING btree (list_id, fic_id);


--
-- TOC entry 2902 (class 1259 OID 16513)
-- Name: I_PAGE_IDS; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_PAGE_IDS" ON public.ffn_pages USING btree (page_id);


--
-- TOC entry 2884 (class 1259 OID 16503)
-- Name: I_SERVER_ID; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_SERVER_ID" ON public.discord_servers USING btree (server_id);


--
-- TOC entry 2914 (class 1259 OID 16515)
-- Name: I_USER_LISTS_PK; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_USER_LISTS_PK" ON public.user_lists USING btree (user_id, list_id);


--
-- TOC entry 2887 (class 1259 OID 16504)
-- Name: I_discord_users; Type: INDEX; Schema: public; Owner: socrates
--

CREATE INDEX "I_discord_users" ON public.discord_users USING btree (user_id);


--
-- TOC entry 2917 (class 1259 OID 16516)
-- Name: sqlite_autoindex_user_settings_1; Type: INDEX; Schema: public; Owner: socrates
--

CREATE UNIQUE INDEX sqlite_autoindex_user_settings_1 ON public.user_settings USING btree (name);


--
-- TOC entry 2918 (class 1259 OID 16517)
-- Name: sqlite_autoindex_user_settings_2; Type: INDEX; Schema: public; Owner: socrates
--

CREATE UNIQUE INDEX sqlite_autoindex_user_settings_2 ON public.user_settings USING btree (value);


-- Completed on 2021-01-03 09:24:40 MSK

--
-- PostgreSQL database dump complete
--

