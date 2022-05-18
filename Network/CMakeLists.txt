cmake_minimum_required(VERSION 2.8)
project(Distributed_Integral_Evaluation C)

set(CMAKE_C_STANDARD 11)

include_directories("includes/")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -pthread")

add_executable(Distributed_Integral_Evaluation main.c
        includes/IntegralCalculator.h
        includes/ParseInfo.h
        src/IntegralCalculator.c
        src/ParseInfo.c
        src/DistributionNetwork.c includes/DistributionNetwork.h)