/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
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

//static const float kLevelAttackMin = 0.01;
//static const float kLevelAttackMax = 100;
static const float kLevelAttackDefault = 0.05f;

//static const float kLevelReleaseMin = 1;
//static const float kLevelReleaseMax = 500;
static const float kLevelReleaseDefault = 100;

#include "PluginProcessor.h"
#include "PluginEditor.h"

#include <algorithm>

#if JUCE_MSVC
// from https://github.com/objectx/strlcpy/blob/master/strlcpy/strlcpy.c
size_t strlcpy(char * dst, const char * src, size_t dstsize)
{
    if (dst == 0 || dstsize == 0) {
		return 0;
    }
    if (src == 0) {
		dst [0] = 0;
		return 0;
    }
    else {
        size_t	src_len = strlen (src);
        size_t	dstlimit = dstsize - 1;

        if (dstlimit < src_len) {
	    src_len = dstlimit;
        }
        memcpy (dst, src, src_len);
        dst [src_len] = 0;
        return src_len;
    }
}
#endif


//==============================================================================
int IndexedAngleCompare(const void *a, const void *b)
{
	IndexedAngle *ia = (IndexedAngle*)a;
	IndexedAngle *ib = (IndexedAngle*)b;
	return (ia->a < ib->a) ? -1 : ((ia->a > ib->a) ? 1 : 0);
}

//==============================================================================
OctogrisAudioProcessor::OctogrisAudioProcessor():mFilters()
{
    
    //SET PARAMETERS
	mParameters.ensureStorageAllocated(kNumberOfParameters);
    for (int i = 0; i < kNumberOfParameters; i++){
        mParameters.add(0);
    }
    
	mParameters.set(kLinkMovement, 0);
	mParameters.set(kSmooth, normalize(kSmoothMin, kSmoothMax, kSmoothDefault));
	mParameters.set(kVolumeNear, normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault));
	mParameters.set(kVolumeMid, normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault));
	mParameters.set(kVolumeFar, normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault));
	mParameters.set(kFilterNear, normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault));
	mParameters.set(kFilterMid, normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault));
	mParameters.set(kFilterFar, normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault));
	mParameters.set(kMaxSpanVolume, normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault));

	mSmoothedParametersInited = false;
	mSmoothedParameters.ensureStorageAllocated(kNumberOfParameters);
    
	for (int i = 0; i < kNumberOfParameters; i++) mSmoothedParameters.add(0);
    
    mNumberOfSources = -1;
    mNumberOfSpeakers = -1;
    
    //SET SOURCES
    setNumberOfSources(JucePlugin_MaxNumInputChannels, true);
    
    //SET SPEAKERS
    setNumberOfSpeakers(JucePlugin_MaxNumOutputChannels, true);
    
	mCalculateLevels = 0;
	mApplyFilter = true;
	mLinkDistances = false;
	mMovementMode = 0;
	mShowGridLines = false;
    mIsNumberSourcesChanged = false;
    mIsNumberSpeakersChanged = false;
	mGuiSize = 1;
	mGuiTab = 0;
	mHostChangedParameter = 0;
	mHostChangedProperty = 0;
	mProcessCounter = 0;
	mLastTimeInSamples = -1;
	mProcessMode = kPanVolumeMode;
    //version 9
    mInputOutputMode = 18;  //by default we have 8 inputs and 16 outputs
    mSrcPlacementMode = 1;
    mSrcSelected = 1;
    mSpPlacementMode = 1;
    mSpSelected = 1;
    m_iTrType = 0;
    m_iTrSrcSelect = -1;//0;
    m_fTrDuration = 1.f;
    m_iTrUnits = 1;
    m_fTrRepeats = 1.f;
	
	mOscLeapSource = 0;
	mOscReceiveEnabled = 0;
	mOscReceivePort = 8000;
	mOscSendEnabled = 0;
	mOscSendPort = 9000;
	setOscSendIp("192.168.1.100");
	

	mSmoothedParametersRamps.resize(kNumberOfParameters);
	
	// default values for parameters
    //whatever the actual mNumberOfSources, mParameters always has the same number of parameters, so might as well initialize
    //everything (JucePlugin_MaxNumInputChannels) instead of just mNumberOfSources
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++){   	//for (int i = 0; i < mNumberOfSources; i++){
        float fDefaultVal = normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance);
        mParameters.set(getParamForSourceD(i), fDefaultVal);
    }

    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++){   	//for (int i = 0; i < mNumberOfSources; i++){
        mParameters.set(getParamForSpeakerA(i), normalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, kSpeakerDefaultAttenuation));
        mParameters.set(getParamForSpeakerM(i), 0);
    }
    

}

OctogrisAudioProcessor::~OctogrisAudioProcessor()
{
    //delete[] mFilters;
}


//==============================================================================
void OctogrisAudioProcessor::setCalculateLevels(bool c)
{
	if (!mCalculateLevels && c)
		for (int i = 0; i < mNumberOfSpeakers; i++)
			mLevels.setUnchecked(i, 0);

	// keep count of number of editors
	if (c) mCalculateLevels++;
	else mCalculateLevels--;
	
}

//==============================================================================
const String OctogrisAudioProcessor::getName() const
{
	String name(JucePlugin_Name);
	name << " ";
	name << mNumberOfSources;
	name << "x";
	name << mNumberOfSpeakers;
    return name;
}

int OctogrisAudioProcessor::getNumParameters()
{
    return kNumberOfParameters;
}

float OctogrisAudioProcessor::getParameter (int index)
{
    return mParameters[index];
}

void OctogrisAudioProcessor::setParameter (int index, float newValue)
{
	mParameters.set(index, newValue);
	mHostChangedParameter++;
}

void OctogrisAudioProcessor::setParameterNotifyingHost (int index, float newValue)
{
	mParameters.set(index, newValue);
    if (index == getParamForSourceX(0)){
        cout << "bill";
    }
    sendParamChangeMessageToListeners(index, newValue);
}

const String OctogrisAudioProcessor::getParameterName (int index)
{
   
    if (index == kLinkMovement) return "Link Movement";
	if (index == kSmooth)		return "Smooth Param";
    if (index ==  kVolumeNear)	return "Volume Near";
	if (index ==  kVolumeMid)	return "Volume Mid";
    if (index ==  kVolumeFar)	return "Volume Far";
	if (index ==  kFilterNear)	return "Filter Near";
	if (index ==  kFilterMid)	return "Filter Mid";
	if (index ==  kFilterFar)	return "Filter Far";
	if (index == kMaxSpanVolume)return "Max span volume";
	
    if (index < mNumberOfSources * kParamsPerSource)
	{
		String s("Source ");
		s << (index / kParamsPerSource + 1);
		switch(index % kParamsPerSource)
		{
			case kSourceX: s << " - X"; break;
			case kSourceY: s << " - Y"; break;
			case kSourceD: s << " - D"; break; 
//			case kSourceUnused: s << " - Unused"; break;
            default: return String::empty;

		}
		return s;
	}
	index -= mNumberOfSources * kParamsPerSource;
	
    if (index < mNumberOfSpeakers * kParamsPerSpeakers)
	{
		String s("Speaker ");
		s << (index / kParamsPerSpeakers + 1);
		switch(index % kParamsPerSpeakers)
		{
//			case kSpeakerX: s << " - X"; break;
//			case kSpeakerY: s << " - Y"; break;
//			case kSpeakerA: s << " - A"; break;
//			case kSpeakerM: s << " - M"; break;
//			case kSpeakerUnused: s << " - Unused"; break;
            default: return String::empty;
		}
		return s;
	}
	
    return String::empty;
}

