pragma Singleton

import QtQuick

QtObject {
    id: theme

    property bool dark: true
    property string accentName: "azure"
    property bool reducedMotion: false
    property bool reducedTransparency: false
    // Liquid Glass (macOS): windows show the desktop, blurred, through their translucent surfaces.
    readonly property bool glass: !reducedTransparency
    property var palette: ({})

    readonly property var accentChoices: ({
        ember: "#FF6B3D",
        teal: "#22D3C5",
        azure: "#0A84FF",
        lime: "#B8F34A",
        rose: "#FF4D8D",
        amber: "#FFB020"
    })

    readonly property var accentTextChoices: ({
        ember: "#C2461C",
        teal: "#0E7C74",
        azure: "#007AFF",
        lime: "#4D7A0A",
        rose: "#C21F5B",
        amber: "#A15C00"
    })

    readonly property var toneHues: ({
        orange: "ember",
        teal: "teal",
        azure: "azure",
        lime: "lime",
        rose: "rose",
        amber: "amber"
    })

    // A shortcut as Qt writes it ("Ctrl+Shift+K") as a Mac shows it ("⇧⌘K"): Qt's Ctrl is the Command key on
    // macOS and its Meta the Control key; modifiers in Apple's order ⌃ ⌥ ⇧ ⌘.
    function keyLabel(sequence: string): string {
        if (sequence === "")
            return ""
        const symbols = { Meta: "⌃", Alt: "⌥", Shift: "⇧", Ctrl: "⌘" }
        const order = ["Meta", "Alt", "Shift", "Ctrl"]
        return sequence.split(", ").map(chord => {
            const parts = chord.endsWith("++") ? chord.slice(0, -2).split("+").concat(["+"]) : chord.split("+")
            const key = parts.pop()
            const held = order.filter(name => parts.indexOf(name) >= 0).map(name => symbols[name]).join("")
            return held + key
        }).join(" ")
    }

    function toneFill(tone: string): color {
        const hue = accentChoices[toneHues[tone] ?? ""]
        if (hue === undefined)
            return colors.statePress
        const base = Qt.color(hue)
        return Qt.rgba(base.r, base.g, base.b, dark ? 0.16 : 0.12)
    }

    function toneInk(tone: string): color {
        const hue = (dark ? accentChoices : accentTextChoices)[toneHues[tone] ?? ""]
        return hue === undefined ? colors.textSecondary : Qt.color(hue)
    }

    final readonly property ColorTokens colors: ColorTokens {
        dark: theme.dark
        accent: theme.accentChoices[theme.accentName] ?? theme.accentChoices.azure
        accentTextOnLight: theme.accentTextChoices[theme.accentName] ?? theme.accentTextChoices.azure
        overrides: (theme.dark ? theme.palette.dark : theme.palette.light) ?? ({})
        glass: theme.glass
    }
    final readonly property Typography type: Typography {}
    final readonly property Size size: Size {}
    readonly property real disabledOpacity: 0.4
    readonly property real fadedOpacity: 0.55
    final readonly property Spacing space: Spacing {}
    final readonly property Radius radius: Radius {}
    final readonly property Shadow shadow: Shadow {
        dark: theme.dark
    }
    final readonly property Motion motion: Motion {
        reduced: theme.reducedMotion
    }
}
