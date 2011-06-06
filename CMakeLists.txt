cmake_minimum_required(VERSION 2.8)

project (ksmtp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DMAKE_KSMTP_LIB")

set(CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/modules")
if(KIMAP_STANDALONE)
    set(KDE4_DATA_DIR "${CMAKE_PREFIX_PATH}/share/apps")
endif(KIMAP_STANDALONE)

find_package(KDE4 REQUIRED)

add_subdirectory(tests)

include_directories(${KDE4_INCLUDES})

set(ksmtp_LIB_SRCS 
	session.cpp
	sessionthread.cpp
	job.cpp
	loginjob.cpp
	sendjob.cpp
)

set(ksmtp_LIB_HDR
	sessionthread_p.h
	session.h
	session_p.h
	job.h
	loginjob.h
	sendjob.h
)

QT4_WRAP_CPP(ksmtp_LIB_MOC ${ksmtp_LIB_HDR})

add_library(ksmtp ${ksmtp_LIB_SRCS} ${ksmtp_LIB_MOC})
target_link_libraries(ksmtp ${KDE4_KDECORE_LIBS} ${QT_QTNETWORK_LIBRARY} kmime ${SASL2_LIBRARIES})

install(TARGETS ksmtp ${INSTALL_TARGETS_DEFAULT_ARGS})
install( FILES ksmtp_export.h session.h job.h loginjob.h sendjob.h DESTINATION ${INCLUDE_INSTALL_DIR}/ksmtp COMPONENT Devel)
