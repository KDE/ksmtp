include(ECMAddTests)
find_package(Qt5Test ${QT_REQUIRED_VERSION} REQUIRED)

ecm_add_test(smtptest.cpp fakeserver.cpp
    LINK_LIBRARIES KPimSMTP Qt5::Test Qt5::Network
    TEST_NAME smtptest
)
