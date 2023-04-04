# SPDX-FileCopyrightText: none
# SPDX-License-Identifier: BSD-3-Clause
include(ECMAddTests)
find_package(Qt6Test ${QT_REQUIRED_VERSION} REQUIRED)

ecm_add_test(smtptest.cpp fakeserver.cpp smtptest.h fakeserver.h
    LINK_LIBRARIES KPim6SMTP Qt::Test Qt::Network
    TEST_NAME smtptest
)
