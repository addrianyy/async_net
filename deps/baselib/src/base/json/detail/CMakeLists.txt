add_subdirectory(serialization)

target_sources(baselib PUBLIC
    Parser.cpp
    Parser.hpp
    Lexer.cpp
    Lexer.hpp
    Printer.cpp
    Printer.hpp
)

