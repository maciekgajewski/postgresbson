#find postgres
#  variables set: PG_INCLUDEDIR, PG_LIBDIR, PG_EXTENSIONDIR

if(NOT Postgres_INCLUDEDIR OR NOT Postgres_LIBDIR OR NOT Postgres_SHAREDIR)

    message(STATUS "Looking for PostgreSQL...")

    execute_process(
        COMMAND pg_config --includedir-server
        OUTPUT_VARIABLE OUT_Postgres_INCLUDEDIR OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(NOT OUT_Postgres_INCLUDEDIR)
        set(Postgres_FOUND FALSE)
        message(FATAL_ERROR "pg_config unavailable. Make sure it is in your path")
    else()
        set(Postgres_FOUND TRUE)
    endif()

    execute_process(
        COMMAND pg_config --libdir
        OUTPUT_VARIABLE OUT_Postgres_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    execute_process(
        COMMAND pg_config --sharedir
        OUTPUT_VARIABLE OUT_Postgres_SHAREDIR OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    set(Postgres_INCLUDEDIR ${OUT_Postgres_INCLUDEDIR} CACHE PATH "Path to PostgreSQL server includes")
    set(Postgres_LIBDIR ${OUT_Postgres_LIBDIR} CACHE PATH "Path to PostgreSQL library")
    set(Postgres_SHAREDIR ${OUT_Postgres_SHAREDIR} CACHE PATH "Path to PostgreSQL shared objects")

else()
    message(STATUS "PostgreSQL location loaded from cache")
    set(Postgres_FOUND TRUE)
endif()

if(Postgres_FOUND)
    message(STATUS "PostgreSQL found: includedir-server=${Postgres_INCLUDEDIR}, libdir=${Postgres_LIBDIR}, sharedir=${Postgres_SHAREDIR}")
endif()
    
set(Postgres_EXTENSIONDIR "${Postgres_SHAREDIR}/extension")
