import QtQuick
import GameScrcpy.Theme

Item {
    id: titleBar

    required property Window window

    readonly property Item minimizeButton: minimizeButton
    readonly property Item maximizeButton: maximizeButton
    readonly property Item closeButton: closeButton

    implicitHeight: 40

    // macOS: the system's traffic lights sit at the left of the bar, and macOS moves the window by its top strip;
    // the rest of the bar moves it too.
    MouseArea {
        anchors.fill: parent
        acceptedButtons: Qt.LeftButton
        onPressed: titleBar.window.startSystemMove()
        onDoubleClicked: {
            if (titleBar.window.visibility === Window.Maximized)
                titleBar.window.showNormal()
            else
                titleBar.window.showMaximized()
        }
    }

    Text {
        anchors.left: parent.left
        anchors.leftMargin: 78
        anchors.verticalCenter: parent.verticalCenter
        textFormat: Text.PlainText
        text: titleBar.window.title
        color: Theme.colors.textTertiary
        font.pixelSize: Theme.type.caption.size
        font.weight: Font.Medium
    }

    Row {
        visible: false
        anchors.right: parent.right
        anchors.top: parent.top
        height: parent.height

        CaptionButton {
            id: minimizeButton
            glyph: "\uE921"
            onClicked: titleBar.window.showMinimized()
        }

        CaptionButton {
            id: maximizeButton
            glyph: titleBar.window.visibility === Window.Maximized ? "\uE923" : "\uE922"
            onClicked: {
                if (titleBar.window.visibility === Window.Maximized)
                    titleBar.window.showNormal()
                else
                    titleBar.window.showMaximized()
            }
        }

        CaptionButton {
            id: closeButton
            glyph: "\uE8BB"
            destructive: true
            onClicked: titleBar.window.close()
        }
    }
}
