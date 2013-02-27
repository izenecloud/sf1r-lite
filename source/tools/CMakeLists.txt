INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/core/
  ${CMAKE_SOURCE_DIR}/bundles/
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/common

  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${izenelib_INCLUDE_DIRS}
  ${iise_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  ${xml2_INCLUDE_DIRS}
  ${sqlite3_INCLUDE_DIRS}
  ${Thrift_INCLUDE_DIR}
  )

ADD_EXECUTABLE(LicenseFileGenerator
  license-file-generator/LicenseFileGenerator.cpp
  )

TARGET_LINK_LIBRARIES(LicenseFileGenerator
  sf1r_configuration_manager
  sf1r_la_manager
  sf1r_query_manager
  sf1r_log_manager
  sf1r_license_manager
  ${izenelib_LIBRARIES}
  ${Boost_LIBRARIES}
  sf1r_product_manager
  sf1r_search_manager
  sf1r_directory_manager
  sf1r_mining_manager
  sf1r_index_manager
  sf1r_ranking_manager
  sf1r_document_manager
  sf1r_node_manager
  sf1r_common
  sf1r_b5m_manager
  sf1r_image_manager
  sf1r_net
  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${ilplib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${xml2_LIBRARIES}

  #external
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )

ADD_EXECUTABLE(CustLicenseFileGenerator
  license-file-generator/CustLicenseFileGenerator.cpp
  )

TARGET_LINK_LIBRARIES(CustLicenseFileGenerator
  sf1r_configuration_manager
  sf1r_la_manager
  sf1r_query_manager
  sf1r_log_manager
  sf1r_license_manager
  ${izenelib_LIBRARIES}
  ${Boost_LIBRARIES}
  sf1r_product_manager
  sf1r_search_manager
  sf1r_directory_manager
  sf1r_mining_manager
  sf1r_index_manager
  sf1r_ranking_manager
  sf1r_document_manager
  sf1r_node_manager
  sf1r_common
  sf1r_b5m_manager
  sf1r_image_manager
  sf1r_net
  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${ilplib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${xml2_LIBRARIES}

  #external
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )

ADD_EXECUTABLE(CassandraTool
  cassandra/CassandraTools.cpp
  )

SET(EXTERNAL_LIBS
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  ${xml2_LIBRARIES}
  )

SET(IZENE_LIBS
  ${idmlib_LIBRARIES}
  ${ilplib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${imllib_LIBRARIES}
  ${izenelib_LIBRARIES}
  )

SET(SF1_LIBS
  sf1r_search_manager
  sf1r_ranking_manager
  sf1r_query_manager
  sf1r_mining_manager
  sf1r_log_manager
  sf1r_product_manager
  sf1r_node_manager
  sf1r_license_manager
  sf1r_la_manager

  # duplicated in order to solve linker error
  sf1r_search_manager
  sf1r_mining_manager
  sf1r_ranking_manager
  sf1r_query_manager
  sf1r_b5m_manager
  sf1r_image_manager
  sf1r_net

  sf1r_index_manager
  sf1r_document_manager
  sf1r_directory_manager
  sf1r_configuration_manager
  sf1r_common
  )

TARGET_LINK_LIBRARIES(CassandraTool
  ${SF1_LIBS}
  ${IZENE_LIBS}
  ${EXTERNAL_LIBS}
  )

SET_TARGET_PROPERTIES(CassandraTool PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/tool
  )
