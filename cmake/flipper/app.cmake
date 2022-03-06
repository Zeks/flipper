set(FLIPPER_SOURCES
    "include/core/db_entity.h"
    "include/core/experimental/fic_relations.h"
    "include/core/fandom.h"
    "include/core/fanfic.h"
    "include/core/fav_list_details.h"
    "include/core/identity.h"
    "include/core/slash_data.h"
    "include/core/url.h"
    "include/pagegetter.h"
    "include/parsers/ffn/desktop_favparser.h"
    "include/parsers/ffn/favparser_wrapper.h"
    "include/parsers/ffn/mobile_favparser.h"
    "src/Interfaces/fandom_lists.cpp"
    "src/core/fandom.cpp"
    "src/core/fanfic.cpp"
    "src/core/fav_list_details.cpp"
    "src/pagegetter.cpp"
    "src/parsers/ffn/desktop_favparser.cpp"
    "src/parsers/ffn/favparser_wrapper.cpp"
    "src/parsers/ffn/mobile_favparser.cpp"
    "include/backups.h"
    "src/backups.cpp"
    "forms.qrc"
    "icons.qrc"
    "include/Interfaces/authors.h"
    "include/Interfaces/base.h"
    "include/Interfaces/db_interface.h"
    "include/Interfaces/interface_sqlite.h"
    "include/Interfaces/fandoms.h"
    "include/Interfaces/fanfics.h"
    "include/Interfaces/ffn/ffn_fanfics.h"
    "include/Interfaces/ffn/ffn_authors.h"
    "include/Interfaces/genres.h"
    "include/Interfaces/pagetask_interface.h"
    "include/Interfaces/recommendation_lists.h"
    "include/Interfaces/tags.h"
    "include/container_utils.h"
    "include/generic_utils.h"
    "include/pageconsumer.h"
    "include/pagetask.h"
    "include/db_fixers.h"
    "include/parsers/ffn/fandomparser.h"
    "include/parsers/ffn/ffnparserbase.h"
    "include/parsers/ffn/ficparser.h"
    "include/parsers/ffn/fandomindexparser.h"
    "include/qml/imageprovider.h"
    "include/tasks/author_cache_reprocessor.h"
    "include/tasks/fandom_task_processor.h"
    "include/tasks/author_task_processor.h"
    "include/timeutils.h"
    "include/webpage.h"
    "src/generic_utils.cpp"
    "src/parsers/ffn/fandomindexparser.cpp"
    "include/parse.h"
    "include/querybuilder.h"
    "include/queryinterfaces.h"
    "include/core/section.h"
    "include/service_functions.h"
    "include/storyfilter.h"
    "include/transaction.h"
    "include/url_utils.h"
    "qml_ficmodel.cpp"
    "qml_ficmodel.h"
    "src/qml/imageprovider.cpp"
    "src/servers/token_processing.cpp"
    "src/tasks/author_cache_reprocessor.cpp"
    "src/tasks/fandom_task_processor.cpp"
    "src/tasks/author_task_processor.cpp"
    "src/pageconsumer.cpp"
    "src/Interfaces/authors.cpp"
    "src/Interfaces/base.cpp"
    "src/Interfaces/db_interface.cpp"
    "src/Interfaces/interface_sqlite.cpp"
    "src/Interfaces/fandoms.cpp"
    "src/Interfaces/fanfics.cpp"
    "src/Interfaces/ffn/ffn_fanfics.cpp"
    "src/Interfaces/ffn/ffn_authors.cpp"
    "src/Interfaces/genres.cpp"
    "src/Interfaces/pagetask_interface.cpp"
    "src/Interfaces/recommendation_lists.cpp"
    "src/Interfaces/tags.cpp"
    "src/pagetask.cpp"
    "src/db_fixers.cpp"
    "src/sqlcontext.cpp"
    "src/parsers/ffn/fandomparser.cpp"
    "src/parsers/ffn/ffnparserbase.cpp"
    "src/parsers/ffn/ficparser.cpp"
    "src/main_flipper.cpp"
    "src/ui/mainwindow_utility.cpp"
    "src//webview/pagegetter_w.cpp"
    "include/webview/pagegetter_w.h"
    "src/pure_sql.cpp"
    "src/querybuilder.cpp"
    "src/regex_utils.cpp"
    "src/core/section.cpp"
    "src/service_functions.cpp"
    "src/sqlitefunctions.cpp"
    "src/storyfilter.cpp"
    "src/transaction.cpp"
    "src/url_utils.cpp"
    "src/page_utils.cpp"
    "include/page_utils.h"
    "src/environment.cpp"
    "include/environment.h"
    "include/statistics_utils.h"
    "src/tasks/author_stats_processor.cpp"
    "include/tasks/author_stats_processor.h"
    "src/tasks/slash_task_processor.cpp"
    "include/tasks/slash_task_processor.h"
    "src/tasks/humor_task_processor.cpp"
    "include/tasks/humor_task_processor.h"
    "src/tasks/recommendations_reload_precessor.cpp"
    "include/tasks/recommendations_reload_precessor.h"
    "include/grpc/grpc_source.h"
    "include/Interfaces/data_source.h"
    "src/grpc/grpc_source.cpp"
    "src/Interfaces/data_source.cpp"
    "include/in_tag_accessor.h"
    "src/in_tag_accessor.cpp"
    "src/rng.cpp"
    "include/rng.h"
    "include/core/author.h"
    "src/core/author.cpp"
    "include/core/recommendation_list.h"
    "src/core/recommendation_list.cpp"
    )
