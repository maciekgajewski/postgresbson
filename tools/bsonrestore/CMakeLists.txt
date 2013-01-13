find_package( Boost 1.36.0 COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})

execute_process(
    COMMAND pg_config --includedir
    OUTPUT_VARIABLE PG_INCLUDEDIR OUTPUT_STRIP_TRAILING_WHITESPACE
)
include_directories(${PG_INCLUDEDIR})

execute_process(
    COMMAND pg_config --libdir
    OUTPUT_VARIABLE PG_LIBDIRR OUTPUT_STRIP_TRAILING_WHITESPACE
)
link_directories(${PG_LIBDIRR})

add_executable(bsonrestore EXCLUDE_FROM_ALL
    main.cpp
)

target_link_libraries(bsonrestore
    ${Boost_LIBRARIES}
    pq)

