#include "daisy_seed.h"
#include "daisysp.h"

#define NUM_OSCS 40

using namespace daisy;
using namespace daisysp;
using namespace seed;

DaisySeed hw;
static Oscillator oscBank[NUM_OSCS];

struct {
	const Pin freq = A0;
	const Pin amp = A1;
	const Pin spread = A2;
	const Pin lpf = A3;
} CVPins;

struct {
	float freq = 110.f;
	float amp = 1.f;
	float spread = 0.f;
	float lpf = 20000.f;
} CVInputs;

void InitOscs() {
	for (size_t i = 0; i < NUM_OSCS; i++)
		oscBank[i].Init(hw.AudioSampleRate());
}

void TuneOscs() {
	
	float freq = 1.f - hw.adc.GetFloat(0);
	float amp = 1.f - hw.adc.GetFloat(1);
	float spread = 1.f - hw.adc.GetFloat(2);
	float lpf = 1.f - hw.adc.GetFloat(3);

	CVInputs.freq = fmap(freq, 40.f, 1280.f, Mapping::EXP);
	CVInputs.amp = fmap(amp, 0.f, 1.f, Mapping::EXP);
	CVInputs.spread = fmap(spread, 0.f, 0.5f, Mapping::EXP);
	CVInputs.lpf = fmap(lpf, 0.f, 20000.f, Mapping::EXP);
	
	if (CVInputs.spread < 0.003f) {
		CVInputs.spread = 0;
	}

	for (size_t i = 0; i < NUM_OSCS; i++) {
		float targetFreq = CVInputs.freq * (i + 1) * (CVInputs.spread * i + 1);
		float targetAmp = (CVInputs.amp * 0.5f) / ((float)i + 1.f);
		oscBank[i].SetFreq(targetFreq);

		if ((targetFreq > (hw.AudioSampleRate() / 2.f)) || (targetFreq > CVInputs.lpf))
			oscBank[i].SetAmp(0);
		else
			oscBank[i].SetAmp(targetAmp);
	}
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size)
{
	TuneOscs();
	
	float oscOut;
	for (size_t s = 0; s < size; s++)
	{
		oscOut = 0;
		for (auto& o : oscBank) {
			oscOut += o.Process();
		}
		out[0][s] = oscOut;
		out[1][s] = oscOut;
	}
	hw.SetLed(CVInputs.freq > 400.f);
}

int main(void)
{
	hw.Init();
	hw.SetAudioBlockSize(255); // number of samples handled per callback
	hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

	InitOscs();
	TuneOscs();

	AdcChannelConfig adcCfg[4];

	adcCfg[0].InitSingle(CVPins.freq);
	adcCfg[1].InitSingle(CVPins.amp);
	adcCfg[2].InitSingle(CVPins.spread);
	adcCfg[3].InitSingle(CVPins.lpf);
	hw.adc.Init(adcCfg, 4);
	hw.adc.Start();

	hw.StartAudio(AudioCallback);
	while(1) {}
}
