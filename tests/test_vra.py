import pathlib, struct, sys, tempfile, unittest, zlib
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'tools'))
from vra import entries

def archive(files):
    raw=struct.pack('<IH',8,max(0,len(files)-3))
    for name,data in files:
        name=name.encode();raw+=struct.pack('<I',len(name))+name+struct.pack('<I',len(data))+data
    return zlib.compress(raw)

class ArchiveTests(unittest.TestCase):
    def read(self,b,**kwargs):
        with tempfile.TemporaryDirectory() as d:
            p=pathlib.Path(d)/'test.vra';p.write_bytes(b);return entries(p,**kwargs)
    def test_valid_and_duplicates_preserved(self):
        files=[('gfx/a.png',b'first'),('gfx/a.png',b'second')]
        magic,count,result,size=self.read(archive(files));self.assertEqual(magic,8);self.assertEqual(result,files)
    def test_truncated_content_rejected(self):
        with self.assertRaises(ValueError):self.read(zlib.compress(zlib.decompress(archive([('a',b'123')]))[:-1]))
    def test_truncated_zlib_rejected(self):
        with self.assertRaises(ValueError):self.read(archive([('a',b'123')])[:-1])
    def test_traversal_and_windows_paths_rejected(self):
        for name in ['../a','a/../../b','/tmp/a','C:/a','..\\a','a\0b']:
            with self.subTest(name=name),self.assertRaises(ValueError):self.read(archive([(name,b'x')]))
    def test_decompression_limit(self):
        with self.assertRaises(ValueError):self.read(archive([('a',b'x'*1000)]),max_bytes=100)
    def test_trailing_stream_rejected(self):
        with self.assertRaises(ValueError):self.read(archive([('a',b'x')])+b'extra')
if __name__=='__main__':unittest.main()
