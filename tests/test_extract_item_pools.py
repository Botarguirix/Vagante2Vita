import tempfile
import unittest
from pathlib import Path

from tools.extract_item_pools import POOL_ASSIGNMENT, _pool_names, _rarity_rows


class ExtractItemPoolsTests(unittest.TestCase):
    def test_names_flail_from_the_pdb_enum_instead_of_guessing(self):
        with tempfile.TemporaryDirectory() as temporary:
            enums = Path(temporary) / "enums.tsv"
            enums.write_text(
                "/vagante.pdb/Item/ItemType\tITEMTYPE_FLAIL\t62\n"
                "/vagante.pdb/ItemGenerator/ItemPoolType\tPOOL_CLUBS\t9\n",
                encoding="utf-8",
            )
            pools, items = _pool_names(enums)
        self.assertEqual(pools, {9: "POOL_CLUBS"})
        self.assertEqual(items[62], "ITEMTYPE_FLAIL")

    def test_recognizes_pool_assignment_through_temporary_item_type_map(self):
        pseudo = """
        local_1008 = (map<Item::ItemType,unsigned_int> *)0x11;
        local_f78 = (map<ItemGenerator::ItemPoolType> *)0x12;
        """
        self.assertEqual(POOL_ASSIGNMENT.findall(pseudo), ["11", "12"])

    def test_recovers_all_immediate_three_word_rows_and_defaults_max_level(self):
        instructions = []
        for index in range(170):
            address = 0x0063CDE0 + index * 0x40
            instructions.extend([
                (address, "LEA EAX,[EBP + -0x10]"),
                (address + 1, f"MOV dword ptr [EBP + -0x10],0x{index % 7:x}"),
                (address + 2, "MOV dword ptr [EBP + -0xc],0x2"),
                (address + 3, f"MOV dword ptr [EBP + -0x8],0x{45 + index % 50:x}"),
                (address + 4, "CALL 0x00413b70"),
                (address + 5, "CALL 0x0063cc40"),
            ])
        rows = _rarity_rows(instructions, b"", 0, [])
        self.assertEqual(len(rows), 170)
        self.assertEqual(rows[0]["min_level"], 0)
        self.assertEqual(rows[0]["max_level"], 99999)
        self.assertEqual(rows[0]["weight"], 2)
        self.assertEqual(rows[0]["item_type"], 45)


if __name__ == "__main__":
    unittest.main()
