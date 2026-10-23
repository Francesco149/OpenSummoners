# tools/ida/finish/sound_dev.py — regenerate src/sound_dev.c: the decomp2c first pass + its hand finish.
#  * 0x5bb3d0 (IDA __userpurge, "a2@esi"): retail pushes esi mid-body (a callee-save inside one path) and IDA took it
#    for an argument of the IDirectSoundBuffer::Unlock call — so IDA invents an esi input, passes it to Unlock as a 6th
#    argument, reads every later stack argument 4 bytes off (a phantom a7) and drops SetVolume's 0.  The asm: 6 stack
#    arguments (retn 18h), Unlock(this, p1, n1, p2, n2), SetVolume(this, 0); the caller 0x5bb250 pushes
#    (0, hModule, res, 1, a5, a6).
#    After the Unlock IDA's frame labels are 4 bytes off too (v20 / hMem / v21 / v25[5] = hMem / v18 / v20 / v25[4]).
#  * 0x5bbd20 (on the zdm / zdm_entry shells, which decomp2c passes as words): by hand from the asm.
# The module header is kept from the current file.  Run inside nix develop: python3 tools/ida/finish/sound_dev.py
import os, re, subprocess, sys
R=os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
VAS=[]
for line in open('/mnt/ephemeral/opensummoners/decompiled-ida/enold/functions_ida.tsv'):
    f=line.split('\t')
    if re.match(r'^[0-9a-f]{8}$', f[0]) and 0x5ba740 <= int(f[0], 16) < 0x5bcff0:
        VAS.append('%x:sd_%x' % (int(f[0], 16), int(f[0], 16)))
out=subprocess.run([sys.executable, R+'/tools/ida/decomp2c.py','--typed','--zero-locals']+VAS, capture_output=True, text=True, cwd=R)
if out.returncode:
    sys.exit(out.stderr)
out=out.stdout
body=out[min(i for i in (out.find('#include'), out.find('#pragma')) if i >= 0):]
def rep(a, b, n=1):
    global body
    assert body.count(a)==n, (a, body.count(a))
    body=body.replace(a, b)
old_sig='int32_t sd_5bb3d0(int32_t * a1, int32_t a2, char * pszFileName, int32_t hModule, uint16_t a5, int32_t a6, int32_t a7, char ** a8, int32_t a9)'
new_sig='int32_t sd_5bb3d0(int32_t * a1, char * pszFileName, int32_t hModule, uint16_t a5, int32_t a6, char ** a8, int32_t a9)'
rep(old_sig+';', new_sig+';')
rep(old_sig+'\n{', '/* (hand finish: 6 stack arguments — IDA\'s esi input and its a7 are the mid-body `push esi`, see the module header) */\n'+new_sig+'\n{')
rep('''      (*(void (__stdcall **)(_DWORD, void *, HGLOBAL, char *, unsigned int, int))(*(_DWORD *)*a1 + 76))(
        *a1,
        (*v23__f),
        (*v20__f),
        v12,
        (*v22__f),
        a2);''', '''      (*(void (__stdcall **)(_DWORD, void *, HGLOBAL, char *, unsigned int))(*(_DWORD *)*a1 + 76))(   /* Unlock */
        *a1,
        (*v23__f),
        (*v20__f),
        v12,
        (*v22__f));''')
rep('''      (*(void (__stdcall **)(int32_t))(*(_DWORD *)*a1 + 60))(*a1);''',
    '''      (*(void (__stdcall **)(int32_t, int32_t))(*(_DWORD *)*a1 + 60))(*a1, 0);   /* SetVolume(0) */''')
rep('sd_5bb3d0(v8, (int)v8, 0, hModule, a4, 1, a5, (char **)a6, (*v10__f));', 'sd_5bb3d0(v8, 0, hModule, a4, 1, (char **)a5, a6);')
# after Unlock IDA's frame labels are 4 off (its sp is 4 low): its v20 / hMem / v21 / v25[5] are really hMem / v18 /
# v20 / v25[4] — free the WAVE block, the out a8 = the bytes locked, the format = desc.lpwfxFormat (v25[4])
rep('''      if ( (*v20__f) != 0 )
      {
        API_GlobalUnlock((intptr_t)((*hMem__f)));
        API_GlobalFree((intptr_t)((*v20__f)));
        (*v20__f) = 0;
      }
      (*hMem__f) = 0;
      if ( a8 != 0 )
        *a8 = (*v21__f);
      v16 = v25[5];''', '''      /* (hand finish: IDA's labels here are 4 bytes off — see the module header) */
      if ( (*hMem__f) != 0 )
      {
        API_GlobalUnlock((intptr_t)((*v18__f)));
        API_GlobalFree((intptr_t)((*hMem__f)));
        (*hMem__f) = 0;
      }
      (*v18__f) = 0;
      if ( a8 != 0 )
        *a8 = (char *)(*v20__f);
      v16 = v25[4];''')
a=body.index('/* 0x5bbd20:'); b=body.index('/* 0x5bbdb0:')
body=body[:a]+'''/* 0x5bbd20: void __thiscall sub_5BBD20(zdm *this, int active) */
/* (hand finish, from the asm: zdm +0x18 entries (56 bytes: +0 dev, +4 track, +0x20 active, +0x28 cookie), +0x1c count,
 * +0x20 the 576-byte tracks) */
void sd_5bbd20(wl_char *self, int32_t active)
{
    intptr_t c = (intptr_t)self;
    uint16_t i = 0;
    intptr_t v4 = 0;
    intptr_t * v5 = 0;

  for ( i = 0; i < F16(c, 0x1c); ++i )
  {
    v4 = F32(c, 0x18) + 56 * i;
    if ( F32(v4, 0) != 0 )
    {
      v5 = (intptr_t *)(F32(c, 0x20) + 576 * F16(v4, 4));
      if ( F32(v4, 0x20) != active )
      {
        if ( active != 0 )
        {
          F64(v4, 0x28) = sd_5bae70(*v5);
          sd_5bad40(*v5);
        }
        else
        {
          sd_5bae30(*v5, F32(v4, 0x28), F32(v4, 0x2c));
          sd_5bad20(*v5);
        }
        F32(v4, 0x20) = active;
      }
    }
  }
}

'''+body[b:]
p=R+'/src/sound_dev.c'
head=open(p).read() if os.path.exists(p) else ''
head=head[:min(i for i in (head.find('#include <windows.h>'), head.find('#pragma')) if i >= 0)] if head else ''
open(p,'w').write(head+body)
print('ok', len(body))
