import QtQuick
import QtQuick.Effects
import GameScrcpy.Theme

Item {
    id: toolbar

    default property alias content: row.data
    // Buttons in a column instead of a row, for a toolbar on the left or right of the picture.
    property bool vertical: false
    // Where tooltips and menus of the buttons open: away from the edge the toolbar sits on.
    property string popupEdge: "bottom"
    property bool pinned: false
    property bool blocked: false
    property bool flat: false
    property int hideDelay: Theme.motion.toolbarHideDelay
    readonly property bool shown: internal.revealed
    readonly property bool hovered: hover.hovered

    function reveal() {
        if (blocked)
            return
        internal.revealed = true
        internal.schedule()
    }

    function conceal() {
        if (pinned || hovered)
            return
        idle.stop()
        internal.revealed = false
    }

    implicitWidth: vertical ? Math.max(Theme.size.toolbar, column.implicitWidth + 2 * Theme.space.xs)
                            : row.implicitWidth + 2 * Theme.space.xs
    implicitHeight: vertical ? column.implicitHeight + 2 * Theme.space.xs : Theme.size.toolbar
    opacity: internal.revealed ? 1 : 0
    visible: opacity > 0

    Accessible.role: Accessible.ToolBar

    onVerticalChanged: internal.arrange()
    Component.onCompleted: internal.arrange()
    onPinnedChanged: internal.schedule()
    onHideDelayChanged: internal.schedule()
    onHoveredChanged: {
        if (hovered && visible && !blocked)
            internal.revealed = true
        internal.schedule()
    }

    Behavior on opacity {
        id: fade

        NumberAnimation {
            duration: fade.targetValue > 0 ? Theme.motion.fast : Theme.motion.base
            easing.type: Easing.BezierSpline
            easing.bezierCurve: fade.targetValue > 0 ? Theme.motion.standard : Theme.motion.exit
        }
    }

    QtObject {
        id: internal

        property bool revealed: false

        function arrange() {
            const from = toolbar.vertical ? row : column
            const to = toolbar.vertical ? column : row
            for (const item of Array.from(from.children)) {
                item.parent = to
                // A row places x and a column y; the other one is left from before and set here.
                if (toolbar.vertical) {
                    item.anchors.horizontalCenter = column.horizontalCenter
                } else {
                    item.anchors.horizontalCenter = undefined
                    item.y = 0
                }
            }
        }

        function schedule() {
            if (revealed && !toolbar.pinned && !toolbar.hovered)
                idle.restart()
            else
                idle.stop()
        }
    }

    Timer {
        id: idle

        interval: toolbar.hideDelay
        onTriggered: toolbar.conceal()
    }

    HoverHandler {
        id: hover
    }

    Item {
        anchors.fill: parent
        transform: Translate {
            x: toolbar.vertical ? (toolbar.opacity - 1) * (Theme.space.xs + Theme.space.xxs) : 0
            y: toolbar.vertical ? 0 : (toolbar.opacity - 1) * (Theme.space.xs + Theme.space.xxs)
        }

        RectangularShadow {
            anchors.fill: face
            visible: !toolbar.flat
            radius: face.radius
            offset.y: Theme.shadow.keyOffset
            blur: Theme.shadow.keyBlur
            color: Theme.shadow.key
        }

        Rectangle {
            id: face

            objectName: "face"
            anchors.fill: parent
            visible: !toolbar.flat
            radius: Math.min(width, height) / 2
            color: Theme.reducedTransparency ? Theme.colors.bgElevated : Theme.colors.bgFrosted
            border.width: Theme.size.stroke
            border.color: Theme.liquid ? Theme.colors.strokeDefault : Theme.colors.strokeStrong

            // Liquid Glass sheen and lit top edge, as on PopupSurface.
            Rectangle {
                anchors.fill: parent
                anchors.margins: Theme.size.stroke
                radius: Math.max(0, face.radius - Theme.size.stroke)
                visible: Theme.liquid
                gradient: Gradient {
                    GradientStop { position: 0; color: Theme.dark ? "#1AFFFFFF" : "#73FFFFFF" }
                    GradientStop { position: 0.5; color: "#00FFFFFF" }
                }
            }

            Rectangle {
                x: face.radius
                y: Theme.size.stroke
                width: Math.max(0, face.width - 2 * face.radius)
                height: Theme.size.stroke
                visible: Theme.liquid
                color: Theme.colors.glassEdge
            }
        }

        Row {
            id: row

            anchors.centerIn: parent
            spacing: Theme.space.xxs
        }

        // The buttons move here while the toolbar is vertical, in the same order.
        Column {
            id: column

            anchors.centerIn: parent
            spacing: Theme.space.xxs
        }
    }
}
