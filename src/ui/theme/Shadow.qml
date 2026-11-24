import QtQuick

QtObject {
    property bool dark: true

    final readonly property color ambient: dark ? "#73000000" : "#1A000000"
    final readonly property int ambientBlur: 64
    final readonly property int ambientOffset: 24
    final readonly property color key: dark ? "#4D000000" : "#12000000"
    final readonly property int keyBlur: 18
    final readonly property int keyOffset: 4
    final readonly property int backdropBlur: 24
}
