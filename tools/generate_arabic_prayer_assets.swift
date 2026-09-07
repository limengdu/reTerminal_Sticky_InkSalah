import AppKit
import Foundation

struct PrayerLabel {
    let enumName: String
    let symbolName: String
    let text: String
}

struct PackedBitmap {
    let width: Int
    let height: Int
    let stride: Int
    let bytes: [UInt8]
}

let labels = [
    PrayerLabel(enumName: "Fajr", symbolName: "Fajr", text: "\u{0641}\u{062C}\u{0631}"),
    PrayerLabel(enumName: "Sunrise", symbolName: "Sunrise", text: "\u{0634}\u{0631}\u{0648}\u{0642}"),
    PrayerLabel(enumName: "Dhuhr", symbolName: "Dhuhr", text: "\u{0638}\u{0647}\u{0631}"),
    PrayerLabel(enumName: "Asr", symbolName: "Asr", text: "\u{0639}\u{0635}\u{0631}"),
    PrayerLabel(enumName: "Maghrib", symbolName: "Maghrib", text: "\u{0645}\u{063A}\u{0631}\u{0628}"),
    PrayerLabel(enumName: "Isha", symbolName: "Isha", text: "\u{0639}\u{0634}\u{0627}\u{0621}"),
    PrayerLabel(enumName: "Qiyam", symbolName: "Qiyam", text: "\u{0642}\u{064A}\u{0627}\u{0645}"),
]

func render(_ text: String) -> PackedBitmap {
    let canvasWidth = 240
    let canvasHeight = 52
    guard let bitmap = NSBitmapImageRep(
        bitmapDataPlanes: nil,
        pixelsWide: canvasWidth,
        pixelsHigh: canvasHeight,
        bitsPerSample: 8,
        samplesPerPixel: 4,
        hasAlpha: true,
        isPlanar: false,
        colorSpaceName: .deviceRGB,
        bytesPerRow: canvasWidth * 4,
        bitsPerPixel: 32
    ), let context = NSGraphicsContext(bitmapImageRep: bitmap) else {
        fatalError("Unable to create the Arabic label bitmap")
    }

    NSGraphicsContext.saveGraphicsState()
    NSGraphicsContext.current = context
    NSColor.white.setFill()
    NSRect(x: 0, y: 0, width: canvasWidth, height: canvasHeight).fill()

    let paragraph = NSMutableParagraphStyle()
    paragraph.alignment = .center
    paragraph.baseWritingDirection = .rightToLeft
    guard let font = NSFont(name: "GeezaPro", size: 24) else {
        fatalError("Geeza Pro is required for shaped Arabic labels")
    }
    let attributes: [NSAttributedString.Key: Any] = [
        .font: font,
        .foregroundColor: NSColor.black,
        .paragraphStyle: paragraph,
    ]
    NSString(string: text).draw(
        in: NSRect(x: 0, y: 7, width: canvasWidth, height: 40),
        withAttributes: attributes
    )
    context.flushGraphics()
    NSGraphicsContext.restoreGraphicsState()

    var minimumX = canvasWidth
    var minimumY = canvasHeight
    var maximumX = -1
    var maximumY = -1
    for y in 0..<canvasHeight {
        for x in 0..<canvasWidth {
            guard let color = bitmap.colorAt(x: x, y: y)?.usingColorSpace(.deviceRGB) else {
                continue
            }
            if color.redComponent < 0.55 {
                minimumX = min(minimumX, x)
                minimumY = min(minimumY, y)
                maximumX = max(maximumX, x)
                maximumY = max(maximumY, y)
            }
        }
    }
    guard maximumX >= minimumX, maximumY >= minimumY else {
        fatalError("Arabic text rendered as an empty bitmap")
    }

    minimumX = max(0, minimumX - 2)
    minimumY = max(0, minimumY - 2)
    maximumX = min(canvasWidth - 1, maximumX + 2)
    maximumY = min(canvasHeight - 1, maximumY + 2)
    let width = maximumX - minimumX + 1
    let height = maximumY - minimumY + 1
    let stride = (width + 7) / 8
    var bytes = [UInt8](repeating: 0, count: stride * height)
    for row in 0..<height {
        for column in 0..<width {
            guard let color = bitmap.colorAt(
                x: minimumX + column,
                y: minimumY + row
            )?.usingColorSpace(.deviceRGB) else {
                continue
            }
            if color.redComponent < 0.55 {
                bytes[row * stride + column / 8] |= UInt8(1 << (7 - column % 8))
            }
        }
    }
    return PackedBitmap(width: width, height: height, stride: stride, bytes: bytes)
}

func byteLines(_ bytes: [UInt8]) -> String {
    var lines: [String] = []
    for start in stride(from: 0, to: bytes.count, by: 12) {
        let end = min(start + 12, bytes.count)
        let values = bytes[start..<end].map { String(format: "0x%02X", $0) }
        lines.append("    " + values.joined(separator: ", ") + ",")
    }
    return lines.joined(separator: "\n")
}

guard CommandLine.arguments.count == 2 else {
    fatalError("Usage: generate_arabic_prayer_assets.swift <output.cpp>")
}

let rendered = labels.map { ($0, render($0.text)) }
var output = """
#include "arabic_prayer_assets.h"

namespace {

"""
for (label, bitmap) in rendered {
    output += "constexpr uint8_t k\(label.symbolName)[] = {\n"
    output += byteLines(bitmap.bytes)
    output += "\n};\n\n"
}
output += """
}  // namespace

ArabicPrayerAsset arabic_time_asset(PrayerTimeSlot prayer)
{
    switch (prayer) {
"""
for (label, bitmap) in rendered {
    output += "    case PrayerTimeSlot::\(label.enumName):\n"
    output += "        return {\(bitmap.width)U, \(bitmap.height)U, \(bitmap.stride)U, k\(label.symbolName)};\n"
}
output += """
    default:
        return {};
    }
}

ArabicPrayerAsset arabic_prayer_asset(PrayerId prayer)
{
    switch (prayer) {
    case PrayerId::Fajr: return arabic_time_asset(PrayerTimeSlot::Fajr);
    case PrayerId::Dhuhr: return arabic_time_asset(PrayerTimeSlot::Dhuhr);
    case PrayerId::Asr: return arabic_time_asset(PrayerTimeSlot::Asr);
    case PrayerId::Maghrib: return arabic_time_asset(PrayerTimeSlot::Maghrib);
    case PrayerId::Isha: return arabic_time_asset(PrayerTimeSlot::Isha);
    default: return {};
    }
}
"""

try output.write(
    to: URL(fileURLWithPath: CommandLine.arguments[1]),
    atomically: true,
    encoding: .utf8
)
