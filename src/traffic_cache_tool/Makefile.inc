#
# Cache Tool Makefile
#
#  Licensed to the Apache Software Foundation (ASF) under one
#  or more contributor license agreements.  See the NOTICE file
#  distributed with this work for additional information
#  regarding copyright ownership.  The ASF licenses this file
#  to you under the Apache License, Version 2.0 (the
#  "License"); you may not use this file except in compliance
#  with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.

bin_PROGRAMS += traffic_cache_tool/traffic_cache_tool

traffic_cache_tool_traffic_cache_tool_CPPFLAGS = \
    $(AM_CPPFLAGS) \
    -I $(abs_top_srcdir)/include \
    -I $(abs_top_srcdir)/lib \
    @SWOC_INCLUDES@ \
    -D__STDC_FORMAT_MACROS

traffic_cache_tool_traffic_cache_tool_SOURCES = \
    traffic_cache_tool/CacheDefs.h \
    traffic_cache_tool/CacheDefs.cc \
    traffic_cache_tool/CacheTool.cc \
    traffic_cache_tool/CacheScan.h \
    traffic_cache_tool/CacheScan.cc

traffic_cache_tool_traffic_cache_tool_LDFLAGS = $(AM_LDFLAGS) @SWOC_LDFLAGS@

traffic_cache_tool_traffic_cache_tool_LDADD = \
    $(top_builddir)/src/tscore/libtscore.a \
    @SWOC_LIBS@ @OPENSSL_LIBS@ @LIBPCRE@
