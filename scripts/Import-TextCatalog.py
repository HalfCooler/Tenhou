#!/usr/bin/env python3
"""Explicit XLSX -> review candidate import. Uses only the Python standard library.

The registered JSON owns semantic keys and rule mappings. This tool refreshes
design fields by OriginId; it never changes the workbook or the registered JSON.
"""
import argparse
import copy
import difflib
import hashlib
import json
from pathlib import Path
import posixpath
import sys
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[1]
NS = {"s": "http://schemas.openxmlformats.org/spreadsheetml/2006/main"}
REL = "http://schemas.openxmlformats.org/officeDocument/2006/relationships"
IMAGE_IDS = {401, 402, 440, 466, 589, 591, 592}
INFO_IDS = set(range(567, 589)) | {704, 705}
FIELDS = ("OriginId", "Label", "Content", "Type", "Note", "Size", "Color", "Style")


def normalized_style(record):
    """User-confirmed corrections; keep the original cells in entry.source."""
    result = dict(record)
    result["Color"] = result["Color"].replace("黑丝", "黑色").replace("白丝", "白色")
    if result["OriginId"] == 481:
        result.update(Size="24", Color="几乎纯白", Style="")
    return result


def read_sheet(path, sheet_name):
    with zipfile.ZipFile(path) as archive:
        workbook = ET.fromstring(archive.read("xl/workbook.xml"))
        sheets = [s for s in workbook.findall("s:sheets/s:sheet", NS) if s.get("name") == sheet_name]
        if len(sheets) != 1:
            raise ValueError(f"Expected one worksheet named {sheet_name}")
        relationships = ET.fromstring(archive.read("xl/_rels/workbook.xml.rels"))
        target = next(r.get("Target") for r in relationships if r.get("Id") == sheets[0].get(f"{{{REL}}}id"))
        member = target.lstrip("/") if target.startswith("/") else posixpath.normpath("xl/" + target)
        xml = archive.read(member)
        shared = []
        if "xl/sharedStrings.xml" in archive.namelist():
            shared = ["".join(t.text or "" for t in item.findall(".//s:t", NS))
                      for item in ET.fromstring(archive.read("xl/sharedStrings.xml")).findall("s:si", NS)]
        rows = []
        for row in ET.fromstring(xml).findall("s:sheetData/s:row", NS):
            cells = {}
            for cell in row:
                column = cell.get("r").rstrip("0123456789")
                if cell.find("s:f", NS) is not None:
                    raise ValueError(f"Formula in specification cell {cell.get('r')}; use literal values")
                value = cell.findtext("s:v", "", NS)
                if cell.get("t") == "s":
                    value = shared[int(value)]
                elif cell.get("t") == "inlineStr":
                    value = "".join(t.text or "" for t in cell.findall(".//s:t", NS))
                cells[column] = value
            if any(cells.values()):
                rows.append((int(row.get("r")), cells))
        if tuple(rows[0][1].get(chr(65 + i), "") for i in range(8)) != FIELDS:
            raise ValueError("Unexpected specification columns")
        records = []
        for row_number, cells in rows[1:]:
            record = {name: cells.get(chr(65 + i), "") for i, name in enumerate(FIELDS)}
            record["OriginId"] = int(record["OriginId"])
            record["row"] = row_number
            records.append(record)
        return records, hashlib.sha256(xml).hexdigest()


def style_for(record, styles):
    # Only fields explicitly blank in Info inherit; no generic forward fill.
    record = normalized_style(record)
    size = record["Size"]
    if record["Content"] == "Info":
        size = 16
    else:
        size = float(size)
    matches = [style["key"] for style in styles
               if style["category"] == record["Content"] and style["size_pt"] == size
               and (record["Content"] != "Info" or style["source_size"] == record["Size"])
               and style["source_color"] == record["Color"] and style["source_style"] == record["Style"]]
    if len(matches) != 1:
        raise ValueError(f"{record['OriginId']}: unregistered style; register and review the new style first")
    return matches[0]


