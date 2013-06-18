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
  sf1r_b5m_manager
  sf1r_log_manager
  sf1r_query_manager
  sf1r_product_manager
  sf1r_la_manager
  sf1r_configuration_manager
  sf1r_mining_manager
  sf1r_b5m_manager
  sf1r_log_manager
  sf1r_query_manager
  sf1r_product_manager
  sf1r_la_manager

  sf1r_directory_manager
  sf1r_license_manager
  sf1r_search_manager
  sf1r_document_manager
  sf1r_index_manager
  sf1r_ranking_manager
  sf1r_node_manager
  sf1r_common
  sf1r_image_manager
  sf1r_net
  
  sf1r_bundle_index
  sf1r_aggregator_manager
  sf1r_bundle_mining
  sf1r_bundle_recommend
  sf1r_recommend_manager

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
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )

ADD_EXECUTABLE(b5m_matcher
b5m_matcher.cc
)
TARGET_LINK_LIBRARIES(b5m_matcher  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(b5m_matcher PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)
ADD_EXECUTABLE(categorize_test
categorize_test.cc
)
TARGET_LINK_LIBRARIES(categorize_test  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(categorize_test PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin/b5m
)
ADD_EXECUTABLE(spu_match_test
spu_match_test.cc
)
TARGET_LINK_LIBRARIES(spu_match_test  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(spu_match_test PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin/b5m
)
ADD_EXECUTABLE(t_commentdb
Runner.cpp
t_commentdb.cc
)
TARGET_LINK_LIBRARIES(t_commentdb  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(t_commentdb PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)


ADD_EXECUTABLE(b5m_categorizing
b5m_categorizing.cc
)
TARGET_LINK_LIBRARIES(b5m_categorizing  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(b5m_categorizing PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)

ADD_EXECUTABLE(b5m_search_keyword
b5m_search_keyword.cc
)
TARGET_LINK_LIBRARIES(b5m_search_keyword  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(b5m_search_keyword PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)

ADD_EXECUTABLE(b5m_category_mapper
b5m_category_mapper.cc
)
TARGET_LINK_LIBRARIES(b5m_category_mapper  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(b5m_category_mapper PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/b5m
)
ADD_EXECUTABLE(t_offerdb
Runner.cpp
t_offerdb.cc
)
TARGET_LINK_LIBRARIES(t_offerdb  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(t_offerdb PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)

ADD_EXECUTABLE(t_offer_db_recorder
Runner.cpp
t_offer_db_recorder.cc
)
TARGET_LINK_LIBRARIES(t_offer_db_recorder  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(t_offer_db_recorder PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)

ADD_EXECUTABLE(t_b5mo_processor
Runner.cpp
t_b5mo_processor.cc
)
TARGET_LINK_LIBRARIES(t_b5mo_processor  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(t_b5mo_processor PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)

ADD_EXECUTABLE(product-matcher-test
b5m_product_matcher.cc
)
TARGET_LINK_LIBRARIES(product-matcher-test ${libs})
SET_TARGET_PROPERTIES(product-matcher-test PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)
ADD_EXECUTABLE(t_product_db
Runner.cpp
t_product_db.cc
)
TARGET_LINK_LIBRARIES(t_product_db  ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}  ${libs})
SET_TARGET_PROPERTIES(t_product_db PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
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

ADD_TEST(t_commentdb "${SF1RENGINE_ROOT}/testbin/t_commentdb")
ADD_TEST(t_offerdb "${SF1RENGINE_ROOT}/testbin/t_offerdb")
ADD_TEST(t_offer_db_recorder "${SF1RENGINE_ROOT}/testbin/t_offer_db_recorder")
ADD_TEST(t_product_db "${SF1RENGINE_ROOT}/testbin/t_product_db")
#ADD_TEST(commentdb "${SF1RENGINE_ROOT}/testbin/t_commentdb")
#ADD_TEST(commentdb "${SF1RENGINE_ROOT}/testbin/t_commentdb")

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
