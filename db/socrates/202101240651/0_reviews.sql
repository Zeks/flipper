CREATE TABLE public.user_reviews (
    user_id text NOT NULL,
    server_id text NOT NULL,
    review_id text NOT NULL PRIMARY KEY,
    content text,
    score integer, 
    reputation integer, 
    raw_url text,
    site_identifier text,
    date_added timestamp without time zone,
    site_type text
);


CREATE INDEX I_USER_REVIEWS_BY_USER ON public.user_reviews (user_id);
CREATE INDEX I_USER_REVIEWS_BY_URL ON public.user_reviews (raw_url);
CREATE INDEX I_USER_REVIEWS_BY_DATE ON public.user_reviews (date_added);
CREATE INDEX I_USER_REVIEWS_BY_SCORE ON public.user_reviews (score);
CREATE INDEX I_USER_REVIEWS_BY_REPUTATION ON public.user_reviews (reputation);
