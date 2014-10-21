/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

#ifndef PLUGINPROCESSOR_H_INCLUDED
#define PLUGINPROCESSOR_H_INCLUDED

#ifndef M_PI // for visual studio 2010
#define M_PI 3.14159265358979323846264338327950288
#endif
#include <stdint.h>

#include "../JuceLibraryCode/JuceHeader.h"

#if JUCE_MSVC
size_t strlcpy(char * dst, const char * src, size_t dstsize);
#endif

#include "FirFilter.h"
#include "Trajectories.h"

//==============================================================================


#define kNumberOfSources JucePlugin_MaxNumInputChannels
#define kNumberOfSpeakers JucePlugin_MaxNumOutputChannels

// x, y, distance
enum { kSourceX = 0, kSourceY = 1, kSourceD = 2, kSourceUnused = 3, kParamsPerSource = 4 };
#define ParamForSourceX(v) (kSourceX + v * kParamsPerSource)
#define ParamForSourceY(v) (kSourceY + v * kParamsPerSource)
#define ParamForSourceD(v) (kSourceD + v * kParamsPerSource)

// x, y, attenuation, mute
enum { kSpeakerX = 0, kSpeakerY = 1, kSpeakerA = 2, kSpeakerM = 3, kSpeakerUnused = 4, kParamsPerSpeakers = 5 };
#define ParamForSpeakerX(v) (kSpeakerX + kNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers)
#define ParamForSpeakerY(v) (kSpeakerY + kNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers)
#define ParamForSpeakerA(v) (kSpeakerA + kNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers)
#define ParamForSpeakerM(v) (kSpeakerM + kNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers)

#define kConstantOffset (kNumberOfSources * kParamsPerSource + kNumberOfSpeakers * kParamsPerSpeakers)
enum
{
	kLinkMovement =			0 + kConstantOffset,
	kSmooth =				1 + kConstantOffset,
	kVolumeNear =			2 + kConstantOffset,
	kVolumeMid =			3 + kConstantOffset,
	kVolumeFar =			4 + kConstantOffset,
	kFilterNear =			5 + kConstantOffset,
	kFilterMid =			6 + kConstantOffset,
	kFilterFar =			7 + kConstantOffset,
	kMaxSpanVolume =		8 + kConstantOffset,
	kConstantParameters =	9
};
#define kNumberOfParameters (kConstantParameters + kConstantOffset)

enum
{
	kFreeVolumeMode = 0,
	kPanVolumeMode = 1,
	kPanSpanMode = 2,
	kNumberOfModes = 3
};

//==============================================================================
// these must be normalized/denormalized for processing
static const float kSourceMinDistance = 2.5 * 0.5;
static const float kSourceMaxDistance = 20 * 0.5;
static const float kSourceDefaultDistance = 5 * 0.5;

static const float kSpeakerMinAttenuation = -70;
static const float kSpeakerMaxAttenuation = 20;
static const float kSpeakerDefaultAttenuation = 0;

static const float kSmoothMin = 1;
static const float kSmoothMax = 200;
static const float kSmoothDefault = 10;

static const float kVolumeNearMin = 0;
static const float kVolumeNearMax = 30;
static const float kVolumeNearDefault = 0;

static const float kVolumeMidMin = -30;
static const float kVolumeMidMax = 10;
static const float kVolumeMidDefault = -6;

static const float kVolumeFarMin = -120;
static const float kVolumeFarMax = 0;
static const float kVolumeFarDefault = -40;

static const float kMaxDistance = 2000;

static const float kFilterNearMin = kMaxDistance;
static const float kFilterNearMax = 0;
static const float kFilterNearDefault = 0;

static const float kFilterMidMin = kMaxDistance;
static const float kFilterMidMax = 0;
static const float kFilterMidDefault = 0;

static const float kFilterFarMin = kMaxDistance;
static const float kFilterFarMax = 0;
static const float kFilterFarDefault = kMaxDistance;

static const float kMaxSpanVolumeMin = 0;
static const float kMaxSpanVolumeMax = 20;
static const float kMaxSpanVolumeDefault = 10;

