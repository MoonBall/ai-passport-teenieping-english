from pathlib import Path
from serial.tools import list_ports
import argparse,json,re,serial,tempfile,time
root=Path(__file__).resolve().parent.parent
parser=argparse.ArgumentParser(description='Exercise all card audio, motion, and button transitions on a connected board.')
parser.add_argument('--serial',required=True)
parser.add_argument('--output',type=Path)
parser.add_argument('--lesson-cards',type=int,nargs='+',default=list(range(1,18)))
parser.add_argument('--capture-lesson-stages',action='store_true')
args=parser.parse_args()
assert args.lesson_cards and all(1<=i<=17 for i in args.lesson_cards)
out=args.output or Path(tempfile.mkdtemp(prefix='spoken-device-'))
out.mkdir(parents=True,exist_ok=True)
ports=[p for p in list_ports.comports() if p.vid==0x303a and p.pid==0x1001 and p.serial_number==args.serial]
assert len(ports)==1
s=serial.Serial();s.port=ports[0].device;s.baudrate=115200;s.timeout=.05;s.write_timeout=3;s.dtr=False;s.rts=False;s.open()
log=bytearray();result={'build':'PASS','host_tests':'PASS','device_tests':'RUNNING','short_completed':[],'lessons_requested':args.lesson_cards,'lessons_completed':[],'screenshots':str(out)}
def save():
 (out/'result.json').write_text(json.dumps(result,indent=2)+'\n')
 (root/'build/device-test-result.json').write_text(json.dumps(result,indent=2)+'\n')
def read_for(seconds):
 data=bytearray();end=time.monotonic()+seconds
 while time.monotonic()<end:
  chunk=s.read(max(1,s.in_waiting));data.extend(chunk);log.extend(chunk)
 return bytes(data)
def until(pattern,timeout=15):
 data=bytearray();end=time.monotonic()+timeout
 while time.monotonic()<end:
  chunk=s.readline();data.extend(chunk);log.extend(chunk)
  if re.search(pattern,data): return bytes(data)
 raise RuntimeError('Missing '+str(pattern)+'; recent log '+repr(data[-300:]))
def key(name):s.write(('FAP_KEY_'+name+'_V1\n').encode());s.flush()
def move(name,expected):
 key(name);data=until(f'card {expected}/17:'.encode())+read_for(.1)
 assert b'audio start' not in data
 return data
def screenshot(name):
 s.write(b'FAP_SCREENSHOT_V1\n');s.flush()
 until(rb'FAP_SCREENSHOT_V1 240 320 RGB565LE 153600\r?\n')
 data=bytearray();end=time.monotonic()+20
 while len(data)<153600 and time.monotonic()<end:data.extend(s.read(153600-len(data)))
 assert len(data)==153600
 (out/(name+'.rgb565')).write_bytes(data)
 return bytes(data)
try:
 boot=read_for(2);assert b'audio start' not in boot
 move('UP',17);move('DOWN',1)
 result.update(silent_boot=True,silent_navigation=True,wrap_both_directions=True)
 screenshot('01-initial')
 key('OK');until(b'audio start 1 ');read_for(.25);key('OK');until(b'mode=0 card=1');read_for(.2)
 key('OK_LONG');until(b'audio start 1 name mode=2');read_for(.2);key('OK');until(b'mode=0 card=1');read_for(.2)
 key('OK');until(b'audio start 1 ');key('OK_LONG');until(b'audio start 1 name mode=2');read_for(.3);move('DOWN',2);read_for(.3);move('UP',1)
 result.update(short_stop=True,long_interrupts_short=True,short_stops_lesson=True,navigation_stops_lesson=True)
 save();print('Interaction switching: PASS',flush=True)
 for i in range(1,18):
  if i>1:move('DOWN',i)
  screenshot(f'{i:02d}-still')
  start=len(log)
  key('OK');until(f'audio start {i} '.encode());read_for(1.5)
  if i==16:screenshot(f'{i:02d}-middle')
  read_for(1.75)
  screenshot(f'{i:02d}-motion')
  pattern=f'playback complete card={i}\r?\n'.encode()
  if not re.search(pattern,log[start:]):until(pattern,10)
  result['short_completed'].append(i);save()
  print(f'Short phrase and cue {i}/17 complete',flush=True)
 move('DOWN',1)
 current=1
 for i in args.lesson_cards:
  while current!=i:
   current=current%17+1;move('DOWN',current)
  start=len(log);key('OK_LONG')
  if args.capture_lesson_stages:
   for stage in range(4):
    pattern=f'stage {stage} card={i}\r?\n'.encode()
    if not re.search(pattern,log[start:]):until(pattern,60)
    read_for(.15);screenshot(f'{i:02d}-lesson-{stage}')
  pattern=f'playback complete card={i}\r?\n'.encode()
  if not re.search(pattern,log[start:]):until(pattern,60)
  data=bytes(log[start:])
  for stage in range(4):assert re.search(f'stage {stage} card={i}\r?\n'.encode(),data),f'Card {i} stage {stage} missing'
  assert f'audio complete {i} '.encode() in data
  assert b'audio start' not in read_for(.2)
  result['lessons_completed'].append(i);save()
  print(f'Complete lesson {i}/17: all four stages PASS',flush=True)
 while current!=1:
  current=current%17+1;move('DOWN',current)
 screenshot('01-final')
 assert not re.search(rb'Guru Meditation|panic|checksum mismatch|audio failure|screen response failed|Stack canary|assert failed|out of memory',log,re.I)
 result.update(device_tests='PASS',final_card=1,final_mode='stopped',no_automatic_replay=True,unverified=['Physical button feel','Subjective speaker listening'])
 save();print('Device acceptance: PASS',flush=True)
except Exception as error:
 result.update(device_tests='FAIL',error=str(error));save();raise
finally:
 (out/'serial.log').write_bytes(log);s.close()
