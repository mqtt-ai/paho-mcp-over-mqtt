cmake_minimum_required(VERSION 3.12)
project(mcp-c-sdk)

enable_testing()

enable_language(C)
set(CMAKE_C_STANDARD 99)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE "Debug")
endif()

find_package(eclipse-paho-mqtt-c REQUIRED)

set(MCP_SOURCES
	src/jsonrpc.c
	src/mcp.c
	src/mcp_server.c
)

add_library(mcp-over-mqtt SHARED)
target_include_directories(mcp-over-mqtt PRIVATE include)
target_sources(mcp-over-mqtt PRIVATE ${MCP_SOURCES}) 

add_executable(server examples/server.c)
target_link_libraries(server mcp-over-mqtt paho-mqtt3a cjson)

include(GNUInstallDirs)
if(UNIX)
	mark_as_advanced(CLEAR
	    CMAKE_INSTALL_BINDIR
		CMAKE_INSTALL_INCLUDEDIR
		CMAKE_INSTALL_LIBDIR
		CMAKE_INSTALL_SYSCONFDIR)
else()
	set(CMAKE_INSTALL_BINDIR bin)
	set(CMAKE_INSTALL_INCLUDEDIR include)
	set(CMAKE_INSTALL_LIBDIR lib)
	set(CMAKE_INSTALL_SYSCONFDIR etc)
endif()
install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/"
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING
        PATTERN "*.h")
install(TARGETS mcp-over-mqtt
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})