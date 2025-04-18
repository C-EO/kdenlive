/*
    SPDX-FileCopyrightText: 2018 Jean-Baptiste Mardelle <jb@kdenlive.org>
    SPDX-License-Identifier: GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

import QtQuick 2.15

Item {
    id: overlay
    property color color

    // Vertical segments
    Rectangle {
        color: overlay.color
        height: parent.height
        width: 1
        x: parent.width / 3
    }
    Rectangle {
        color: overlay.color
        height: parent.height
        width: 1
        x: (parent.width / 3 ) * 2
    }

    // Horizontal segments
    Rectangle {
        color: overlay.color
        width: parent.width
        height: 1
        y: parent.height / 3
    }
    Rectangle {
        color: overlay.color
        width: parent.width
        height: 1
        y: (parent.height / 3) * 2
    }
}
