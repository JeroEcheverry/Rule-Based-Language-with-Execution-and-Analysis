cmake_minimum_required(VERSION 4.1)
project(Rule_Based_Language_with_Execution_and_Analysis)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(Rule_Based_Language_with_Execution_and_Analysis
        ast.h
        lexer.cpp
        lexer.h
        main.cpp
        parser.cpp
        parser.h
        interpreter.cpp
        interpreter.h
        Analyzer.cpp
        Analyzer.h)
