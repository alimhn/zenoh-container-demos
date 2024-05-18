cmake_minimum_required(VERSION 3.22)
project(
  throughput LANGUAGES C CXX
  VERSION "0.10.1"
  )


# if(NOT TARGET CycloneDDS::ddsc)
#   # Find the CycloneDDS package.
#   find_package(CycloneDDS REQUIRED)
# endif()

if(NOT TARGET CycloneDDS-CXX::ddscxx)
  find_package(CycloneDDS-CXX REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_package(Boost 1.83 REQUIRED COMPONENTS program_options)
find_package(reelay REQUIRED)
find_package(simdjson REQUIRED)

set(Boost_USE_STATIC_LIBS TRUE)
set(Boost_USE_STATIC_RUNTIME TRUE)

# This is a convenience function, provided by the CycloneDDS package,
# that will supply a library target related the the given idl file.
# In short, it takes the idl file, generates the source files with
# the proper data types and compiles them into a library.
# idlc_generate(TARGET Throughput_lib FILES Throughput.idl)
idlcxx_generate(TARGET Throughput_lib_cxx FILES Throughput.idl WARNINGS no-implicit-extensibility)

# Both executables have only one related source file.
add_executable(cxxThroughputPublisher publisher.cpp)
add_executable(cxxThroughputSubscriber subscriber.cpp)

# Both executables need to be linked to the idl data type library and
# the ddsc API library.
target_link_libraries(cxxThroughputPublisher PRIVATE Throughput_lib_cxx CycloneDDS-CXX::ddscxx reelay::reelay simdjson::simdjson Boost::program_options /usr/local/lib/libboost_program_options.a)
target_link_libraries(cxxThroughputSubscriber PRIVATE Throughput_lib_cxx CycloneDDS-CXX::ddscxx reelay::reelay simdjson::simdjson Boost::program_options /usr/local/lib/libboost_program_options.a)

install(TARGETS cxxThroughputPublisher cxxThroughputSubscriber
RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}")