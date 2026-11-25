# Rebuilds all Phase C offline-baked HD assets:
#   1. Compiles Tenhou.Client/tools/svg_bake.cpp (zero external deps, Direct2D ID2D1DeviceContext5 + WIC)
#   2. Bakes 44 mahjong_graphic SVGs into Tenhou.Client/assets/images/hd_tiles/faces.png (3072x3072)
#   3. Regenerates 7 vector UI sprites (401, 402, 440, 466, 589, 591, 592) and bakes HD PNGs
#   4. Re-extracts 694 4-point perspective tile face quads into Tenhou.Client/src/hd_tile_quads.hpp
#   5. Recompiles hd_tile_shader.hlsl into Tenhou.Client/src/hd_tile_shader_bc.hpp via fxc.exe
[CmdletBinding()]
param()

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$repoRoot = Split-Path -Parent $PSScriptRoot
Push-Location $repoRoot
try {
    $vswhere = 'C:\Program Files (x86)\Microsoft Visual Studio\Installer\vswhere.exe'
    $vsPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if (-not $vsPath) { throw 'Visual Studio C++ x64 toolset not found.' }
    $vcvars = Join-Path $vsPath 'Common7\Tools\VsDevCmd.bat'

    $objDir = Join-Path $repoRoot 'Tenhou.Client\obj\tools'
    New-Item -ItemType Directory -Force -Path $objDir | Out-Null
    $baker = Join-Path $objDir 'svg_bake.exe'
    $bakerObj = Join-Path $objDir 'svg_bake.obj'

    $compileCmd = "`"$vcvars`" -arch=x64 -host_arch=x64 -no_logo && cl /nologo /std:c++17 /O2 /W4 /WX /utf-8 /EHsc /MT Tenhou.Client\tools\svg_bake.cpp /Fe:`"$baker`" /Fo:`"$bakerObj`""
    cmd.exe /c $compileCmd
    if ($LASTEXITCODE -ne 0) { throw 'Failed to compile svg_bake.cpp' }

    & $baker --atlas Tenhou.Client\assets\sources\mahjong_graphic\atlas.manifest.txt `
             --out Tenhou.Client\assets\images\hd_tiles\faces.png `
             --cell-width 384 --cell-height 512 --cols 8 --rows 6
    if ($LASTEXITCODE -ne 0) { throw 'Failed to bake HD tile face atlas' }

    python Tenhou.Client\tools\generate_hd_sprites.py
    if ($LASTEXITCODE -ne 0) { throw 'Failed to generate HD sprite SVGs' }

    $spriteSpecs = @(
        @(401, 56, 104),
        @(402, 56, 104),
        @(440, 560, 204),
        @(466, 456, 56),
        @(589, 976, 688),
        @(591, 1456, 660),
        @(592, 1180, 860)
    )
    foreach ($s in $spriteSpecs) {
        $id, $w, $h = $s
        & $baker --svg "Tenhou.Client\assets\sources\sprites\$id.svg" `
                 --out "Tenhou.Client\assets\images\sprites\hd\$id.png" `
                 --width $w --height $h --stretch
        if ($LASTEXITCODE -ne 0) { throw "Failed to bake sprite $id" }
    }

    python Tenhou.Client\tools\extract_tile_quads.py
    if ($LASTEXITCODE -ne 0) { throw 'Failed to extract perspective tile quads' }

    $fxc = Get-ChildItem -Path 'C:\Program Files (x86)\Windows Kits\10\bin' -Filter fxc.exe -Recurse -ErrorAction SilentlyContinue |
           Where-Object { $_.FullName -match '\\x64\\fxc\.exe$' } |
           Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
    if (-not $fxc) { throw 'Windows SDK x64 fxc.exe not found.' }

    $vsHdr = Join-Path $objDir 'hd_vs.h'
    $psHdr = Join-Path $objDir 'hd_ps.h'
    & $fxc /nologo /T vs_4_0 /E VSMain /Fh $vsHdr /Vn kHdTileVsBytecode Tenhou.Client\src\hd_tile_shader.hlsl
    if ($LASTEXITCODE -ne 0) { throw 'fxc VS compilation failed' }
    & $fxc /nologo /T ps_4_0 /E PSMain /Fh $psHdr /Vn kHdTilePsBytecode Tenhou.Client\src\hd_tile_shader.hlsl
    if ($LASTEXITCODE -ne 0) { throw 'fxc PS compilation failed' }

    python -c "
import pathlib
vs = pathlib.Path(r'$vsHdr').read_text(encoding='utf-8')
ps = pathlib.Path(r'$psHdr').read_text(encoding='utf-8')
hdr = '''// Precompiled D3D11 shader bytecode from hd_tile_shader.hlsl (fxc /T vs_4_0, ps_4_0).
// Zero runtime d3dcompiler dependency.
#pragma once

namespace tenhou {
''' + vs.replace('const BYTE ', 'inline constexpr unsigned char ') + '\n' + ps.replace('const BYTE ', 'inline constexpr unsigned char ') + '''
} // namespace tenhou
'''
pathlib.Path(r'Tenhou.Client\src\hd_tile_shader_bc.hpp').write_text(hdr, encoding='utf-8')
"
    if ($LASTEXITCODE -ne 0) { throw 'Failed to write hd_tile_shader_bc.hpp' }
    Write-Host 'All HD assets, quads, sprites, and shaders baked successfully.' -ForegroundColor Green
}
finally {
    Pop-Location
}