static const float kRadiusMax = 2;
static const float kHalfCircle = M_PI;
static const float kQuarterCircle = M_PI / 2;
static const float kThetaMax = M_PI * 2;
static const float kThetaLockRadius = 0.05;
static const float kThetaLockRampRadius = 0.025;

//==============================================================================
static inline float normalize(float min, float max, float value)
{
	return (value - min) / (max - min);
}
static inline float denormalize(float min, float max, float value)
{
	return min + value * (max - min);
}
static inline float dbToLinear(float db)
{
	return powf(10.f, (db) * 0.05f);
}
static inline float linearToDb(float linear)
{
	return log10f(linear) * 20.f;
}

typedef Point<float> FPoint;

typedef struct
{
	int i;
	float a;
} IndexedAngle;
int IndexedAngleCompare(const void *a, const void *b);

//==============================================================================
class OctogrisAudioProcessor : public AudioProcessor
{
public:
    //==============================================================================
    OctogrisAudioProcessor();
    ~OctogrisAudioProcessor();

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock);
    void releaseResources();

    void processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);
	void processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages);

    //==============================================================================
    AudioProcessorEditor* createEditor();
    bool hasEditor() const;

    //==============================================================================
    const String getName() const;

    int getNumParameters();

    float getParameter (int index);
    void setParameter (int index, float newValue);
	void setParameterNotifyingHost (int index, float newValue);

    const String getParameterName (int index);
    const String getParameterText (int index);

    const String getInputChannelName (int channelIndex) const;
    const String getOutputChannelName (int channelIndex) const;
    bool isInputChannelStereoPair (int index) const;
    bool isOutputChannelStereoPair (int index) const;

    bool acceptsMidi() const;
    bool producesMidi() const;
    bool silenceInProducesSilenceOut() const;
    double getTailLengthSeconds() const;

    //==============================================================================
    int getNumPrograms();
    int getCurrentProgram();
    void setCurrentProgram (int index);
    const String getProgramName (int index);
    void changeProgramName (int index, const String& newName);

    //==============================================================================
    void getStateInformation (MemoryBlock& destData);
    void setStateInformation (const void* data, int sizeInBytes);

    //==============================================================================
	// For editor
	int getNumberOfSources() const { return kNumberOfSources; }
	int getParamForSourceX(int index) const { return ParamForSourceX(index); }
	int getParamForSourceY(int index) const { return ParamForSourceY(index); }
	int getParamForSourceD(int index) const { return ParamForSourceD(index); }
	float getSourceX(int index) const { return mParameters.getUnchecked(ParamForSourceX(index)); }
	float getSourceY(int index) const { return mParameters.getUnchecked(ParamForSourceY(index)); }
	float getSourceD(int index) const { return mParameters.getUnchecked(ParamForSourceD(index)); }
	float getDenormedSourceD(int index) const { return denormalize(kSourceMinDistance, kSourceMaxDistance, getSourceD(index)); }
	
	int getNumberOfSpeakers() const { return kNumberOfSpeakers; }
	int getParamForSpeakerX(int index) const { return ParamForSpeakerX(index); }
	int getParamForSpeakerY(int index) const { return ParamForSpeakerY(index); }
	int getParamForSpeakerA(int index) const { return ParamForSpeakerA(index); }
	int getParamForSpeakerM(int index) const { return ParamForSpeakerM(index); }
	float getSpeakerX(int index) const { return mParameters.getUnchecked(ParamForSpeakerX(index)); }
	float getSpeakerY(int index) const { return mParameters.getUnchecked(ParamForSpeakerY(index)); }
	float getSpeakerA(int index) const { return mParameters.getUnchecked(ParamForSpeakerA(index)); }
	float getSpeakerM(int index) const { return mParameters.getUnchecked(ParamForSpeakerM(index)); }
	float getDenormedSpeakerA(int index) const { return denormalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, getSpeakerA(index)); }
	
	bool getApplyFilter() const { return mApplyFilter; }
	void setApplyFilter(bool s) { mApplyFilter = s; }
	
	bool getShowGridLines() const { return mShowGridLines; }
	void setShowGridLines(bool s) { mShowGridLines = s; }
	
	bool getLinkMovement() { return getParameter(kLinkMovement) > 0.5; }
	void setLinkMovement(bool s) { setParameterNotifyingHost(kLinkMovement, s ? 1 : 0); }
	
	int getMovementMode() const { return mMovementMode; }
	void setMovementMode(int s) { mMovementMode = s; }
	
	bool getLinkDistances() const { return mLinkDistances; }
	void setLinkDistances(bool s) { mLinkDistances = s; }
		
	int getProcessMode() const { return mProcessMode; }
	void setProcessMode(int s) { mProcessMode = s; jassert(mProcessMode >= 0 && mProcessMode < kNumberOfModes); }
	
	int getGuiSize() const { return mGuiSize; }
	void setGuiSize(int s) { mGuiSize = s; }
	
	int getGuiTab() const { return mGuiTab; }
	void setGuiTab(int s) { mGuiTab = s; }
	
	int getOscLeapSource() const { return mOscLeapSource; }
	void setOscLeapSource(int s) { mOscLeapSource = s; }
	
	int getOscReceiveEnabled() const { return mOscReceiveEnabled; }
	void setOscReceiveEnabled(int s) { mOscReceiveEnabled = s; }
	
	int getOscReceivePort() const { return mOscReceivePort; }
	void setOscReceivePort(int s) { mOscReceivePort = s; }
	
	int getOscSendEnabled() const { return mOscSendEnabled; }
	void setOscSendEnabled(int s) { mOscSendEnabled = s; }
	
	int getOscSendPort() const { return mOscSendPort; }
	void setOscSendPort(int s) { mOscSendPort = s; }
	
	const char * getOscSendIp() const { return mOscSendIp; }
	void setOscSendIp(const char *s) { strlcpy(mOscSendIp, s, sizeof(mOscSendIp)); }
	
	float getLevel(int index) const { return mLevels.getUnchecked(index); }
	void setCalculateLevels(bool c);
	
	uint64_t getHostChangedParameter() { return mHostChangedParameter; }
	uint64_t getHostChangedProperty() { return mHostChangedProperty; }
	uint64_t getProcessCounter() { return mProcessCounter; }
	
	// convenience functions for gui:
	FPoint convertXY01(float r, float t)
	{
		float x = r * cosf(t);
		float y = r * sinf(t);
		return FPoint((x + kRadiusMax)/(kRadiusMax*2), (y + kRadiusMax)/(kRadiusMax*2));
	}
	
	// these return in the interval [-kRadiusMax .. kRadiusMax]
	FPoint getSourceXY(int i)
	{
		float x = getSourceX(i) * (2*kRadiusMax) - kRadiusMax;
		float y = getSourceY(i) * (2*kRadiusMax) - kRadiusMax;
		return FPoint(x, y);
	}
	FPoint getSourceXY01(int i)
	{
		float x = getSourceX(i);
		float y = getSourceY(i);
		return FPoint(x, y);
	}
	FPoint getSpeakerXY(int i)
	{
		float x = getSpeakerX(i) * (2*kRadiusMax) - kRadiusMax;
		float y = getSpeakerY(i) * (2*kRadiusMax) - kRadiusMax;
		if (mProcessMode != kFreeVolumeMode)
		{
			// force radius to 1
			float r = hypotf(x, y);
			if (r == 0) return FPoint(1, 0);
			x /= r;
			y /= r;
		}
		return FPoint(x, y);
	}
	FPoint getSpeakerRT(int i)
	{
		FPoint p = getSpeakerXY(i);
		float r = hypotf(p.x, p.y);
		float t = atan2f(p.y, p.x);
		if (t < 0) t += kThetaMax;
		return FPoint(r, t);
	}
	FPoint getSourceRT(int i)
	{
		FPoint p = getSourceXY(i);
		float r = hypotf(p.x, p.y);
		float t = atan2f(p.y, p.x);
		if (t < 0) t += kThetaMax;
		return FPoint(r, t);
	}
	
	FPoint convertRT(FPoint p)
	{
		float vx = p.x;
		float vy = p.y;
		float r = sqrtf(vx*vx + vy*vy) / kRadiusMax;
		if (r > 1) r = 1;
		float t = atan2f(vy, vx);
		if (t < 0) t += kThetaMax;
		t /= kThetaMax;
		return FPoint(r, t);
	}
	FPoint convertRT01(FPoint p)
	{
		return convertRT(FPoint(p.x * (kRadiusMax*2) - kRadiusMax, p.y * (kRadiusMax*2) - kRadiusMax));
	}
	FPoint clampRadius01(FPoint p)
	{
		float dx = p.x - 0.5f;
		float dy = p.y - 0.5f;
		float r = hypotf(dx, dy);
		if (r > 0.5f)
		{
			float c = 0.5f / r;
			dx *= c;
			dy *= c;
			p.x = dx + 0.5f;
			p.y = dy + 0.5f;
		}
		return p;
	}
	void setSourceXY01(int i, FPoint p)
	{
		p = clampRadius01(p);
		setParameterNotifyingHost(getParamForSourceX(i), p.x);
		setParameterNotifyingHost(getParamForSourceY(i), p.y);
	}
	void setSourceXY(int i, FPoint p)
	{
		float r = hypotf(p.x, p.y);
		if (r > kRadiusMax)
		{
			float c = kRadiusMax / r;
			p.x *= c;
			p.y *= c;
		}
		p.x = (p.x + kRadiusMax) / (kRadiusMax*2);
		p.y = (p.y + kRadiusMax) / (kRadiusMax*2);
		setParameterNotifyingHost(getParamForSourceX(i), p.x);
		setParameterNotifyingHost(getParamForSourceY(i), p.y);
	}
	void setSourceRT(int i, FPoint p)
	{
		float x = p.x * cosf(p.y);
		float y = p.x * sinf(p.y);
		setSourceXY(i, FPoint(x, y));
	}
	void setSpeakerXY01(int i, FPoint p)
	{
		p = clampRadius01(p);
		setParameterNotifyingHost(getParamForSpeakerX(i), p.x);
		setParameterNotifyingHost(getParamForSpeakerY(i), p.y);
	}
	void setSpeakerRT(int i, FPoint p)
	{
		float x = p.x * cosf(p.y);
		float y = p.x * sinf(p.y);
		setParameterNotifyingHost(getParamForSpeakerX(i), (x + kRadiusMax) / (kRadiusMax*2));
		setParameterNotifyingHost(getParamForSpeakerY(i), (y + kRadiusMax) / (kRadiusMax*2));
	}
	
	// trajectories
	Trajectory::Ptr getTrajectory() { return mTrajectory; }
	void setTrajectory(Trajectory::Ptr t) { mTrajectory = t; }
	
