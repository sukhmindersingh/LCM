
execute_process(COMMAND ${CMAKE_COMMAND} -E create_symlink ${AlbanyPath}
                        ${CMAKE_CURRENT_BINARY_DIR}/Albany)

# Copy Input file from source to binary dir
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/input_explicit_fe.yaml
               ${CMAKE_CURRENT_BINARY_DIR}/input_explicit_fe.yaml COPYONLY)
