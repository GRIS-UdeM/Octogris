/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginProcessor.h
 
 Developers: Antoine Missout, Vincent Berthiaume
 
 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.
 
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.
 
 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ==============================================================================
 */

#ifndef USE_LEAP
#define USE_LEAP 1
#endif

#if !WIN32
#ifndef USE_JOYSTICK
#define USE_JOYSTICK 1
#endif
#endif

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
#include "Routing.h"

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
    kParamsPerSpeakers
};

enum
{
    kTrReady,
    kTrWriting
};

enum AllTrajectoryTypes {
    Circle = 1,
    EllipseTr, //Ellipse was clashing with some random windows class...
    Spiral,
    Pendulum,
    RandomTrajectory,
    RandomTarget,
    SymXTarget,
    SymYTarget,
    ClosestSpeakerTarget,
    TotalNumberTrajectories
};

#define kConstantOffset (JucePlugin_MaxNumInputChannels * kParamsPerSource + JucePlugin_MaxNumOutputChannels * kParamsPerSpeakers)

enum
{
	kSmooth =				0 + kConstantOffset,
	kVolumeNear =			1 + kConstantOffset,
	kVolumeMid =			2 + kConstantOffset,
	kVolumeFar =			3 + kConstantOffset,
	kFilterNear =			4 + kConstantOffset,
	kFilterMid =			5 + kConstantOffset,
	kFilterFar =			6 + kConstantOffset,
	kMaxSpanVolume =		7 + kConstantOffset,
	kRoutingVolume =		8 + kConstantOffset,
	kConstantParameters =	9
};

#define kNumberOfParameters (kConstantParameters + kConstantOffset)

static const int s_iMaxAreas = 3; //this number is used as a multiplicator of mNumberOfSpeakers
static const bool s_bUseNewGui = false;

//these have to start at 0 because of backwards-compatibility
enum InputOutputModes {
    i1o2 = 0, i1o4, i1o6, i1o8, i1o16, i2o2, i2o4, i2o6, i2o8, i2o16, i4o4, i4o6, i4o8, i4o16, i6o6, i6o8, i6o16, i8o8, i8o16
};

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

static const float kVolumeNearMin = -10;
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
static const float kMaxSpanVolumeDefault = 0;

static const float kRoutingVolumeMin = -120;
static const float kRoutingVolumeMax = 6;
static const float kRoutingVolumeDefault = 0;

static const float kRadiusMax = 2;
static const float kHalfCircle = M_PI;
static const float kQuarterCircle = M_PI / 2;
static const float kThetaMax = M_PI * 2;
static const float kThetaLockRadius = 0.05;
static const float kThetaLockRampRadius = 0.025;
static const float kSourceDefaultRadius = 1.f;

static const int    kMargin = 10;
static const int    kCenterColumnWidth = 180;
static const int    kDefaultFieldSize = 500;
static const int    kRightColumnWidth = 340;
static const int    kDefaultWidth  = kMargin + kDefaultFieldSize + kMargin + kCenterColumnWidth + kMargin + kRightColumnWidth + kMargin;
static const int    kDefaultHeight = kMargin + kDefaultFieldSize + kMargin;

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