void OctogrisAudioProcessor::setInputOutputMode (int p_iInputOutputMode){
    
    mInputOutputMode = p_iInputOutputMode;
    
    switch (p_iInputOutputMode){
            
        case i1o2:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(2, false);
            break;
        case i1o4:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(4, false);
            break;
        case i1o6:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(6, false);
            break;
        case i1o8:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(8, false);
            break;
        case i1o16:
            setNumberOfSources(1, false);
            setNumberOfSpeakers(16, false);
            break;
        case i2o2:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(2, false);
            break;
        case i2o4:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(4, false);
            break;
        case i2o6:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(6, false);
            break;
        case i2o8:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(8, false);
            break;
        case i2o16:
            setNumberOfSources(2, false);
            setNumberOfSpeakers(16, false);
            break;
        case i4o4:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(4, false);
            break;
        case i4o6:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(6, false);
            break;
        case i4o8:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(8, false);
            break;
        case i4o16:
            setNumberOfSources(4, false);
            setNumberOfSpeakers(16, false);
            break;
        case i6o6:
            setNumberOfSources(6, false);
            setNumberOfSpeakers(6, false);
            break;
        case i6o8:
            setNumberOfSources(6, false);
            setNumberOfSpeakers(8, false);
            break;
        case i6o16:
            setNumberOfSources(6, false);
            setNumberOfSpeakers(16, false);
            break;
        case i8o8:
            setNumberOfSources(8, false);
            setNumberOfSpeakers(8, false);
            break;
        case i8o16:
            setNumberOfSources(8, false);
            setNumberOfSpeakers(16, false);
            break;
    }
}

void OctogrisAudioProcessor::setSrcPlacementMode(int p_i){
    mSrcPlacementMode = p_i;
}

void OctogrisAudioProcessor::setSpPlacementMode(int p_i){
    mSpPlacementMode = p_i;
}

void OctogrisAudioProcessor::setSrcSelected(int p_i){
    mSrcSelected = p_i;
}

void OctogrisAudioProcessor::setSpSelected(int p_i){
    mSpSelected = p_i;
}

void OctogrisAudioProcessor::setNumberOfSources(int p_iNewNumberOfSources, bool bUseDefaultValues){
    
    //if new number of sources is same as before, return
    if (p_iNewNumberOfSources == mNumberOfSources){
        return;
    } else {
        mIsNumberSourcesChanged = true;
    }
    
    //prevents audio process thread from running
    suspendProcessing (true);
    
    mNumberOfSources = p_iNewNumberOfSources;
  
    mFilters.clear();
    mFilters.resize(mNumberOfSources);
    
    mLockedThetas.ensureStorageAllocated(mNumberOfSources);
    for (int i = 0; i < mNumberOfSources; i++){
        mLockedThetas.add(0);
    }
    mInputsCopy.resize(mNumberOfSources);
    
//    if (mNumberOfSources == 1)
//	{
//		setSourceRT(0, FPoint(0, 0));
//	}
//    else
    if (!bUseDefaultValues){
        for (int i = 0; i < mNumberOfSources; ++i){
            setSourceRT(i, getSourceRT(i));
        }
        
    }
	else
	{
		double anglePerSource = 360 / mNumberOfSources;
		double offset, axisOffset;
        
		if(mNumberOfSources%2 == 0) //if the number of speakers is even we will assign them as stereo pairs
		{
			axisOffset = anglePerSource / 2;
			for (int i = 0; i < mNumberOfSources; i++)
			{
				if(i%2 == 0)
				{
					offset = 90 + axisOffset;
				}
				else
				{
					offset = 90 - axisOffset;
					axisOffset += anglePerSource;
				}
				if (offset < 0) offset += 360;
				else if (offset > 360) offset -= 360;
				
				setSourceRT(i, FPoint(kSourceDefaultRadius, offset/360*kThetaMax));
			}
		}
		else //odd number of speakers, assign in circular fashion
		{
			offset = (anglePerSource + 180) / 2 - anglePerSource;
			for (int i = 0; i < mNumberOfSources; i++)
			{
				if (offset < 0) offset += 360;
				else if (offset > 360) offset -= 360;
				
				setSourceRT(i, FPoint(kSourceDefaultRadius, offset/360*kThetaMax));
				offset += anglePerSource;
			}
		}
	}
    for (int i = 0; i < mNumberOfSources; i++){
		mLockedThetas.set(i, getSourceRT(i).y);
    }
    mHostChangedParameter++;
    
    //restart audio processing
    suspendProcessing (false);
}

void OctogrisAudioProcessor::setNumberOfSpeakers(int p_iNewNumberOfSpeakers, bool bUseDefaultValues){
    
    //if new number of speakers is same as before, return
    if (p_iNewNumberOfSpeakers == mNumberOfSpeakers){
        return;
    } else {
        mIsNumberSpeakersChanged = true;
    }
    
    //prevents audio process thread from running
    suspendProcessing (true);
    
    mNumberOfSpeakers = p_iNewNumberOfSpeakers;

    mLevels.ensureStorageAllocated(mNumberOfSpeakers);
    
    for (int i = 0; i < mNumberOfSpeakers; i++){
        mLevels.add(0);
        mParameters.set(getParamForSpeakerA(i), mParameters[getParamForSpeakerA(i)]);
        mParameters.set(getParamForSpeakerM(i), mParameters[getParamForSpeakerM(i)]);
    }

    if (!mHost.isReaper()){
        updateSpeakerLocation(true, false, true);
    }

    
	mHostChangedParameter++;
    
    //starts audio processing again
    suspendProcessing (false);
}

void OctogrisAudioProcessor::updateSpeakerLocation(bool p_bAlternate, bool p_bStartAtTop, bool p_bClockwise){

    float anglePerSp = kThetaMax / getNumberOfSpeakers();
    
    if (p_bAlternate)
    {
        float offset = p_bStartAtTop
        ? (p_bClockwise ? kQuarterCircle : (kQuarterCircle - anglePerSp))
        : (kQuarterCircle - anglePerSp/2);
        float start = offset;
        for (int i = p_bClockwise ? 0 : 1; i < getNumberOfSpeakers(); i += 2)
        {
            setSpeakerRT(i, FPoint(1, offset));
            offset -= anglePerSp;
        }
        
        offset = start + anglePerSp;
        for (int i = p_bClockwise ? 1 : 0; i < getNumberOfSpeakers(); i += 2)
        {
            setSpeakerRT(i, FPoint(1, offset));
            offset += anglePerSp;
        }
    }
    else
    {
        float offset = p_bStartAtTop ? kQuarterCircle : kQuarterCircle + anglePerSp/2;
        float delta = p_bClockwise ? -anglePerSp : anglePerSp;
        for (int i = 0; i < getNumberOfSpeakers(); i++)
        {
            setSpeakerRT(i, FPoint(1, offset));
            offset += delta;
        }
    }
    
}


const String OctogrisAudioProcessor::getParameterText (int index)
{
    return String::empty;
	// return String (getParameter (index), 2);
}

const String OctogrisAudioProcessor::getInputChannelName (int channelIndex) const
{
    return String(channelIndex + 1);
}

const String OctogrisAudioProcessor::getOutputChannelName (int channelIndex) const
{
    return String(channelIndex + 1);
}

bool OctogrisAudioProcessor::isInputChannelStereoPair (int index) const
{
    return index == 0 && mNumberOfSources == 2;
	//return false;
}

bool OctogrisAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return index == 0 && mNumberOfSpeakers == 2;
	//return false;
}

bool OctogrisAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool OctogrisAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool OctogrisAudioProcessor::silenceInProducesSilenceOut() const
{
    return false;
}

double OctogrisAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int OctogrisAudioProcessor::getNumPrograms()
{
    return 1;
}

int OctogrisAudioProcessor::getCurrentProgram()
{
    return 1;
}

void OctogrisAudioProcessor::setCurrentProgram (int index)
{
}

const String OctogrisAudioProcessor::getProgramName (int index)
{
    return String::empty;
}

void OctogrisAudioProcessor::changeProgramName (int index, const String& newName)
{
}

//==============================================================================
void OctogrisAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    
//    int iSources = getNumInputChannels();//mNumberOfSources;
//    int iSpeakers = getNumOutputChannels();//mNumberOfSpeakers;
//    cout << "PREPARE TO PLAY\n";
//    cout << "iSources = " << iSources << endl;
//    cout << "iSpeakers = " << iSpeakers << endl;

    //set sources and speakers
    if (mHost.isReaper()) {
        setNumberOfSources(mNumberOfSources, true);
        setNumberOfSpeakers(mNumberOfSpeakers, true);
    } else {
        int sources = getNumInputChannels();
        int speakers = getNumOutputChannels();
        setNumberOfSources(sources, true);
        setNumberOfSpeakers(speakers, true);
    }
    
	if (mCalculateLevels)
		for (int i = 0; i < mNumberOfSpeakers; i++)
			mLevels.setUnchecked(i, 0);
		
	mSmoothedParametersInited = false;
	
	int sr = sampleRate;

    for (int i = 0; i < mNumberOfSources; i++)
    {
        mFilters[i].setSampleRate(sr);
        mFilters[i].reset();
    }

}

void OctogrisAudioProcessor::releaseResources()
{

}

void OctogrisAudioProcessor::processBlockBypassed (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	//fprintf(stderr, "pb bypass\n");
	//for (int c = mNumberOfSources; c < mNumberOfSpeakers; c++)
	//	buffer.clear(c, 0, buffer.getNumSamples());
}

void OctogrisAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	
	double sampleRate = getSampleRate();
	unsigned int oriFramesToProcess = buffer.getNumSamples();
	unsigned int inFramesToProcess = oriFramesToProcess;
	
	Trajectory::Ptr trajectory = mTrajectory;
	if (trajectory)
	{
		AudioPlayHead::CurrentPositionInfo cpi;
		getPlayHead()->getCurrentPosition(cpi);
		
		if (cpi.timeInSamples != mLastTimeInSamples)
		{
			// we're playing!
			mLastTimeInSamples = cpi.timeInSamples;
	
			double bps = cpi.bpm / 60;
			float seconds = oriFramesToProcess / sampleRate;
			float beats = seconds * bps;
			
			bool done = trajectory->process(seconds, beats);
			if (done) mTrajectory = NULL;
		}
	}
	
	// cache values
	float params[kNumberOfParameters];
	for (int i = 0; i < kNumberOfParameters; i++)
		params[i] = mParameters[i];
		
	if (mProcessMode != kFreeVolumeMode)
	{
		params[kVolumeNear] = denormalize(kVolumeNearMin, kVolumeNearMax, params[kVolumeNear]);
		params[kVolumeMid] = denormalize(kVolumeMidMin, kVolumeMidMax, params[kVolumeMid]);
		params[kVolumeFar] = denormalize(kVolumeFarMin, kVolumeFarMax, params[kVolumeFar]);
		params[kFilterNear] = denormalize(kFilterNearMin, kFilterNearMax, params[kFilterNear]);
		params[kFilterMid] = denormalize(kFilterMidMin, kFilterMidMax, params[kFilterMid]);
		params[kFilterFar] = denormalize(kFilterFarMin, kFilterFarMax, params[kFilterFar]);
	}
	if (mProcessMode == kPanSpanMode)
	{
		params[kMaxSpanVolume] = denormalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, params[kMaxSpanVolume]);
	}
	
	//float *inputs[iActualNumberOfSources];
	float **inputs = new float* [mNumberOfSources];
	for (int i = 0; i < mNumberOfSources; i++)
	{
		inputs[i] = buffer.getWritePointer(i);

        if (mProcessMode == kFreeVolumeMode){
            float fValue = denormalize(kSourceMinDistance, kSourceMaxDistance, params[getParamForSourceD(i)]);
            params[getParamForSourceD(i)] = fValue;
        }
		params[getParamForSourceX(i)] = params[getParamForSourceX(i)] * (2*kRadiusMax) - kRadiusMax;
		params[getParamForSourceY(i)] = params[getParamForSourceY(i)] * (2*kRadiusMax) - kRadiusMax;
	}
	
	//float *outputs[iActualNumberOfSpeakers];
	float **outputs = new float*[mNumberOfSpeakers];
	for (int o = 0; o < mNumberOfSpeakers; o++)
	{
		outputs[o] = buffer.getWritePointer(o);
		params[getParamForSpeakerA(o)] = denormalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, params[getParamForSpeakerA(o)]);
		
		if (mProcessMode == kFreeVolumeMode)
		{
			params[getParamForSpeakerX(o)] = params[getParamForSpeakerX(o)] * (2*kRadiusMax) - kRadiusMax;
			params[getParamForSpeakerY(o)] = params[getParamForSpeakerY(o)] * (2*kRadiusMax) - kRadiusMax;
		}
		else
		{
			float x = params[getParamForSpeakerX(o)] * (2*kRadiusMax) - kRadiusMax;
			float y = params[getParamForSpeakerY(o)] * (2*kRadiusMax) - kRadiusMax;
			float t = atan2f(y, x);
			if (t < 0) t += kThetaMax;
			params[getParamForSpeakerX(o)] = t;
		}
	}
	
	// make sure the smoothed parameters table is inited
	if (!mSmoothedParametersInited)
	{
		for (int i = 0; i < kNumberOfParameters; i++)
			mSmoothedParameters.setUnchecked(i, params[i]);
		mSmoothedParametersInited = true;
	}
	
	bool processesInPlaceIsIgnored = true;
	
	// process data
	while(1)
	{
		unsigned int todo = (inFramesToProcess > kChunkSize) ? kChunkSize : inFramesToProcess;
		
		if (processesInPlaceIsIgnored)
		{
			//float *inputsCopy[iActualNumberOfSources];
			float **inputsCopy = new float* [mNumberOfSources];

			for (int i = 0; i < mNumberOfSources; i++)
			{
				memcpy(mInputsCopy.getReference(i).b, inputs[i], todo * sizeof(float));
				inputsCopy[i] = mInputsCopy.getReference(i).b;
			}
			
			ProcessData(inputsCopy, outputs, params, sampleRate, todo);
			delete[] inputsCopy;
		}
		else
			ProcessData(inputs, outputs, params, sampleRate, todo);
		
		inFramesToProcess -= todo;
		if (inFramesToProcess == 0) break;
		
		for (int i = 0; i < mNumberOfSources; i++)
			inputs[i] += todo;
			
		for (int o = 0; o < mNumberOfSpeakers; o++)
			outputs[o] += todo; 
	}
	
	if (mCalculateLevels)
	{
		const float attack = kLevelAttackDefault; //params[kLevelAttackParam]; // milliseconds
		const float release = kLevelReleaseDefault; //params[kLevelReleaseParam]; // milliseconds
		const float ag = powf(0.01f, 1000.f / (attack * sampleRate));
		const float rg = powf(0.01f, 1000.f / (release * sampleRate));
		
		for (int o = 0; o < mNumberOfSpeakers; o++)
		{
			float *output = buffer.getWritePointer(o);
			float env = mLevels[o];
			
			for (unsigned int f = 0; f < oriFramesToProcess; f++)
			{
				float s = fabsf(output[f]);
				float g = (s > env) ? ag : rg;
				env = g * env + (1.f - g) * s;
			}
			
			mLevels.setUnchecked(o, env);
		}
	}
	
	delete[] inputs, outputs;
	mProcessCounter++;
}

