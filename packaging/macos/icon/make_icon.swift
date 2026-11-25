// SPDX-License-Identifier: GPL-3.0-or-later
// Copyright (C) 2026 Yubo Wu and the GameScrcpy for macOS contributors
// Draws GameScrcpy's macOS icon: the product mark (a phone on its side with a game crosshair) on the macOS icon
// shape, at any size.
//
//   swift packaging/macos/icon/make_icon.swift <variant> <size> <out.png>
//   variant: azure (the app icon: the app's blue on graphite), ember (the original orange), light
import AppKit
import CoreGraphics

struct Palette {
    let top: CGColor, bottom: CGColor, phone: CGColor, mark: CGColor, edge: CGColor, sheen: CGFloat
}

func rgb(_ r: Int, _ g: Int, _ b: Int, _ a: CGFloat = 1) -> CGColor {
    CGColor(srgbRed: CGFloat(r) / 255, green: CGFloat(g) / 255, blue: CGFloat(b) / 255, alpha: a)
}

let palettes: [String: Palette] = [
    "ember": Palette(top: rgb(40, 42, 48), bottom: rgb(16, 17, 20), phone: rgb(244, 245, 247),
                     mark: rgb(255, 107, 61), edge: rgb(255, 255, 255, 0.14), sheen: 0.07),
    "azure": Palette(top: rgb(40, 42, 48), bottom: rgb(16, 17, 20), phone: rgb(244, 245, 247),
                     mark: rgb(10, 132, 255), edge: rgb(255, 255, 255, 0.14), sheen: 0.07),
    "light": Palette(top: rgb(255, 255, 255), bottom: rgb(226, 228, 234), phone: rgb(29, 29, 31),
                     mark: rgb(0, 122, 255), edge: rgb(0, 0, 0, 0.08), sheen: 0.0),
]

// The macOS icon shape: a superellipse (n = 5), close to Apple's continuous-corner square.
func squircle(_ rect: CGRect) -> CGPath {
    let path = CGMutablePath()
    let n = 5.0, steps = 720
    let a = rect.width / 2, b = rect.height / 2, cx = rect.midX, cy = rect.midY
    for i in 0...steps {
        let t = Double(i) / Double(steps) * 2 * .pi
        let c = cos(t), s = sin(t)
        let x = cx + a * CGFloat(copysign(pow(abs(c), 2 / n), c))
        let y = cy + b * CGFloat(copysign(pow(abs(s), 2 / n), s))
        i == 0 ? path.move(to: CGPoint(x: x, y: y)) : path.addLine(to: CGPoint(x: x, y: y))
    }
    path.closeSubpath()
    return path
}

// The mark's two parts, on the 1024 canvas of a finished icon (the body is 824 wide there).
func drawPhone(_ ctx: CGContext, _ color: CGColor) {
    let phone = CGRect(x: 512 - 300, y: 512 - 180, width: 600, height: 360)
    ctx.addPath(CGPath(roundedRect: phone, cornerWidth: 100, cornerHeight: 100, transform: nil))
    ctx.setStrokeColor(color)
    ctx.setLineWidth(46)
    ctx.strokePath()
    ctx.setFillColor(color)
    ctx.fillEllipse(in: CGRect(x: phone.minX + 62 - 15, y: 512 - 15, width: 30, height: 30))
}

func drawCrosshair(_ ctx: CGContext, _ color: CGColor) {
    ctx.setStrokeColor(color)
    ctx.setLineWidth(36)
    ctx.setLineCap(.round)
    ctx.strokeEllipse(in: CGRect(x: 512 - 56, y: 512 - 56, width: 112, height: 112))
    for (dx, dy) in [(1.0, 0.0), (-1.0, 0.0), (0.0, 1.0), (0.0, -1.0)] {
        ctx.move(to: CGPoint(x: 512 + 90 * dx, y: 512 + 90 * dy))
        ctx.addLine(to: CGPoint(x: 512 + 124 * dx, y: 512 + 124 * dy))
    }
    ctx.strokePath()
}

func write(_ ctx: CGContext, _ path: String) {
    let rep = NSBitmapImageRep(cgImage: ctx.makeImage()!)
    try! rep.representation(using: .png, properties: [:])!.write(to: URL(fileURLWithPath: path))
}

let args = CommandLine.arguments
let space = CGColorSpace(name: CGColorSpace.sRGB)!
func canvas(_ size: Int) -> CGContext {
    CGContext(data: nil, width: size, height: size, bitsPerComponent: 8, bytesPerRow: 0, space: space,
              bitmapInfo: CGImageAlphaInfo.premultipliedLast.rawValue)!
}

// The layers of the Icon Composer icon (packaging/macos/icon/GameScrcpy.icon): each part alone on a 1024 canvas
// that is the whole icon, edge to edge (macOS draws the shape, the margin and the glass itself).
if args.count == 3 && args[1] == "layers" {
    for (name, draw) in [("phone", { drawPhone($0, rgb(244, 245, 247)) }),
                         ("crosshair", { drawCrosshair($0, rgb(10, 132, 255)) })] as [(String, (CGContext) -> Void)] {
        let ctx = canvas(1024)
        ctx.translateBy(x: 512, y: 512)
        ctx.scaleBy(x: 1024.0 / 824.0, y: 1024.0 / 824.0)
        ctx.translateBy(x: -512, y: -512)
        draw(ctx)
        write(ctx, args[2] + "/" + name + ".png")
    }
    exit(0)
}

guard args.count == 4, let palette = palettes[args[1]], let size = Int(args[2]) else {
    FileHandle.standardError.write("usage: make_icon.swift azure|ember|light <size> <out.png> | layers <dir>\n".data(using: .utf8)!)
    exit(1)
}
let ctx = canvas(size)
let k = CGFloat(size) / 1024
ctx.scaleBy(x: k, y: k)

// The body on Apple's grid: 824 of 1024, with the standard soft shadow below it.
let body = CGRect(x: 100, y: 100, width: 824, height: 824)
let shape = squircle(body)
ctx.saveGState()
ctx.setShadow(offset: CGSize(width: 0, height: -10), blur: 24, color: rgb(0, 0, 0, 0.30))
ctx.addPath(shape)
ctx.setFillColor(palette.bottom)
ctx.fillPath()
ctx.restoreGState()

ctx.saveGState()
ctx.addPath(shape)
ctx.clip()
let fill = CGGradient(colorsSpace: space, colors: [palette.bottom, palette.top] as CFArray, locations: [0, 1])!
ctx.drawLinearGradient(fill, start: CGPoint(x: 512, y: 100), end: CGPoint(x: 512, y: 924), options: [])
if palette.sheen > 0 {
    let sheen = CGGradient(colorsSpace: space, colors: [rgb(255, 255, 255, 0), rgb(255, 255, 255, palette.sheen)] as CFArray,
                           locations: [0, 1])!
    ctx.drawLinearGradient(sheen, start: CGPoint(x: 512, y: 560), end: CGPoint(x: 512, y: 924), options: [])
}
ctx.restoreGState()
// A hairline edge, as light catches the rim of a glass tile.
ctx.addPath(shape)
ctx.setStrokeColor(palette.edge)
ctx.setLineWidth(3)
ctx.strokePath()

drawPhone(ctx, palette.phone)
drawCrosshair(ctx, palette.mark)
write(ctx, args[3])