static bool areSame(double a, double b)
{
    return fabs(a - b) < .0001;
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
	int getNumberOfSources() const { return mNumberOfSources; }
	int getParamForSourceX(int index) const { return kSourceX + index * kParamsPerSource; }
	int getParamForSourceY(int index) const { return kSourceY + index * kParamsPerSource; }
	int getParamForSourceD(int index) const { return kSourceD + index * kParamsPerSource; }
	
	float getSourceX(int index) const { return mParameters.getUnchecked(kSourceX + index * kParamsPerSource); }
	float getSourceY(int index) const { return mParameters.getUnchecked(kSourceY + index * kParamsPerSource); }
	float getSourceD(int index) const { return mParameters.getUnchecked(kSourceD + index * kParamsPerSource); }
	float getDenormedSourceD(int index) const { return denormalize(kSourceMinDistance, kSourceMaxDistance, getSourceD(index)); }
	
	int getNumberOfSpeakers() const { return mNumberOfSpeakers; }
    
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
    
    bool getIndependentMode() const { return mTrSeparateAutomationMode; }
    void setIndependentMode(bool b) { mTrSeparateAutomationMode = b; }
    
	int getMovementMode() const { return mMovementMode; }
	void setMovementMode(int s) { mMovementMode = s; }
	
	bool getLinkDistances() const { return mLinkDistances; }
	void setLinkDistances(bool s) { mLinkDistances = s; }
		
	int getProcessMode() const { return mProcessMode; }
	void setProcessMode(int s) { mProcessMode = s; jassert(mProcessMode >= 0 && mProcessMode < kNumberOfModes); }
	
    
    void setJustSelectedEndPoint(bool selected){ m_bJustSelectedEndPoint = selected;}
    bool justSelectedEndPoint(){ return m_bJustSelectedEndPoint;}
    
	int getRoutingMode() const { return mRoutingMode; }
	void setRoutingMode(int s) {
        mRoutingMode = s;
        if (mRoutingMode == 1){
            updateRoutingTemp();
        }
    }
	void updateRoutingTemp();
    
    int getGuiWidth() const{return mGuiWidth;}
    int getGuiHeight() const{return mGuiHeight;}
    
    void setGuiWidth(int w) {mGuiWidth = w;}
    void setGuiHeight(int h) {mGuiHeight = h;}

    int getInputOutputMode() const {return mInputOutputMode+1;}
    void setInputOutputMode(int i);
    void updateInputOutputMode();
    
    int getSrcPlacementMode() const {return mSrcPlacementMode;}
    void setSrcPlacementMode(int i);

    int getSpPlacementMode() const {return mSpPlacementMode;}
    void setSpPlacementMode(int i);

    int getTrType() const {return m_iTrType+1;}
    void setTrType(int i){m_iTrType = i-1;}
    
    int getTrDirection() const {return m_iTrDirection;}
    void setTrDirection(int i){m_iTrDirection = i;}
    
    int getTrReturn() const {return m_iTrReturn;}
    void setTrReturn(int i){m_iTrReturn = i;}

    int getTrSrcSelect() const {return m_iTrSrcSelect;}
    void setTrSrcSelect(int i){m_iTrSrcSelect = i;}

    float getTrDuration() const {return m_fTrDuration;}
    void setTrDuration(float i){m_fTrDuration = i;}
    
    int getTrUnits() const {return m_iTrUnits + 1;}
    void setTrUnits(int i){m_iTrUnits = i - 1;}

    float getTrRepeats() const {return m_fTrRepeats;}
    void setTrRepeats(float i){m_fTrRepeats = i;}

    float getTrDampening() const {return m_fTrDampening;}
    void setTrDampening(float i){m_fTrDampening = i;}

    float getTrDeviation() const {return m_fTrDeviation;}
    void setTrDeviation(float i){m_fTrDeviation = i;}

    float getTrTurns() const {return m_fTrTurns;}
    void setTrTurns(float i){m_fTrTurns = i;}
    
    int getTrState() const {return mTrState;}
    void setTrState(int tr) {mTrState = tr;}
    
	int getGuiTab() const { return mGuiTab; }
	void setGuiTab(int s) {
        mGuiTab = s;
        ++mHostChangedParameter;
    }
    
    int getIsJoystickEnabled() const { return mJoystickEnabled; }
    void setIsJoystickEnabled(int s) { mJoystickEnabled = s; }
    
    int getOscJoystickSource() const { return mOscJoystickSource; }
    void setOscJoystickSource(int s) { mOscJoystickSource = s; }
    
    int getIsLeapEnabled() const { return mLeapEnabled; }
    void setIsLeapEnabled(int s) { mLeapEnabled = s; }
	
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
    
	const String getOscSendIp() const { return mOscSendIp; }
	void setOscSendIp(String s) { mOscSendIp = s;}
	
	float getLevel(int index) const { return mLevels.getUnchecked(index); }
	void setCalculateLevels(bool c);
	
	bool getIsAllowInputOutputModeSelection(){
		return m_bAllowInputOutputModeSelection;
	}

	uint64_t getHostChangedParameter() { return mHostChangedParameter; }
	uint64_t getHostChangedProperty() { return mHostChangedProperty; }
	uint64_t getProcessCounter() { return mProcessCounter; }
	
	// convenience functions for gui:
	//01 here means that the output is normalized to [0,1]
	FPoint getSourceXY01(int i)	{
		float x = getSourceX(i);
		float y = getSourceY(i);
		return FPoint(x, y);
	}

	// these return in the interval [-kRadiusMax .. kRadiusMax]
	FPoint getSourceXY(int i) {
		float x = getSourceX(i) * (2*kRadiusMax) - kRadiusMax;
		float y = getSourceY(i) * (2*kRadiusMax) - kRadiusMax;
		return FPoint(x, y);
	}

	FPoint getSpeakerXY(int i) {
		float x = getSpeakerX(i) * (2*kRadiusMax) - kRadiusMax;
		float y = getSpeakerY(i) * (2*kRadiusMax) - kRadiusMax;
		if (mProcessMode != kFreeVolumeMode) {
			// force radius to 1
			float r = hypotf(x, y);
			if (r == 0) return FPoint(1, 0);
			x /= r;
			y /= r;
		}
		return FPoint(x, y);
	}

	FPoint getSpeakerRT(int i) {
		FPoint p = getSpeakerXY(i);
		float r = hypotf(p.x, p.y);
		float t = atan2f(p.y, p.x);
		if (t < 0) t += kThetaMax;
		return FPoint(r, t);
	}

	FPoint getSourceRT(int i) {
		FPoint p = getSourceXY(i);
		float r = hypotf(p.x, p.y);
		float t = atan2f(p.y, p.x);
		if (t < 0) t += kThetaMax;
		return FPoint(r, t);
	}
    
    FPoint convertRt2Xy(FPoint p) {
        float x = p.x * cosf(p.y);
        float y = p.x * sinf(p.y);
        return FPoint(x, y);
    }
    
    
    //01 here means that the output is normalized to [0,1]
    FPoint convertRt2Xy01(float r, float t) {
        float x = r * cosf(t);
        float y = r * sinf(t);
        return FPoint((x + kRadiusMax)/(kRadiusMax*2), (y + kRadiusMax)/(kRadiusMax*2));
    }
	
	FPoint convertXy2Rt01(FPoint p) {
		float vx = p.x;
		float vy = p.y;
		float r = sqrtf(vx*vx + vy*vy) / kRadiusMax;
		if (r > 1) r = 1;
		float t = atan2f(vy, vx);
		if (t < 0) t += kThetaMax;
		t /= kThetaMax;
		return FPoint(r, t);
	}

    FPoint convertXy012Rt01(FPoint p) {
        return convertXy2Rt01(FPoint(p.x * (kRadiusMax*2) - kRadiusMax, p.y * (kRadiusMax*2) - kRadiusMax));
    }
    
    FPoint convertXy2Rt(FPoint p, bool p_bLimitR = true) {
        float vx = p.x;
        float vy = p.y;
        float r = sqrtf(vx*vx + vy*vy);
        if (p_bLimitR && r > 1) r = 1;
        float t = atan2f(vy, vx);
        if (t < 0) t += kThetaMax;
        return FPoint(r, t);
    }
    
    FPoint convertXy012Rt(FPoint p, bool p_bLimitR = true) {
        return convertXy2Rt(FPoint(p.x * (kRadiusMax*2) - kRadiusMax, p.y * (kRadiusMax*2) - kRadiusMax), p_bLimitR);
    }

	FPoint clampRadius01(FPoint p) {
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

	void setSourceXY01(int i, FPoint p, bool p_bNotifyHost = true) {
		p = clampRadius01(p);
        if (p_bNotifyHost){
            setParameterNotifyingHost(getParamForSourceX(i), p.x);
            setParameterNotifyingHost(getParamForSourceY(i), p.y);
        } else {
            setParameter(getParamForSourceX(i), p.x);
            setParameter(getParamForSourceY(i), p.y);
        }
	}

	void setSourceXY(int i, FPoint p, bool p_bNotifyHost = true) {
		float r = hypotf(p.x, p.y);
		if (r > kRadiusMax)
		{
			float c = kRadiusMax / r;
			p.x *= c;
			p.y *= c;
		}
		p.x = (p.x + kRadiusMax) / (kRadiusMax*2);
		p.y = (p.y + kRadiusMax) / (kRadiusMax*2);
        if (p_bNotifyHost){
            setParameterNotifyingHost(getParamForSourceX(i), p.x);
            setParameterNotifyingHost(getParamForSourceY(i), p.y);
        } else {
            setParameter(getParamForSourceX(i), p.x);
            setParameter(getParamForSourceY(i), p.y);
        }
	}

	void setSourceRT(int i, FPoint p, bool p_bNotifyHost = true) {
		float x = p.x * cosf(p.y);
		float y = p.x * sinf(p.y);
		setSourceXY(i, FPoint(x, y), p_bNotifyHost);
	}
 
	void setSpeakerXY01(int i, FPoint p) {
		p = clampRadius01(p);
        setParameter(getParamForSpeakerX(i), p.x);
        setParameter(getParamForSpeakerY(i), p.y);
	}

	void setSpeakerRT(int i, FPoint p) {
		float x = p.x * cosf(p.y);
		float y = p.x * sinf(p.y);
        setParameter(getParamForSpeakerX(i), (x + kRadiusMax) / (kRadiusMax*2));
        setParameter(getParamForSpeakerY(i), (y + kRadiusMax) / (kRadiusMax*2));
    }
	
	Trajectory::Ptr getTrajectory() { return mTrajectory; }
	void setTrajectory(Trajectory::Ptr t) { mTrajectory = t; }
    
    bool getIsSourcesChanged(){ return mIsNumberSourcesChanged;}
    bool getIsSpeakersChanged(){ return mIsNumberSpeakersChanged;}
    
    void setIsSourcesChanged(bool pIsNumberSourcesChanged){ mIsNumberSourcesChanged = pIsNumberSourcesChanged;}
    void setIsSpeakersChanged(bool pIsNumberSpeakersChanged){ mIsNumberSpeakersChanged = pIsNumberSpeakersChanged;}
    
    void setIsRecordingAutomation(bool b)   {
        m_bIsRecordingAutomation = b;
    }
    bool getIsRecordingAutomation()         { return m_bIsRecordingAutomation;  }

    void setSourceLocationChanged(int i)   {
        m_iSourceLocationChanged = i;
    }
    int  getSourceLocationChanged()        { return m_iSourceLocationChanged;  }

    int getSrcSelected() const {return mSrcSelected;}
    int getSpSelected() const  {return mSpSelected;}
    
    void setSrcSelected(int p_i){
    	mSrcSelected = p_i;
        mHostChangedParameter++;
	}

	void setSpSelected(int p_i){ mSpSelected = p_i; }

    void setPreventSourceLocationUpdate(bool b){ m_bPreventSourceLocationUpdate = b; }
    
    void setIsSettingEndPoint(bool isSetting){ m_bIsSettingEndPoint = isSetting; }
    bool isSettingEndPoint(){ return m_bIsSettingEndPoint; }
    
    std::pair<float, float> getEndLocationXY(){ return m_fEndLocationXY; }
    void setEndLocationXY(std::pair<float, float> pair){ m_fEndLocationXY = pair; }
    
    void storeCurrentLocations();
    void restoreCurrentLocations(int p_iLocToRestore = -1);
	void reset();
    
    void updateSpeakerLocation(bool p_bAlternate, bool p_bStartAtTop, bool p_bClockwise);
    
    FPoint  getOldSrcLocRT(int id){return mOldSrcLocRT[id];}
    void    setOldSrcLocRT(int id, FPoint pointRT){
        mOldSrcLocRT[id] = pointRT;
    }

    
	
private:

	bool m_bAllowInputOutputModeSelection;
	Trajectory::Ptr mTrajectory;

    FPoint mOldSrcLocRT[JucePlugin_MaxNumInputChannels];

    
	Array<float> mParameters;
	
	int mCalculateLevels;
	Array<float> mLevels;
	
	bool mApplyFilter;
	bool mLinkDistances;
	int mMovementMode;
	bool mShowGridLines;
    bool mTrSeparateAutomationMode;
    int mGuiWidth;
    int mGuiHeight;

    //version 9
    int mInputOutputMode;
    int mSrcPlacementMode;
    int mSrcSelected;
    int mSpPlacementMode;
    int mSpSelected;
    int m_iTrType;
    
    int m_iTrDirection;
    int m_iTrReturn;
    
    int m_iTrSrcSelect;
    float m_fTrDuration;
    int m_iTrUnits; //0 = beats, 1 = seconds
    float m_fTrRepeats;
    float m_fTrDampening;
    float m_fTrTurns;
    float m_fTrDeviation;
    int mTrState;

    int mGuiTab;
    int mJoystickEnabled;
    int mOscJoystickSource;
    int mLeapEnabled;
	int mOscLeapSource;
	int mOscReceiveEnabled;
	int mOscReceivePort;
	int mOscSendEnabled;
	int mOscSendPort;
    String mOscSendIp;
	//char mOscSendIp[64]; // if changing size, change kDataVersion
	uint64_t mHostChangedParameter;
	uint64_t mHostChangedProperty;
	uint64_t mProcessCounter;
	//int64 mLastTimeInSamples;
	
	int mProcessMode;
	int mRoutingMode;
	AudioSampleBuffer mRoutingTemp;
    
    bool mIsNumberSourcesChanged;
    bool mIsNumberSpeakersChanged;
	
	bool mSmoothedParametersInited;
	Array<float> mSmoothedParameters;
	
	Array<float> mLockedThetas;
	
	#define kChunkSize (256)
	struct IOBuf { float b[kChunkSize]; };
	Array<IOBuf> mInputsCopy;
	Array<IOBuf> mSmoothedParametersRamps;
    
    float mBufferSrcLocX[JucePlugin_MaxNumInputChannels];
    float mBufferSrcLocY[JucePlugin_MaxNumInputChannels];
    float mBufferSrcLocD[JucePlugin_MaxNumInputChannels];
    
    float mBufferSpLocX[JucePlugin_MaxNumOutputChannels];
    float mBufferSpLocY[JucePlugin_MaxNumOutputChannels];
    float mBufferSpLocA[JucePlugin_MaxNumOutputChannels];
    float mBufferSpLocM[JucePlugin_MaxNumOutputChannels];
    
    void setNumberOfSources(int p_iNewNumberOfSources, bool bUseDefaultValues);
    void setNumberOfSpeakers(int p_iNewNumberOfSpeakers, bool bUseDefaultValues);
	
	void findLeftAndRightSpeakers(float t, float *params, int &left, int &right, float &dLeft, float &dRight, int skip = -1);
    
	void addToOutput(float s, float **outputs, int o, int f);
	void ProcessData(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataFreeVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataPanVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);
	void ProcessDataPanSpanMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames);

    int mNumberOfSources;
    int mNumberOfSpeakers;
    std::vector<FirFilter> mFilters;
    bool m_bIsRecordingAutomation;
    int m_iSourceLocationChanged;
    
    bool m_bPreventSourceLocationUpdate;
    bool m_bIsSettingEndPoint;
    std::pair <float, float> m_fEndLocationXY;
    bool m_bJustSelectedEndPoint;
    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctogrisAudioProcessor)
};

#endif  // PLUGINPROCESSOR_H_INCLUDED
