import novasvg
from novasvg_fixtures import *

class TestBindings:
    """Test cases for direct C++ class bindings."""

    def test_document_load_from_string(self, sample_svg_content):
        doc = novasvg.Document.load_from_data(sample_svg_content)
        assert doc is not None
        assert doc.width == 100
        assert doc.height == 100

    def test_document_get_element_by_id(self):
        svg = '<svg id="root" width="10" height="10"><rect id="myRect" x="5" y="5" width="5" height="5"/></svg>'
        doc = novasvg.Document.load_from_data(svg)
        rect = doc.get_element_by_id("myRect")
        assert not rect.is_null()
        assert rect.has_attribute("x")

    def test_matrix_operations(self):
        m = novasvg.Matrix()
        assert m.a == 1.0
        assert m.e == 0.0
        
        m.translate(10, 20)
        assert m.e == 10.0
        assert m.f == 20.0

    def test_bitmap_properties(self):
        # Create a 10x10 bitmap
        bmp = novasvg.Bitmap(10, 10)
        assert not bmp.is_null()
        assert bmp.width == 10
        assert bmp.height == 10
        assert bmp.stride == 40 # 10 * 4 bytes (ARGB)

    def test_bitmap_clear_and_convert(self):
        bmp = novasvg.Bitmap(10, 10)
        # Clear with Red (0xFF0000FF)
        bmp.clear(0xFF0000FF)
        # Just ensure it doesn't crash
        bmp.convert_to_rgba()

# --- Bitmap image encoders (to_png / to_bmp / to_tga / to_jpg / to_bytes / write*) ---

import struct
import zlib

import pytest


def _decode_png_rgba(data):
    """Minimal 8-bit RGBA non-interlaced PNG decoder (all five row filters),
    just enough to check what novasvg's encoder actually wrote -- pixels as
    (r, g, b, a) tuples, row-major -- without depending on Pillow."""
    assert data[:8] == b"\x89PNG\r\n\x1a\n"
    pos, idat = 8, b""
    width = height = None
    while pos < len(data):
        length, kind = struct.unpack(">I4s", data[pos:pos + 8])
        body = data[pos + 8:pos + 8 + length]
        if kind == b"IHDR":
            width, height, depth, ctype, _, _, interlace = struct.unpack(">IIBBBBB", body)
            assert (depth, ctype, interlace) == (8, 6, 0)
        elif kind == b"IDAT":
            idat += body
        pos += 12 + length
    raw, stride = zlib.decompress(idat), width * 4
    rows, prev = [], bytes(stride)
    for y in range(height):
        f, line = raw[y * (stride + 1)], bytearray(raw[y * (stride + 1) + 1:(y + 1) * (stride + 1)])
        for i in range(stride):
            a = line[i - 4] if i >= 4 else 0
            b, c = prev[i], (prev[i - 4] if i >= 4 else 0)
            if f == 1:
                line[i] = (line[i] + a) & 255
            elif f == 2:
                line[i] = (line[i] + b) & 255
            elif f == 3:
                line[i] = (line[i] + (a + b) // 2) & 255
            elif f == 4:
                p = a + b - c
                pa, pb, pc = abs(p - a), abs(p - b), abs(p - c)
                pred = a if pa <= pb and pa <= pc else (b if pb <= pc else c)
                line[i] = (line[i] + pred) & 255
        rows.append(bytes(line))
        prev = bytes(line)
    return width, height, [tuple(r[x * 4:x * 4 + 4]) for r in rows for x in range(width)]


def _solid_bitmap(fill, opacity=1.0, size=8):
    svg = (f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}">'
           f'<rect width="{size}" height="{size}" fill="{fill}" fill-opacity="{opacity}"/></svg>')
    return novasvg.Document.load_from_data(svg).render_to_bitmap(size, size, 0)


