add_subdirectory(detail)

target_sources(baselib PUBLIC
    Json.cpp
    Json.hpp
    Parser.cpp
    Parser.hpp
    Printer.hpp
    Printer.cpp
    Serialization.cpp
    Serialization.hpp
)