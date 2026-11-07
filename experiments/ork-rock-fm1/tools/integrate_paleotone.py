#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Integrate into an explicitly supplied isolated Paleotone checkout.
Never run against the user's active checkout; refuses a dirty tracked tree.
This only prepares source. It does not build, upload samples, or flash hardware.
"""
import argparse,pathlib,shutil,subprocess
def integrate(root):
 project=pathlib.Path(__file__).resolve().parents[1];root=root.resolve()
 if root==project or not (root/'firmware/src/core.h').exists():raise ValueError('expected isolated Paleotone checkout')
 if subprocess.check_output(['git','-C',str(root),'status','--porcelain','--untracked-files=no'],text=True).strip():raise ValueError('tracked changes exist; use a clean isolated checkout')
 core=root/'firmware/src/core.h';eng=root/'firmware/src/engines.c';main=root/'firmware/src/main.c'
 c=core.read_text();e=eng.read_text();m=main.read_text()
 if '#define NENGINES 16u' not in c or '&ENG_SAURUS,' not in e or '    audio_init();' not in m:raise ValueError('unrecognized platform snapshot; review adapter before integrating')
 c=c.replace('#define NENGINES 16u','#define NENGINES 17u',1)
 anchor='#include "eng_saurus.c"';pos=e.index('\n',e.index(anchor))+1;e=e[:pos]+'#include "eng_ork.c"\n'+e[pos:]
 anchor='    &ENG_SAURUS,';pos=e.index('\n',e.index(anchor))+1;e=e[:pos]+'    &ENG_ORK,                    /* 16: recorded ORK prototype */\n'+e[pos:]
 anchor='    15,                          /* SAURUS';pos=e.index('\n',e.index(anchor))+1;e=e[:pos]+'    16,                          /* ORK */\n'+e[pos:]
 m=m.replace('    audio_init();','    ork_felucca_prepare();       /* bank CRC before the audio ISR starts */\n    audio_init();',1)
 target=root/'firmware/src/ork';target.mkdir(exist_ok=True)
 for name in ['ork.c','ork.h']:shutil.copyfile(project/'src'/name,target/name)
 shutil.copyfile(project/'device/paleotone_adapter.c',root/'firmware/src/eng_ork.c')
 core.write_text(c);eng.write_text(e);main.write_text(m)
 print('Prepared isolated ORK engine source; firmware build and hardware qualification are still required.')
if __name__=='__main__':
 p=argparse.ArgumentParser();p.add_argument('checkout',type=pathlib.Path);a=p.parse_args();integrate(a.checkout)