void OctogrisAudioProcessor::ProcessData(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames)
{
	switch(mProcessMode)
	{
		case kFreeVolumeMode:	ProcessDataFreeVolumeMode(inputs, outputs, params, sampleRate, frames);	break;
		case kPanVolumeMode:	ProcessDataPanVolumeMode(inputs, outputs, params, sampleRate, frames);	break;
		case kPanSpanMode:		ProcessDataPanSpanMode(inputs, outputs, params, sampleRate, frames);	break;
	}
}


void OctogrisAudioProcessor::findLeftAndRightSpeakers(float p_fTargetSpeakerAngle, float *params, int &p_piLeftSpeaker, int &p_piRightSpeaker,
                                                      float &p_pfDeltaAngleToLeftSpeaker, float &p_pfDeltaAngleToRightSpeaker, int p_iTargetSpeaker)
{
    p_piLeftSpeaker = -1;
    p_piRightSpeaker = -1;
    p_pfDeltaAngleToLeftSpeaker = kThetaMax;
    p_pfDeltaAngleToRightSpeaker = kThetaMax;
    
    int iFirstSpeaker = -1, iLastSpeaker = -1;
    float fMaxAngle = -1, fMinAngle = 9999999;
    
    for (int iCurSpeaker = 0; iCurSpeaker < mNumberOfSpeakers; iCurSpeaker++){
        
        float fCurSpeakerAngle = params[getParamForSpeakerX(iCurSpeaker)];
        float fCurDeltaAngle = -1.;
        
        //find out which is the first and last speakers
        if (fCurSpeakerAngle < fMinAngle){
            fMinAngle = fCurSpeakerAngle;
            iFirstSpeaker = iCurSpeaker;
        }
        if (fCurSpeakerAngle > fMaxAngle){
            fMaxAngle = fCurSpeakerAngle;
            iLastSpeaker = iCurSpeaker;
        }
        
        //skip the rest for the target speaker
        if (iCurSpeaker == p_iTargetSpeaker){
            continue;
        }
        
        //if curSpeaker is on left of target speaker
        if (fCurSpeakerAngle < p_fTargetSpeakerAngle){
            //check if curAngle is smaller than previous smallest left angle
            fCurDeltaAngle = p_fTargetSpeakerAngle - fCurSpeakerAngle;
            if (fCurDeltaAngle < p_pfDeltaAngleToLeftSpeaker){
                p_pfDeltaAngleToLeftSpeaker = fCurDeltaAngle;
                p_piLeftSpeaker = iCurSpeaker;
            }
            
        }
        //if curSpeaker is on right of target speaker
        else {
            fCurDeltaAngle = fCurSpeakerAngle - p_fTargetSpeakerAngle;
            if (fCurDeltaAngle < p_pfDeltaAngleToRightSpeaker){
                p_pfDeltaAngleToRightSpeaker = fCurDeltaAngle;
                p_piRightSpeaker = iCurSpeaker;
            }
        }
    }
    
    //if we haven't found the right speaker and the target is the first speaker, the left speaker is the first one
    if (p_iTargetSpeaker == iFirstSpeaker && p_piLeftSpeaker == -1){
        p_piLeftSpeaker = iLastSpeaker;
        p_pfDeltaAngleToLeftSpeaker = fMinAngle + (kThetaMax - fMaxAngle);
    }
    
    //if we haven't found the left speaker and the target is the last speaker, the left speaker is the first one
    else if (p_iTargetSpeaker == iLastSpeaker && p_piRightSpeaker == -1){
        p_piRightSpeaker = iFirstSpeaker;
        p_pfDeltaAngleToRightSpeaker = fMinAngle + (kThetaMax - fMaxAngle);
    }
}


void OctogrisAudioProcessor::addToOutput(float s, float **outputs, int o, int f)
{
	float *output_m = mSmoothedParametersRamps.getReference(getParamForSpeakerM(o)).b;
	float *output_a = mSmoothedParametersRamps.getReference(getParamForSpeakerA(o)).b;
	float a = dbToLinear(output_a[f]);
	float m = 1 - output_m[f];
	float output_adj = a * m;
	float *output = outputs[o];
	output[f] += s * output_adj;
}