def import_catalog(workbook, catalog):
    records, sheet_hash = read_sheet(workbook, catalog["source"]["worksheet"])
    ids = [r["OriginId"] for r in records]
    if len(set(ids)) != len(ids):
        raise ValueError("Duplicate OriginId in workbook")
    if set(ids) != set(range(401, 706)) - {434, 593}:
        raise ValueError("Expected the P0 OriginId inventory (401–705 except 434 and 593)")
    if {r["OriginId"] for r in records if r["Type"] == "Image"} != IMAGE_IDS:
        raise ValueError("Image classification differs from the seven registered images")
    if sum(r["Type"] == "Text" for r in records) != 296:
        raise ValueError("Expected 296 Text records")
    previous = {e["origin_id"]: e for e in catalog["texts"] + catalog["images"]}
    if set(previous) != set(ids):
        raise ValueError("Register semantic keys for added/removed OriginIds before importing")
    info = next(r for r in records if r["OriginId"] == 566)
    result = copy.deepcopy(catalog)
    result["source"]["sha256"] = hashlib.sha256(workbook.read_bytes()).hexdigest()
    result["source"]["worksheet_sha256"] = sheet_hash
    if result["source"]["sha256"] != catalog["source"]["sha256"]:
        result["review"]["status"] = "pending_human_review"
        result["review"]["confirmations"] = {}
        result["review"]["calibration_status"] = "pending_human_review"
    result["texts"], result["images"] = [], []
    labels = {label["key"]: label for label in result["labels"]}
    imported_labels = {}
    for raw in records:
        origin = raw["OriginId"]
        entry = copy.deepcopy(previous[origin])
        entry["source"] = {k.lower(): v for k, v in raw.items() if k != "OriginId"}
        if raw["Type"] == "Image":
            if "label" in entry or "label_key" in entry or "style" in entry:
                raise ValueError(f"{origin}: image entry contains text fields")
            result["images"].append(entry)
            continue
        if raw["Type"] != "Text" or not raw["Label"]:
            raise ValueError(f"{origin}: invalid Type or empty Label")
        normalized = dict(raw)
        inherited = []
        if origin in INFO_IDS and raw["Content"] == "Info":
            for field in ("Size", "Color", "Style"):
                if not normalized[field]:
                    normalized[field] = info[field]
                    inherited.append(field.lower())
        label_key = entry["label_key"]
        if label_key not in labels:
            raise ValueError(f"{origin}: unregistered label key {label_key}")
        if label_key in imported_labels and imported_labels[label_key] != raw["Label"]:
            raise ValueError(f"{origin}: shared Label differs between states; review label keys first")
        imported_labels[label_key] = raw["Label"]  # Preserve spaces and glyphs verbatim.
        labels[label_key]["label"] = raw["Label"]
        entry["category"] = raw["Content"]
        entry["style"] = style_for(normalized, result["styles"])
        entry["inherited_fields"] = inherited
        entry["inherits_origin_id"] = 566 if inherited else None
        entry["overrides"] = {"size_pt": 24, "style": "yaku",
                              "reason": "User confirmed 24 pt and all other settings identical to ordinary yaku"} if origin == 481 else {}
        result["texts"].append(entry)
    return result


