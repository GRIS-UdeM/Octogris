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

#include <memory>
using namespace std;

//==============================================================================

// x, y, distance
enum {
    kSourceX = 0,
    kSourceY,
    kSourceD,
    kSourceUnused,
    kParamsPerSource
};


// x, y, attenuation, mute
enum {
    kSpeakerX = 0,
    kSpeakerY,
    kSpeakerA,
    kSpeakerM,
    kSpeakerUnused,
    kParamsPerSpeakers };



//#define mNumberOfSources JucePlugin_MaxNumInputChannels
//#define mNumberOfSpeakers JucePlugin_MaxNumOutputChannels
//
//#define ParamForSourceX(v) (kSourceX + v * kParamsPerSource)
//#define ParamForSourceY(v) (kSourceY + v * kParamsPerSource)
//#define ParamForSourceD(v) (kSourceD + v * kParamsPerSource)
//
//#define ParamForSpeakerX(v) (kSpeakerX + mNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers)
//#define ParamForSpeakerY(v) (kSpeakerY + mNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers)
//#define ParamForSpeakerA(v) (kSpeakerA + mNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers)
//#define ParamForSpeakerM(v) (kSpeakerM + mNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers)
//
//#define kConstantOffset (mNumberOfSources * kParamsPerSource + mNumberOfSpeakers * kParamsPerSpeakers)
#define kConstantOffset (JucePlugin_MaxNumInputChannels * kParamsPerSource + JucePlugin_MaxNumOutputChannels * kParamsPerSpeakers)
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
	kConstantParameters =	8
};

#define kNumberOfParameters (kConstantParameters + kConstantOffset)



enum InputOutputModes {
    i1o2 = 0, i1o4, i1o6, i1o8, i1o16, i2o2, i2o4, i2o6, i2o8, i2o16, i4o4, i4o6, i4o8, i4o16, i6o6, i6o8, i6o16, i8o8, i8o16
};



enum
{
	kFreeVolumeMode = 0,
	kPanVolumeMode = 1,
	kNumberOfModes = 2
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

static const float kRadiusMax = 2;
static const float kHalfCircle = M_PI;
static const float kQuarterCircle = M_PI / 2;
static const float kThetaMax = M_PI * 2;
static const float kThetaLockRadius = 0.05;
static const float kThetaLockRampRadius = 0.025;
static const float kSourceDefaultRadius = 1.005f;

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


    
//    //const int kConstantOffset = mNumberOfSources * kParamsPerSource + mNumberOfSpeakers * kParamsPerSpeakers;
//    const int kConstantOffset = JucePlugin_MaxNumInputChannels * kParamsPerSource + JucePlugin_MaxNumOutputChannels * kParamsPerSpeakers;
//    
//    const int kLinkMovement =		0 + kConstantOffset;
//    const int kSmooth =				1 + kConstantOffset;
//    const int kVolumeNear =			2 + kConstantOffset;
//    const int kVolumeMid =			3 + kConstantOffset;
//    const int kVolumeFar =			4 + kConstantOffset;
//    const int kFilterNear =			5 + kConstantOffset;
//    const int kFilterMid =			6 + kConstantOffset;
//    const int kFilterFar =			7 + kConstantOffset;
//    const int kConstantParameters =	8;
//    
//    const int kNumberOfParameters = kConstantParameters + kConstantOffset;
    


    
    
	int getNumberOfSources() const { return mNumberOfSources; }
	int getParamForSourceX(int index) const { return kSourceX + index * kParamsPerSource; }
	int getParamForSourceY(int index) const { return kSourceY + index * kParamsPerSource; }
	int getParamForSourceD(int index) const { return kSourceD + index * kParamsPerSource; }
	float getSourceX(int index) const { return mParameters.getUnchecked(kSourceX + index * kParamsPerSource); }
	float getSourceY(int index) const { return mParameters.getUnchecked(kSourceY + index * kParamsPerSource); }
	float getSourceD(int index) const { return mParameters.getUnchecked(kSourceD + index * kParamsPerSource); }
	float getDenormedSourceD(int index) const { return denormalize(kSourceMinDistance, kSourceMaxDistance, getSourceD(index)); }
	
