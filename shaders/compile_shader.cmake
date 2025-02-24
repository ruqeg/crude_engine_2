macro(COMPILE_SHADER SHADER_FILE)
  message(DEBUG "Compiling shader ${SHADER_FILE}")
  COMPILE_SPIRV_SHADER(${SHADER_FILE})
  get_filename_component(SHADER_FOLDER ${SHADER_FILE} DIRECTORY)
  get_filename_component(SHADER_NAME ${SHADER_FILE} NAME)
  message(DEBUG "Compiled file ${COMPILE_SPIRV_SHADER_RETURN}")
  set(SHADER_HEADER_NAME "${CRUDE_SHADERS_DIR}/include/crude_shaders/${SHADER_NAME}.inl")
  add_custom_command(
    COMMAND ${CMAKE_COMMAND} -DCONFIG_FILE="${CRUDE_SHADERS_DIR}/shader_data.inl.in" -DTARGET_NAME="crude_shaders" -DSHADER_FILE="${SHADER_FILE}" -DSHADER_SPIRV="${COMPILE_SPIRV_SHADER_RETURN}" -DSHADER_HEADER_NAME="${SHADER_HEADER_NAME}" -P ${CRUDE_SHADERS_DIR}/create_shader_header.cmake
    OUTPUT ${SHADER_HEADER_NAME}
    DEPENDS ${COMPILE_SPIRV_SHADER_RETURN}
    COMMENT "Making Shader Header ${SHADER_HEADER_NAME}"
  )
  target_sources(crude_shaders PRIVATE ${SHADER_FILE})
  target_sources(crude_shaders PRIVATE ${SHADER_HEADER_NAME})
  source_group("shaders" FILES ${SHADER_FILE})
  source_group("compiled shaders" FILES ${SHADER_HEADER_NAME})
endmacro()
