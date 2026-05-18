# Running the Models

## Build RDPG

Build the solution in Release mode first.

```bash
cd RDPG
msbuild /p:Configuration=Release ReactionDiffusion.sln
```

This generates:

```text
generated/bin/Release/ReactionDiffusion.exe
```

## Run the Models

```bash
cd models
python run.py
```

The script will:

- Copy `ReactionDiffusion.exe` into each model directory (`B` through `O`)
- Run each model sequentially
- Wait for each simulation to finish before starting the next
