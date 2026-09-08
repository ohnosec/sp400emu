import pathlib
import struct
import sys
import tempfile
import unittest


PROJECT_ROOT = pathlib.Path(__file__).resolve().parents[1]
sys.path.insert(0, str(PROJECT_ROOT / "scripts"))

import embed_cursor_assets


class EmbedCursorAssetsTest(unittest.TestCase):
    def test_decodes_bottom_up_bgra_pixels(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "two_pixels.bmp"
            header = bytearray(54)
            header[0:2] = b"BM"
            struct.pack_into("<I", header, 2, 62)
            struct.pack_into("<I", header, 10, 54)
            struct.pack_into("<I", header, 14, 40)
            struct.pack_into("<i", header, 18, 1)
            struct.pack_into("<i", header, 22, 2)
            struct.pack_into("<H", header, 26, 1)
            struct.pack_into("<H", header, 28, 32)
            struct.pack_into("<I", header, 34, 8)
            bottom_green = bytes((0, 255, 0, 255))
            top_red = bytes((0, 0, 255, 128))
            path.write_bytes(header + bottom_green + top_red)

            bitmap = embed_cursor_assets.read_bmp(path)

            self.assertEqual(
                bitmap.rgba, bytes((255, 0, 0, 128, 0, 255, 0, 255))
            )

    def test_decodes_monochrome_cursors_with_transparency(self):
        for name in ("open_hand.bmp", "closed_hand.bmp"):
            bitmap = embed_cursor_assets.read_bmp(
                PROJECT_ROOT / "assets" / "cursors" / name
            )

            self.assertEqual((bitmap.width, bitmap.height), (32, 32))
            alphas = bitmap.rgba[3::4]
            self.assertIn(0, alphas)
            self.assertIn(255, alphas)
            for offset in range(0, len(bitmap.rgba), 4):
                red, green, blue, alpha = bitmap.rgba[offset : offset + 4]
                if alpha != 0:
                    self.assertEqual(red, green)
                    self.assertEqual(green, blue)
                    self.assertIn(red, (0, 255))

    def test_generated_header_contains_both_cursor_assets(self):
        header = embed_cursor_assets.generate_header(
            PROJECT_ROOT / "assets" / "cursors" / "open_hand.bmp",
            PROJECT_ROOT / "assets" / "cursors" / "closed_hand.bmp",
        )

        self.assertIn("OPEN_HAND_RGBA", header)
        self.assertIn("CLOSED_HAND_RGBA", header)
        self.assertIn("CURSOR_WIDTH = 32", header)
        self.assertIn("CURSOR_HEIGHT = 32", header)

    def test_rejects_non_32_bit_bitmap(self):
        with tempfile.TemporaryDirectory() as directory:
            path = pathlib.Path(directory) / "invalid.bmp"
            header = bytearray(54)
            header[0:2] = b"BM"
            struct.pack_into("<I", header, 10, 54)
            struct.pack_into("<I", header, 14, 40)
            struct.pack_into("<i", header, 18, 1)
            struct.pack_into("<i", header, 22, 1)
            struct.pack_into("<H", header, 26, 1)
            struct.pack_into("<H", header, 28, 24)
            path.write_bytes(header + b"\x00\x00\x00\x00")

            with self.assertRaisesRegex(ValueError, "32-bit"):
                embed_cursor_assets.read_bmp(path)


if __name__ == "__main__":
    unittest.main()
