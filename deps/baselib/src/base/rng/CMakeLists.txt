add_subdirectory(detail)

target_sources(baselib PUBLIC
    SeedFromSystemRng.cpp
    SeedFromSystemRng.hpp
    PseudoRng.cpp
    PseudoRng.hpp
    Xorshift.cpp
    Xorshift.hpp
    SystemRng.cpp
    SystemRng.hpp
    RandomDistributor.cpp
    RandomDistributor.hpp
)
