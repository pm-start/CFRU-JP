@ 戦闘中の進化（patches/0860・src/hack_mid_battle_evo.c）: DPE の HackMoemon_EvoFrontSheet の番地の置き場（1 語）。
@ tools/build/build_rom.py の patch_evo_front_bridge が DPE を組み立てたあと DPE の offsets.ini から書く（| 1）。0 なら表を直に引く。
@ （C の const だと 0 に畳まれるので .s に置く。前例: DPE の assembly/hack_sd_bridge.s）
.text
.align 2
.global gHackEvoFrontBridge
gHackEvoFrontBridge:
.word 0