private:
	Trajectory::Ptr mTrajectory;

	Array<float> mParameters;
	
	int mCalculateLevels;
	Array<float> mLevels;
	
	bool mApplyFilter;
	bool mLinkDistances;
	int mMovementMode;
	bool mShowGridLines;
	int mGuiSize;
	int mGuiTab;
	int mOscLeapSource;
	int mOscReceiveEnabled;
	int mOscReceivePort;
	int mOscSendEnabled;
	int mOscSendPort;
	char mOscSendIp[64]; // if changing size, change kDataVersion
	uint64_t mHostChangedParameter;
	uint64_t mHostChangedProperty;
	uint64_t mProcessCounter;
	int64 mLastTimeInSamples;
	
	int mProcessMode;
	
	bool mSmoothedParametersInited;
	Array<float> mSmoothedParameters;
	
	Array<float> mLockedThetas;
	
	#define kChunkSize (256)
	struct IOBuf { float b[kChunkSize]; };
	Array<IOBuf> mInputsCopy;
	Array<IOBuf> mSmoothedParametersRamps;
	
	FirFilter mFilters[kNumberOfSources];
	
	void findSpeakers(float t, float *params, int &left, int &right, float &dLeft, float &dRight, int skip = -1);
	void addToOutput(float s, float **outputs, int o, int f);
	void ProcessData(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataFreeVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataPanVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataPanSpanMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctogrisAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
