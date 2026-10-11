"""Save an immutable local iteration: VPK, source snapshot, build log and hashes."""
import argparse, hashlib, json, pathlib, shutil, subprocess, tempfile

def main():
    p=argparse.ArgumentParser()
    p.add_argument('iteration', help='Three digits, e.g. 004')
    p.add_argument('vpk',type=pathlib.Path)
    p.add_argument('--build-log',type=pathlib.Path,required=True)
    p.add_argument('--git',default='git')
    args=p.parse_args()
    if len(args.iteration)!=3 or not args.iteration.isdigit():p.error('Use three digits')
    root=pathlib.Path(__file__).resolve().parent.parent
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
        report=dict(iteration=args.iteration,commit=git('rev-parse','HEAD').decode().strip(),files={})
        for file in dest.iterdir():
            report['files'][file.name]=dict(bytes=file.stat().st_size,sha256=hashlib.sha256(file.read_bytes()).hexdigest())
        (dest/'manifest.json').write_text(json.dumps(report,indent=2)+'\n')
        dest.rename(output)
    print(output)

if __name__=='__main__':main()
