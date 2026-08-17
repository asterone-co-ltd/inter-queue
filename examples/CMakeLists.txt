# --- producer ---
set(producer_sources ./producer.cpp)
add_executable(producer ${producer_sources})
target_include_directories(producer PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(producer PUBLIC iqueue)

set_target_properties(producer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# --- consumer ---
set(consumer_sources ./consumer.cpp)
add_executable(consumer ${consumer_sources})
target_include_directories(consumer PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(consumer PUBLIC iqueue)

set_target_properties(consumer PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

# --- test ---
set(test_sources ./test.cpp)
add_executable(test ${test_sources})
target_include_directories(test PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(test PUBLIC iqueue)

set_target_properties(test PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
    ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)
