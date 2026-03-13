if(NOT DEFINED INPUT_UF2)
    message(FATAL_ERROR "INPUT_UF2 is required")
endif()
if(NOT DEFINED OUTPUT_DIR)
    message(FATAL_ERROR "OUTPUT_DIR is required")
endif()
if(NOT DEFINED BUILD_KIND)
    message(FATAL_ERROR "BUILD_KIND is required")
endif()
if(NOT DEFINED GIT_HASH)
    message(FATAL_ERROR "GIT_HASH is required")
endif()

if(NOT EXISTS "${INPUT_UF2}")
    message(FATAL_ERROR "Input UF2 not found: ${INPUT_UF2}")
endif()

string(TIMESTAMP ARCHIVE_TIMESTAMP "%Y%m%d-%H%M%S")

set(LATEST_PATH "${OUTPUT_DIR}/${BUILD_KIND}.uf2")
set(ARCHIVE_PATH "${OUTPUT_DIR}/${BUILD_KIND}-${ARCHIVE_TIMESTAMP}-${GIT_HASH}.uf2")

file(MAKE_DIRECTORY "${OUTPUT_DIR}")

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${INPUT_UF2}" "${LATEST_PATH}"
    RESULT_VARIABLE COPY_LATEST_RESULT
)
if(NOT COPY_LATEST_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to update latest UF2: ${LATEST_PATH}")
endif()

execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${INPUT_UF2}" "${ARCHIVE_PATH}"
    RESULT_VARIABLE COPY_ARCHIVE_RESULT
)
if(NOT COPY_ARCHIVE_RESULT EQUAL 0)
    message(FATAL_ERROR "Failed to archive UF2: ${ARCHIVE_PATH}")
endif()

message(STATUS "UF2 archived: ${ARCHIVE_PATH}")
