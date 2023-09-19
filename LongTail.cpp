#include "daisy_seed.h"
#include "daisysp.h"

#define SAMPLE_RATE 48000.f
#define MAX_DELAY 96000
#define MUX_CHANNELS_COUNT 8
#define PIN_MUX_CH_A 12
#define PIN_MUX_CH_B 13
#define PIN_MUX_CH_C 14

// TODO: PICK PIN FOR THE SWITCH!
#define PIN_EFFECT_MODE_SWITCH_1 -1
#define PIN_EFFECT_MODE_SWITCH_2 -1

#define PIN_ADC_MUX1_IN daisy::seed::A0
#define PIN_ADC_MUX2_IN daisy::seed::A1
#define PIN_ADC_REVERB_DECAY_IN daisy::seed::A2
#define PIN_ADC_REVERB_MIX_IN daisy::seed::A3
#define PIN_ADC_REVERB_CUTOFF_IN daisy::seed::A4

using namespace daisy;
using namespace daisysp;

enum AdcChannels { 
	ADC_MUX1, 
	ADC_MUX2, 
	ADC_REVERB_DECAY, 
	ADC_REVERB_MIX, 
	ADC_REVERB_CUTOFF, 
	ADC_CHANNELS_COUNT 
};
enum mux1Signals { 
	ADC_MUX1_FX_TIME, 
	ADC_MUX1_FX_TIME_ADJ, 
	ADC_MUX1_FX_DECAY, 
	ADC_MUX1_FX_DECAY_ADJ, 
	ADC_MUX1_FX_MIX, 
	ADC_MUX1_FX_MIX_ADJ,
	ADC_MUX1_FX_LFO_RATE, 
	ADC_MUX1_FX_LFO_SHAPE
};
enum mux2Signals{ 
	ADC_MUX2_ECHO_TIME, 
	ADC_MUX2_ECHO_TIME_ADJ,
	ADC_MUX2_ECHO_DECAY, 
	ADC_MUX2_ECHO_DECAY_ADJ,
	ADC_MUX2_ECHO_MIX, 
	ADC_MUX2_ECHO_MIX_ADJ,
	ADC_MUX2_ECHO_LFO_RATE, 
	ADC_MUX2_ECHO_LFO_SHAPE
};
enum effectModes{
	FX_MODE_CHORUS,
	FX_MODE_FLANGER,
	FX_MODE_PHASER
};

DaisySeed hw;
DelayLine<float, MAX_DELAY> delay; // type of buffer, length of buffer
ReverbSc reverb;
Phaser phaser;
Flanger flanger;
Chorus chorus;
int fxMode = 0;
float fxMixValue,
	echoMixValue, 
	reverbMixValue,
	echoFeedbackValue;

