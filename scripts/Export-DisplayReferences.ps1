#requires -Version 7.2
<#
.SYNOPSIS
Export historical sprites and a pixel-exact size inventory from the original client archive.
.EXAMPLE
pwsh -File ./scripts/Export-DisplayReferences.ps1
#>
[CmdletBinding()]
param(
    [string]$Pack = (Join-Path $PSScriptRoot '../Tenhou.Client.Origin/tenhou/_res2.dat'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '../build/display-reference')
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not ('Tenhou.DisplayReferences' -as [type])) {
    $references = @('System.Drawing.Common', 'System.Drawing.Primitives', 'System.IO.Compression', 'System.Collections', 'System.Runtime.InteropServices')
    $references += @(Get-ChildItem -LiteralPath $PSHOME -Filter 'System.Private.Windows.*.dll' | ForEach-Object FullName)
    Add-Type -ReferencedAssemblies $references -TypeDefinition @'
using System;
using System.IO;
using System.IO.Compression;
using System.Collections.Generic;
using System.Drawing;
using System.Drawing.Imaging;
using System.Runtime.InteropServices;

namespace Tenhou {
    public class SpriteReference {
        public uint Id { get; set; }
        public int Width { get; set; }
        public int Height { get; set; }
        public int StoredBytes { get; set; }
        public int PngBytes { get; set; }
        public int InkX { get; set; }
        public int InkY { get; set; }
        public int InkWidth { get; set; }
        public int InkHeight { get; set; }
        public int AlphaMin { get; set; }
        public int AlphaMax { get; set; }
        public string File { get; set; }
    }
    public static class DisplayReferences {
        static byte[] Expand(byte[] bytes, bool gzip, int limit) {
            using (var input = new MemoryStream(bytes))
            using (Stream stream = gzip
                ? (Stream)new GZipStream(input, CompressionMode.Decompress)
                : new ZLibStream(input, CompressionMode.Decompress))
            using (var output = new MemoryStream()) {
                var buffer = new byte[32768]; int count;
                while ((count = stream.Read(buffer, 0, buffer.Length)) != 0) {
                    if (output.Length + count > limit) throw new InvalidDataException("Resource too large");
                    output.Write(buffer, 0, count);
                }
                return output.ToArray();
            }
        }
        public static SpriteReference[] Export(string source, string directory) {
            var decoded = Expand(System.IO.File.ReadAllBytes(source), true, 64 * 1024 * 1024);
            if (decoded.Length < 4 || System.Text.Encoding.ASCII.GetString(decoded, 0, 4) != "res2")
                throw new InvalidDataException("Expected HD res2 pack");
            Directory.CreateDirectory(Path.Combine(directory, "sprites"));
            var result = new List<SpriteReference>(); var ids = new HashSet<uint>();
            using (var reader = new BinaryReader(new MemoryStream(decoded))) {
                reader.ReadUInt32();
                while (reader.BaseStream.Position < decoded.Length) {
                    if (decoded.Length - reader.BaseStream.Position < 12) throw new InvalidDataException("Truncated header");
                    uint id = reader.ReadUInt32(), metadata = reader.ReadUInt32(), size = reader.ReadUInt32();
                    if (!ids.Add(id) || size > decoded.Length - reader.BaseStream.Position)
                        throw new InvalidDataException("Invalid record");
                    var payload = reader.ReadBytes(checked((int)size));
                    if (metadata == 0) continue; // WAV, deliberately excluded.
                    int width = (int)(metadata & 65535), height = (int)(metadata >> 16);
                    if (width < 1 || height < 1 || width > 8192 || height > 8192)
                        throw new InvalidDataException("Invalid dimensions");
                    int length = checked(width * height * 4);
                    if (length > 64 * 1024 * 1024) throw new InvalidDataException("Image too large");
                    var pixels = payload.Length == length ? payload : Expand(payload, false, length);
                    if (pixels.Length != length) throw new InvalidDataException("Pixel count mismatch");
                    var item = new SpriteReference { Id = id, Width = width, Height = height,
                        StoredBytes = payload.Length, AlphaMin = 255, AlphaMax = 0,
                        File = "sprites/" + id + ".png" };
                    int left = width, top = height, right = -1, bottom = -1;
                    for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
                        int alpha = pixels[(y * width + x) * 4 + 3];
                        item.AlphaMin = Math.Min(item.AlphaMin, alpha); item.AlphaMax = Math.Max(item.AlphaMax, alpha);
                        if (alpha == 0) continue;
                        left = Math.Min(left, x); top = Math.Min(top, y);
                        right = Math.Max(right, x); bottom = Math.Max(bottom, y);
                    }
                    if (right >= 0) { item.InkX = left; item.InkY = top; item.InkWidth = right - left + 1; item.InkHeight = bottom - top + 1; }
                    using (var bitmap = new Bitmap(width, height, PixelFormat.Format32bppArgb)) {
                        var bits = bitmap.LockBits(new Rectangle(0, 0, width, height), ImageLockMode.WriteOnly, PixelFormat.Format32bppArgb);
                        try { for (int y = 0; y < height; ++y) Marshal.Copy(pixels, y * width * 4, IntPtr.Add(bits.Scan0, y * bits.Stride), width * 4); }
                        finally { bitmap.UnlockBits(bits); }
                        string path = Path.Combine(directory, item.File);
                        bitmap.Save(path, ImageFormat.Png);
                        item.PngBytes = checked((int)new FileInfo(path).Length);
                        // Verify the actual PNG output, including transparent pixels.
                        using (var check = new Bitmap(path)) {
                            if (check.Width != width || check.Height != height)
                                throw new InvalidDataException("PNG dimensions changed: " + id);
                            var read = check.LockBits(new Rectangle(0, 0, width, height), ImageLockMode.ReadOnly, PixelFormat.Format32bppArgb);
                            try {
                                var row = new byte[width * 4];
                                for (int y = 0; y < height; ++y) {
                                    Marshal.Copy(IntPtr.Add(read.Scan0, y * read.Stride), row, 0, row.Length);
                                    for (int x = 0; x < row.Length; ++x)
                                        if (row[x] != pixels[y * row.Length + x])
                                            throw new InvalidDataException("PNG pixels changed: " + id);
                                }
                            } finally { check.UnlockBits(read); }
                        }
                    }
                    result.Add(item);
                }
            }
            return result.ToArray();
        }
        public static void Sheet(SpriteReference[] items, string directory, string filename, int first, int last) {
            var selected = Array.FindAll(items, i => i.Id >= first && i.Id <= last);
            if (selected.Length == 0) return;
            const int columns = 3, cellWidth = 330, cellHeight = 112;
            int rows = (selected.Length + columns - 1) / columns;
            using (var bitmap = new Bitmap(columns * cellWidth, rows * cellHeight))
            using (var graphics = Graphics.FromImage(bitmap))
            using (var font = new Font("Consolas", 12, GraphicsUnit.Pixel))
            using (var light = new SolidBrush(Color.FromArgb(70, 70, 70))) {
                graphics.Clear(Color.FromArgb(48, 48, 48));
                for (int i = 0; i < selected.Length; ++i) {
                    int x = i % columns * cellWidth, y = i / columns * cellHeight;
                    graphics.DrawString(selected[i].Id + "   " + selected[i].Width + " x " + selected[i].Height, font, Brushes.White, x + 8, y + 5);
                    for (int cy = 28; cy < cellHeight; cy += 12) for (int cx = 0; cx < cellWidth; cx += 12)
                        if ((cx / 12 + cy / 12) % 2 == 0) graphics.FillRectangle(light, x + cx, y + cy, 12, Math.Min(12, cellHeight - cy));
                    using (var image = System.Drawing.Image.FromFile(Path.Combine(directory, selected[i].File)))
                        graphics.DrawImageUnscaled(image, x + 8, y + 35);
                }
                bitmap.Save(Path.Combine(directory, filename), ImageFormat.Png);
            }
        }
    }
}
'@
}
$source = (Resolve-Path -LiteralPath $Pack).Path
$destination = [System.IO.Path]::GetFullPath($OutputDirectory)
$items = [Tenhou.DisplayReferences]::Export($source, $destination)
$catalog = Get-Content -LiteralPath (Join-Path $PSScriptRoot '../Tenhou.Client/assets/text/catalog.json') -Raw -Encoding utf8 |
    ConvertFrom-Json
