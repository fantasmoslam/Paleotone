# SCAPE redesign candidate

This branch starts by retiring the SLICER insert and its FX/editor page. Numeric track parameter slots 45..48 remain reserved so projects, presets, MIDI descriptors and automation keep their existing layout. Stored insert values are ignored by the audio path. The performance layer retains its own 32 KiB stereo buffer and all existing effects. Removing SLICER therefore does not reclaim that buffer.

## Next implementation

Replace the disconnected composition/environment workflow with one musical director: shared key, chord and slow activity arc; explicit bed, melody, bass and pulse roles; coordinated spaces between calls and musical phrases. Retain normal transport and instrument editing. Keep environmental layers procedural on the device and preserve the native dual-screen renderer.

Proposed overview: Activity, Evolution, Motion, Chord time. Music: Bed, Melody, Bass, Pulse levels. Habitat: Biome, Day/night, Distance, Creature activity. Detailed environmental mixing remains accessible. The shared director, role assignment and these pages are design targets, not implemented behavior in the SLICER-removal commit.

Define backward-compatible storage before adding controls. Do not reinterpret old global IDs as new controls. Validate unchanged ordinary sequences and sounds, deterministic evolution, rests/phrase boundaries, project round trips, voice budgets, target headroom and matched native/WASM rendering before installing a new candidate.
