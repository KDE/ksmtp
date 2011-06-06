project(ksmtp)

if(KDE4_BUILD_TESTS)

include(FindBoost)
find_package(Boost REQUIRED)

set( EXECUTABLE_OUTPUT_PATH ${CMAKE_CURRENT_BINARY_DIR} )
set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${KDE4_ENABLE_EXCEPTIONS}" )

include_directories(${CMAKE_SOURCE_DIR}/ksmtp ${KDE4_INCLUDES} ${Boost_INCLUDE_DIR} )

SET( source_files
	fakeserver.cpp
	smtptest.cpp
)
SET(source_headers fakeserver.h smtptest.h)

QT4_WRAP_CPP(source_headers_moc ${source_headers})

kde4_add_executable( ksmtp-test NOGUI TEST ${source_files} )

target_link_libraries(ksmtp-test ksmtp ${KDE4_KDECORE_LIBS} ${QT_QTNETWORK_LIBRARY} ${QT_QTTEST_LIBRARY} kmime ${SASL2_LIBRARIES})

install(TARGETS ksmtp-test ${INSTALL_TARGETS_DEFAULT_ARGS})
endif(KDE4_BUILD_TESTS)
