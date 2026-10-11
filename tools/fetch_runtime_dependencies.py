"""Vendor official sources at recorded commit IDs for the small native runtime."""
import hashlib,json,pathlib,urllib.request
def get(url):
    with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'Vagante2Vita'}),timeout=60) as r:return r.read()
root=pathlib.Path('third_party');manifest={}
for repo,names,folder in [('richgel999/miniz',['miniz.c','miniz.h','miniz_common.h','miniz_tinfl.c','miniz_tinfl.h','miniz_tdef.h','miniz_zip.h','LICENSE'],'miniz'),('nothings/stb',['stb_image.h','LICENSE'],'stb')]:
    sha=json.loads(get('https://api.github.com/repos/'+repo+'/commits/HEAD'))['sha'];dest=root/folder;dest.mkdir(parents=True,exist_ok=True)
    manifest[repo]={'commit':sha,'files':{}}
    for name in names:
        data=get('https://raw.githubusercontent.com/'+repo+'/'+sha+'/'+name);(dest/name).write_bytes(data);manifest[repo]['files'][name]=hashlib.sha256(data).hexdigest()
(root/'miniz/miniz_export.h').write_text('#pragma once\n#define MINIZ_EXPORT\n')
(root/'sources.json').write_text(json.dumps(manifest,indent=2)+'\n');print(json.dumps({k:v['commit'] for k,v in manifest.items()},indent=2))