def review_report(catalog):
    labels = {label["key"]: label["label"] for label in catalog["labels"]}
    styles = {style["key"]: style for style in catalog["styles"]}

    def cell(value):
        return str(value).replace("|", "\\|").replace("\n", "<br>")

    confirmed = catalog["review"].get("confirmations", {})
    p1_confirmed = bool(confirmed.get("p1_visual_review"))
    checked = lambda key: "x" if confirmed.get(key) else " "
    lines = ["# TrueType P0 人工核对表", "",
             ("状态：P0 已验证，P1 已完成人工视觉验证及反馈修正；交付参数见 TRUETYPE-TEXT-P1-REVIEW.md。" if p1_confirmed else
              "状态：用户已确认当前文案及下列规格；P1 效果参数仍待样张校准。") if confirmed else
             "状态：待人工核对。此表由显式导入工具生成；空白核对栏不代表通过。", "",
             f"来源：`{catalog['source']['path']}`，工作表 `{catalog['source']['worksheet']}`。", "",
             f"XLSX SHA-256：`{catalog['source']['sha256']}`。", "",
             "机械检查：296 Text、7 Image；481 使用通常役种的完整 24 pt 样式；仅 Info 空白格式继承 566。", "",
             "## 核对要点", "",
             f"- [{checked('labels')}] 全部显示文案以当前版本为准，保留原先未使用文案，并更新代码回退。",
             f"- [{checked('color_spelling')}] 黑色、白色为正确描述；旧源表中的输入法错字只保留在来源记录中。",
             "- [ ] 确认普通/悬停复用文案；自动选项的 checked/unchecked 含义正确。",
             f"- [{checked('button_states')}] 普通按钮黑边、悬停按钮橙边；441–449 / 595–598 为悬停，450–458 / 599–602 为普通。",
             f"- [{checked('yaku_481')}] 481 为 24 pt，其他设定与通常役种一致：近白、非粗体、无描边、无阴影、零额外字距。",
             f"- [{checked('chi_selection_size')}] 459 右下角小字为 12 pt，偏移留 P1 样张。",
             f"- [{checked('info_gradient')}] 566 三段独立渐变已逐像素确认；Alpha、边距和字宽基准留 P1。",
             "- [ ] 核对 101 条规则匹配名、显示文字和缺省分值；规则匹配不使用显示名反推。",
             f"- [{checked('p1_visual_review')}] P1 样张确定近白、深灰、棕、橙、红的 RGBA，以及描边/阴影参数。", "",
             "## 样式", "",
             "| 样式键 | pt | 字距 pt | 粗体 | 源颜色描述 | 源样式描述 | 核对 |",
             "| --- | ---: | ---: | --- | --- | --- | --- |"]
    for style in catalog["styles"]:
        lines.append("| " + " | ".join(map(cell, (style["key"], style["size_pt"], style["spacing_pt"],
                                                     style["bold"], style["source_color"], style["source_style"], ""))) + " |")
    lines += ["", "## 文字目录", "",
             "| OriginId | Label | 语义键 | 样式 / pt | 说明 | 核对 |",
             "| ---: | --- | --- | --- | --- | --- |"]
    for entry in catalog["texts"]:
        note = "继承 566" if entry["inherits_origin_id"] else ""
        if entry["origin_id"] == 481:
            note = "已确认 24 pt，全部样式与通常役种一致"
        if entry["composition"] != "plain":
            note = "主字 24 pt＋右下角小字 12 pt；偏移待 P1"
        style = styles[entry["style"]]
        lines.append("| " + " | ".join(map(cell, (entry["origin_id"], labels[entry["label_key"]], entry["key"],
                                                     f"{entry['style']} / {style['size_pt']:g}", note,
                                                     "文案已确认" if confirmed.get("labels") else ""))) + " |")
    lines += ["", "## 保留图像", "", "| OriginId | 目录键 | 核对 |", "| ---: | --- | --- |"]
    for entry in catalog["images"]:
        lines.append(f"| {entry['origin_id']} | {entry['key']} | |")
    lines += ["", "## 规则映射", "", "| 玩法 | 服务端名 | 显示名 | 缺省分值 | 核对 |",
              "| --- | --- | --- | ---: | --- |"]
    texts = {entry["key"]: entry for entry in catalog["texts"]}
    for pattern in catalog["rule_patterns"]:
        label = labels[texts[pattern["text"]]["label_key"]]
        lines.append("| " + " | ".join(map(cell, (pattern["rule"], pattern["server_name"], label,
                                                     pattern["default_value"], ""))) + " |")
    return "\n".join(lines) + "\n"


def serialize(value):
    return json.dumps(value, ensure_ascii=False, indent=2) + "\n"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--workbook", type=Path, required=True, help="Path to an external specification workbook")
    parser.add_argument("--catalog", type=Path, default=ROOT / "Tenhou.Client/assets/text/catalog.json")
    parser.add_argument("--output", type=Path, help="Candidate path (default: catalog.candidate.json)")
    parser.add_argument("--check", action="store_true", help="Read-only comparison; exit 1 on differences")
    parser.add_argument("--report", type=Path, help="Also write a Markdown checklist for the candidate (explicit only)")
    args = parser.parse_args()
    try:
        catalog = json.loads(args.catalog.read_text(encoding="utf-8"))
        candidate = import_catalog(args.workbook, catalog)
        before, after = serialize(catalog), serialize(candidate)
        if before != after:
            sys.stdout.writelines(difflib.unified_diff(before.splitlines(True), after.splitlines(True),
                                                     fromfile=str(args.catalog), tofile="import candidate"))
        else:
            print("Catalog matches specification: 296 Text, 7 Image; 481=24 pt; Info inheritance explicit.")
        if args.check and args.report:
            raise ValueError("--check is read-only; omit --report")
        if args.check:
            return int(before != after)
        output = args.output or args.catalog.with_name("catalog.candidate.json")
        if output.resolve() in {args.catalog.resolve(), args.workbook.resolve()}:
            raise ValueError("Output must be a separate review candidate")
        if args.report:
            if args.report.resolve() in {args.catalog.resolve(), args.workbook.resolve(), output.resolve()}:
                raise ValueError("Report path must be separate from all inputs and the JSON candidate")
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(after, encoding="utf-8")
        if args.report:
            args.report.parent.mkdir(parents=True, exist_ok=True)
            args.report.write_text(review_report(candidate), encoding="utf-8")
        print(f"Review candidate: {output}")
        return 0
    except (ValueError, KeyError, StopIteration, OSError, zipfile.BadZipFile, ET.ParseError) as error:
        print(f"Text catalog import failed: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    sys.stdout.reconfigure(encoding="utf-8")
    sys.stderr.reconfigure(encoding="utf-8")
    sys.exit(main())
