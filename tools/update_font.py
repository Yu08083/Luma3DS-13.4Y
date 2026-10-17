import re
from pathlib import Path

root = Path(__file__).resolve().parents[1]
glyphs = {}
for block in (root / 'assets/fonts/misaki_gothic_2nd.bdf').read_text(encoding='utf-8').split('STARTCHAR ')[1:]:
    codepoint = int(re.search(r'^ENCODING (\d+)$', block, re.M)[1])
    width, height, dx, dy = map(int, re.search(r'^BBX ([^\n]+)', block, re.M)[1].split())
    rows = [0] * 8
    for row, value in enumerate(block.split('BITMAP\n')[1].split('ENDCHAR')[0].split()):
        y = 7 - dy - height + row
        if 0 <= y < 8:
            bits = int(value, 16)
            rows[y] = ((bits >> dx) if dx >= 0 else (bits << -dx)) & 255
    glyphs[codepoint] = rows

targets = [
    ('Luma3DS_v13_4 Rosalina', root / 'Luma3DS_v13_4/sysmodules/rosalina/source', root / 'Luma3DS_v13_4/sysmodules/rosalina/include'),
    ('Luma3DS_v13_4 boot', root / 'Luma3DS_v13_4/arm9/source', root / 'Luma3DS_v13_4/arm9/source'),
]
for version, source_directory, include_directory in targets:
    required = set()
    for path in source_directory.rglob('*.c'):
        for match in re.finditer(r'"(?:\\.|[^"\\])*"', path.read_text(encoding='utf-8-sig')):
            required.update(ord(c) for c in match[0] if ord(c) >= 128)
    missing = required - glyphs.keys()
    if missing:
        raise ValueError('Missing font characters: ' + ''.join(chr(c) for c in sorted(missing)))
    characters = sorted(codepoint for codepoint in glyphs if codepoint >= 128)
    if any(codepoint > 0xFFFF for codepoint in characters):
        raise ValueError('Font codepoints must fit the 16-bit lookup table')
    text = '#pragma once\n\n#include <stdint.h>\n\nstatic const uint16_t japaneseCodepoints[] =\n{\n'
    for i in range(0, len(characters), 12):
        text += '    ' + ', '.join(f'0x{c:04X}' for c in characters[i:i+12]) + ',\n'
    text += '};\n\nstatic const uint8_t japaneseGlyphs[][8] =\n{\n'
    text += '\n'.join('    { ' + ', '.join(f'0x{row:02X}' for row in glyphs[cp]) + ' },' for cp in characters)
    text += '\n};\n'
    (include_directory / 'japanese_font.h').write_bytes(text.encode('utf-8'))
    print(version, len(characters), 'glyphs')
