#requires -Version 7.2
<#
.SYNOPSIS
Export the original res2 display sprites and a pixel-exact size inventory.
.EXAMPLE
pwsh -File ./scripts/Export-DisplayReferences.ps1
#>
[CmdletBinding()]
param(
    [string]$Pack = (Join-Path $PSScriptRoot '../Tenhou.Client/assets/packs/_res2.dat'),
    [string]$OutputDirectory = (Join-Path $PSScriptRoot '../docs/Tenhou.Client/asset-reference')
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
$labels = @{
    440 = '按钮底板'; 441 = 'ポン'; 442 = 'カン'; 443 = 'チー'; 444 = 'ロン'; 445 = 'ツモ'
    446 = 'リーチ'; 447 = '流局'; 448 = '抜く'; 449 = 'パス'
    473 = 'ポン'; 474 = 'カン'; 475 = 'チー'; 476 = 'ロン'; 477 = 'ツモ'
    478 = 'リーチ'; 479 = 'パオ'; 480 = 'キタ'
}
foreach ($id in 441..449) { $labels[$id + 9] = $labels[$id] + '（悬停）' }
$settlement = Get-Content -LiteralPath (Join-Path $PSScriptRoot '../Tenhou.Client/src/settlement.cpp') -Raw -Encoding utf8
$nameBlock = [regex]::Match($settlement, 'static const char \*names\[\]\{([\s\S]*?)\};')
if (-not $nameBlock.Success) { throw 'Cannot find original yaku labels in settlement.cpp' }
$names = [regex]::Matches($nameBlock.Groups[1].Value, '"([^"]+)"')
for ($i = 0; $i -lt $names.Count; ++$i) { $labels[481 + $i] = $names[$i].Groups[1].Value }
$items | Sort-Object Id | Select-Object Id, @{Name='Label'; Expression={ $labels[[int]$_.Id] }},
    Width, Height, StoredBytes, PngBytes, InkX, InkY, InkWidth, InkHeight, AlphaMin, AlphaMax, File |
    Export-Csv -LiteralPath (Join-Path $destination 'original-display.csv') -NoTypeInformation -Encoding utf8BOM
[Tenhou.DisplayReferences]::Sheet($items, $destination, 'buttons.png', 440, 458)
[Tenhou.DisplayReferences]::Sheet($items, $destination, 'actions.png', 473, 480)
[Tenhou.DisplayReferences]::Sheet($items, $destination, 'yaku.png', 481, 535)
[ordered]@{
    source = 'Tenhou.Client/assets/packs/_res2.dat'
    sha256 = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
    canvas = @(1440, 1080)
    image_count = $items.Count
    pixels = 'top-down straight-alpha BGRA -> lossless RGBA PNG; no resizing'
} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $destination 'source.json') -Encoding utf8

# Authoring plan only: these paths are proposed names, not registered runtime assets.
$work = [System.Collections.Generic.List[object]]::new()
function Add-WorkItem($priority, $group, $text, $file, $width, $height, $reference, $state, $notes) {
    $work.Add([pscustomobject][ordered]@{
        Priority=$priority; Group=$group; Text=$text; ProposedFile=$file
        Width=$width; Height=$height; Reference=$reference; CurrentState=$state; Notes=$notes
    })
}
foreach ($text in @('选择完毕', '补花', '吃', '碰', '杠', '荣和', '自摸')) {
    $new = $text -in @('选择完毕', '补花')
    Add-WorkItem $(if ($new) { 'P0' } else { 'P1' }) 'action' $text "actions/$text.png" 220 72 '473–480' `
        $(if ($new) { '楷体动态文字' } else { '复用日文原图' }) '白字、黑色投影；四字内容须单独检查适配'
}
foreach ($text in @('万', '筒', '索', '确认换牌', '吃', '碰', '杠', '荣和', '自摸', '过')) {
    foreach ($state in @('normal', 'hover')) {
        $new = $text -in @('万', '筒', '索', '确认换牌')
        Add-WorkItem $(if ($new) { 'P0' } else { 'P1' }) 'button' $text "buttons/$text-$state.png" 140 51 `
            $(if ($state -eq 'normal') { '441–449' } else { '450–458' }) `
            $(if ($new) { '楷体动态文字，已有底板' } else { '复用日文原图，已有底板' }) `
            '仅文字透明层；共用440底板；normal参考金色光晕，hover参考白字；禁用态由程序压暗'
    }
}
$sichuanSource = Get-Content -LiteralPath (Join-Path $PSScriptRoot '../Tenhou.Server/src/Rules/Sichuan/SichuanMatch.cs') -Raw -Encoding utf8
$sichuanNames = @('底和', '清一色', '七对', '清七对', '龙七对', '清龙七对', '对对和', '清对', '带幺九', '清幺九',
    '将对', '金钩钓', '清金钩钓', '天和', '地和', '杠上花', '抢杠', '杠上炮')
