INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}/core
  ${izenelib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
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
  sf1r_search_manager
  sf1r_document_manager
  sf1r_ranking_manager
  sf1r_la_manager
  sf1r_directory_manager

  sf1r_configuration_manager
  sf1r_index_manager
  sf1r_node_manager
  sf1r_common
  sf1r_net
  sf1r_aggregator_manager
  sf1r_bundle_index
  sf1r_bundle_mining

  ${ilplib_LIBRARIES}
  ${idmlib_LIBRARIES}
  ${imllib_LIBRARIES}
  ${izenecma_LIBRARIES}
  ${izenejma_LIBRARIES}
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
  ADD_EXECUTABLE(t_split_ustr
    Runner.cpp
    t_split_ustr.cpp
  )
  TARGET_LINK_LIBRARIES(t_split_ustr ${libs})
  SET_TARGET_PROPERTIES(t_split_ustr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_split_ustr")

  ADD_EXECUTABLE(t_GroupManager
    Runner.cpp
    t_GroupManager.cpp
    NumericPropertyTableBuilderStub.cpp
    GroupManagerTestFixture.cpp
  )
  TARGET_LINK_LIBRARIES(t_GroupManager ${libs})
  SET_TARGET_PROPERTIES(t_GroupManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_GroupManager")

  ADD_EXECUTABLE(t_AttrManager
    Runner.cpp
    t_AttrManager.cpp
  )
  TARGET_LINK_LIBRARIES(t_AttrManager ${libs})
  SET_TARGET_PROPERTIES(t_AttrManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_AttrManager")

  ADD_EXECUTABLE(t_PropIdTable
    Runner.cpp
    t_PropIdTable.cpp
  )
  TARGET_LINK_LIBRARIES(t_PropIdTable ${libs})
  SET_TARGET_PROPERTIES(t_PropIdTable PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_PropIdTable")

  ADD_EXECUTABLE(t_DateStrParser
    Runner.cpp
    t_DateStrParser.cpp
  )
  TARGET_LINK_LIBRARIES(t_DateStrParser ${libs})
  SET_TARGET_PROPERTIES(t_DateStrParser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_DateStrParser")

  ADD_EXECUTABLE(t_ProductScoreManager
    Runner.cpp
    t_ProductScoreManager.cpp
    ProductScoreManagerTestFixture.cpp
    RandomScorerFactory.cpp
    RandomScorer.cpp
  )
  TARGET_LINK_LIBRARIES(t_ProductScoreManager ${libs})
  SET_TARGET_PROPERTIES(t_ProductScoreManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(product_score "${SF1RENGINE_ROOT}/testbin/t_ProductScoreManager")

  ADD_EXECUTABLE(t_ProductScoreAverage
    Runner.cpp
    t_ProductScoreAverage.cpp
    ProductScoreAverageTestFixture.cpp
  )
  TARGET_LINK_LIBRARIES(t_ProductScoreAverage ${libs})
  SET_TARGET_PROPERTIES(t_ProductScoreAverage PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(product_score "${SF1RENGINE_ROOT}/testbin/t_ProductScoreAverage")

  ADD_EXECUTABLE(t_ProductRanker
    Runner.cpp
    t_ProductRanker.cpp
    ProductRankerTestFixture.cpp)
  TARGET_LINK_LIBRARIES(t_ProductRanker ${libs})
  SET_TARGET_PROPERTIES(t_ProductRanker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin)
  ADD_TEST(product_score "${SF1RENGINE_ROOT}/testbin/t_ProductRanker")

  ADD_EXECUTABLE(t_TrieProductTokenizer
    Runner.cpp
    t_TrieProductTokenizer.cpp
    )
  TARGET_LINK_LIBRARIES(t_TrieProductTokenizer ${libs})

  ADD_EXECUTABLE(t_ad_ctr
    t_ad_ctr.cpp
  )
  TARGET_LINK_LIBRARIES(t_ad_ctr ${libs})
  SET_TARGET_PROPERTIES(t_ad_ctr PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  #ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_ad_ctr")

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)

