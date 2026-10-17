from pathlib import Path
import re
import struct
import wave
import zlib

root = Path(__file__).resolve().parents[1]
font_path = root.parent / 'Luma3DS_v13_4/sysmodules/rosalina/include/font.h'
font = bytes(int(value, 16) for value in re.findall(r'0x([0-9A-Fa-f]{2})', font_path.read_text()))
purple = bytes((0xCB, 0x9C, 0xFF))


def pixel(pixels, width, height, x, y):
    if 0 <= x < width and 0 <= y < height:
        offset = (y * width + x) * 3
        pixels[offset:offset + 3] = purple


def draw_text(pixels, width, height, text, y, scale):
    origin = (width - len(text) * 6 * scale) // 2
    for index, character in enumerate(text):
        for row in range(10):
            bits = font[ord(character) * 10 + row]
            for column in range(6):
                if bits & (1 << (6 - column)):
                    for yy in range(scale):
                        for xx in range(scale):
                            pixel(pixels, width, height, origin + (index * 6 + column) * scale + xx,
                                  y + row * scale + yy)


def draw_arrow(pixels, width, height, top, half, stem_half, arrow_height):
    center = width // 2
    for row in range(half + 1):
        for x in range(center - row, center + row + 1):
            pixel(pixels, width, height, x, top + row)
    for row in range(half, arrow_height):
        for x in range(center - stem_half, center + stem_half + 1):
            pixel(pixels, width, height, x, top + row)


def chunk(kind, data):
    return struct.pack('>I', len(data)) + kind + data + struct.pack('>I', zlib.crc32(kind + data))


def png(path, width, height, pixels):
    raw = b''.join(b'\0' + pixels[y * width * 3:(y + 1) * width * 3] for y in range(height))
    data = b'\x89PNG\r\n\x1a\n'
    data += chunk(b'IHDR', struct.pack('>IIBBBBB', width, height, 8, 2, 0, 0, 0))
    data += chunk(b'IDAT', zlib.compress(raw, 9))
    data += chunk(b'IEND', b'')
    path.write_bytes(data)


def main():
    assets = root / 'assets'
    assets.mkdir(exist_ok=True)
    icon = bytearray(48 * 48 * 3)
    draw_arrow(icon, 48, 48, 5, 8, 1, 20)
    draw_text(icon, 48, 48, '13.4Y', 32, 1)
    png(assets / 'icon48x48.png', 48, 48, icon)
    banner = bytearray(256 * 128 * 3)
    draw_arrow(banner, 256, 128, 12, 18, 4, 46)
    draw_text(banner, 256, 128, '13.4Y', 76, 4)
    png(assets / 'banner256x128.png', 256, 128, banner)
    with wave.open(str(assets / 'silent.wav'), 'wb') as audio:
        audio.setnchannels(1)
        audio.setsampwidth(2)
        audio.setframerate(32000)
        audio.writeframes(bytes(8000 * 2))


if __name__ == '__main__':
    main()
