INCLUDE_DIRECTORIES(		 
  ${CMAKE_SOURCE_DIR}/core
  ${izenelib_INCLUDE_DIRS}
  ${ilplib_INCLUDE_DIRS}
  ${idmlib_INCLUDE_DIRS}
  ${imllib_INCLUDE_DIRS}
  ${wisekma_INCLUDE_DIRS}
  ${izenecma_INCLUDE_DIRS}
  ${Boost_INCLUDE_DIRS}
  ${TokyoCabinet_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
  )

# sequences is important for some linker
# if a dpendes b, a must precede b
SET(libs

  #sf1r_query_manager
  #sf1r_search_manager
  #sf1r_ranking_manager
  #sf1r_document_manager
  #sf1r_mining_manager
  #sf1r_directory_manager

  #sf1r_log_manager
  #sf1r_license_manager
  #sf1r_la_manager
  #sf1r_configuration_manager
  #sf1r_index_manager
  #sf1r_common
  sf1r_recommend_manager

  #${ilplib_LIBRARIES}
  #${idmlib_LIBRARIES}
  #${imllib_LIBRARIES}
  #${wisekma_LIBRARIES}
  #${izenecma_LIBRARIES}
  #${iise_LIBRARIES}
  ${izenelib_LIBRARIES}

  #external
  ${Boost_LIBRARIES}
  ${TokyoCabinet_LIBRARIES}
  ${Glog_LIBRARIES}
  ${xml2_LIBRARIES}
  ${sqlite3_LIBRARIES}
  )
SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )

  ADD_EXECUTABLE(t_UserManager
    Runner.cpp
    t_UserManager.cpp
  )
  TARGET_LINK_LIBRARIES(t_UserManager ${libs})
  SET_TARGET_PROPERTIES(t_UserManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(recommend-manager "${SF1RENGINE_ROOT}/testbin/t_UserManager")

  ADD_EXECUTABLE(t_ItemManager
    Runner.cpp
    t_ItemManager.cpp
  )
  TARGET_LINK_LIBRARIES(t_ItemManager ${libs})
  SET_TARGET_PROPERTIES(t_ItemManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(recommend-manager "${SF1RENGINE_ROOT}/testbin/t_ItemManager")

  ADD_EXECUTABLE(t_VisitManager
    Runner.cpp
    t_VisitManager.cpp
  )
  TARGET_LINK_LIBRARIES(t_VisitManager ${libs})
  SET_TARGET_PROPERTIES(t_VisitManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(recommend-manager "${SF1RENGINE_ROOT}/testbin/t_VisitManager")

  ADD_EXECUTABLE(t_PurchaseManager
    Runner.cpp
    t_PurchaseManager.cpp
  )
  TARGET_LINK_LIBRARIES(t_PurchaseManager ${libs})
  SET_TARGET_PROPERTIES(t_PurchaseManager PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(recommend-manager "${SF1RENGINE_ROOT}/testbin/t_PurchaseManager")

  ADD_EXECUTABLE(t_IDGenerator
    Runner.cpp
    t_IDGenerator.cpp
  )
  TARGET_LINK_LIBRARIES(t_IDGenerator ${libs})
  SET_TARGET_PROPERTIES(t_IDGenerator PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${SF1RENGINE_ROOT}/testbin
    )
  ADD_TEST(recommend-manager "${SF1RENGINE_ROOT}/testbin/t_IDGenerator")

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)