void OctogrisAudioProcessor::ProcessDataPanVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames)
{
	const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds
	const float sm_o = powf(0.01f, 1000.f / (smooth * sampleRate));
	const float sm_n = 1 - sm_o;
	
	// ramp all the parameters, except constant ones and speaker thetas
	const int sourceParameters = JucePlugin_MaxNumInputChannels * kParamsPerSource;//const int sourceParameters = mNumberOfSources * kParamsPerSource;
	const int speakerParameters = JucePlugin_MaxNumOutputChannels * kParamsPerSpeakers;//const int speakerParameters = mNumberOfSpeakers * kParamsPerSpeakers;
	for (int i = 0; i < (kNumberOfParameters - kConstantParameters); i++)
	{
		bool isSpeakerXY = (i >= sourceParameters && i < (sourceParameters + speakerParameters) && ((i - sourceParameters) % kParamsPerSpeakers) <= kSpeakerY);
		if (isSpeakerXY) continue;
	
		float currentParam = mSmoothedParameters[i];
		float targetParam = params[i];
		float *ramp = mSmoothedParametersRamps.getReference(i).b;
	
		//float ori = currentParam;
		
		for (unsigned int f = 0; f < frames; f++)
		{
			currentParam = currentParam * sm_o + targetParam * sm_n;
			ramp[f] = currentParam;
		}
		
		//if (i == 0 && ori != currentParam) printf("param %i -> %f -> %f\n", i, ori, currentParam);

		mSmoothedParameters.setUnchecked(i, currentParam);
	}
	
	// clear outputs
	for (int o = 0; o < mNumberOfSpeakers; o++)
	{
		float *output = outputs[o];
		memset(output, 0, frames * sizeof(float));
	}
	

	// compute
	// in this context: source T, R are actually source X, Y
	for (int i = 0; i < mNumberOfSources; i++)
	{
		float *input = inputs[i];
		float *input_x = mSmoothedParametersRamps.getReference(getParamForSourceX(i)).b;
		float *input_y = mSmoothedParametersRamps.getReference(getParamForSourceY(i)).b;
	
		for (unsigned int f = 0; f < frames; f++)
		{
			float s = input[f];
			float x = input_x[f];
			float y = input_y[f];
			
			// could use the Accelerate framework on mac for these
			float r = hypotf(x, y);
			if (r > kRadiusMax) r = kRadiusMax;
			
			float it = atan2f(y, x);
			if (it < 0) it += kThetaMax;
			
			if (mApplyFilter)
			{
				float distance;
				if (r >= 1) distance = denormalize(params[kFilterMid], params[kFilterFar], (r - 1));
				else distance = denormalize(params[kFilterNear], params[kFilterMid], r);
				s = mFilters[i].process(s, distance);
			}
			
			// adjust input volume
			{
				float dbSource;
				if (r >= 1) dbSource = denormalize(params[kVolumeMid], params[kVolumeFar], (r - 1));
				else dbSource = denormalize(params[kVolumeNear], params[kVolumeMid], r);
				s *= dbToLinear(dbSource);
			}
			
			float t;
			if (r >= kThetaLockRadius)
			{
				t = it;
				mLockedThetas.setUnchecked(i, it);
			}
			else
			{
				float c = (r >= kThetaLockRampRadius) ? ((r - kThetaLockRampRadius) / (kThetaLockRadius - kThetaLockRampRadius)) : 0;
				float lt = mLockedThetas.getUnchecked(i);
				float dt = lt - it;
				if (dt < 0) dt = -dt;
				if (dt > kQuarterCircle)
				{
					// assume flipped side
					if (lt > it) lt -= kHalfCircle;
					else lt += kHalfCircle;
				}
				t = c * it + (1 - c) * lt;
				
				if (t < 0) t += kThetaMax;
				else if (t >= kThetaMax) t -= kThetaMax;
				
				//if (f == 0) printf("it: %f lt: %f lt2: %f t: %f c: %f\n", it, mLockedThetas.getUnchecked(i), lt, t, c);
			}
			
			if (r >= 1)
			{
				// find left and right speakers
				int left, right;
				float dLeft, dRight;
				findLeftAndRightSpeakers(t, params, left, right, dLeft, dRight);
				
				// add to output
				if (left >= 0 && right >= 0)
				{
					float dTotal = dLeft + dRight;
					float vLeft = 1 - dLeft / dTotal;
					float vRight = 1 - dRight / dTotal;
					
					addToOutput(s * vLeft, outputs, left, f);
					addToOutput(s * vRight, outputs, right, f);
				}
				else
				{
					// one side is empty!
					int o = (left >= 0) ? left : right;
					jassert(o >= 0);
					
					addToOutput(s, outputs, o, f);
				}
			}
			else
			{
				// find front left, right
				int frontLeft, frontRight;
				float dFrontLeft, dFrontRight;
				findLeftAndRightSpeakers(t, params, frontLeft, frontRight, dFrontLeft, dFrontRight);
				
				float bt = t + kHalfCircle;
				if (bt > kThetaMax) bt -= kThetaMax;
				
				// find back left, right
				int backLeft, backRight;
				float dBackLeft, dBackRight;
				findLeftAndRightSpeakers(bt, params, backLeft, backRight, dBackLeft, dBackRight);
			
				float front = r * 0.5f + 0.5f;
				float back = 1 - front;
				
				// add to front output
				if (frontLeft >= 0 && frontRight >= 0)
				{
					float dTotal = dFrontLeft + dFrontRight;
					float vLeft = 1 - dFrontLeft / dTotal;
					float vRight = 1 - dFrontRight / dTotal;
					
					addToOutput(s * vLeft * front, outputs, frontLeft, f);
					addToOutput(s * vRight * front, outputs, frontRight, f);
				}
				else
				{
					// one side is empty!
					int o = (frontLeft >= 0) ? frontLeft : frontRight;
					jassert(o >= 0);
					
					addToOutput(s * front, outputs, o, f);
				}
				
				// add to back output
				if (backLeft >= 0 && backRight >= 0)
				{
					float dTotal = dBackLeft + dBackRight;
					float vLeft = 1 - dBackLeft / dTotal;
					float vRight = 1 - dBackRight / dTotal;
					
					addToOutput(s * vLeft * back, outputs, backLeft, f);
					addToOutput(s * vRight * back, outputs, backRight, f);
				}
				else
				{
					// one side is empty!
					int o = (backLeft >= 0) ? backLeft : backRight;
					jassert(o >= 0);
					
					addToOutput(s * back, outputs, o, f);
				}
			}
		}
	}
}

class Area
{
public:
    Area() {}
    Area(int sp, float ix1, float iy1, float ix2, float iy2)
    :	speaker(sp),
    x1(ix1), x2(ix2) //, y1(iy1), y2(iy2)
    {
        m = (iy2-iy1) / (ix2-ix1);
        b = iy1 - m * ix1;
    }
    
    float eval(float x) const { return m * x + b; }
    
    int speaker;
    float x1, x2;
    //float y1, y2;
    float m, b;
};


static void AddArea(int speaker, float ix1, float iy1, float ix2, float iy2, vector<Area> &areas, int &areaCount, int &speakerCount)
{
    jassert(ix1 < ix2);
    
    //fprintf(stderr, "speaker: %d x1: %f x2: %f dc: %f\n", speaker, ix1, ix2, ix2 - ix1);
    //fflush(stderr);
    
    if (ix1 < 0)
    {
        jassert(ix2 >= 0 && ix2 <= kThetaMax);
        jassert(ix1 + kThetaMax > 0);
        
        float yc = (0 - ix1) / (ix2 - ix1) * (iy2 - iy1) + iy1;
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, kThetaMax + ix1, iy1, kThetaMax, yc);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, 0, yc, ix2, iy2);
    }
    else if (ix2 > kThetaMax)
    {
        jassert(ix1 >= 0 && ix1 <= kThetaMax);
        jassert(ix2 - kThetaMax < kThetaMax);
        
        float yc = (kThetaMax - ix1) / (ix2 - ix1) * (iy2 - iy1) + iy1;
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, ix1, iy1, kThetaMax, yc);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, 0, yc, ix2 - kThetaMax, iy2);
    }
    else
    {
        jassert(ix1 >= 0 && ix2 <= kThetaMax);
        
        jassert(areaCount < speakerCount * s_iMaxAreas);
        areas[areaCount++] = Area(speaker, ix1, iy1, ix2, iy2);
    }
}
static void Integrate(float x1, float x2, const vector<Area> &areas, int areaCount, float *outFactors, float factor)
{
    if (x1 == x2)
    {
        //fprintf(stderr, "x1 == x2 (%f == %f)\n", x1, x2);
        return;
    }
    
    jassert(x1 < x2);
    jassert(x1 >= 0);
    jassert(x2 <= kThetaMax);
    
    for (int a = 0; a < areaCount; a++)
    {
        const Area &area = areas[a];
        
        if (x2 <= area.x1 || x1 >= area.x2)
            continue; // completely outside
        
        float c1 = (x1 < area.x1) ? area.x1 : x1;
        float c2 = (x2 > area.x2) ? area.x2 : x2;
        
        float y1 = area.eval(c1);
        float y2 = area.eval(c2);
        
        float dc = c2 - c1;
        //fprintf(stderr, "x1: %f x2: %f area.x1: %f area.x2: %f c1: %f c2: %f dc: %f\n", x1, x2, area.x1, area.x2, c1, c2, dc);
        //fflush(stderr);
        jassert(dc > 0);
        
        float v = dc * (y1+y2); // * 0.5f;
        if (v <= 0) v = 1e-6;
        
        outFactors[area.speaker] += v * factor;
    }
}

