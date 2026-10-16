cmake_minimum_required(VERSION 3.16)
project(KryptoBezp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(KryptoBezp src/VigenersCiphre.cpp)
target_compile_definitions(KryptoBezp PRIVATE
        DEFAULT_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")

# Prudova sifra ma vlastny main, preto sa zostavuje samostatne.
add_executable(StreamCipher src/StreamCipher.cpp)
target_compile_definitions(StreamCipher PRIVATE
        DEFAULT_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data")
