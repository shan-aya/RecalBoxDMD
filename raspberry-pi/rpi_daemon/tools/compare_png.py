"""Compare, pixel par pixel, le PNG ecrit par `dmd_show --png` avec le decodage de reference du toolkit
(tools/raw565_convert.py : raw565_to_png). Usage :
    python compare_png.py image.raw565 sortie_dmd_show.png
Code de sortie 0 = identique."""
import sys
import tempfile
from pathlib import Path

from PIL import Image, ImageChops

sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "tools"))
from raw565_convert import raw565_to_png  # noqa: E402


def main(raw: Path, got: Path, zoom: int = 1) -> int:
    with tempfile.TemporaryDirectory() as d:
        ref_path = Path(d) / "ref.png"
        raw565_to_png(raw, ref_path, scale=zoom)  # NEAREST, comme le zoom de dmd_show/dmd_play
        ref = Image.open(ref_path).convert("RGB")
        out = Image.open(got).convert("RGB")
        if ref.size != out.size:
            print(f"{raw.name}: tailles differentes {ref.size} / {out.size}")
            return 1
        diff = ImageChops.difference(ref, out).getbbox()
        if diff:
            print(f"{raw.name}: DIFFERENT dans la zone {diff}")
            return 1
        print(f"{raw.name}: identique ({ref.size[0]}x{ref.size[1]})")
        return 0


if __name__ == "__main__":
    sys.exit(main(Path(sys.argv[1]), Path(sys.argv[2]), int(sys.argv[3]) if len(sys.argv) > 3 else 1))
