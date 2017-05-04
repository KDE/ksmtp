include_directories(${KSMTP_SOURCE_DIR}/src ${KSMTP_BINARY_DIR}/src)

add_executable(logintest logintest.cpp)
target_link_libraries(logintest KPimSMTP Qt5::Core)