foreach ($text in $sichuanNames) {
    if (-not $sichuanSource.Contains('"' + $text + '"')) { throw "Sichuan label no longer in source: $text" }
    Add-WorkItem 'P0' 'sichuan-pattern' $text "patterns/sichuan/$text.png" '按文字实测' 34 '481–535' '楷体动态文字' '仅名称；左右约3–5px透明余量；白字；不把翻数画进名称'
}
Add-WorkItem 'P0' 'sichuan-pattern' '根' 'patterns/sichuan/根.png' '按文字实测' 34 '481–535' '根×N动态文字' '根名称与动态×N分开；也可制作实际用到的根×1至根×4完整图片'
$fanSource = Get-Content -LiteralPath (Join-Path $PSScriptRoot '../Tenhou.Server/src/Rules/Guobiao/FanCatalog.cs') -Raw -Encoding utf8
$fans = [regex]::Matches($fanSource, '\["([^"]+)"\]\s*=\s*(\d+)')
if ($fans.Count -eq 0) { throw 'Cannot find Guobiao fan catalog' }
foreach ($fan in $fans) {
    $text = $fan.Groups[1].Value
    Add-WorkItem 'P0' 'guobiao-pattern' $text "patterns/guobiao/$text.png" '按文字实测' 34 '481–535' '楷体动态文字' `
        ("目录值$($fan.Groups[2].Value)番；仅名称；数量×N单独处理" + $(if ($text -eq '明暗杠') { '；项目扩展项' } else { '' }))
}
foreach ($text in @('翻', '番', '×', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9')) {
    Add-WorkItem 'P0' 'counter' $text "counters/$text.png" 34 34 '552–565' '动态文字；已有立直数字图片' '数字可复用原图；翻/番须区分，原图562是日文飜；×N需要后续拼接'
}
foreach ($text in @('梅', '兰', '竹', '菊', '春', '夏', '秋', '冬')) {
    Add-WorkItem 'P0' 'flower-face' $text "flowers/$text.png" '待定' '待定' '_obj2.dat普通牌框；VARIANT-INTERACTION.md第6节' '尚无专用八花图片' '独立图案层；先核对普通牌有效区域、方向、遮罩再定尺寸；不使用220×72文字模板'
}
foreach ($text in @('查花猪', '查大叫', '退税', '已和牌', '换三张', '定缺', '盘末结算')) {
    Add-WorkItem 'P2' 'optional-status' $text "status/$text.png" '待定' '待定' '按最终显示位置确定' '文字或组合结算原因；无独立图片槽' '装饰性候选；当前协议未逐项提供查花猪/查大叫/退税演出事件，接入需另做设计'
}
$work | Export-Csv -LiteralPath (Join-Path $destination '制作清单.csv') -NoTypeInformation -Encoding utf8BOM
Write-Host "Exported $($items.Count) original sprites to $destination"
Write-Host "Wrote $($work.Count) authoring rows, including $($sichuanNames.Count) fixed Sichuan patterns and $($fans.Count) Guobiao patterns."
