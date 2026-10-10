"""Create the requested research repo with Git Credential Manager; never log secrets."""
import json, os, subprocess, urllib.request, urllib.error, pathlib
git=os.environ.get('V2V_GIT','git')
env=os.environ.copy();env['GIT_TERMINAL_PROMPT']='0';env['GCM_INTERACTIVE']='never'
result=subprocess.run([git,'credential','fill'],input='protocol=https\nhost=github.com\n\n',text=True,capture_output=True,env=env)
if result.returncode:raise SystemExit('No usable GitHub credential in Git Credential Manager')
fields=dict(line.split('=',1) for line in result.stdout.splitlines() if '=' in line)
token=fields.get('password')
if not token:raise SystemExit('No GitHub credential')
def api(path,data=None):
    req=urllib.request.Request('https://api.github.com'+path,data=json.dumps(data).encode() if data is not None else None,headers={'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json','User-Agent':'Vagante2Vita','Content-Type':'application/json'})
    with urllib.request.urlopen(req,timeout=60) as r:return json.load(r)
owner=api('/user')['login']
try:repo=api('/repos/'+owner+'/Vagante2Vita')
except urllib.error.HTTPError as e:
    if e.code!=404:raise SystemExit('GitHub API status '+str(e.code))
    repo=api('/user/repos',dict(name='Vagante2Vita',description='Technical feasibility and reproducible reverse-engineering tools for a native Vagante PS Vita port. Not a playable port.',private=False,auto_init=False))
print(repo['html_url'])
pathlib.Path('_local/github.json').write_text(json.dumps({'url':repo['html_url'],'clone_url':repo['clone_url'],'owner':owner},indent=2))
