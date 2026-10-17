import unittest

from tools.audit_chest_pool_graph import chest_weight_references, extract_graph


class ChestPoolGraphTests(unittest.TestCase):
    def test_preserves_edges_and_does_not_invent_dynamic_weights(self):
        pseudo = """
        ItemPoolRarity::ItemPoolRarity(&local_c0,POOL_WEAPONS,(float)uVar86);
        ItemPoolRarity::ItemPoolRarity(&local_b8,POOL_ARMOR,(float)piVar92);
        ItemPoolRarity::ItemPoolRarity(&local_b0,POOL_OTHER,(float)otherWeight);
        ItemPoolRarity::ItemPoolRarity(&local_a8,POOL_WANDS,(float)wandWeight);
        local_f78 = (map<ItemPoolType> *)0x17;
        ItemPool::operator=(pIVar80,pIVar78);
        """
        result = extract_graph(pseudo)
        self.assertTrue(result["verified_structure"])
        self.assertFalse(result["verified_weights"])
        self.assertEqual(result["root"], {"pool": "POOL_CHEST", "enum": 23})
        self.assertEqual([entry["pool"] for entry in result["categories"]],
                         ["POOL_WEAPONS", "POOL_ARMOR", "POOL_OTHER", "POOL_WANDS"])
        self.assertEqual(result["categories"][0]["weight_expression"], "uVar86")

    def test_rejects_wrong_pool_assignment(self):
        pseudo = """
        ItemPoolRarity::ItemPoolRarity(&local_c0,POOL_WEAPONS,(float)1);
        ItemPoolRarity::ItemPoolRarity(&local_b8,POOL_ARMOR,(float)1);
        ItemPoolRarity::ItemPoolRarity(&local_b0,POOL_OTHER,(float)1);
        ItemPoolRarity::ItemPoolRarity(&local_a8,POOL_WANDS,(float)1);
        local_f78 = (map<ItemPoolType> *)0x16;
        ItemPool::operator=(pIVar80,pIVar78);
        """
        with self.assertRaisesRegex(ValueError, "enum 23"):
            extract_graph(pseudo)

    def test_reads_static_weight_addresses_for_chest_categories(self):
        lines = []
        for pool, address in [(15, "0xb204fc"), (6, "0xb205a4"),
                              (22, "0xb203b4"), (11, "0xb203fc")]:
            lines.extend([
                f"MOVSS XMM2,dword ptr [{address}]",
                f"PUSH 0x{pool:x}",
                "CALL 0x0063cd10",
            ])
        lines.extend([
            "CALL 0x00413b70",
            "MOV dword ptr [EBP + 0xfffff08c],0x17",
            "CALL 0x00645d90",
        ])
        refs = chest_weight_references("\n".join(
            f"{0x641000 + i:08x}  {line}" for i, line in enumerate(lines)))
        self.assertEqual([row["pool_id"] for row in refs], [15, 6, 22, 11])
        self.assertEqual([row["weight_va"] for row in refs],
                         [0xb204fc, 0xb205a4, 0xb203b4, 0xb203fc])


if __name__ == "__main__":
    unittest.main()
