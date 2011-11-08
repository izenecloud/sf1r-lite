INCLUDE_DIRECTORIES(
  ${CMAKE_SOURCE_DIR}
  ${CMAKE_SOURCE_DIR}/core
  ${Boost_INCLUDE_DIRS}
  ${Glog_INCLUDE_DIRS}
    )

SET(Boost_USE_STATIC_LIBS OFF)
FIND_PACKAGE(Boost ${Boost_FIND_VERSION}
  COMPONENTS unit_test_framework)

IF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
  INCLUDE_DIRECTORIES(
    ${Boost_INCLUDE_DIRS}
  )
  
  FILE(GLOB t_node_SRC
    Runner.cpp
    t_node_manager.cpp
  )
  
#  ADD_EXECUTABLE(t_node_manager
#    ${t_node_SRC}
#    )
#  TARGET_LINK_LIBRARIES(t_node_manager 
#      ${Boost_UNIT_TEST_FRAMEWORK_LIBRARY}
#      
#      sf1r_node_manager
#      sf1r_common
#    
#      ${izenelib_LIBRARIES}
#      
#      #external
#      ${Boost_LIBRARIES}
#      ${Glog_LIBRARIES}
#      )   
      
  ADD_EXECUTABLE(t_node_manager_manual
    t_node_manager_manual.cpp
    )
  TARGET_LINK_LIBRARIES(t_node_manager_manual      
      sf1r_node_manager
      sf1r_common    
      ${izenelib_LIBRARIES}      
      #external
      ${Boost_LIBRARIES}
      ${Glog_LIBRARIES}
      )
      
  ADD_EXECUTABLE(t_scd_sharding
    t_scd_sharding.cpp
    )
  TARGET_LINK_LIBRARIES(t_scd_sharding      
      sf1r_node_manager
      sf1r_common    
      ${izenelib_LIBRARIES}      
      #external
      ${Boost_LIBRARIES}
      ${Glog_LIBRARIES}
      )

ENDIF(Boost_FOUND AND Boost_UNIT_TEST_FRAMEWORK_FOUND)
