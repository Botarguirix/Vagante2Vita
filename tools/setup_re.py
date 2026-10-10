"""Download pinned-by-release local Ghidra/JDK tools; never committed."""
import hashlib, json, pathlib, urllib.request, zipfile

root=pathlib.Path('_local/toolchain');root.mkdir(parents=True,exist_ok=True)
def get(url):
    return urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'Vagante2Vita-research'}),timeout=60)
def download(url,path,checksum=None):
    if not path.exists():
        partial=path.with_suffix(path.suffix+'.part')
        with get(url) as r,partial.open('wb') as f:
            while chunk:=r.read(1024*1024):f.write(chunk)
        partial.replace(path)
    if checksum:
        with path.open('rb') as f:actual=hashlib.file_digest(f,'sha256').hexdigest()
        if actual!=checksum:raise ValueError('Checksum mismatch: '+path.name)
    print(path.name,path.stat().st_size,flush=True)
release=json.load(get('https://api.github.com/repos/NationalSecurityAgency/ghidra/releases/tags/Ghidra_12.1.4_build'))
asset=next(a for a in release['assets'] if a['name'].endswith('.zip'))
digest=asset.get('digest','');checksum=digest.removeprefix('sha256:') if digest.startswith('sha256:') else None
archive=root/asset['name'];download(asset['browser_download_url'],archive,checksum)
if not list(root.glob('ghidra_*/support/analyzeHeadless.bat')):
    with zipfile.ZipFile(archive) as z:z.extractall(root)
jdk={'name':'OpenJDK21U-jdk_x64_windows_hotspot_21.0.12.1_1.zip','link':'https://github.com/adoptium/temurin21-binaries/releases/download/jdk-21.0.12.1%2B1/OpenJDK21U-jdk_x64_windows_hotspot_21.0.12.1_1.zip','checksum':'f9d6e191ab098c0d416e7d588a24420a8621cd2f4720dab2459b8b7b2d2d8b4e'}
archive=root/jdk['name'];download(jdk['link'],archive,jdk['checksum'])
if not list(root.glob('jdk-*/bin/java.exe')):
    with zipfile.ZipFile(archive) as z:z.extractall(root)
(root/'versions.json').write_text(json.dumps({'ghidra_release':release['tag_name'],'ghidra_asset':asset['name'],'ghidra_sha256':checksum,'jdk':jdk['name'],'jdk_sha256':jdk['checksum']},indent=2))
print('Ready',flush=True)
