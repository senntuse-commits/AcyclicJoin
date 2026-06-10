set(APP_NAME app)

set(EDL_FILE ${CMAKE_SOURCE_DIR}/Enclave/Enclave.edl)
set(GEN_DIR  ${CMAKE_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${GEN_DIR}/App)
file(MAKE_DIRECTORY ${GEN_DIR}/Enclave)

add_custom_command(
  OUTPUT ${GEN_DIR}/App/Enclave_u.c ${GEN_DIR}/App/Enclave_u.h
  COMMAND ${SGX_EDGER8R} --untrusted ${EDL_FILE}
          --search-path ${SGX_COMMON_INC}
          --search-path ${CMAKE_SOURCE_DIR}/Enclave
          --untrusted-dir ${GEN_DIR}/App
  DEPENDS ${EDL_FILE}
  COMMENT "Running sgx_edger8r (untrusted) on ${EDL_FILE}"
)

# The untrusted bridge object marshals app-side calls into enclave ECALLs.
add_library(Enclave_u_obj OBJECT ${GEN_DIR}/App/Enclave_u.c)
target_include_directories(Enclave_u_obj PRIVATE ${GEN_DIR}/App ${SGX_COMMON_INC})

# App.cpp is the benchmark driver: it parses datasets/modes, starts the enclave,
# and prints result sizes plus optional profiling summaries.
add_executable(${APP_NAME} App.cpp $<TARGET_OBJECTS:Enclave_u_obj>)
target_include_directories(${APP_NAME} PRIVATE
  ${GEN_DIR}/App
  ${SGX_COMMON_INC}
  ${CMAKE_CURRENT_SOURCE_DIR}
)

# Link the correct SGX untrusted runtime for hardware or simulation mode.
if(${SGX_MODE} STREQUAL "HW")
  target_link_directories(${APP_NAME} PRIVATE /usr/lib/x86_64-linux-gnu)
  target_link_libraries(${APP_NAME} sgx_urts)
else()
  target_link_directories(${APP_NAME} PRIVATE ${SGX_LIB_DIR})
  target_link_libraries(${APP_NAME} sgx_urts_sim)
endif()

add_dependencies(${APP_NAME} enclave_signed)
