#include "daisy_seed.h"
#include "daisysp.h"

using namespace daisy;
using namespace daisysp;

DaisySeed hw;
// TODO: ADD EFFECTS: DELAYLINE, CHORUSE, FLANGER, PHASER, REVERB
DelayLine<float, 100> delay; // type of buffer, length of buffer
ReverbSc reverb;
Svf filter;
Phaser phaser;
Flanger flanger;
Chorus chorus;

// SIGNAL FLOW: chorus/flanger/phaser -> delay -> reverb
// lfo can be routed to effect, delay, and reverb(?)





void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
	for (size_t i = 0; i < size; i++){
		out[0][i] = in[0][i];
		out[1][i] = in[1][i];
	}
}

int main(void){
	hw.Init();
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAudio(AudioCallback);
	while(1) {}
}
