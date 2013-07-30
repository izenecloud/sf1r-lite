INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core
  ${izenelib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${izenejma_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  )

# sequences is important for some linker
# if a dpendes b, a must precede b
SET(libs

  sf1r_mining_manager
  sf1r_query_manager
  sf1r_log_manager
  sf1r_product_manager
  sf1r_search_manager
  sf1r_document_manager
  sf1r_ranking_manager
  sf1r_la_manager
  sf1r_directory_manager

  sf1r_license_manager
  sf1r_configuration_manager
  sf1r_index_manager
  sf1r_node_manager
  sf1r_common
  sf1r_b5m_manager
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
  ${XML2_LIBRARIES}
  ${SQLITE3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )

INCLUDE_DIRECTORIES(
${Boost_INCLUDE_DIRS}
)
ADD_EXECUTABLE(unsupervised_wordseg
unsupervised_wordseg.cc
)
TARGET_LINK_LIBRARIES(unsupervised_wordseg ${libs}  -lfst -ldl)
SET_TARGET_PROPERTIES(unsupervised_wordseg PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)

ADD_EXECUTABLE(xinhaoDectect
xinhaoDectect.cc
)
TARGET_LINK_LIBRARIES(xinhaoDectect ${libs})
SET_TARGET_PROPERTIES(xinhaoDectect PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)



ADD_EXECUTABLE(parseScd
parseScd.cpp
)
TARGET_LINK_LIBRARIES(parseScd ${libs})
SET_TARGET_PROPERTIES(parseScd PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)