set(UI_SOURCES
    "src/ui/fandomlistwidget.cpp"
    "include/ui/fandomlistwidget.h"
    "UI/fandomlistwidget.ui"
    "UI/mainwindow.ui"
    "include/ui/mainwindow.h"
    "src/ui/mainwindow.cpp"
    "UI/tagwidget.ui"
    "include/ui/tagwidget.h"
    "src/ui/tagwidget.cpp"
    "UI/fanficdisplay.ui"
    "include/ui/fanficdisplay.h"
    "src/ui/fanficdisplay.cpp"
    "UI/actionprogress.ui"
    "src/ui/actionprogress.cpp"
    "include/ui/actionprogress.h"
    "UI/initialsetupdialog.ui"
    "src/ui/initialsetupdialog.cpp"
    "include/ui/initialsetupdialog.h"
    "UI/welcomedialog.ui"
    "src/ui/welcomedialog.cpp"
    "include/ui/welcomedialog.h")

set(SQL_SOURCES
     $ENV{SQLITE_FOLDER}/sqlite3.c
     $ENV{SQLITE_FOLDER}/sqlite3.h
    )

set(QR_SOURCES
    "third_party/qr/BitBuffer.cpp"
    "third_party/qr/BitBuffer.hpp"
    "third_party/qr/QrCode.cpp"
    "third_party/qr/QrCode.hpp"
    "third_party/qr/QrSegment.cpp"
    "third_party/qr/QrSegment.hpp")

if(NOT EXISTS $ENV{SQLITE_FOLDER}/sqlite3.c)
    message(FATAL_ERROR "Could not find sqlite source files")
    return()
endif()

target_include_directories(flipper PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_CURRENT_SOURCE_DIR}/../../third_party
    ${CMAKE_CURRENT_SOURCE_DIR}/../../libs
    ${CMAKE_CURRENT_SOURCE_DIR}/../../proto
    ${CMAKE_CURRENT_SOURCE_DIR}/../..//third_party/fmt/include)

function(add_sources ABSOLUTE_PATH)
    set(REPOSITORY_ROOT ${CMAKE_CURRENT_LIST_DIR}/../..)
    foreach(file ${ARGN})
        if(ABSOLUTE_PATH)
            target_sources(flipper PRIVATE ${file})
        else()
            target_sources(flipper PRIVATE ${REPOSITORY_ROOT}/${file})
        endif()
    endforeach()
endfunction()

add_sources(0 ${FLIPPER_SOURCES})
add_sources(0 ${UI_SOURCES})
add_sources(0 ${QR_SOURCES})
add_sources(1 ${SQL_SOURCES})

install(TARGETS flipper DESTINATION ${CMAKE_CURRENT_LIST_DIR}/../../debug)
