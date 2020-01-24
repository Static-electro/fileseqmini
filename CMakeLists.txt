cmake_minimum_required( VERSION 3.8 )

if( NOT DEFINED PROJECT_NAME )
    set( BUILD_FILESEQ_EXECUTABLES ON )
endif()

project( fileseq-mini )

add_library( ${PROJECT_NAME} INTERFACE )
target_include_directories( ${PROJECT_NAME} INTERFACE include )

if ( BUILD_FILESEQ_EXECUTABLES )
	add_executable( fileseq-test misc/test.cpp )
	target_link_libraries( fileseq-test fileseq-mini )
	configure_file( misc/test.suite ${CMAKE_CURRENT_BINARY_DIR}/test.suite COPYONLY )

	add_executable( fileseq misc/fileseq.cpp )
	target_link_libraries( fileseq fileseq-mini )
	
	add_executable( fileseq-lazy misc/fileseq.cpp )
	target_link_libraries( fileseq-lazy fileseq-mini )
	target_compile_definitions( fileseq-lazy PRIVATE -DFILESEQ_LAZY )
endif()
