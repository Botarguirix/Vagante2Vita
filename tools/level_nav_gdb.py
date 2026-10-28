# GDB Python, READ ONLY. No calls into the inferior, no memory writes or captures.
# Configure: set $profile_engine = <guest engine address from the trace>.
import gdb,json,struct
inf=gdb.selected_inferior();base=int(gdb.parse_and_eval('guest_base'));engine=int(gdb.parse_and_eval('$profile_engine'))
def mem(a,n):
 if a<0 or a+n>0x11800000:raise ValueError('guest read outside 280 MiB window')
 return bytes(inf.read_memory(base+a,n))
def u(a):return struct.unpack('<I',mem(a,4))[0]
def f(a):return struct.unpack('<f',mem(a,4))[0]
def b(a):return mem(a,1)[0]
level=u(engine+0x870);players=[];entities=[];tiles=[]
for slot in range(u(engine+0x874),u(engine+0x878),8):
 p=u(slot)
 if not p:continue
 held=u(p+0x474)
 players.append(dict(address=hex(p),x=f(p+0xac),y=f(p+0xb0),hp=f(p+0x10c),gameOver=b(p+0x800),held=hex(held),heldType=b(held+0x50) if held else None))
for slot in range(u(engine+0x328),u(engine+0x32c),8):
 p=u(slot)
 if not p:continue
 t=b(p+0x50)
 entities.append(dict(address=hex(p),type=t,x=f(p+0xac),y=f(p+0xb0)))
width=u(level+0x9c);height=u(level+0xa0);columns=u(level+0xf5c)
for x in range(width):
 column=u(columns+4*x);tiles.append([list(mem(column+40*y,1))+[b(column+40*y+0x13),b(column+40*y+0x18)] for y in range(height)])
state=dict(engine=hex(engine),level=hex(level),floor=u(engine+0x3c0),intro=b(engine+0x3c8),levelFloor=u(level+0xa4),act=u(level+0xa8),width=width,height=height,exit=[u(level+0xfc),u(level+0x100)],players=players,entities=entities,tiles=tiles)
print('NAV_STATE_JSON '+json.dumps(state,separators=(',',':')))