void OctogrisAudioProcessor::ProcessDataPanSpanMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames)
{
    const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds
    const float sm_o = powf(0.01f, 1000.f / (smooth * sampleRate));
    const float sm_n = 1 - sm_o;
    
    // ramp all the parameters, except constant ones and speaker thetas
    const int sourceParameters = mNumberOfSources * kParamsPerSource;
    const int speakerParameters = mNumberOfSpeakers * kParamsPerSpeakers;
    
    for (int iCurParamId= 0; iCurParamId < (kNumberOfParameters - kConstantParameters); iCurParamId++)
    {
        bool isSpeakerXY = (iCurParamId >= sourceParameters && iCurParamId < (sourceParameters + speakerParameters) && ((iCurParamId - sourceParameters) % kParamsPerSpeakers) <= kSpeakerY);
        if (isSpeakerXY) continue;
        
        float currentParam = mSmoothedParameters[iCurParamId];
        float targetParam = params[iCurParamId];
        float *ramp = mSmoothedParametersRamps.getReference(iCurParamId).b;
        
        //float ori = currentParam;
        
        for (unsigned int f = 0; f < frames; f++)
        {
            currentParam = currentParam * sm_o + targetParam * sm_n;
            ramp[f] = currentParam;
        }
        
        //if (i == 0 && ori != currentParam) printf("param %i -> %f -> %f\n", i, ori, currentParam);
        
        mSmoothedParameters.setUnchecked(iCurParamId, currentParam);
    }
    
    // clear outputs
    for (int o = 0; o < mNumberOfSpeakers; o++)
    {
        float *output = outputs[o];
        memset(output, 0, frames * sizeof(float));
    }
    
    vector<Area> areas;
    areas.resize(mNumberOfSpeakers * s_iMaxAreas);

    int areaCount = 0;

    
    
    if (mNumberOfSpeakers > 2)
    {
        for (int iCurSpeaker = 0; iCurSpeaker < mNumberOfSpeakers; iCurSpeaker++)
        {
            float fCurAngle = params[getParamForSpeakerX(iCurSpeaker)];
            int left, right;
            float dLeft, dRight;
            findLeftAndRightSpeakers(fCurAngle, params, left, right, dLeft, dRight, iCurSpeaker);
            
            jassert(left >= 0 && right >= 0);
            jassert(dLeft > 0 && dRight > 0);
            
            AddArea(iCurSpeaker, fCurAngle - dLeft, 0, fCurAngle, 1, areas, areaCount, mNumberOfSpeakers);
            AddArea(iCurSpeaker, fCurAngle, 1, fCurAngle + dRight, 0, areas, areaCount, mNumberOfSpeakers);
        }
    }
    else if (mNumberOfSpeakers == 2)
    {
        int s1 = (params[getParamForSpeakerX(0)] < params[getParamForSpeakerX(1)]) ? 0 : 1;
        int s2 = 1 - s1;
        float t1 = params[getParamForSpeakerX(s1)];
        float t2 = params[getParamForSpeakerX(s2)];
        
        AddArea(s1, t2 - kThetaMax, 0, t1, 1, areas, areaCount, mNumberOfSpeakers);
        AddArea(s1, t1, 1, t2, 0, areas, areaCount, mNumberOfSpeakers);
        
        AddArea(s2, t1, 0, t2, 1, areas, areaCount, mNumberOfSpeakers);
        AddArea(s2, t2, 1, t1 + kThetaMax, 0, areas, areaCount, mNumberOfSpeakers);
    }
    else
    {
        AddArea(0, 0, 1, kThetaMax, 1, areas, areaCount, mNumberOfSpeakers);
    }
//    if (mNumberOfSpeakers > 2)//if (mNumberOfSpeakers > 1)
//    {
//        for (int o = 0; o < mNumberOfSpeakers; o++)
//        {
//            float t = params[getParamForSpeakerX(o)];
//            
//            int left, right;
//            float dLeft, dRight;
//            findSpeakers(t, params, left, right, dLeft, dRight, o);
//            
//            //std::cout << "speaker " << o << ": left= " << left << ", right: " << right << ", dLeft: " << dLeft << ", dRight: " << dRight << "\n";
//            jassert(left >= 0 && right >= 0);
//            jassert(dLeft > 0 && dRight > 0);
//            
//            AddArea(o, t - dLeft, 0, t, 1, areas, areaCount, mNumberOfSpeakers);
//            AddArea(o, t, 1, t + dRight, 0, areas, areaCount, mNumberOfSpeakers);
//        }
//    }
//    else
//    {
//        AddArea(0, 0, 1, kThetaMax, 1, areas, areaCount, mNumberOfSpeakers);
//    }
    
    
    
    jassert(areaCount > 0);
    
    
    // compute
    // in this context: source T, R are actually source X, Y
    for (int i = 0; i < mNumberOfSources; i++)
    {
        float *input = inputs[i];
        float *input_x = mSmoothedParametersRamps.getReference(getParamForSourceX(i)).b;
        float *input_y = mSmoothedParametersRamps.getReference(getParamForSourceY(i)).b;
        float *input_d = mSmoothedParametersRamps.getReference(getParamForSourceD(i)).b;
        
        for (unsigned int f = 0; f < frames; f++)
        {
            float s = input[f];
            float x = input_x[f];
            float y = input_y[f];
            float d = input_d[f];
            
            if (d > 1){
               d = normalize(kSourceMinDistance, kSourceMaxDistance, d);
            }
            
            float tv = dbToLinear(d * params[kMaxSpanVolume]);
            
            // could use the Accelerate framework on mac for these
            float r = hypotf(x, y);
            if (r > kRadiusMax) r = kRadiusMax;
            
            float it = atan2f(y, x);
            if (it < 0) it += kThetaMax;
            
            //if (r < 1 && d > 0.5) d = 0.5;
            if (d < 1e-6) d = 1e-6;
            float angle = d * M_PI;
            
            if (mApplyFilter)
            {
                float distance;
                if (r >= 1) distance = denormalize(params[kFilterMid], params[kFilterFar], (r - 1));
                else distance = denormalize(params[kFilterNear], params[kFilterMid], r);
                s = mFilters[i].process(s, distance);
            }
            
            // adjust input volume
            {
                float dbSource;
                if (r >= 1) dbSource = denormalize(params[kVolumeMid], params[kVolumeFar], (r - 1));
                else dbSource = denormalize(params[kVolumeNear], params[kVolumeMid], r);
                s *= dbToLinear(dbSource);
            }
            
            float t;
            if (r >= kThetaLockRadius)
            {
                t = it;
                mLockedThetas.setUnchecked(i, it);
            }
            else
            {
                float c = (r >= kThetaLockRampRadius) ? ((r - kThetaLockRampRadius) / (kThetaLockRadius - kThetaLockRampRadius)) : 0;
                float lt = mLockedThetas.getUnchecked(i);
                float dt = lt - it;
                if (dt < 0) dt = -dt;
                if (dt > kQuarterCircle)
                {
                    // assume flipped side
                    if (lt > it) lt -= kHalfCircle;
                    else lt += kHalfCircle;
                }
                t = c * it + (1 - c) * lt;
                
                if (t < 0) t += kThetaMax;
                else if (t >= kThetaMax) t -= kThetaMax;
                
                //if (f == 0) printf("it: %f lt: %f lt2: %f t: %f c: %f\n", it, mLockedThetas.getUnchecked(i), lt, t, c);
            }
            
            jassert(t >= 0 && t <= kThetaMax);
            jassert(angle > 0 && angle <= kHalfCircle);
            
            //float outFactors[mNumberOfSpeakers];
            //memset(outFactors, 0, sizeof(outFactors));
			float *outFactors = new float[mNumberOfSpeakers]();

            
            float factor = (r < 1) ? (r * 0.5f + 0.5f) : 1;
            
            for (int side = 0; side < 2; side++)
            {
                float tl = t - angle, tr = t + angle;
                
                if (tl < 0)
                {
                    Integrate(tl + kThetaMax, kThetaMax, areas, areaCount, outFactors, factor);
                    Integrate(0, tr, areas, areaCount, outFactors, factor);
                }
                else if (tr > kThetaMax)
                {
                    Integrate(tl, kThetaMax, areas, areaCount, outFactors, factor);
                    Integrate(0, tr - kThetaMax, areas, areaCount, outFactors, factor);
                }
                else
                {
                    Integrate(tl, tr, areas, areaCount, outFactors, factor);
                }
                
                if (r < 1)
                {
                    factor = 1 - factor;
                    t = (t < kHalfCircle) ? (t + kHalfCircle) : (t - kHalfCircle);
                }
                else break;
            }
            
            float total = 0;
            for (int o = 0; o < mNumberOfSpeakers; o++) total += outFactors[o];
            jassert(total > 0);
            
            float adj = tv / total;
            for (int o = 0; o < mNumberOfSpeakers; o++)
                if (outFactors[o])
                    addToOutput(s * outFactors[o] * adj, outputs, o, f);
			delete[] outFactors;
        }
    }
}

