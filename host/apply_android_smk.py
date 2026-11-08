#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
import argparse,shutil,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parent
PIN='3e3ea79b729da53c11196513e0ebb9a56ef42ab5'
def apply(remote):
    for relative in (ROOT/'smk/targets.txt').read_text().splitlines():
        (remote/relative).write_bytes(subprocess.check_output(['git','show',f'{PIN}:{relative}'],cwd=remote))
    subprocess.run(['git','apply','--check',str(ROOT/'smk/runtime.patch')],cwd=remote,check=True)
    subprocess.run(['git','apply',str(ROOT/'smk/runtime.patch')],cwd=remote,check=True)
    for kind in ['main','test']:
        dest=remote/f'app/src/{kind}/java/com/quixotic7/remotefm1';dest.mkdir(parents=True,exist_ok=True)
        for src in (ROOT/'smk'/kind).glob('*.kt'):shutil.copy2(src,dest/src.name)
    shutil.copy2(ROOT/'smk/native/SmkProfile.h',remote/'native/rfmcore/SmkProfile.h')
    p=remote/'app/src/main/java/com/quixotic7/remotefm1/AppModel.kt';s=p.read_text()
    old='''    private fun refreshMidi() {
        midiEntries = RfmMidi.entries()
    }'''
    new='''    private var autoBridgeDevice: Int? = null
    private fun refreshMidi() {
        midiEntries = RfmMidi.entries()
        val own = midiEntries.firstOrNull { it.desc.displayName.contains("Instrument Host SMK", ignoreCase = true) }?.desc?.id
        if (own != autoBridgeDevice) {
            autoBridgeDevice = own
            if (own != null && source != Source.FM1) useSource(Source.FM1)
        }
    }'''
    assert old in s or new in s
    p.write_text(s.replace(old,new))
    p=remote/'app/src/main/java/com/quixotic7/remotefm1/ui/SheetPages.kt';s=p.read_text().replace('SheetSectionTitle("Firmware runs on")','SheetSectionTitle("Controller connection")').replace('"Emulator, play by touch"','"Touch and ordinary MIDI"').replace('The FM-1 plays through its own speaker and headphone jack; this ${a.deviceWord} is its remote.','The ${a.deviceWord} runs the instrument. Bridge firmware sends controls and receives its screen and lights.')
    # Audio destination is independent of the selected controller source.
    s=s.replace('''            } else {
                SheetList(Modifier.padding(top = 10.dp)) {''','''            }
            run {
                SheetList(Modifier.padding(top = 10.dp)) {''',1)
    p.write_text(s)
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('--remote',type=Path,required=True);apply(p.parse_args().remote.resolve())
