add_custom_target(test EXCLUDE_FROM_ALL
    ${CMAKE_SOURCE_DIR}/test/test.sh
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/test)
