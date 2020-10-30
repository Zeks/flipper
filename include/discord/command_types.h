#pragma once

namespace discord {
    enum ECommandType{
            ct_none = 0,
            ct_fill_recommendations = 1,
            ct_display_page = 2,
            ct_ignore_fics = 4,
            ct_list = 5,
            ct_tag = 6,
            ct_set_identity = 7,
            ct_set_fandoms = 8,
            ct_ignore_fandoms = 9,
            ct_display_help = 10,
            ct_display_invalid_command = 12,
            ct_timeout_active = 13,
            ct_no_user_ffn = 14,
            ct_display_rng = 15,
            ct_change_server_prefix = 16,
            ct_insufficient_permissions = 17,
            ct_null_command = 18,
            ct_force_list_params = 19,
            ct_filter_liked_authors = 20,
            ct_filter_fresh = 21,
            ct_filter_out_dead= 22,
            ct_filter_complete = 23,
            ct_show_favs = 24,
            ct_purge = 25,
            ct_reset_filters = 26,
            ct_create_similar_fics_list = 27,
            ct_create_recs_from_mobile_page = 28,
            ct_set_wordcount_limit = 29,
        };

}