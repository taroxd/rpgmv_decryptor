# RPG Maker MV Native C++ Decrypter

This directory contains a Visual Studio C++ project that builds a native RPG Maker MV resource decrypter:

- Project file: `native_cpp/rpgmv_decrypt/rpgmv_decrypt.vcxproj`
- Solution file: `native_cpp/rpgmv_decrypt.sln`
- Output executable: `rpgmv_decrypt.exe`

The tool is implemented with C++17 standard library only and configured for static CRT linking:

- Debug: `/MTd`
- Release: `/MT`

## What it does

1. Reads `encryptionKey` from `www/data/System.json`
2. Scans all subdirectories under `www/img`
3. Decrypts encrypted RPGMV resources:
   - `.rpgmvp -> .png`
   - `.rpgmvo -> .ogg`
   - `.rpgmvm -> .m4a`
4. Writes decrypted files to the output directory with mirrored relative paths

## Supported input folder layout

The first argument can be either:

- `<input>/www/...`
- `<input>/<any_subdir>/www/...`

## Build (Visual Studio)

1. Open `rpgmv_decrypt.sln`
2. Select configuration (`Release|x64` recommended)
3. Build solution

Binary output path (default):

- `native_cpp/bin/x64/Release/rpgmv_decrypt.exe`

## Usage

```powershell
rpgmv_decrypt.exe "path/to/project" "output_dir"
```

After execution:

- Input root is resolved to a `www` folder
- Files under `www/img` are recursively decrypted
- Output directories are auto-created
- Example output:
  - Input: `...\www\img\pictures\xxx.rpgmvp`
  - Output: `\output_dir\pictures\xxx.png`

## Notes

- If file header is not standard RPGMV fake header, that file is reported as failed and skipped.
- If `System.json` has no valid hex `encryptionKey`, the tool exits with error.
