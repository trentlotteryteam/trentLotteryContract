file(GLOB ABI_FILES "*.abi")
configure_file("${ABI_FILES}" "${CMAKE_CURRENT_BINARY_DIR}" COPYONLY)

add_wast_executable(TARGET trentlottery
  INCLUDE_FOLDERS "${STANDARD_INCLUDE_FOLDERS}" ${CMAKE_SOURCE_DIR}/libraries/fc/include
  LIBRARIES libc libc++ eosiolib
  DESTINATION_FOLDER ${CMAKE_CURRENT_BINARY_DIR}
)