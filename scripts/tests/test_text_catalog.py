"""P0 import and generator failure checks; explicit developer tool, not a build dependency."""
import copy
import importlib.util
import json
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest
import xml.etree.ElementTree as ET
import zipfile

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location("text_import", ROOT / "scripts/Import-TextCatalog.py")
IMPORT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(IMPORT)
CATALOG = ROOT / "Tenhou.Client/assets/text/catalog.json"


class TextCatalogChecks(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.catalog = json.loads(CATALOG.read_text(encoding="utf-8"))

    def write_workbook(self, destination, origin=None, field=None, value=None):
        """Build the import fixture from the original cells recorded in the catalog."""
        namespace = IMPORT.NS["s"]
        sheet = ET.Element("worksheet", xmlns=namespace)
        rows = ET.SubElement(sheet, "sheetData")

        def add_row(number, cells):
            row = ET.SubElement(rows, "row", r=str(number))
            for index, cell_value in enumerate(cells):
                if cell_value != "":
                    cell = ET.SubElement(row, "c", r=f"{chr(65 + index)}{number}", t="inlineStr")
                    ET.SubElement(ET.SubElement(cell, "is"), "t").text = str(cell_value)

        add_row(1, IMPORT.FIELDS)
        entries = sorted(self.catalog["texts"] + self.catalog["images"], key=lambda entry: entry["source"]["row"])
        for entry in entries:
            record = {"OriginId": entry["origin_id"]}
            record.update((name, entry["source"][name.lower()]) for name in IMPORT.FIELDS[1:])
            if entry["origin_id"] == origin:
                record[field] = value
            add_row(entry["source"]["row"], (record[name] for name in IMPORT.FIELDS))

        with zipfile.ZipFile(destination, "w") as workbook:
            workbook.writestr("xl/workbook.xml", f'<workbook xmlns="{namespace}" xmlns:r="{IMPORT.REL}">'
                                                '<sheets><sheet name="Sprites705" r:id="rId1"/></sheets></workbook>')
            workbook.writestr("xl/_rels/workbook.xml.rels",
                              '<Relationships><Relationship Id="rId1" Target="worksheets/sheet1.xml"/></Relationships>')
            workbook.writestr("xl/worksheets/sheet1.xml", ET.tostring(sheet, encoding="utf-8"))

    def test_catalog_content_round_trips_through_workbook(self):
        with tempfile.TemporaryDirectory() as temporary:
            workbook = Path(temporary) / "source.xlsx"
            self.write_workbook(workbook)
            candidate = IMPORT.import_catalog(workbook, self.catalog)
        expected = copy.deepcopy(self.catalog)
        for field in ("sha256", "worksheet_sha256"):
            expected["source"][field] = candidate["source"][field]
        expected["review"].update(status="pending_human_review", confirmations={},
                                  calibration_status="pending_human_review")
        self.assertEqual(candidate, expected)
        texts = {entry["origin_id"]: entry for entry in candidate["texts"]}
        self.assertEqual(texts[481]["overrides"]["size_pt"], 24)
        self.assertEqual(texts[481]["style"], "yaku")
        self.assertEqual(candidate["compositions"]["chi_selection"]["secondary_size_pt"], 12)
        for origin in IMPORT.INFO_IDS:
            self.assertEqual(texts[origin]["inherits_origin_id"], 566)
            self.assertEqual(texts[origin]["inherited_fields"], ["size", "color", "style"])
        self.assertIsNone(texts[566]["inherits_origin_id"])

    def test_unicode_spaces_and_inline_cells_survive_import(self):
        with tempfile.TemporaryDirectory() as temporary:
            workbook = Path(temporary) / "source.xlsx"
            label = " 竜 幺 13 交換 カンリョウ 国標 "
            self.write_workbook(workbook, 705, "Label", label)
            candidate = IMPORT.import_catalog(workbook, self.catalog)
            entry = next(e for e in candidate["texts"] if e["origin_id"] == 705)
            self.assertEqual(next(e["label"] for e in candidate["labels"] if e["key"] == entry["label_key"]), label)
            self.assertNotEqual(candidate["source"]["sha256"], self.catalog["source"]["sha256"])

    def test_state_labels_cannot_silently_diverge(self):
        with tempfile.TemporaryDirectory() as temporary:
            workbook = Path(temporary) / "source.xlsx"
            self.write_workbook(workbook, 450, "Label", "changed hover label")
            with self.assertRaisesRegex(ValueError, "shared Label differs"):
                IMPORT.import_catalog(workbook, self.catalog)

    def test_missing_format_outside_info_does_not_inherit(self):
        with tempfile.TemporaryDirectory() as temporary:
            workbook = Path(temporary) / "source.xlsx"
            self.write_workbook(workbook, 482, "Size", "")
            with self.assertRaises(ValueError):
                IMPORT.import_catalog(workbook, self.catalog)

    @unittest.skipUnless(shutil.which("cmake"), "CMake is required for generator rejection checks")
    def test_cmake_rejects_invalid_registered_data(self):
        menzen_style = next(i for i, s in enumerate(self.catalog["styles"]) if s["key"] == "yaku")
        inherited_info = next(i for i, e in enumerate(self.catalog["texts"]) if e["origin_id"] == 567)
        cases = [
            ("duplicate_origin", ("texts", 1, "origin_id"), 403),
            ("missing_style", ("texts", 0, "style"), "does_not_exist"),
            ("missing_label", ("texts", 0, "label_key"), "does_not_exist"),
            ("image_as_text", ("texts", 0, "origin_id"), 401),
            ("wrong_481_size", ("styles", menzen_style, "size_pt"), 56),
            ("wrong_info_inheritance", ("texts", inherited_info, "inherits_origin_id"), 565),
            ("unknown_rule_text", ("rule_patterns", 0, "text"), "does_not_exist"),
            ("wrong_chi_size", ("compositions", "chi_selection", "secondary_size_pt"), 18),
        ]
        with tempfile.TemporaryDirectory() as temporary:
            directory = Path(temporary)
            runner = directory / "generate.cmake"
            runner.write_text(f'cmake_minimum_required(VERSION 3.24)\ninclude("{ROOT.as_posix()}/Tenhou.Client/cmake/TextCatalog.cmake")\n'
                              'tenhou_generate_text_catalog("${INPUT}" "${OUTPUT}")\n', encoding="utf-8")
            for name, path, value in cases:
                with self.subTest(name=name):
                    candidate = copy.deepcopy(self.catalog)
                    target = candidate
                    for step in path[:-1]:
                        target = target[step]
                    target[path[-1]] = value
                    source = directory / f"{name}.json"
                    source.write_text(IMPORT.serialize(candidate), encoding="utf-8")
                    result = subprocess.run(["cmake", f"-DINPUT={source.as_posix()}",
                                             f"-DOUTPUT={(directory / 'out.hpp').as_posix()}", "-P", str(runner)],
                                            capture_output=True, text=True, encoding="utf-8")
                    self.assertNotEqual(result.returncode, 0, result.stdout)
                    self.assertIn("Text catalog:", result.stderr)


if __name__ == "__main__":
    unittest.main()
