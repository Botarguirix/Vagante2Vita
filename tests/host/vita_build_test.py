"""Build cache / code-only packaging contracts. No game data required."""
import importlib.util
from pathlib import Path
import tempfile
import time
import unittest
from unittest.mock import patch
import zipfile

ROOT=Path(__file__).resolve().parents[2]
spec=importlib.util.spec_from_file_location('vita_build',ROOT/'recomp/build_vita.py')
build=importlib.util.module_from_spec(spec); spec.loader.exec_module(build)

class BuildTests(unittest.TestCase):
    def test_cache_tracks_flags_source_dependencies_and_missing_object(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); src=root/'unit.c'; obj=root/'unit.o'; dep=root/'guest.h'; cc=root/'cc.exe'
            for p in (src,dep,cc): p.write_text('fixture')
            def compile_command(command):
                Path(command[-1]).write_bytes(b'object'); return ''
            with patch.object(build,'run',side_effect=compile_command) as run:
                self.assertTrue(build.compile_one(src,obj,['-Os'],[dep],cc))
                self.assertFalse(build.compile_one(src,obj,['-Os'],[dep],cc))
                self.assertTrue(build.compile_one(src,obj,['-O2'],[dep],cc))
                time.sleep(.02); src.write_text('changed source')
                self.assertTrue(build.compile_one(src,obj,['-O2'],[dep],cc))
                time.sleep(.02); dep.write_text('changed header')
                self.assertTrue(build.compile_one(src,obj,['-O2'],[dep],cc))
                obj.unlink()
                self.assertTrue(build.compile_one(src,obj,['-O2'],[dep],cc))
                self.assertEqual(run.call_count,5)

    def test_failed_compile_does_not_replace_a_good_cached_object(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); src=root/'unit.c'; obj=root/'unit.o'; cc=root/'cc.exe'
            src.write_text('source'); cc.write_text('compiler'); obj.write_bytes(b'good object')
            def fail(command):
                Path(command[-1]).write_bytes(b'truncated')
                raise RuntimeError('compiler interrupted')
            with patch.object(build,'run',side_effect=fail):
                with self.assertRaises(RuntimeError): build.compile_one(src,obj,['-Os'],[],cc)
            self.assertEqual(obj.read_bytes(),b'good object')
            self.assertFalse(list(root.glob('*.tmp')))

    def test_snapshot_retries_when_lifter_changes_input_during_copy(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); gen=root/'gen'; snap=root/'snapshot'; gen.mkdir()
            for name in ('unit_000.c','table.c','lifted.h'): (gen/name).write_text(name)
            calls=[]
            def change_once(delay):
                if not calls: (gen/'unit_000.c').write_text('regenerated function set')
                calls.append(delay)
            with patch.object(build.time,'sleep',side_effect=change_once):
                build.snapshot_generated(gen,snap)
            self.assertEqual((snap/'unit_000.c').read_text(),'regenerated function set')
            self.assertGreater(len(calls),1)

    def test_exact_text_section_not_berkeley_code_plus_rodata(self):
        sizes=build.section_sizes('section size addr\n.text 123456 16\n.rodata 4096 123472\n.bss 800 0\nTotal 128352\n')
        self.assertEqual(sizes['.text'],123456)
        with self.assertRaises(RuntimeError): build.section_sizes('text data bss dec hex\n10 20 30 60 3c')

    def test_vpk_whitelist_excludes_game_even_when_files_are_present(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder)
            for name in ('eboot.bin','param.sfo','font.rgba','vagante.exe','data.vra'):
                (root/name).write_bytes(name.encode())
            vpk=build.package(root,{'assets/font.rgba':root/'font.rgba'})
            with zipfile.ZipFile(vpk) as z:
                self.assertEqual(set(z.namelist()),{'eboot.bin','sce_sys/param.sfo','assets/font.rgba'})
                self.assertIsNone(z.testzip())

    def test_snapshot_keeps_unchanged_files_incremental_and_removes_obsolete_units(self):
        with tempfile.TemporaryDirectory() as folder:
            root=Path(folder); gen=root/'gen'; snap=root/'snapshot'; gen.mkdir()
            for name in ('unit_000.c','table.c','lifted.h'): (gen/name).write_text(name)
            with patch.object(build.time,'sleep'):
                build.snapshot_generated(gen,snap)
                mtime=(snap/'unit_000.c').stat().st_mtime_ns
                (snap/'unit_old.c').write_text('obsolete')
                build.snapshot_generated(gen,snap)
            self.assertEqual((snap/'unit_000.c').stat().st_mtime_ns,mtime)
            self.assertFalse((snap/'unit_old.c').exists())

if __name__=='__main__': unittest.main()