void OctogrisAudioProcessor::ProcessDataFreeVolumeMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames)
{
	const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds
	const float sm_o = powf(0.01f, 1000.f / (smooth * sampleRate));
	const float sm_n = 1 - sm_o;
	
	// ramp all the parameters, except constant ones
	for (int i = 0; i < (kNumberOfParameters - kConstantParameters); i++)
	{
		float currentParam = mSmoothedParameters[i];
		float targetParam = params[i];
		float *ramp = mSmoothedParametersRamps.getReference(i).b;
	
		//float ori = currentParam;
		
		for (unsigned int f = 0; f < frames; f++)
		{
			currentParam = currentParam * sm_o + targetParam * sm_n;
			ramp[f] = currentParam;
		}
		
		//if (i == 0 && ori != currentParam) printf("param %i -> %f -> %f\n", i, ori, currentParam);
		
		mSmoothedParameters.setUnchecked(i, currentParam);
	}
	
	// in this context: T, R are actually X, Y
	
	// compute
	const float adj_factor = 1 / sqrtf(2);
	for (int o = 0; o < mNumberOfSpeakers; o++)
	{
		float *output = outputs[o];
		float *output_x = mSmoothedParametersRamps.getReference(getParamForSpeakerX(o)).b;
		float *output_y = mSmoothedParametersRamps.getReference(getParamForSpeakerY(o)).b;
		float output_adj[kChunkSize];
		{
			float *output_m = mSmoothedParametersRamps.getReference(getParamForSpeakerM(o)).b;
			float *output_a = mSmoothedParametersRamps.getReference(getParamForSpeakerA(o)).b;
			
			for (unsigned int f = 0; f < frames; f++)
			{
				float a = dbToLinear(output_a[f]);
				float m = 1 - output_m[f];
				output_adj[f] = a * m;
			}
		}
		
		for (int i = 0; i < mNumberOfSources; i++)
		{
			float *input = inputs[i];
			float *input_x = mSmoothedParametersRamps.getReference(getParamForSourceX(i)).b;
			float *input_y = mSmoothedParametersRamps.getReference(getParamForSourceY(i)).b;
			float *input_d = mSmoothedParametersRamps.getReference(getParamForSourceD(i)).b;
			
			if (i == 0)
				for (unsigned int f = 0; f < frames; f++)
				{
					float dx = input_x[f] - output_x[f];
					float dy = input_y[f] - output_y[f];
					float d = sqrtf(dx*dx + dy*dy);
					float da = d * adj_factor * input_d[f];
					//if (da > 1) da = 0; else da = 1 - da;
					//da *= output_adj[f];
					//change distance scale for new calculations
					//"1" means far now instead of "0"
					if (da > 1) da = 1;
					//New distance vs amplitude calculations:
					//if da < 0.1 the log value will skyrocket:
					if (da < 0.1) da = 0.1;
					da = -log10f(da);
					da *= output_adj[f];
					
					output[f] = da * input[f];
				}
			else
				for (unsigned int f = 0; f < frames; f++)
				{
					float dx = input_x[f] - output_x[f];
					float dy = input_y[f] - output_y[f];
					float d = sqrtf(dx*dx + dy*dy);
					float da = d * adj_factor * input_d[f];
					//if (da > 1) da = 0; else da = 1 - da;
					//change distance scale for new calculations
					//"1" means far now instead of "0"
					if (da > 1) da = 1;
					//New distance vs amplitude calculations:
					//if da < 0.1 the log value will skyrocket:
					if (da < 0.1) da = 0.1;
					da = -log10f(da);
					da *= output_adj[f];
					
					output[f] += da * input[f];
				}
		}
	}
}

//==============================================================================
bool OctogrisAudioProcessor::hasEditor() const
{
    return true;
}

AudioProcessorEditor* OctogrisAudioProcessor::createEditor()
{
    return new OctogrisAudioProcessorEditor (this);
}

//==============================================================================
static inline void appendIntData(MemoryBlock& destData, int32_t d)
{
	destData.append(&d, sizeof(d));
}
static inline int32_t readIntData(const void* &data, int &dataLength, int32_t defaultValue)
{
	int32_t r;
	if (dataLength >= sizeof(float))
	{
		const int32_t *pv = static_cast<const int32_t*>(data);
		r = *pv++;
		data = static_cast<const void*>(pv);
		dataLength -= sizeof(int32_t);
	}
	else
	{
		fprintf(stderr, "readIntData failed: not enough data...\n");
		r = defaultValue;
		dataLength = 0;
	}
	return r;
}
static inline void appendFloatData(MemoryBlock& destData, float d)
{
	destData.append(&d, sizeof(d));
}
static inline float readFloatData(const void* &data, int &dataLength, float defaultValue)
{
	float r;
	if (dataLength >= sizeof(float))
	{
		const float *pv = static_cast<const float*>(data);
		r = *pv++;
		data = static_cast<const void*>(pv);
		dataLength -= sizeof(float);
	}
	else
	{
		fprintf(stderr, "readFloatData failed: not enough data...\n");
		r = defaultValue;
		dataLength = 0;
	}
	return r;
}
static inline void appendStringData(MemoryBlock& destData, const char *d, int length)
{
	destData.append(d, length);
}
static inline void readStringData(const void* &data, int &dataLength, const char *defaultValue, char *d, int length)
{
	if (dataLength >= length)
	{
		const char *pv = static_cast<const char*>(data);
		memcpy(d, pv, length);
		d[length-1] = 0;
		data = static_cast<const void*>(pv + length);
		dataLength -= length;
	}
	else
	{
		fprintf(stderr, "readStringData failed: not enough data...\n");
		strlcpy(d, defaultValue, length);
		dataLength = 0;
	}
}

void OctogrisAudioProcessor::storeCurrentLocations(){
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++)//for (int i = 0; i < mNumberOfSources; i++)
    {
        mBufferSrcLocX[i] = mParameters[getParamForSourceX(i)];
        mBufferSrcLocY[i] = mParameters[getParamForSourceY(i)];
        float fValye = mParameters[getParamForSourceD(i)];
        mBufferSrcLocD[i] = fValye;

    }
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++)//for (int i = 0; i < mNumberOfSpeakers; i++)
    {
        mBufferSpLocX[i] =  mParameters[getParamForSpeakerX(i)];
        mBufferSpLocY[i] = mParameters[getParamForSpeakerY(i)];
        mBufferSpLocA[i] = mParameters[getParamForSpeakerA(i)];
        mBufferSpLocM[i] = mParameters[getParamForSpeakerM(i)];
    }
}

void OctogrisAudioProcessor::restoreCurrentLocations(){
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++)//for (int i = 0; i < mNumberOfSources; i++)
    {
        mParameters.set(getParamForSourceX(i), mBufferSrcLocX[i]);
        mParameters.set(getParamForSourceY(i), mBufferSrcLocY[i]);
        float fValue = mBufferSrcLocD[i];
        mParameters.set(getParamForSourceD(i), fValue);
    }
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++)//for (int i = 0; i < mNumberOfSpeakers; i++)
    {
        mParameters.set(getParamForSpeakerX(i), mBufferSpLocX[i]);
        mParameters.set(getParamForSpeakerY(i), mBufferSpLocY[i]);
        mParameters.set(getParamForSpeakerA(i), mBufferSpLocA[i]);
        mParameters.set(getParamForSpeakerM(i), mBufferSpLocM[i]);
    }
}

