"""Reproduce the original T19 placeholder sounds using Python's standard library.

No recordings, downloads or third-party samples. 48 kHz, mono, 16-bit PCM.
"""
from pathlib import Path
import hashlib
import json
import math
import random
import struct
import wave

ROOT = Path(__file__).resolve().parents[1]
OUT = ROOT / 'SourceArt/Feedback'
SOUNDS = {
    'S_Rifle': (.09, 540, 160, .7),
    'S_Hit': (.07, 1900, 1300, .18),
    'S_Damage': (.16, 190, 80, .4),
    'S_Launcher': (.28, 220, 950, .12),
    'S_Bomb': (.36, 110, 35, .85),
    'S_Emitter': (.22, 750, 1400, .08),
    'S_Pulse': (.08, 1200, 650, .12),
    'S_Warning': (.26, 880, 880, 0),
}


def generate():
    OUT.mkdir(parents=True, exist_ok=True)
    report = []
    for index, (name, (duration, start, end, noise)) in enumerate(SOUNDS.items()):
        rng = random.Random(1900 + index)
        samples, phase, low_noise = [], 0., 0.
        for frame in range(round(duration * 48000)):
            t = frame / 48000
            u = t / duration
            phase += 2 * math.pi * (start + (end - start) * u) / 48000
            low_noise = .6 * low_noise + .4 * rng.uniform(-1, 1)
            envelope = min(1, t / .004) * (1 - u)**2
            signal = (1 - noise) * math.sin(phase) + noise * low_noise
            if name == 'S_Warning':
                signal *= .15 if .085 < t < .14 else 1
            samples.append(signal * envelope)
        gain = .7 / max(abs(v) for v in samples)
        pcm = b''.join(struct.pack('<h', round(v * gain * 32767)) for v in samples)
        path = OUT / (name + '.wav')
        with wave.open(str(path), 'wb') as sound:
            sound.setnchannels(1)
            sound.setsampwidth(2)
            sound.setframerate(48000)
            sound.writeframes(pcm)
        report.append(dict(name=name, seconds=len(samples)/48000, sample_rate=48000,
                           channels=1, peak=.7, sha256=hashlib.sha256(path.read_bytes()).hexdigest()))
    (OUT / 'AudioManifest.json').write_text(json.dumps(dict(
        author='YYchainsAw/MothEffect project; synthesized for T19',
        source='scripts/generate_t19_audio.py; no third-party recordings or samples',
        sounds=report), indent=2) + '\n', encoding='utf-8')
    print(f'Generated {len(report)} original feedback sounds in {OUT}')


if __name__ == '__main__':
    generate()
