#######################
#
#  Licensed to the Apache Software Foundation (ASF) under one or more contributor license
#  agreements.  See the NOTICE file distributed with this work for additional information regarding
#  copyright ownership.  The ASF licenses this file to you under the Apache License, Version 2.0
#  (the "License"); you may not use this file except in compliance with the License.  You may obtain
#  a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software distributed under the License
#  is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express
#  or implied. See the License for the specific language governing permissions and limitations under
#  the License.
#
#######################

add_library(inkcont STATIC InkContInternal.cc)
add_library(ts::inkcont ALIAS inkcont)

target_link_libraries(inkcont
  PRIVATE
    ts::http_remap
    ts::inkevent
    ts::tscore
)

add_executable(traffic_server
        Crash.cc
        EventName.cc
        FetchSM.cc
        HostStatus.cc
        InkAPI.cc
        InkIOCoreAPI.cc
        SocksProxy.cc
        traffic_server.cc
        RpcAdminPubHandlers.cc
        ${CMAKE_SOURCE_DIR}/src/shared/overridable_txn_vars.cc
        )
target_include_directories(traffic_server PRIVATE
        ${IOCORE_INCLUDE_DIRS}
        ${PROXY_INCLUDE_DIRS}
        ${CMAKE_SOURCE_DIR}/mgmt
)
target_link_libraries(traffic_server
    PRIVATE
        ts::tscore
        ts::tsapi
        ts::inkcont
        ts::http
        ts::http_remap
        ts::http2
        ts::logging
        ts::hdrs
        ts::configmanager
        ts::diagsconfig
        ts::inkutils
        ts::inkdns
        ts::inkhostdb
        ts::inkcache
        ts::aio
        ts::proxy
        ts::inknet
        ts::records
        ts::inkevent
        libswoc
        ts::jsonrpc_protocol
        ts::jsonrpc_server
        ts::rpcpublichandlers
)

if(TS_USE_QUIC)
    target_link_libraries(traffic_server
        PRIVATE
            ts::http3
            ts::quic
    )
endif()

if(TS_HAS_PROFILER)
    target_link_libraries(traffic_server PRIVATE gperftools::profiler)
endif()

if (TS_USE_LINUX_IO_URING)
    target_link_libraries(traffic_server PRIVATE inkuring uring)
endif (TS_USE_LINUX_IO_URING)
if (TS_USE_LINUX_NATIVE_AIO)
    target_link_libraries(traffic_server PRIVATE aio)
endif (TS_USE_LINUX_NATIVE_AIO)

set_target_properties(traffic_server PROPERTIES ENABLE_EXPORTS ON)

install(TARGETS traffic_server)