	int getNumberOfSpeakers() const { return mNumberOfSpeakers; }
    
    
//    inline int getParamForSpeakerX(int index) const { return kSpeakerX + mNumberOfSources * kParamsPerSource + index * kParamsPerSpeakers; }
//	inline int getParamForSpeakerY(int index) const { return kSpeakerY + mNumberOfSources * kParamsPerSource + index * kParamsPerSpeakers; }
//	inline int getParamForSpeakerA(int index) const { return kSpeakerA + mNumberOfSources * kParamsPerSource + index * kParamsPerSpeakers; }
//	inline int getParamForSpeakerM(int index) const { return kSpeakerM + mNumberOfSources * kParamsPerSource + index * kParamsPerSpeakers; }
    inline int getParamForSpeakerX(int index) const { return kSpeakerX + JucePlugin_MaxNumInputChannels * kParamsPerSource + index * kParamsPerSpeakers; }
    inline int getParamForSpeakerY(int index) const { return kSpeakerY + JucePlugin_MaxNumInputChannels * kParamsPerSource + index * kParamsPerSpeakers; }
    inline int getParamForSpeakerA(int index) const { return kSpeakerA + JucePlugin_MaxNumInputChannels * kParamsPerSource + index * kParamsPerSpeakers; }
    inline int getParamForSpeakerM(int index) const { return kSpeakerM + JucePlugin_MaxNumInputChannels * kParamsPerSource + index * kParamsPerSpeakers; }

    
    
    float getSpeakerX(int index) const { return mParameters.getUnchecked(getParamForSpeakerX(index)); }
	float getSpeakerY(int index) const { return mParameters.getUnchecked(getParamForSpeakerY(index)); }
	float getSpeakerA(int index) const { return mParameters.getUnchecked(getParamForSpeakerA(index)); }
	float getSpeakerM(int index) const { return mParameters.getUnchecked(getParamForSpeakerM(index)); }
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
    
    int getInputOutputMode() const {return mInputOutputMode;}
    void setInputOutputMode(int i);

    int getSrcPlacementMode() const {return mSrcPlacementMode;}
    void setSrcPlacementMode(int i);

    int getSpPlacementMode() const {return mSpPlacementMode;}
    void setSpPlacementMode(int i);
    
    int getSrcSelected() const {return mSrcSelected;}
    void setSrcSelected(int i);
    
    int getSpSelected() const {return mSpSelected;}
    void setSpSelected(int i);
	
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
    
    bool getIsSourcesChanged(){ return mIsNumberSourcesChanged;}
    bool getIsSpeakersChanged(){ return mIsNumberSpeakersChanged;}
    
    void setIsSourcesChanged(bool pIsNumberSourcesChanged){ mIsNumberSourcesChanged = pIsNumberSourcesChanged;}
    void setIsSpeakersChanged(bool pIsNumberSpeakersChanged){ mIsNumberSpeakersChanged = pIsNumberSpeakersChanged;}
	
private:
    PluginHostType mHost;
	Trajectory::Ptr mTrajectory;

	Array<float> mParameters;
	
	int mCalculateLevels;
	Array<float> mLevels;
	
	bool mApplyFilter;
	bool mLinkDistances;
	int mMovementMode;
	bool mShowGridLines;
	int mGuiSize;
    int mInputOutputMode;
    int mSrcPlacementMode;
    int mSrcSelected;
    int mSpPlacementMode;
    int mSpSelected;
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
    
    bool mIsNumberSourcesChanged;
    bool mIsNumberSpeakersChanged;
	
	bool mSmoothedParametersInited;
	Array<float> mSmoothedParameters;
	
	Array<float> mLockedThetas;
	
	#define kChunkSize (256)
	struct IOBuf { float b[kChunkSize]; };
	Array<IOBuf> mInputsCopy;
	Array<IOBuf> mSmoothedParametersRamps;
    
    
    ///////////////////////////
    int mNumberOfSources;   //JucePlugin_MaxNumInputChannels;
    int mNumberOfSpeakers;  //JucePlugin_MaxNumOutputChannels;
    
    void setNumberOfSources(int p_iNewNumberOfSources);
    void setNumberOfSpeakers(int p_iNewNumberOfSpeakers);
    
    //int inline ParamForSourceX(int v) {return kSourceX + v * kParamsPerSource;}
    //int inline ParamForSourceY(int v) {return kSourceY + v * kParamsPerSource;}
    //int inline ParamForSourceD(int v) {return kSourceD + v * kParamsPerSource;}
    
//    int inline ParamForSpeakerX(int v) {return kSpeakerX + mNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers;}
//    int inline ParamForSpeakerY(int v) {return kSpeakerY + mNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers;}
//    int inline ParamForSpeakerA(int v) {return kSpeakerA + mNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers;}
//    int inline ParamForSpeakerM(int v) {return kSpeakerM + mNumberOfSources * kParamsPerSource + v * kParamsPerSpeakers;}
    

    
    
    ///////////////////////////
    
//	FirFilter* mFilters;
    //std::unique_ptr<FirFilter[]> mFilters;
    std::vector<FirFilter> mFilters;

	
	void findSpeakers(float t, float *params, int &left, int &right, float &dLeft, float &dRight);
	void addToOutput(float s, float **outputs, int o, int f);
	void ProcessData(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataFreeVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataPanVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctogrisAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
