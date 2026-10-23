# tools/ida/finish/frame_load.py — regenerate src/frame_load.c: the decomp2c first pass + its hand finish (0x5b7310's
# 24-bit loop: IDA garbles it — its ebx "argument" is the scratch register whose low word the loop writes first).  The
# module header is kept from the current file.  Run inside nix develop: python3 tools/ida/finish/frame_load.py
import os, subprocess, sys
R=os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
VAS=("4178e0:fl_4178e0 4184a0:fl_4184a0 4188b0:fl_4188b0 499850:fl_499850 5b6e40:bs_5b6e40 5b6e70:bs_5b6e70 "
     "5b6e80:bs_5b6e80 5b6e90:bs_5b6e90 5b6eb0:bs_5b6eb0 5b6ec0:bs_5b6ec0 5b6ee0:bs_5b6ee0 5b6ef0:bs_5b6ef0 "
     "5b6f00:bs_5b6f00 5b6f10:bs_5b6f10 5b6f30:bs_5b6f30 5b6f80:bs_5b6f80 5b71f0:bs_5b71f0 5b7270:bs_5b7270 "
     "5b7310:bs_5b7310 5b74f0:bs_5b74f0 5b7620:bs_5b7620 5b7800:bs_5b7800 5b7970:bs_5b7970 5b7b90:bs_5b7b90 "
     "5b7bd0:bs_5b7bd0 5b7c10:bs_5b7c10 5b7cd0:bs_5b7cd0 5b9280:bs_5b9280 5b9630:bs_5b9630 5b9910:bs_5b9910 "
     "5ba390:bs_5ba390").split()
out=subprocess.run([sys.executable, R+'/tools/ida/decomp2c.py','--typed','--zero-locals']+VAS, capture_output=True, text=True, cwd=R)
if out.returncode:
    sys.exit(out.stderr)
out=out.stdout
body=out[min(i for i in (out.find('#include'), out.find('#pragma')) if i >= 0):]
a=body.index('      v15 = 0;\n      v28 = 0;\n      v16 = v12;')
b=body.index('      while ( v29 != 0 );\n', a)+len('      while ( v29 != 0 );\n')
body=body[:a]+'''      /* (hand finish: the asm's loop — per source triple b, g, r: (r >> [a3+16]) << [a3], (g >> [a3+17]) << [a3+1],
       * (b >> [a3+18]) << [a3+2], OR'd, the low word stored; each channel is shifted as a byte, the registers'
       * upper halves (the caller's ebx among them) never reach the stored word) */
      v15 = (uint8_t *)F32(c, 0);
      v16 = v12;
      for ( v29 = v13; v29 != 0; --v29, v15 += 3, ++v16 )
        *v16 = (uint16_t)(((uint32_t)(uint8_t)(v15[2] >> a3[16]) << a3[0])
                          | ((uint32_t)(uint8_t)(v15[1] >> a3[17]) << a3[1])
                          | ((uint32_t)(uint8_t)(v15[0] >> a3[18]) << a3[2]));
'''+body[b:]
x='  *c = (*v27__f);\n'
assert body.count(x)==1
body=body.replace(x,'  F32(c, 0) = (*v27__f);\n')
for dead in ('    intptr_t v17 = 0;\n    uint8_t * v18 = 0;\n', '    uint8_t * v28 = 0;\n'):
    i=body.index('/* 0x5b7310:'); j=body.index('/* 0x5b74f0:')
    assert body[i:j].count(dead)==1
    body=body[:i]+body[i:j].replace(dead,'')+body[j:]
p=R+'/src/frame_load.c'
head=open(p).read() if os.path.exists(p) else ''
head=head[:head.index('#include <windows.h>')] if '#include <windows.h>' in head else head
open(p,'w').write(head+body)
print('ok', len(body))
