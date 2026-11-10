/* 生成物: tools/fishing/gen_nushi.py（元は tools/fishing/nushi_src/nushi_audio.h・直に編集しない）。
 * 由来: liru 氏 pm-start/fishing-rework@aad8bcf（詳細は tools/fishing/README.md）。 */
#ifndef NUSHI_AUDIO_H
#define NUSHI_AUDIO_H

/* Native M4A, one hardware square channel, no PCM sample or timer IRQ.
 * Bytecode: TEMPO=150 BPM, VOICE=0, VOL=80, PAN=center,
 * N36 A4 / W36 / FINE (strike), N04 A3 / W04 / FINE (nibble).
 * 24 ticks per quarter note => 600 ms / 66.7 ms nominal gate lengths.
 * Encoding follows pret/pokefirered sound/MPlayDef.s and
 * asm/macros/music_voice.inc (voice_square_2), not a guessed SE name. */
#include "../include/m4a.h"
#include "../include/gba/m4a_internal.h"

static const struct ToneData nfFishingTone = {
    .type=2, .key=60, .length=0, .pan_sweep=0,
    .wav=(const struct WaveData *)2, /* 50% square-wave duty cycle */
    .attack=0, .decay=0, .sustain=15, .release=0
};
static const unsigned char nfFishingStrikeTrack[] = {
    0xbb,75, 0xbd,0, 0xbe,80, 0xbf,64,
    0xeb,81,100, 0x9c, 0xb1
};
static const unsigned char nfFishingNibbleTrack[] = {
    0xbb,75, 0xbd,0, 0xbe,80, 0xbf,64,
    0xd3,69,100, 0x84, 0xb1
};
/* Compatible fixed one-track representation of M4A's flexible SongHeader. */
static const struct NfFishingSong {
    unsigned char trackCount,blockCount,priority,reverb;
    const struct ToneData *tone;
    const unsigned char *part[1];
} nfFishingStrikeSong={1,0,5,0,&nfFishingTone,{nfFishingStrikeTrack}},
  nfFishingNibbleSong={1,0,5,0,&nfFishingTone,{nfFishingNibbleTrack}};

static void NfFishingSound(unsigned char strike)
{
    MPlayStart(&gMPlayInfo_SE1,(const struct SongHeader *)(strike ?
        &nfFishingStrikeSong : &nfFishingNibbleSong));
}
#endif
