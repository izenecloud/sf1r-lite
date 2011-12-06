INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core/
  ${CMAKE_SOURCE_DIR}

  ${izenelib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${iise_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  ${Thrift_INCLUDE_DIR}
)

FILE(GLOB_RECURSE configuration_SRC "configuration-manager/*.cpp")
FILE(GLOB_RECURSE la_SRC "la-manager/*.cpp")
FILE(GLOB_RECURSE license_SRC "license-manager/*.cpp")
FILE(GLOB_RECURSE ranking_SRC "ranking-manager/*.cpp")
FILE(GLOB_RECURSE common_SRC "common/*.cpp")
FILE(GLOB_RECURSE index_SRC "index-manager/*.cpp")
FILE(GLOB_RECURSE log_SRC "log-manager/*.cpp")
FILE(GLOB_RECURSE directory_SRC "directory-manager/*.cpp")
FILE(GLOB_RECURSE search_SRC "search-manager/*.cpp")
FILE(GLOB_RECURSE query_SRC "query-manager/*.cpp")
FILE(GLOB_RECURSE recommend_SRC "recommend-manager/*.cpp")
FILE(GLOB_RECURSE aggregator_SRC "aggregator-manager/*.cpp")
FILE(GLOB_RECURSE node_SRC "node-manager/*.cpp")
FILE(GLOB_RECURSE product_SRC "product-manager/*.cc")
# FILE(GLOB product_SRC
#   "product-manager/collection_product_data_source.cc"
#   "product-manager/collection_product_data_source.cc"
#   "product-manager/collection_product_data_source.cc"
#   "product-manager/collection_product_data_source.cc"
#   "product-manager/collection_product_data_source.cc"
#   "product-manager/collection_product_data_source.cc"
#   )


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
  "mining-manager/taxonomy-generation-submanager/*.cpp"
  "mining-manager/duplicate-detection-submanager/*.cpp"
  "mining-manager/duplicate-detection-submanager/*.cc"
  "mining-manager/query-recommend-submanager/*.cpp"
  "mining-manager/similarity-detection-submanager/*.cpp"
#  "mining-manager/classification-submanager/*.cc"
#  "mining-manager/classification-submanager/*.cpp"
#  "mining-manager/ise-submanager/*.cpp"
  "mining-manager/query-correction-submanager/*.cpp"
  "mining-manager/auto-fill-submanager/*.cpp"
  "mining-manager/faceted-submanager/*.cc"
  "mining-manager/group-label-logger/*.cpp"
  "mining-manager/ec-submanager/*.cc"
  "mining-manager/summarization-submanager/*.cpp"
  "mining-manager/summarization-submanager/svd/*.c"
)

ADD_DEFINITIONS("-fno-strict-aliasing")
ADD_LIBRARY(sf1r_configuration_manager ${configuration_SRC})
ADD_LIBRARY(sf1r_la_manager ${la_SRC})
ADD_LIBRARY(sf1r_license_manager ${license_SRC})
ADD_LIBRARY(sf1r_ranking_manager ${ranking_SRC})
ADD_LIBRARY(sf1r_common ${common_SRC})
ADD_LIBRARY(sf1r_index_manager ${index_SRC} )
ADD_LIBRARY(sf1r_log_manager ${log_SRC})
ADD_LIBRARY(sf1r_directory_manager ${directory_SRC})
ADD_LIBRARY(sf1r_query_manager ${query_SRC})
ADD_LIBRARY(sf1r_search_manager ${search_SRC})
ADD_LIBRARY(sf1r_mining_manager ${mining_SRC})
ADD_LIBRARY(sf1r_document_manager ${document_SRC})
ADD_LIBRARY(sf1r_recommend_manager ${recommend_SRC})
ADD_LIBRARY(sf1r_aggregator_manager ${aggregator_SRC})
ADD_LIBRARY(sf1r_node_manager ${node_SRC})
ADD_LIBRARY(sf1r_product_manager ${product_SRC})

INSTALL(TARGETS
    sf1r_configuration_manager
    sf1r_la_manager
    sf1r_license_manager
    sf1r_ranking_manager
    sf1r_common
    sf1r_index_manager
    sf1r_log_manager
    sf1r_directory_manager
    sf1r_query_manager
    sf1r_search_manager
    sf1r_mining_manager
    sf1r_document_manager
    sf1r_recommend_manager
    sf1r_aggregator_manager
    sf1r_node_manager
    sf1r_product_manager
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib
    COMPONENT sf1r_libraries)
