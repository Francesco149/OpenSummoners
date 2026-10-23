# tools/ida/finish/title_menu.py — regenerate src/title_menu.c with today's decomp2c (tools/ida/decomp2c_regen.py, the
# module's hand ports between functions kept) + its hand finish: 0x562ea0's display-mode switch — on a set cooperative
# level retail jumps to the switch's default case (it carries on into the screen set-up); IDA's decompile returns
# there.  Run inside nix develop: python3 tools/ida/finish/title_menu.py
import os, re, subprocess, sys
R=os.path.dirname(os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__)))))
r=subprocess.run([sys.executable, R+'/tools/ida/decomp2c_regen.py', 'src/title_menu.c', '--force'], cwd=R, capture_output=True, text=True)
sys.stdout.write(r.stdout); sys.stderr.write(r.stderr)
if r.returncode:
    sys.exit(r.returncode)
p=R+'/src/title_menu.c'; t=open(p).read()
a=t.index('/* 0x562ea0:'); b=t.index('\n/* 0x', a + 10)
f=t[a:b]
pat=re.compile(r"(g_eng\.h5bf5db\(0\);\n      \}\n)      return result;\n(    case 2u:|    default:)")
f2, n = pat.subn(lambda m: m.group(1) + "      break;   /* (retail: jnz to the default case; IDA: a return) */\n" + m.group(2), f)
if n == 0 and f.count('(retail: jnz to the default case') == 2:
    n = 2   # (already finished: the regeneration kept it)
assert n == 2, n
open(p,'w').write(t[:a]+f2+t[b:])
print('ok: 0x562ea0 finished')