class TestBitmapEncoders:
    def test_to_png_is_a_valid_png_with_correct_colors(self):
        # Asymmetric on purpose: a red/blue swap (double unpremultiply/swizzle)
        # must not be able to hide behind a symmetric color.
        w, h, px = _decode_png_rgba(_solid_bitmap("#336699").to_png())
        assert (w, h) == (8, 8)
        assert set(px) == {(0x33, 0x66, 0x99, 255)}

    def test_to_png_keeps_semi_transparent_color(self):
        _, _, px = _decode_png_rgba(_solid_bitmap("#ff0000", opacity=0.5).to_png())
        r, g, b, a = px[0]
        assert (r, g, b) == (255, 0, 0)  # straight (not premultiplied) alpha
        assert abs(a - 127) <= 1

    def test_convert_to_rgba_before_encoding_corrupts_colors(self):
        """Documents *why* the docstrings say not to: the encoders already
        convert internally, so converting first converts twice."""
        bmp = _solid_bitmap("#336699")
        bmp.convert_to_rgba()
        _, _, px = _decode_png_rgba(bmp.to_png())
        assert px[0] != (0x33, 0x66, 0x99, 255)

    def test_to_png_matches_write_to_png_file(self, tmp_path):
        bmp = _solid_bitmap("#336699")
        path = tmp_path / "out.png"
        assert bmp.write_to_png(str(path)) is True
        assert path.read_bytes() == bmp.to_png()

    def test_to_bmp_tga_jpg_produce_their_formats(self):
        bmp = _solid_bitmap("#336699")
        assert bmp.to_bmp()[:2] == b"BM"
        assert len(bmp.to_tga()) > 18
        jpg = bmp.to_jpg()
        assert jpg[:2] == b"\xff\xd8" and jpg[-2:] == b"\xff\xd9"

    def test_to_jpg_quality_changes_output(self):
        svg = ('<svg xmlns="http://www.w3.org/2000/svg" width="64" height="64">'
               '<circle cx="32" cy="32" r="20" fill="#e91e63"/><rect x="4" y="4" width="20" height="9" fill="#1e88e5"/></svg>')
        bmp = novasvg.Document.load_from_data(svg).render_to_bitmap(64, 64, 0xFFFFFFFF)
        assert len(bmp.to_jpg(quality=10)) < len(bmp.to_jpg(quality=95))

    @pytest.mark.parametrize("fmt,magic", [("png", b"\x89PNG"), ("PNG", b"\x89PNG"), (".png", b"\x89PNG"),
                                            ("bmp", b"BM"), ("jpg", b"\xff\xd8"), ("jpeg", b"\xff\xd8"), (".JPG", b"\xff\xd8")])
    def test_to_bytes_dispatches_on_format(self, fmt, magic):
        assert _solid_bitmap("#336699").to_bytes(fmt).startswith(magic)

    def test_to_bytes_default_is_png(self):
        assert _solid_bitmap("#336699").to_bytes().startswith(b"\x89PNG")

    def test_to_bytes_rejects_unknown_format(self):
        with pytest.raises(ValueError, match="unknown image format"):
            _solid_bitmap("#336699").to_bytes("gif")

    @pytest.mark.parametrize("method", ["to_png", "to_bmp", "to_tga", "to_jpg"])
    def test_null_bitmap_raises_instead_of_returning_empty(self, method):
        with pytest.raises(ValueError, match="null Bitmap"):
            getattr(novasvg.Bitmap(), method)()

    def test_write_picks_format_from_extension(self, tmp_path):
        bmp = _solid_bitmap("#336699")
        for name, magic in [("a.png", b"\x89PNG"), ("a.bmp", b"BM"), ("a.jpg", b"\xff\xd8"), ("a.JPEG", b"\xff\xd8"), ("a.unknown", b"\x89PNG")]:
            path = tmp_path / name
            assert bmp.write(str(path)) is True
            assert path.read_bytes().startswith(magic), name

    def test_write_to_bmp_tga_jpg_files(self, tmp_path):
        bmp = _solid_bitmap("#336699")
        assert bmp.write_to_bmp(str(tmp_path / "a.bmp")) and (tmp_path / "a.bmp").read_bytes()[:2] == b"BM"
        assert bmp.write_to_tga(str(tmp_path / "a.tga")) and (tmp_path / "a.tga").stat().st_size > 18
        assert bmp.write_to_jpg(str(tmp_path / "a.jpg"), 90) and (tmp_path / "a.jpg").read_bytes()[:2] == b"\xff\xd8"
