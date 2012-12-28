project(pgbson)
cmake_minimum_required(VERSION 2.8)
find_package( Boost 1.36.0 COMPONENTS date_time thread filesystem)
set(CMAKE_BUILD_TYPE Debug)

#find postgres
#  variables set: PG_INCLUDEDIR, PG_LIBDIR, PG_EXTENSIONDIR

execute_process(
    COMMAND pg_config --includedir-server
    OUTPUT_VARIABLE PG_INCLUDEDIR OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND pg_config --libdir
    OUTPUT_VARIABLE PG_LIBDIR OUTPUT_STRIP_TRAILING_WHITESPACE
)

execute_process(
    COMMAND pg_config --sharedir
    OUTPUT_VARIABLE PG_SHAREDIR OUTPUT_STRIP_TRAILING_WHITESPACE
)
set(PG_EXTENSIONDIR "${PG_SHAREDIR}/extension")

# building

include_directories( ${PG_INCLUDEDIR} )
set(MONGO_SRC "mongo-cxx-driver-v2.2/src/")
include_directories(BEFORE ${MONGO_SRC} ${MONGO_SRC}/mongo)

add_library(pgbson SHARED
    pgbson.cpp

    # mongo sources (list copied from ${MONGO_SRC}/SConscript.client)
    ${MONGO_SRC}/mongo/bson/oid.cpp
    ${MONGO_SRC}/mongo/buildinfo.cpp
    ${MONGO_SRC}/mongo/client/authentication_table.cpp
    ${MONGO_SRC}/mongo/client/clientAndShell.cpp
    ${MONGO_SRC}/mongo/client/clientOnly.cpp
    ${MONGO_SRC}/mongo/client/connection_factory.cpp
    ${MONGO_SRC}/mongo/client/connpool.cpp
    ${MONGO_SRC}/mongo/client/dbclient.cpp
    ${MONGO_SRC}/mongo/client/dbclient_rs.cpp
    ${MONGO_SRC}/mongo/client/dbclientcursor.cpp
    ${MONGO_SRC}/mongo/client/distlock.cpp
    ${MONGO_SRC}/mongo/client/gridfs.cpp
    ${MONGO_SRC}/mongo/client/model.cpp
    ${MONGO_SRC}/mongo/client/syncclusterconnection.cpp
    ${MONGO_SRC}/mongo/db/jsobj.cpp
    ${MONGO_SRC}/mongo/db/json.cpp
    ${MONGO_SRC}/mongo/db/lasterror.cpp
    ${MONGO_SRC}/mongo/db/namespace.cpp
    ${MONGO_SRC}/mongo/db/nonce.cpp
    ${MONGO_SRC}/mongo/db/dbmessage.cpp
    ${MONGO_SRC}/mongo/pch.cpp
    ${MONGO_SRC}/mongo/util/assert_util.cpp
    ${MONGO_SRC}/mongo/util/background.cpp
    ${MONGO_SRC}/mongo/util/base64.cpp
    ${MONGO_SRC}/mongo/util/concurrency/rwlockimpl.cpp
    ${MONGO_SRC}/mongo/util/concurrency/spin_lock.cpp
    ${MONGO_SRC}/mongo/util/concurrency/synchronization.cpp
    ${MONGO_SRC}/mongo/util/concurrency/task.cpp
    ${MONGO_SRC}/mongo/util/concurrency/thread_pool.cpp
    ${MONGO_SRC}/mongo/util/concurrency/mutexdebugger.cpp
    ${MONGO_SRC}/mongo/util/debug_util.cpp
    ${MONGO_SRC}/mongo/util/stacktrace.cpp
    ${MONGO_SRC}/mongo/util/file_allocator.cpp
    ${MONGO_SRC}/mongo/util/histogram.cpp
    ${MONGO_SRC}/mongo/util/intrusive_counter.cpp
    ${MONGO_SRC}/mongo/util/log.cpp
    ${MONGO_SRC}/mongo/util/md5.cpp
    ${MONGO_SRC}/mongo/util/md5main.cpp
    ${MONGO_SRC}/mongo/util/net/httpclient.cpp
    ${MONGO_SRC}/mongo/util/net/listen.cpp
    ${MONGO_SRC}/mongo/util/net/message.cpp
    ${MONGO_SRC}/mongo/util/net/message_port.cpp
    ${MONGO_SRC}/mongo/util/net/sock.cpp
    ${MONGO_SRC}/mongo/util/password.cpp
    ${MONGO_SRC}/mongo/util/ramlog.cpp
    ${MONGO_SRC}/mongo/util/signal_handlers.cpp
    ${MONGO_SRC}/mongo/util/stringutils.cpp
    ${MONGO_SRC}/mongo/util/text.cpp
    ${MONGO_SRC}/mongo/util/time_support.cpp
    ${MONGO_SRC}/mongo/util/timer.cpp
    ${MONGO_SRC}/mongo/util/trace.cpp
    ${MONGO_SRC}/mongo/util/util.cpp
)

target_link_libraries(pgbson ${Boost_LIBRARIES})

# installation
install(TARGETS pgbson DESTINATION ${PG_LIBDIR}
    PERMISSIONS WORLD_EXECUTE GROUP_EXECUTE OWNER_EXECUTE WORLD_READ GROUP_READ OWNER_READ OWNER_WRITE)
install(FILES pgbson.control pgbson--0.5.sql DESTINATION ${PG_EXTENSIONDIR})