Switch effectModeSwitch1, effectModeSwitch2;

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size){
	for (size_t i = 0; i < size; i++){
		float signal = in[0][i];
		switch( fxMode ){
			case FX_MODE_CHORUS:
				float chorusSignal = chorus.Process(signal);
				signal = chorusSignal * fxMixValue + signal * (1.f - fxMixValue);
				break;
			case FX_MODE_FLANGER:
				float flangerSignal = flanger.Process(signal);
				signal = flangerSignal * fxMixValue + signal * (1.f - fxMixValue);
				break;
			case FX_MODE_PHASER:
				float phaserSignal = phaser.Process(signal);
				signal = phaserSignal * fxMixValue + signal * (1.f - fxMixValue);
		}
		float feedbackSignal = delay.Read() * echoFeedbackValue;
		float compositeSignal = (feedbackSignal * echoFeedbackValue) * echoMixValue + signal * (1.f - echoMixValue);
		delay.Write( compositeSignal );
		signal = delay.Read(signal);
		reverb.Process( signal, NULL, NULL, NULL );
		out[0][i] = out[1][i] = signal;
	}
}
void initAdc(){
	AdcChannelConfig adcConfig[ ADC_CHANNELS_COUNT ];
	adcConfig[ ADC_MUX1 ].InitMux( // INIT MUX 1
        PIN_ADC_MUX1_IN, 
        MUX_CHANNELS_COUNT, 
        hw.GetPin( PIN_MUX_CH_A ), 
        hw.GetPin( PIN_MUX_CH_B ), 
        hw.GetPin( PIN_MUX_CH_C )
    );
	adcConfig[ ADC_MUX2 ].InitMux( // INIT MUX 2
        PIN_ADC_MUX2_IN, 
        MUX_CHANNELS_COUNT, 
        hw.GetPin( PIN_MUX_CH_A ), 
        hw.GetPin( PIN_MUX_CH_B ), 
        hw.GetPin( PIN_MUX_CH_C )
    );
	// INIT NON-MUXED PINS	
	adcConfig[ ADC_REVERB_DECAY ].InitSingle( PIN_ADC_REVERB_DECAY_IN );
	adcConfig[ ADC_REVERB_MIX ].InitSingle( PIN_ADC_REVERB_MIX_IN );
	adcConfig[ ADC_REVERB_CUTOFF ].InitSingle( PIN_ADC_REVERB_CUTOFF_IN );
	hw.adc.Init( adcConfig, ADC_CHANNELS_COUNT );
    hw.adc.Start();
}
void initDsp(){
	chorus.Init(SAMPLE_RATE);
	flanger.Init(SAMPLE_RATE);
	phaser.Init(SAMPLE_RATE);
	reverb.Init(SAMPLE_RATE);
}
void handleFxModeSwitch(){
	effectModeSwitch1.Debounce();
	effectModeSwitch2.Debounce();
	if( effectModeSwitch1.Pressed() ) fxMode = FX_MODE_CHORUS;
	else if( effectModeSwitch2.Pressed() ) fxMode = FX_MODE_PHASER;
	else fxMode = FX_MODE_FLANGER;
}
void handleKnobs(){
	float fxDelayValue = hw.adc.GetMuxFloat( ADC_MUX1, ADC_MUX1_FX_TIME ),
		fxFeedbackValue = hw.adc.GetMuxFloat( ADC_MUX1, ADC_MUX1_FX_DECAY ),		
		echoTimeValue = hw.adc.GetMuxFloat( ADC_MUX2, ADC_MUX2_ECHO_TIME ),			
		reverbDecayValue = hw.adc.GetFloat( ADC_REVERB_DECAY ),		
		reverbCutoffValue = hw.adc.GetFloat( ADC_REVERB_CUTOFF );
	fxMixValue = hw.adc.GetMuxFloat( ADC_MUX1, ADC_MUX1_FX_MIX);
	echoMixValue = hw.adc.GetMuxFloat( ADC_MUX2, ADC_MUX2_ECHO_MIX );
	reverbMixValue = hw.adc.GetFloat( ADC_REVERB_MIX );
	echoFeedbackValue = hw.adc.GetMuxFloat( ADC_MUX2, ADC_MUX2_ECHO_DECAY );		
	chorus.SetDelay( fxDelayValue );
	flanger.SetDelay( fxDelayValue );
	// phaser.SetPoles( delayValue );
	chorus.SetFeedback( fxFeedbackValue );
	flanger.SetFeedback( fxFeedbackValue );
	phaser.SetFeedback( fxFeedbackValue );
	delay.SetDelay(echoTimeValue * MAX_DELAY);
	reverb.SetFeedback(reverbMixValue);
	reverb.SetLpFreq(reverbCutoffValue);	
}
int main(void){
	hw.Init();
	initDsp();
	initAdc();
	effectModeSwitch1.Init( hw.GetPin( PIN_EFFECT_MODE_SWITCH_1 ), 100 );
	effectModeSwitch2.Init( hw.GetPin( PIN_EFFECT_MODE_SWITCH_2 ), 100 );	
	hw.SetAudioBlockSize(4); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);
	hw.StartAudio(AudioCallback);
	while(true) {
		handleKnobs();
		handleFxModeSwitch();
	}
}