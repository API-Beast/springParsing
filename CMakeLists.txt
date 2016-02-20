cmake_minimum_required(VERSION 3.0)

set(H_FILES UTF8.h UTF8_Templates.hpp Unicode.h trex.h)
set(CPP_FILES UTF8.cpp Parser.cpp trex.c)

add_library(springParsing STATIC ${H_FILES} ${CPP_FILES})
target_compile_options(springParsing PUBLIC -std=c++11)

add_executable(springParsingTest test.cpp)

target_link_libraries(springParsingTest springParsing)
target_compile_options(springParsingTest PUBLIC -std=c++11)

add_test("Parser-Combinator" springParsingTest)