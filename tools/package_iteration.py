"""Save an immutable local iteration: VPK, source snapshot, build log and hashes."""
import argparse, hashlib, json, pathlib, shutil, subprocess, tempfile, struct, zipfile

def vpk_identity(path):
    with zipfile.ZipFile(path) as archive:
        data=archive.read('sce_sys/param.sfo')
    magic,version,key_start,value_start,count=struct.unpack_from('<5I',data)
    if magic!=0x46535000:raise ValueError('Invalid SFO')
    values={}
    for i in range(count):
        key,fmt,length,capacity,offset=struct.unpack_from('<HHIII',data,20+i*16)
        name=data[key_start+key:].split(b'\0',1)[0].decode()
        if fmt==0x204:
            values[name]=data[value_start+offset:value_start+offset+length].split(b'\0',1)[0].decode()
    return {key:values[key] for key in ('TITLE_ID','TITLE','APP_VER')}

def main():
    p=argparse.ArgumentParser()
    p.add_argument('iteration', help='Three digits, e.g. 004')
    p.add_argument('vpk',type=pathlib.Path)
    p.add_argument('--build-log',type=pathlib.Path,required=True)
    p.add_argument('--git',default='git')
    p.add_argument('--menu-archive',type=pathlib.Path,help='Optional private menu-only archive, never committed')
    args=p.parse_args()
    if len(args.iteration)!=3 or not args.iteration.isdigit():p.error('Use three digits')
    root=pathlib.Path(__file__).resolve().parent.parent
    identity=vpk_identity(args.vpk)
    if identity['TITLE_ID']!='VAGV00006' or identity['TITLE']!='Vagante2Vita':
        raise ValueError('Future iterations must update the standard VAGV00006 / Vagante2Vita application')
    command=[args.git,'-c',f'safe.directory={root.as_posix()}','-C',str(root)]
    def git(*a):return subprocess.check_output(command+list(a))
    if git('status','--porcelain').strip():raise RuntimeError('Commit tracked source changes before packaging')
    output=root/'artifacts'/f'iteration{args.iteration}'
    if output.exists():raise FileExistsError(f'Iteration already preserved: {output}')
    output.parent.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(dir=output.parent) as temporary:
        dest=pathlib.Path(temporary)
        shutil.copyfile(args.vpk,dest/args.vpk.name)
        shutil.copyfile(args.build_log,dest/'build.log')
        (dest/'source.zip').write_bytes(git('archive','--format=zip','HEAD'))
        shutil.copyfile(root/'docs/ROADMAP.md',dest/'ROADMAP.md')
        if args.menu_archive:
            shutil.copyfile(args.menu_archive,dest/'menu-only.vra')
            metadata=args.menu_archive.with_suffix('.json')
            if metadata.exists():shutil.copyfile(metadata,dest/'menu-only.json')
        report=dict(iteration=args.iteration,identity=identity,commit=git('rev-parse','HEAD').decode().strip(),files={})
        for file in dest.iterdir():
            report['files'][file.name]=dict(bytes=file.stat().st_size,sha256=hashlib.sha256(file.read_bytes()).hexdigest())
        (dest/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
        dest.rename(output)
    print(output)

if __name__=='__main__':main()
