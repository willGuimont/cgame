import math
import struct
import wave
import os

def write_wav(filename, sample_rate, duration, generator_func):
    num_samples = int(sample_rate * duration)
    data = bytearray()
    for i in range(num_samples):
        t = i / sample_rate
        val = generator_func(t, duration)
        val = max(-1.0, min(1.0, val))
        int_val = int(val * 32767)
        data.extend(struct.pack('<h', int_val))
        
    os.makedirs(os.path.dirname(filename), exist_ok=True)
    with wave.open(filename, 'wb') as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(sample_rate)
        w.writeframes(data)
    print(f"Generated {filename}")

def click_gen(t, duration):
    freq = 1000 - 800 * (t / duration)
    amplitude = math.exp(-40 * t)
    return amplitude * math.sin(2 * math.pi * freq * t)

def move_gen(t, duration):
    # Claves: high pitch (~2200 Hz) with an inharmonic wood resonance overtone and steep decay
    freq = 2200.0
    val = math.sin(2 * math.pi * freq * t) + 0.3 * math.sin(2 * math.pi * freq * 2.72 * t)
    amplitude = math.exp(-110.0 * t)
    return amplitude * val

def merge_gen(t, duration):
    freq1 = 400 + 100 * (t / duration)
    freq2 = 600 + 150 * (t / duration)
    amplitude = math.exp(-8 * t)
    wave_val = 0.6 * math.sin(2 * math.pi * freq1 * t) + 0.4 * math.sin(2 * math.pi * freq2 * t)
    return amplitude * wave_val

def win_gen(t, duration):
    notes = [261.63, 329.63, 392.00, 523.25]  # C4, E4, G4, C5
    delays = [0.0, 0.12, 0.24, 0.36]
    val = 0.0
    for note_freq, delay in zip(notes, delays):
        if t >= delay:
            note_t = t - delay
            amp = math.exp(-4.0 * note_t)
            val += amp * (0.6 * math.sin(2 * math.pi * note_freq * note_t) + 
                          0.3 * math.sin(2 * math.pi * (note_freq * 2) * note_t) +
                          0.1 * math.sin(2 * math.pi * (note_freq * 3) * note_t))
    fade = 1.0
    if t > 0.6:
        fade = max(0.0, 1.0 - (t - 0.6) / 0.2)
    return val * 0.25 * fade

def reset_gen(t, duration):
    freq = 300 - 220 * (t / duration)
    amplitude = math.exp(-7 * t)
    return amplitude * math.sin(2 * math.pi * freq * t)

def undo_gen(t, duration):
    freq = 150 + 200 * (t / duration)
    amplitude = math.sin(math.pi * (t / duration)) * math.exp(-3 * t)
    return amplitude * math.sin(2 * math.pi * freq * t)

def main():
    sample_rate = 44100
    base_dir = "/home/wigum/Projects/cgame/assets/audio"
    
    write_wav(os.path.join(base_dir, "click.wav"), sample_rate, 0.05, click_gen)
    write_wav(os.path.join(base_dir, "move.wav"), sample_rate, 0.15, move_gen)
    write_wav(os.path.join(base_dir, "merge.wav"), sample_rate, 0.25, merge_gen)
    write_wav(os.path.join(base_dir, "win.wav"), sample_rate, 0.8, win_gen)
    write_wav(os.path.join(base_dir, "reset.wav"), sample_rate, 0.3, reset_gen)
    write_wav(os.path.join(base_dir, "undo.wav"), sample_rate, 0.15, undo_gen)

if __name__ == "__main__":
    main()
