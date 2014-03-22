INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core/
  ${CMAKE_SOURCE_DIR}

  ${izenelib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  ${Thrift_INCLUDE_DIR}
)

FILE(GLOB_RECURSE configuration_SRC "configuration-manager/*.cpp")
FILE(GLOB_RECURSE la_SRC "la-manager/*.cpp")
FILE(GLOB_RECURSE ranking_SRC "ranking-manager/*.cpp")
FILE(GLOB_RECURSE common_SRC "common/*.cpp")
FILE(GLOB_RECURSE log_SRC "log-manager/*.cpp")
FILE(GLOB_RECURSE directory_SRC "directory-manager/*.cpp")
FILE(GLOB_RECURSE search_SRC "search-manager/*.cpp")
FILE(GLOB_RECURSE query_SRC "query-manager/*.cpp")
FILE(GLOB_RECURSE aggregator_SRC "aggregator-manager/*.cpp")
FILE(GLOB_RECURSE node_SRC "node-manager/*.cpp")
FILE(GLOB_RECURSE product_SRC "product-manager/*.cc")
FILE(GLOB b5m_SRC "b5m-manager/*.cc")
FILE(GLOB sf1r_net_SRC "sf1r-net/*.cpp")

FILE(GLOB index_SRC
  "index-manager/zambezi-manager/*.cpp"
  "index-manager/*.cpp"
  "index-manager/zambezi-tokenizer/*.cpp")

FILE(GLOB document_SRC
  "document-manager/*.cpp"
  "document-manager/snippet-generation-submanager/*.cpp"
  "document-manager/text-summarization-submanager/*.cpp"
  "document-manager/text-summarization-submanager/text-summarization/*.cpp"
  "document-manager/text-summarization-submanager/text-summarization/*.C"
  "document-manager/highlighter/*.cpp"
)

FILE(GLOB mining_SRC
  "mining-manager/*.cpp"
  "mining-manager/*.cc"
  "mining-manager/util/*.cpp"
  "mining-manager/faceted-submanager/*.cc"
  "mining-manager/group-manager/*.cpp"
  "mining-manager/attr-manager/*.cpp"
  "mining-manager/group-label-logger/*.cpp"
  "mining-manager/merchant-score-manager/*.cpp"
  "mining-manager/product-scorer/*.cpp"
  "mining-manager/product-score-manager/*.cpp"
  "mining-manager/product-ranker/*.cpp"
  "mining-manager/product-forward/*.cpp"
  "mining-manager/product-tokenizer/*.cpp"
  "mining-manager/custom-rank-manager/*.cpp"
  "mining-manager/suffix-match-manager/*.cpp"
  "mining-manager/query-intent/*.cpp"
  "mining-manager/query-abbreviation/*.cpp"
  "mining-manager/query-recommendation/*.cpp"
  "mining-manager/query-recommendation/pinyin/*.cpp"
  "mining-manager/query-recommendation/parser/*.cpp"
  "mining-manager/query-recommendation/tokenize/*.cpp"
  "mining-manager/query-recommendation/evaluate/*.cpp"
  "mining-manager/category-classify/*.cpp"
  "mining-manager/title-scorer/*.cpp"
  "mining-manager/ad-index-manager/*.cpp"
)

ADD_DEFINITIONS("-fno-strict-aliasing")
ADD_LIBRARY(sf1r_configuration_manager ${configuration_SRC})
ADD_LIBRARY(sf1r_la_manager ${la_SRC})
ADD_LIBRARY(sf1r_ranking_manager ${ranking_SRC})
ADD_LIBRARY(sf1r_common ${common_SRC})
ADD_LIBRARY(sf1r_index_manager ${index_SRC} )
ADD_LIBRARY(sf1r_log_manager ${log_SRC})
ADD_LIBRARY(sf1r_directory_manager ${directory_SRC})
ADD_LIBRARY(sf1r_query_manager ${query_SRC})
ADD_LIBRARY(sf1r_search_manager ${search_SRC})
ADD_LIBRARY(sf1r_mining_manager ${mining_SRC})
ADD_LIBRARY(sf1r_document_manager ${document_SRC})
ADD_LIBRARY(sf1r_aggregator_manager ${aggregator_SRC})
ADD_LIBRARY(sf1r_node_manager ${node_SRC})
ADD_LIBRARY(sf1r_product_manager ${product_SRC})
ADD_LIBRARY(sf1r_b5m_manager ${b5m_SRC})
ADD_LIBRARY(sf1r_net ${sf1r_net_SRC})

INSTALL(TARGETS
    sf1r_configuration_manager
    sf1r_la_manager
    sf1r_ranking_manager
    sf1r_common
    sf1r_index_manager
    sf1r_log_manager
    sf1r_directory_manager
    sf1r_query_manager
    sf1r_search_manager
    sf1r_mining_manager
    sf1r_document_manager
    sf1r_aggregator_manager
    sf1r_node_manager
    sf1r_product_manager
    sf1r_b5m_manager
    sf1r_net
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    COMPONENT sf1r_libraries)
