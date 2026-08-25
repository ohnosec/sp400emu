foreach(required_variable
    PYTHON_EXECUTABLE
    GENERATOR
    OPCODE_CSV
    OUTPUT_FILE)
  if(NOT DEFINED ${required_variable})
    message(FATAL_ERROR "${required_variable} was not provided")
  endif()
endforeach()

execute_process(
  COMMAND "${PYTHON_EXECUTABLE}" "${GENERATOR}"
          "${OPCODE_CSV}" m68hc05 optable
  OUTPUT_FILE "${OUTPUT_FILE}"
  RESULT_VARIABLE generator_result
)

if(NOT generator_result EQUAL 0)
  message(FATAL_ERROR "m68 opcode table generation failed: ${generator_result}")
endif()
