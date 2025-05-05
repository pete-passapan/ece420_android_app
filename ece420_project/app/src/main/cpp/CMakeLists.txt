cmake_minimum_required(VERSION 3.10.2)
project("ece420_project")

add_library(
        native-lib
        SHARED
        mfcc.cpp
        recognizer.cpp
        native-lib.cpp
        kissfft/kiss_fft.c
        kissfft/kiss_fftr.c
)

target_include_directories(native-lib PRIVATE kissfft .)
target_link_libraries(native-lib android log)