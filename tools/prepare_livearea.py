"""Encode the project's generated launcher artwork as Vita PNG-8 assets."""
import argparse, pathlib
from PIL import Image, ImageOps

def indexed(image,path):
    image=image.convert('RGB').quantize(colors=128,method=Image.Quantize.MEDIANCUT,dither=Image.Dither.NONE)
    image.info.clear()
    image.save(path,bits=8,optimize=False)

def main():
    p=argparse.ArgumentParser()
    p.add_argument('icon',type=pathlib.Path)
    p.add_argument('background',type=pathlib.Path)
    p.add_argument('--out',type=pathlib.Path,default=pathlib.Path('vita/sce_sys'))
    args=p.parse_args();root=args.out;contents=root/'livearea/contents'
    contents.mkdir(parents=True,exist_ok=True)
    icon=Image.open(args.icon).convert('RGB');bg=Image.open(args.background).convert('RGB')
    indexed(ImageOps.fit(icon,(128,128),method=Image.Resampling.LANCZOS),root/'icon0.png')
    indexed(ImageOps.fit(bg,(840,500),method=Image.Resampling.LANCZOS),contents/'bg0.png')
    indexed(ImageOps.fit(bg,(960,544),method=Image.Resampling.LANCZOS),root/'pic0.png')
    startup=Image.new('RGB',(280,158),(0,29,35))
    startup.paste(ImageOps.fit(icon,(104,104),method=Image.Resampling.LANCZOS),(88,8))
    indexed(startup,contents/'startup.png')
    xml='''<?xml version="1.0" encoding="utf-8"?>
<livearea style="psmobile" format-ver="01.00" content-rev="6">
  <livearea-background><image>bg0.png</image></livearea-background>
  <gate><startup-image>startup.png</startup-image></gate>
</livearea>
'''
    (contents/'template.xml').write_bytes(xml.replace('\n','\r\n').encode())
    for file in root.rglob('*.png'):
        with Image.open(file) as image:
            assert image.mode=='P' and file.stat().st_size<420*1024
            print(file,image.size,file.stat().st_size)

if __name__=='__main__':main()
