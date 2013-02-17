INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/core
  ${idmlib_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  ${izenelib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${ImageMagick_INCLUDE_DIRS}
  "/usr/local/include/ImageMagick"
  )
MESSAGE(STATUE "include path: ${INCLUDE_DIRECTORIES}")
SET(libs
  sf1r_b5m_manager
  sf1r_mining_manager
  sf1r_search_manager
  sf1r_query_manager
  sf1r_product_manager
  sf1r_node_manager
  sf1r_ranking_manager
  sf1r_document_manager
  sf1r_directory_manager
  sf1r_log_manager
  sf1r_license_manager
  sf1r_la_manager
  sf1r_configuration_manager
  sf1r_index_manager
  sf1r_common
  sf1r_image_manager
  sf1r_net
  ${ilplib_LIBRARIES}
  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${wisekma_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
  ${iise_LIBRARIES}
  ${izenelib_LIBRARIES}
  #external
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${xml2_LIBRARIES}
  ${sqlite3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
)
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)
FIND_PACKAGE(ImageMagick COMPONENTS Magick++)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )

#  FILE(GLOB t_node_SRC
#    Runner.cpp
#    t_node_manager.cpp
#  )
  ADD_EXECUTABLE(t_osaca_test
    t_osaca_test.cpp
    )
    TARGET_LINK_LIBRARIES(t_osaca_test
    ${libs}
    )

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
