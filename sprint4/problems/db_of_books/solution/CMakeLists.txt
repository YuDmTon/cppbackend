cmake_minimum_required(VERSION 3.11)

project(book_manager CXX)
set(CMAKE_CXX_STANDARD 20)

include(${CMAKE_BINARY_DIR}/conanbuildinfo_multi.cmake)
# обратите внимание на аргумент TARGETS у команды conan_basic_setup
conan_basic_setup(TARGETS)

add_executable(book_manager
    src/book_manager.cpp
)
# используем "импортированную" цель CONAN_PKG::boost
target_include_directories(book_manager PRIVATE CONAN_PKG::boost)
target_link_libraries(book_manager CONAN_PKG::boost CONAN_PKG::libpqxx)
    
