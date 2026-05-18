from pathlib import Path
import shutil
import subprocess

BASE_DIR = Path(__file__).resolve().parent

source_exe = (
    BASE_DIR.parent
    / "generated"
    / "bin"
    / "Release"
    / "ReactionDiffusion.exe"
)

if not source_exe.exists():
    raise FileNotFoundError(
        f"Could not find {source_exe}. Please build RDPG in Release first."
    )

# Optional frame output
ENABLE_FRAME_OUTPUT = False
FRAME_OUTPUT_FREQ = 99

# Model directories
model_dirs = [chr(c) for c in range(ord('B'), ord('O') + 1)]
model_dirs += [
    "202512_Figure6",
    "Prepatterns",
]

# Sequentially run models
for model in model_dirs:
    target_dir = BASE_DIR / model

    if not target_dir.exists():
        print(f"Skipping missing directory: {target_dir}")
        continue

    target_exe = target_dir / source_exe.name
    shutil.copy2(source_exe, target_exe)

    print(f"Copied to: {target_exe}")

    args = [
        str(target_exe),
        "Run",
        "SaveOnExit",
        "Steps=200000",
    ]

    if ENABLE_FRAME_OUTPUT:
        args.append("ScreenShot")
        args.append(f"FrameOutputFreq={FRAME_OUTPUT_FREQ}")

    result = subprocess.run(
        args,
        cwd=target_dir,
    )

    print(f"Finished {model} with return code {result.returncode}")