$labels = @{ 440 = '按钮底板' }
foreach ($entry in $catalog.texts) { $labels[[int]$entry.origin_id] = $entry.source.label }
$items | Sort-Object Id | Select-Object Id, @{Name='Label'; Expression={ $labels[[int]$_.Id] }},
    Width, Height, StoredBytes, PngBytes, InkX, InkY, InkWidth, InkHeight, AlphaMin, AlphaMax, File |
    Export-Csv -LiteralPath (Join-Path $destination 'original-display.csv') -NoTypeInformation -Encoding utf8BOM
[Tenhou.DisplayReferences]::Sheet($items, $destination, 'buttons.png', 440, 458)
[Tenhou.DisplayReferences]::Sheet($items, $destination, 'actions.png', 473, 480)
[Tenhou.DisplayReferences]::Sheet($items, $destination, 'yaku.png', 481, 535)
[ordered]@{
    source = [System.IO.Path]::GetRelativePath((Split-Path -Parent $PSScriptRoot), $source).Replace('\', '/')
    sha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
    canvas = @(1440, 1080)
    image_count = $items.Count
    pixels = 'top-down straight-alpha BGRA -> lossless RGBA PNG; no resizing'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $destination 'source.json') -Encoding utf8

Write-Host "Exported $($items.Count) original sprites to $destination"