static const int kDataVersion = 10;
void OctogrisAudioProcessor::getStateInformation (MemoryBlock& destData)
{
    //printf("Octogris: getStateInformation\n");
    appendIntData(destData, kDataVersion);
    appendIntData(destData, mShowGridLines);
    appendIntData(destData, mMovementMode);
    appendIntData(destData, mLinkDistances);
    appendIntData(destData, mGuiSize);
    appendIntData(destData, mGuiTab);
    appendIntData(destData, mOscLeapSource);
    appendIntData(destData, mOscReceiveEnabled);
    appendIntData(destData, mOscReceivePort);
    appendIntData(destData, mOscSendEnabled);
    appendIntData(destData, mOscSendPort);
    appendStringData(destData, mOscSendIp, sizeof(mOscSendIp));
    appendIntData(destData, mProcessMode);
    appendIntData(destData, mApplyFilter);
    
    //version 9
    appendIntData(destData, mInputOutputMode);
    appendIntData(destData, mSrcPlacementMode);
    appendIntData(destData, mSpPlacementMode);
    appendIntData(destData, mSrcSelected);
    appendIntData(destData, mSpSelected);
    appendIntData(destData, m_iTrType);
    appendIntData(destData, m_iTrSrcSelect);
    appendFloatData(destData, m_fTrDuration);
    appendIntData(destData, m_iTrUnits);
    appendFloatData(destData, m_fTrRepeats);
    appendIntData(destData, mLeapEnabled);
    
    //version 10
    appendFloatData(destData, mParameters[kMaxSpanVolume]);
    
    appendFloatData(destData, mParameters[kLinkMovement]);
    appendFloatData(destData, mParameters[kSmooth]);
    appendFloatData(destData, mParameters[kVolumeNear]);
    appendFloatData(destData, mParameters[kVolumeMid]);
    appendFloatData(destData, mParameters[kVolumeFar]);
    appendFloatData(destData, mParameters[kFilterNear]);
    appendFloatData(destData, mParameters[kFilterMid]);
    appendFloatData(destData, mParameters[kFilterFar]);
    
    for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++)//for (int i = 0; i < mNumberOfSources; i++)
    {
        appendFloatData(destData, mParameters[getParamForSourceX(i)]);
        appendFloatData(destData, mParameters[getParamForSourceY(i)]);
        appendFloatData(destData, mParameters[getParamForSourceD(i)]);
    }
    for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++)//for (int i = 0; i < mNumberOfSpeakers; i++)
    {
        appendFloatData(destData, mParameters[getParamForSpeakerX(i)]);
        appendFloatData(destData, mParameters[getParamForSpeakerY(i)]);
        appendFloatData(destData, mParameters[getParamForSpeakerA(i)]);
        float mute = mParameters[getParamForSpeakerM(i)];
        appendFloatData(destData, mute);
    }
}

void OctogrisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	//the weird order here is because the order has to match what is in getStateInformation
    int version = readIntData(data, sizeInBytes, 0);
	if (version >= 1)
	{
		mShowGridLines = readIntData(data, sizeInBytes, 0);
		if (version < 7) readIntData(data, sizeInBytes, 0); // old show levels
		if (version < 7) readIntData(data, sizeInBytes, 0); // old account for attenuation
		if (version < 2) readIntData(data, sizeInBytes, 0); // old link movement
		mMovementMode = readIntData(data, sizeInBytes, 0);
		mLinkDistances = readIntData(data, sizeInBytes, 0);
		mGuiSize = readIntData(data, sizeInBytes, 1);
		if (version >= 8)
		{
			mGuiTab = readIntData(data, sizeInBytes, 0);
			mOscLeapSource = readIntData(data, sizeInBytes, 0);
			mOscReceiveEnabled = readIntData(data, sizeInBytes, 0);
			mOscReceivePort = readIntData(data, sizeInBytes, 8000);
			mOscSendEnabled = readIntData(data, sizeInBytes, 0);
			mOscSendPort = readIntData(data, sizeInBytes, 9000);
			readStringData(data, sizeInBytes, "192.168.1.100", mOscSendIp, sizeof(mOscSendIp));
		}
		if (version >= 3) mProcessMode = readIntData(data, sizeInBytes, kPanVolumeMode);
		if (version >= 6) mApplyFilter = readIntData(data, sizeInBytes, 1);
        
        if (version >= 9){
            mInputOutputMode = readIntData(data, sizeInBytes, 1);
            mSrcPlacementMode = readIntData(data, sizeInBytes, 1);
            mSpPlacementMode = readIntData(data, sizeInBytes, 1);
            mSrcSelected = readIntData(data, sizeInBytes, 1);
            mSpSelected = readIntData(data, sizeInBytes, 1);
            
            m_iTrType = readIntData(data, sizeInBytes, 0);
            m_iTrSrcSelect = readIntData(data, sizeInBytes, 1);
            m_fTrDuration = readFloatData(data, sizeInBytes, 1);
            m_iTrUnits = readIntData(data, sizeInBytes, 0);
            m_fTrRepeats = readFloatData(data, sizeInBytes, 1);
            mLeapEnabled = readIntData(data, sizeInBytes, 0);
        }
        
        if (version >= 10){
            mParameters.set(kMaxSpanVolume, readFloatData(data, sizeInBytes, normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault)));
        }
		
		if (version >= 4)
		{
			mParameters.set(kLinkMovement, readFloatData(data, sizeInBytes, 0));
			mParameters.set(kSmooth, readFloatData(data, sizeInBytes, normalize(kSmoothMin, kSmoothMax, kSmoothDefault)));
			mParameters.set(kVolumeNear, readFloatData(data, sizeInBytes, normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault)));
			if (version >= 5) mParameters.set(kVolumeMid, readFloatData(data, sizeInBytes, normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault)));
			mParameters.set(kVolumeFar, readFloatData(data, sizeInBytes, normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault)));
			mParameters.set(kFilterNear, readFloatData(data, sizeInBytes, normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault)));
			if (version >= 5) mParameters.set(kFilterMid, readFloatData(data, sizeInBytes, normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault)));
			mParameters.set(kFilterFar, readFloatData(data, sizeInBytes, normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault)));
            for (int i = 0; i < JucePlugin_MaxNumInputChannels; i++)//for (int i = 0; i < mNumberOfSources; i++)
			{
				mParameters.set(getParamForSourceX(i), readFloatData(data, sizeInBytes, 0));
				mParameters.set(getParamForSourceY(i), readFloatData(data, sizeInBytes, 0));
                float distance = readFloatData(data, sizeInBytes, normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance));
				mParameters.set(getParamForSourceD(i), distance);
			}
            for (int i = 0; i < JucePlugin_MaxNumOutputChannels; i++)//for (int i = 0; i < mNumberOfSpeakers; i++)
			{
				mParameters.set(getParamForSpeakerX(i), readFloatData(data, sizeInBytes, 0));
				mParameters.set(getParamForSpeakerY(i), readFloatData(data, sizeInBytes, 0));
                float att = readFloatData(data, sizeInBytes, normalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, kSpeakerDefaultAttenuation));
				mParameters.set(getParamForSpeakerA(i), att);
                float mute = readFloatData(data, sizeInBytes, 0);
				mParameters.set(getParamForSpeakerM(i), mute );
			}
		}
	}
	mHostChangedParameter++;
	mHostChangedProperty++;
}

//==============================================================================
// This creates new instances of the plugin..
AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new OctogrisAudioProcessor();
}
