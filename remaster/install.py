"""Prepare a local Windows install from the HD packs and an owned Syndicate DATA folder."""
import argparse
from pathlib import Path
import shutil
from zipfile import ZipFile

project = Path(__file__).resolve().parent
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("data", type=Path, help="Original Syndicate DATA directory")
args = parser.parse_args()
data = args.data.resolve()
for filename in ("INTRO.DAT", "HBLK01.DAT", "GAME01.DAT", "MSPR-0.DAT"):
    if not (data / filename).is_file():
        parser.error(f"Missing {filename} in {data}. Select the base game's DATA folder.")
play = project / "play"
if play.exists():
    parser.error(f"{play} already exists. Move it aside to keep its settings and saves.")
play.mkdir()
try:
    for pack in ('windows', 'art-sprites-a-01', 'art-sprites-a-02', 'art-sprites-a-03', 'art-sprites-a-04', 'art-sprites-a-05', 'art-sprites-b-01', 'art-sprites-b-02', 'art-sprites-b-03', 'art-sprites-b-04', 'art-sprites-b-05', 'art-terrain-01', 'art-terrain-02', 'art-terrain-03', 'art-terrain-04'):
        with ZipFile(project / f"{pack}.zip") as archive:
            archive.extractall(play)
    shutil.copytree(data, play / "original-data")
except Exception:
    # Remove only the newly created incomplete installation.
    if play.exists():
        shutil.rmtree(play)
    raise
print("Start the game with", play / "Syndicate-HD.exe")
