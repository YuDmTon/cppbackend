cmake_minimum_required(VERSION 3.11)

project(game_server CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
# обратите внимание на аргумент TARGETS у команды conan_basic_setup
conan_basic_setup(TARGETS)

#find_package(Boost 1.78.0 REQUIRED)
#if(Boost_FOUND)
#  include_directories(${Boost_INCLUDE_DIRS})
#endif()

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_executable(game_server
    src/main.cpp
    src/http_server.cpp
    src/http_server.h
    src/sdk.h
    src/model.h
    src/model.cpp
    src/tagged.h
    src/boost_json.cpp
    src/json_loader.h
    src/json_loader.cpp
    src/request_handler.cpp
    src/request_handler.h
    # ---
    src/api_handler.h
    src/api_handler.cpp
    src/app.h
    src/app.cpp
    src/logger.h
    src/logger.cpp
    src/response.h
    # ---
    src/loot_generator.h
    src/loot_generator.cpp
    # ---
    src/geom.h
    src/collision_detector.h
    src/collision_detector.cpp
    # ---
    src/serializer.h
    src/serializer.cpp
    # ---
    src/postgres.h
    src/postgres.cpp
)
# используем "импортированную" цель CONAN_PKG::boost
target_include_directories(game_server PRIVATE CONAN_PKG::boost)
target_link_libraries(game_server PRIVATE Threads::Threads CONAN_PKG::boost CONAN_PKG::libpq CONAN_PKG::libpqxx)
    
