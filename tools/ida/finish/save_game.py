# tools/ida/finish/save_game.py — regenerate src/save_game.c: the decomp2c first pass + its hand finish (0x41a890's
# inlined logger copies -> sv_last_error / sv_fatal, a5's deref; 0x55f620 hands on 0x55f790's whole eax).  The helpers +
# the module header are kept from the current file.  Run inside nix develop: python3 tools/ida/finish/save_game.py
import re, subprocess, sys
import os
R=os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
VAS="41a430:sv_41a430 41a890:sv_41a890 41b670:sv_41b670 41bd20:sv_41bd20 41bf60:sv_41bf60 55f620:sv_55f620 561560:sv_561560 561bf0:sv_561bf0 561cf0:sv_561cf0 561d20:sv_561d20 560530:sv_560530 41cfd0:sv_41cfd0".split()
out=subprocess.run([sys.executable, R+'/tools/ida/decomp2c.py','--typed','--zero-locals']+VAS, capture_output=True, text=True, cwd=R).stdout
body=out[out.index('#include'):] if out.lstrip().startswith('#include') else out[min(i for i in (out.find('#include'), out.find('#pragma')) if i >= 0):]
a=body.index('/* 0x41a890:'); b=body.index('/* 0x41b670:')
w=body[a:b]
def cut(w, start, end_pat, repl):
    i=w.index(start); j=w.index(end_pat, i)+len(end_pat)
    return w[:i]+repl+w[j:]
w=cut(w,'      LastError = API_GetLastError();','      g_eng.h5bf5db(0);',
      '      sv_last_error(Buffer, OutputString);\n      sv_fatal((char *)D(0x854ef4), FileName);   /* "Unable to write files, cannot save game. ..." */')
w=cut(w,'    v18 = API_GetLastError();','    g_eng.h5bf5db(0);',
      '    sv_last_error(Buffer, OutputString);\n    sv_fatal((char *)D(0x854ee0), v77);   /* "Disk Logic Error" */')
w=cut(w,'      v43 = API_GetLastError();','      g_eng.h5bf5db(0);',
      '      sv_last_error(Buffer, OutputString);\n      sv_fatal((char *)D(0x854ef4), FileName);')
w=cut(w,'    API_OutputDebugStringA((intptr_t)(((char *)D(0x854ef4))));','      CRT_fclose(v41);\n    }',
      '    g_eng.log_fail_406440((void *)D(0x8a9530), (char *)D(0x854ef4), FileName);   /* (0x406440 inlined in retail) */')
assert w.count('(*v69__f) = *a5;')==1
w=w.replace('(*v69__f) = *a5;','(*v69__f) = F32(a5, 0);   /* a5: a __time32_t * */')
body=body[:a]+w+body[b:]
x='  SETLO(result, W_room_to_scene((intptr_t)(v3), (intptr_t)(a2)));'
assert body.count(x)==1
body=body.replace(x, '  result = W_room_to_scene((intptr_t)(v3), (intptr_t)(a2));   /* eax as 0x55f790 leaves it (IDA: LOWORD, an __int16 return) */')
helpers=open(R+'/src/save_game.c').read()
hs=helpers[helpers.index('/* retail\'s inline error blocks'):helpers.index('/* 0x41a430:')]
head=helpers[:helpers.index('#include "event.h"') if '#include "event.h"' in helpers else helpers.index('#pragma')]
k=body.index('/* 0x41a430:')
body=body[:k]+hs+body[k:]
open(R+'/src/save_game.c','w').write(head+body)
print('ok', len(body))
