#! /bin/sh
# SPDX-License-Identifier: BSD-3-Clause
# SPDX-FileCopyrightText: 2017-2026 Laurent Montel <montel@kde.org> 
$XGETTEXT `find . -name "*.cpp" -o -name "*.h" | grep -v '/tests/'` -o $podir/libksmtp6.pot
