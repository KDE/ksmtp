# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
include(ECMAddTests)
find_package(Qt${QT_MAJOR_VERSION}Test ${QT_REQUIRED_VERSION} REQUIRED)

ecm_add_test(smtptest.cpp fakeserver.cpp smtptest.h fakeserver.h
    LINK_LIBRARIES KPim${KF_MAJOR_VERSION}SMTP Qt::Test Qt::Network
    TEST_NAME smtptest
)
