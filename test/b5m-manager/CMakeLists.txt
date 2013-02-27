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
  ${SQLITE3_LIBRARIES}
  ${MYSQL_LIBRARIES}
  ${LibCURL_LIBRARIES}
  ${ImageMagick_LIBRARIES}
  )
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

ADD_EXECUTABLE(b5m_matcher
b5m_matcher.cc
)
TARGET_LINK_LIBRARIES(b5m_matcher ${libs})
SET_TARGET_PROPERTIES(b5m_matcher PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)
ADD_EXECUTABLE(b5m_categorizing
b5m_categorizing.cc
)
TARGET_LINK_LIBRARIES(b5m_categorizing ${libs})
SET_TARGET_PROPERTIES(b5m_categorizing PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)

ADD_EXECUTABLE(image_upload
image_upload.cpp
)
TARGET_LINK_LIBRARIES(image_upload ${libs})
SET_TARGET_PROPERTIES(image_upload PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)


ADD_EXECUTABLE(b5m_psm
b5m_psm.cc
)
TARGET_LINK_LIBRARIES(b5m_psm ${libs})
SET_TARGET_PROPERTIES(b5m_psm PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)

ADD_EXECUTABLE(simhash_train_id
simhash_train_id.cc
)
TARGET_LINK_LIBRARIES(simhash_train_id ${libs})
SET_TARGET_PROPERTIES(simhash_train_id PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)
