cmake_minimum_required(VERSION 3.15)
project(acyclic_join_sgx C CXX)

# The build expects the Intel SGX SDK. Override SGX_SDK when the SDK is not
# installed in the default Linux location.
if(NOT DEFINED SGX_SDK)
  set(SGX_SDK /opt/intel/sgxsdk)
endif()

# SGX_MODE=HW links the hardware runtime; SGX_MODE=SIM links the simulator.
if(NOT DEFINED SGX_MODE)
  set(SGX_MODE HW)
endif()

# Keep debug enclaves enabled by default so benchmark runs can emit profiling
# logs while the code is being evaluated.
if(NOT DEFINED SGX_DEBUG)
  set(SGX_DEBUG 1)
endif()

message(STATUS "SGX_SDK = ${SGX_SDK}")
message(STATUS "SGX_MODE = ${SGX_MODE}")
message(STATUS "SGX_DEBUG = ${SGX_DEBUG}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -g")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

set(SGX_EDGER8R ${SGX_SDK}/bin/x64/sgx_edger8r)
set(SGX_SIGN    ${SGX_SDK}/bin/x64/sgx_sign)
set(SGX_COMMON_INC ${SGX_SDK}/include)
set(SGX_LIB_DIR    ${SGX_SDK}/lib64)

include_directories(${SGX_COMMON_INC})

if(SGX_DEBUG EQUAL 1)
  add_compile_definitions(DEBUG)
  set(SGX_DEBUG_FLAG SGX_DEBUG_FLAG)
else()
  set(SGX_DEBUG_FLAG 0)
endif()

add_subdirectory(Enclave)
add_subdirectory(App)
