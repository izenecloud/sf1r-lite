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

  ADD_EXECUTABLE(t_AutofillManager
    Runner.cpp
    t_AutofillManager.cpp
  )
  TARGET_LINK_LIBRARIES(t_AutofillManager ${libs})
  SET_TARGET_PROPERTIES(t_AutofillManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_AutofillManager")

  ADD_EXECUTABLE(t_GroupLabelLogger
    Runner.cpp
    t_GroupLabelLogger.cpp
    GroupLabelLoggerTestFixture.cpp
  )
  TARGET_LINK_LIBRARIES(t_GroupLabelLogger ${libs})
  SET_TARGET_PROPERTIES(t_GroupLabelLogger PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_GroupLabelLogger")

  ADD_EXECUTABLE(t_ec_manager
    Runner.cpp
    t_ec_manager.cc
  )
  TARGET_LINK_LIBRARIES(t_ec_manager ${libs})
  SET_TARGET_PROPERTIES(t_ec_manager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_ec_manager")

  ADD_EXECUTABLE(t_MerchantScoreManager
    Runner.cpp
    t_MerchantScoreManager.cpp
    MerchantScoreManagerTestFixture.cpp
  )
  TARGET_LINK_LIBRARIES(t_MerchantScoreManager ${libs})
  SET_TARGET_PROPERTIES(t_MerchantScoreManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_MerchantScoreManager")

  ADD_EXECUTABLE(t_PropIdTable
    Runner.cpp
    t_PropIdTable.cpp
  )
  TARGET_LINK_LIBRARIES(t_PropIdTable ${libs})
  SET_TARGET_PROPERTIES(t_PropIdTable PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_PropIdTable")

  ADD_EXECUTABLE(t_fcontainer_febird
    Runner.cpp
    t_fcontainer_febird.cpp
    FContainerTestFixture.cpp
  )
  TARGET_LINK_LIBRARIES(t_fcontainer_febird ${libs})
  SET_TARGET_PROPERTIES(t_fcontainer_febird PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_fcontainer_febird")

  ADD_EXECUTABLE(t_DateStrParser
    Runner.cpp
    t_DateStrParser.cpp
  )
  TARGET_LINK_LIBRARIES(t_DateStrParser ${libs})
  SET_TARGET_PROPERTIES(t_DateStrParser PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_DateStrParser")

  ADD_EXECUTABLE(t_DateGroupTable
    Runner.cpp
    t_DateGroupTable.cpp
  )
  TARGET_LINK_LIBRARIES(t_DateGroupTable ${libs})
  SET_TARGET_PROPERTIES(t_DateGroupTable PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_DateGroupTable")

  ADD_EXECUTABLE(t_CustomRankManager
    Runner.cpp
    t_CustomRankManager.cpp
  )
  TARGET_LINK_LIBRARIES(t_CustomRankManager ${libs})
  SET_TARGET_PROPERTIES(t_CustomRankManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(group "${SF1RENGINE_ROOT}/testbin/t_CustomRankManager")

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

  ADD_EXECUTABLE(t_NumericPropertyScorer
    Runner.cpp
    t_NumericPropertyScorer.cpp
    NumericPropertyScorerTestFixture.cpp
  )
  TARGET_LINK_LIBRARIES(t_NumericPropertyScorer ${libs})
  SET_TARGET_PROPERTIES(t_NumericPropertyScorer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(product_score "${SF1RENGINE_ROOT}/testbin/t_NumericPropertyScorer")

  ADD_EXECUTABLE(t_ProductRanker
    Runner.cpp
    t_ProductRanker.cpp
    ProductRankerTestFixture.cpp)
  TARGET_LINK_LIBRARIES(t_ProductRanker ${libs})
  SET_TARGET_PROPERTIES(t_ProductRanker PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin)
  ADD_TEST(product_score "${SF1RENGINE_ROOT}/testbin/t_ProductRanker")

  ADD_EXECUTABLE(t_ProductTokenizer
    Runner.cpp
    t_ProductTokenizer.cpp
    )
  TARGET_LINK_LIBRARIES(t_ProductTokenizer ${libs})

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)

INCLUDE_DIRECTORIES(
${Boost_INCLUDE_DIRS}
)
ADD_EXECUTABLE(dd_group_gen
dd_group_gen.cc
)
TARGET_LINK_LIBRARIES(dd_group_gen ${libs})
SET_TARGET_PROPERTIES(dd_group_gen PROPERTIES
RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
)
