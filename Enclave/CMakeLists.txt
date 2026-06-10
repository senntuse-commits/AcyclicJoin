set(ENCLAVE_NAME enclave)
set(SIGNED_ENCLAVE ${ENCLAVE_NAME}.signed.so)

set(EDL_FILE ${CMAKE_SOURCE_DIR}/Enclave/Enclave.edl)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)

file(MAKE_DIRECTORY ${GEN_DIR}/App)
file(MAKE_DIRECTORY ${GEN_DIR}/Enclave)

add_custom_command(
  OUTPUT ${GEN_DIR}/Enclave/Enclave_t.c ${GEN_DIR}/Enclave/Enclave_t.h
  COMMAND ${SGX_EDGER8R} --trusted ${EDL_FILE}
          --search-path ${SGX_COMMON_INC}
          --search-path ${CMAKE_SOURCE_DIR}/Enclave
          --trusted-dir ${GEN_DIR}/Enclave
  DEPENDS ${EDL_FILE}
  COMMENT "Running sgx_edger8r (trusted) on ${EDL_FILE}"
)

add_library(Enclave_t_obj OBJECT ${GEN_DIR}/Enclave/Enclave_t.c)
target_include_directories(Enclave_t_obj PRIVATE
  ${GEN_DIR}/Enclave
  ${SGX_COMMON_INC}
  ${CMAKE_SOURCE_DIR}/include
  ${SGX_SDK}/include/tlibc
)

file(GLOB ACYCLIC_JOIN_SRCS ${CMAKE_SOURCE_DIR}/implement/*.cpp)
add_library(Enclave_obj OBJECT Enclave.cpp sgx_log.cpp ${ACYCLIC_JOIN_SRCS})
target_include_directories(Enclave_obj PRIVATE
  ${GEN_DIR}/Enclave
  ${SGX_COMMON_INC}
  ${CMAKE_SOURCE_DIR}/include
  ${SGX_SDK}/include/tlibc
  ${SGX_SDK}/include/libcxx
  /usr/lib/gcc/x86_64-linux-gnu/11/include
)

target_compile_options(Enclave_obj PRIVATE
  -fPIC -fvisibility=hidden -fno-builtin -Wno-attributes -O2
  -fopenmp
  -nostdinc++
  -include ${CMAKE_SOURCE_DIR}/include/sgx_no_timing.h
)
target_compile_definitions(Enclave_obj PRIVATE SGX_ENCLAVE_BUILD SGX_SILENCE_ALGORITHM_LOG)

add_dependencies(Enclave_obj Enclave_t_obj)

add_custom_command(
  OUTPUT ${CMAKE_BINARY_DIR}/enclave.so
  COMMAND ${CMAKE_CXX_COMPILER}
    -shared
    -o ${CMAKE_BINARY_DIR}/enclave.so
    $<TARGET_OBJECTS:Enclave_obj>
    $<TARGET_OBJECTS:Enclave_t_obj>
    -Wl,--no-undefined
    -nostdlib -nodefaultlibs -nostartfiles
    -Wl,-Bsymbolic
    -Wl,-pie,-eenclave_entry
    -Wl,--defsym,__ImageBase=0
    -Wl,--no-as-needed
    -L${SGX_LIB_DIR}
    -Wl,--whole-archive -lsgx_trts -Wl,--no-whole-archive
    -Wl,--version-script=${CMAKE_SOURCE_DIR}/Enclave/Enclave.lds
    -Wl,--export-dynamic
    -Wl,--start-group
      -lsgx_tstdc
      -lsgx_tcxx
      -lsgx_tcrypto
      -lsgx_tservice
      -lsgx_pthread
      -lsgx_omp
    -Wl,--end-group
    -L/usr/lib/gcc/x86_64-linux-gnu/11
    -static-libgcc
    -lgcc
    -l:libgcc_eh.a
    -Wl,--undefined=g_global_data
    -Wl,--undefined=g_global_data_end
    -Wl,--undefined=g_global_data_size
  DEPENDS Enclave_obj Enclave_t_obj
  COMMENT "Linking enclave.so with SGX trusted libraries"
  VERBATIM
  COMMAND_EXPAND_LISTS
)

add_custom_target(${ENCLAVE_NAME} ALL DEPENDS ${CMAKE_BINARY_DIR}/enclave.so)

set(SIGN_KEY ${CMAKE_SOURCE_DIR}/keys/Enclave_private.pem)

if(NOT EXISTS ${SIGN_KEY})
  message(STATUS "Enclave private key not found, generating new key at ${SIGN_KEY}")
  file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/keys)
  execute_process(
    COMMAND openssl genrsa -3 -out ${SIGN_KEY} 3072
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    RESULT_VARIABLE GEN_KEY_RESULT
  )
  if(NOT GEN_KEY_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to generate enclave private key with openssl")
  endif()
endif()

add_custom_target(enclave_signed ALL
  COMMAND ${SGX_SIGN} sign
          -key ${SIGN_KEY}
          -enclave ${CMAKE_BINARY_DIR}/enclave.so
          -out ${CMAKE_BINARY_DIR}/${SIGNED_ENCLAVE}
          -config ${CMAKE_SOURCE_DIR}/Enclave/Enclave.config.xml
  DEPENDS ${ENCLAVE_NAME}
  COMMENT "Signing enclave -> ${SIGNED_ENCLAVE}"
)

add_custom_command(TARGET enclave_signed POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy ${CMAKE_BINARY_DIR}/${SIGNED_ENCLAVE}
          ${CMAKE_BINARY_DIR}/../${SIGNED_ENCLAVE}
)
