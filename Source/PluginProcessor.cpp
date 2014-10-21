/*
  ==============================================================================

    This file was auto-generated!

    It contains the basic startup code for a Juce application.

  ==============================================================================
*/

//static const float kSourceDefaultRadius = 0.4f;

//static const float kLevelAttackMin = 0.01;
//static const float kLevelAttackMax = 100;
static const float kLevelAttackDefault = 0.05f;

//static const float kLevelReleaseMin = 1;
//static const float kLevelReleaseMax = 500;
static const float kLevelReleaseDefault = 100;

#include "PluginProcessor.h"
#include "PluginEditor.h"

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
OctogrisAudioProcessor::OctogrisAudioProcessor()
{
	mParameters.ensureStorageAllocated(kNumberOfParameters);
	for (int i = 0; i < kNumberOfParameters; i++) mParameters.add(0);
	
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

	mCalculateLevels = 0;
	mLevels.ensureStorageAllocated(kNumberOfSpeakers);
	for (int i = 0; i < kNumberOfSpeakers; i++) mLevels.add(0);
	
	mLockedThetas.ensureStorageAllocated(kNumberOfSources);
	for (int i = 0; i < kNumberOfSources; i++) mLockedThetas.add(0);
	
	mApplyFilter = true;
	mLinkDistances = false;
	mMovementMode = 0;
	mShowGridLines = false;
	mGuiSize = 1;
	mGuiTab = 0;
	mHostChangedParameter = 0;
	mHostChangedProperty = 0;
	mProcessCounter = 0;
	mLastTimeInSamples = -1;
	mProcessMode = kPanVolumeMode;
	
	mOscLeapSource = 0;
	mOscReceiveEnabled = 0;
	mOscReceivePort = 8000;
	mOscSendEnabled = 0;
	mOscSendPort = 9000;
	setOscSendIp("192.168.1.100");
	
	mInputsCopy.resize(kNumberOfSources);
	mSmoothedParametersRamps.resize(kNumberOfParameters);
	
	// default values for parameters
	for (int i = 0; i < kNumberOfSources; i++)
		mParameters.set(ParamForSourceD(i), normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance));
	
	if (kNumberOfSources == 1)
	{
		setSourceRT(0, FPoint(0, 0));
	}
	else
	{
		double anglePerSource = 360 / kNumberOfSources;
		double offset, axisOffset;

		if(kNumberOfSources%2 == 0) //if the number of speakers is even we will assign them as stereo pairs
		{
			axisOffset = anglePerSource / 2;
			for (int i = 0; i < kNumberOfSources; i++)
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
				
				setSourceRT(i, FPoint(1, offset/360*kThetaMax));
			}
		}
		else //odd number of speakers, assign in circular fashion
		{
			offset = (anglePerSource + 180) / 2 - anglePerSource;
			for (int i = 0; i < kNumberOfSources; i++)
			{
				if (offset < 0) offset += 360;
				else if (offset > 360) offset -= 360;
				
				setSourceRT(i, FPoint(1, offset/360*kThetaMax));
				offset += anglePerSource;
			}
		}
	}
	for (int i = 0; i < kNumberOfSources; i++)
		mLockedThetas.set(i, getSourceRT(i).y);
	
	// - - - - - - - - - - - - - -
	// Speakers default parameters
	{
		double anglePerSpeakers = 360 / kNumberOfSpeakers;
		double offset, axisOffset;
		
		if(kNumberOfSpeakers%2 == 0) //if the number of speakers is even we will assign them as stereo pairs
		{
			axisOffset = anglePerSpeakers / 2;
			for (int i = 0; i < kNumberOfSpeakers; i++)
			{
				mParameters.set(ParamForSpeakerA(i), normalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, kSpeakerDefaultAttenuation));
				mParameters.set(ParamForSpeakerM(i), 0);
				
				if(i%2 == 0)
				{
					offset = 90 + axisOffset;
				}
				else
				{
					offset = 90 - axisOffset;
					axisOffset += anglePerSpeakers;
				}
				
				if (offset < 0) offset += 360;
				else if (offset > 360) offset -= 360;
				
				setSpeakerRT(i, FPoint(1, offset/360*kThetaMax));
			}
		}
		else //odd number of speakers, assign in circular fashion
		{
			offset = (anglePerSpeakers + 180) / 2 - anglePerSpeakers;
			for (int i = 0; i < kNumberOfSpeakers; i++)
			{
				mParameters.set(ParamForSpeakerA(i), kSpeakerDefaultAttenuation);
				mParameters.set(ParamForSpeakerM(i), 0);
				
				if (offset < 0) offset += 360;
				else if (offset > 360) offset -= 360;
				
				setSpeakerRT(i, FPoint(1, offset/360*kThetaMax));
				offset += anglePerSpeakers;
			}
		}
	}
}

OctogrisAudioProcessor::~OctogrisAudioProcessor()
{
}


//==============================================================================
void OctogrisAudioProcessor::setCalculateLevels(bool c)
{
	if (!mCalculateLevels && c)
		for (int i = 0; i < kNumberOfSpeakers; i++)
			mLevels.setUnchecked(i, 0);

	// keep count of number of editors
	if (c) mCalculateLevels++;
	else mCalculateLevels--;
	
	//fprintf(stderr, "mCalculateLevels: %d\n", mCalculateLevels);
}

//==============================================================================
const String OctogrisAudioProcessor::getName() const
{
	String name(JucePlugin_Name);
	name << " ";
	name << kNumberOfSources;
	name << "x";
	name << kNumberOfSpeakers;
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
    sendParamChangeMessageToListeners(index, newValue);
}

const String OctogrisAudioProcessor::getParameterName (int index)
{
	switch(index)
	{
		case kLinkMovement:	return "Link Movement";
		case kSmooth:		return "Smooth Param";
		case kVolumeNear:	return "Volume Near";
		case kVolumeMid:	return "Volume Mid";
		case kVolumeFar:	return "Volume Far";
		case kFilterNear:	return "Filter Near";
		case kFilterMid:	return "Filter Mid";
		case kFilterFar:	return "Filter Far";
		case kMaxSpanVolume:return "Max span volume";
	}

	if (index < kNumberOfSources * kParamsPerSource)
	{
		String s("Source ");
		s << (index / kParamsPerSource + 1);
		switch(index % kParamsPerSource)
		{
			case kSourceX: s << " - X"; break;
			case kSourceY: s << " - Y"; break;
			case kSourceD: s << " - D"; break;
			case kSourceUnused: s << " - Unused"; break;
		}
		return s;
	}
	index -= kNumberOfSources * kParamsPerSource;
	
	if (index < kNumberOfSpeakers * kParamsPerSpeakers)
	{
		String s("Speaker ");
		s << (index / kParamsPerSpeakers + 1);
		switch(index % kParamsPerSpeakers)
		{
			case kSpeakerX: s << " - X"; break;
			case kSpeakerY: s << " - Y"; break;
			case kSpeakerA: s << " - A"; break;
			case kSpeakerM: s << " - M"; break;
			case kSpeakerUnused: s << " - Unused"; break;
		}
		return s;
	}
	
    return String::empty;
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
    return index == 0 && kNumberOfSources == 2;
	//return false;
}

bool OctogrisAudioProcessor::isOutputChannelStereoPair (int index) const
{
    return index == 0 && kNumberOfSpeakers == 2;
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
    return 0;
}

int OctogrisAudioProcessor::getCurrentProgram()
{
    return 0;
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
	if (mCalculateLevels)
		for (int i = 0; i < kNumberOfSpeakers; i++)
			mLevels.setUnchecked(i, 0);
		
	mSmoothedParametersInited = false;
	
	int sr = sampleRate;
	for (int i = 0; i < kNumberOfSources; i++)
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
	//for (int c = kNumberOfSources; c < kNumberOfSpeakers; c++)
	//	buffer.clear(c, 0, buffer.getNumSamples());
}

void OctogrisAudioProcessor::processBlock (AudioSampleBuffer& buffer, MidiBuffer& midiMessages)
{
	//fprintf(stderr, "pb\n");
	jassert(kNumberOfSources == getNumInputChannels());
	jassert(kNumberOfSpeakers == getNumOutputChannels());
	
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
	
	float *inputs[kNumberOfSources];
	for (int i = 0; i < kNumberOfSources; i++)
	{
		inputs[i] = buffer.getSampleData(i);
		
		if (mProcessMode == kFreeVolumeMode)
			params[ParamForSourceD(i)] = denormalize(kSourceMinDistance, kSourceMaxDistance, params[ParamForSourceD(i)]);
		
		params[ParamForSourceX(i)] = params[ParamForSourceX(i)] * (2*kRadiusMax) - kRadiusMax;
		params[ParamForSourceY(i)] = params[ParamForSourceY(i)] * (2*kRadiusMax) - kRadiusMax;
	}
	
	float *outputs[kNumberOfSpeakers];
	for (int o = 0; o < kNumberOfSpeakers; o++)
	{
		outputs[o] = buffer.getSampleData(o);
		params[ParamForSpeakerA(o)] = denormalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, params[ParamForSpeakerA(o)]);
		
		if (mProcessMode == kFreeVolumeMode)
		{
			params[ParamForSpeakerX(o)] = params[ParamForSpeakerX(o)] * (2*kRadiusMax) - kRadiusMax;
			params[ParamForSpeakerY(o)] = params[ParamForSpeakerY(o)] * (2*kRadiusMax) - kRadiusMax;
		}
		else
		{
			float x = params[ParamForSpeakerX(o)] * (2*kRadiusMax) - kRadiusMax;
			float y = params[ParamForSpeakerY(o)] * (2*kRadiusMax) - kRadiusMax;
			float t = atan2f(y, x);
			if (t < 0) t += kThetaMax;
			params[ParamForSpeakerX(o)] = t;
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
			float *inputsCopy[kNumberOfSources];
			for (int i = 0; i < kNumberOfSources; i++)
			{
				memcpy(mInputsCopy.getReference(i).b, inputs[i], todo * sizeof(float));
				inputsCopy[i] = mInputsCopy.getReference(i).b;
			}
			
			ProcessData(inputsCopy, outputs, params, sampleRate, todo);
		}
		else
			ProcessData(inputs, outputs, params, sampleRate, todo);
		
		inFramesToProcess -= todo;
		if (inFramesToProcess == 0) break;
		
		for (int i = 0; i < kNumberOfSources; i++)
			inputs[i] += todo;
			
		for (int o = 0; o < kNumberOfSpeakers; o++)
			outputs[o] += todo; 
	}
	
	if (mCalculateLevels)
	{
		const float attack = kLevelAttackDefault; //params[kLevelAttackParam]; // milliseconds
		const float release = kLevelReleaseDefault; //params[kLevelReleaseParam]; // milliseconds
		const float ag = powf(0.01f, 1000.f / (attack * sampleRate));
		const float rg = powf(0.01f, 1000.f / (release * sampleRate));
		
		for (int o = 0; o < kNumberOfSpeakers; o++)
		{
			float *output = buffer.getSampleData(o);
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

void OctogrisAudioProcessor::findSpeakers(float t, float *params, int &left, int &right, float &dLeft, float &dRight, int skip)
{
	left = -1;
	right = -1;
	dLeft = kThetaMax;
	dRight = kThetaMax;
	for (int o = 0; o < kNumberOfSpeakers; o++)
	{
		if (o == skip) continue;
		
		float speakerT = params[ParamForSpeakerX(o)];
		float d = speakerT - t;
		if (d >= 0)
		{
			if (d > kHalfCircle)
			{
				// right
				d = kThetaMax - d;
				if (d < dRight)
				{
					dRight = d;
					right = o;
				}
			}
			else
			{
				// left
				if (d < dLeft)
				{
					dLeft = d;
					left = o;
				}
			}
		}
		else
		{
			d = -d;
			if (d > kHalfCircle)
			{
				// left
				d = kThetaMax - d;
				if (d < dLeft)
				{
					dLeft = d;
					left = o;
				}
			}
			else
			{
				// right
				if (d < dRight)
				{
					dRight = d;
					right = o;
				}
			}
		}
	}
}

void OctogrisAudioProcessor::addToOutput(float s, float **outputs, int o, int f)
{
	float *output_m = mSmoothedParametersRamps.getReference(ParamForSpeakerM(o)).b;
	float *output_a = mSmoothedParametersRamps.getReference(ParamForSpeakerA(o)).b;
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
	const int sourceParameters = kNumberOfSources * kParamsPerSource;
	const int speakerParameters = kNumberOfSpeakers * kParamsPerSpeakers;
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
	for (int o = 0; o < kNumberOfSpeakers; o++)
	{
		float *output = outputs[o];
		memset(output, 0, frames * sizeof(float));
	}
	

	// compute
	// in this context: source T, R are actually source X, Y
	for (int i = 0; i < kNumberOfSources; i++)
	{
		float *input = inputs[i];
		float *input_x = mSmoothedParametersRamps.getReference(ParamForSourceX(i)).b;
		float *input_y = mSmoothedParametersRamps.getReference(ParamForSourceY(i)).b;
	
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
				findSpeakers(t, params, left, right, dLeft, dRight);
				
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
				findSpeakers(t, params, frontLeft, frontRight, dFrontLeft, dFrontRight);
				
				float bt = t + kHalfCircle;
				if (bt > kThetaMax) bt -= kThetaMax;
				
				// find back left, right
				int backLeft, backRight;
				float dBackLeft, dBackRight;
				findSpeakers(bt, params, backLeft, backRight, dBackLeft, dBackRight);
			
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

#define kMaxAreas (kNumberOfSpeakers * 3)
static void AddArea(int speaker, float ix1, float iy1, float ix2, float iy2, Area *areas, int &areaCount)
{
	assert(ix1 < ix2);
	
	//fprintf(stderr, "speaker: %d x1: %f x2: %f dc: %f\n", speaker, ix1, ix2, ix2 - ix1);
	//fflush(stderr);
	
	if (ix1 < 0)
	{
		assert(ix2 >= 0 && ix2 <= kThetaMax);
		assert(ix1 + kThetaMax > 0);
		
		float yc = (0 - ix1) / (ix2 - ix1) * (iy2 - iy1) + iy1;
		
		assert(areaCount < kMaxAreas);
		areas[areaCount++] = Area(speaker, kThetaMax + ix1, iy1, kThetaMax, yc);
		
		assert(areaCount < kMaxAreas);
		areas[areaCount++] = Area(speaker, 0, yc, ix2, iy2);
	}
	else if (ix2 > kThetaMax)
	{
		assert(ix1 >= 0 && ix1 <= kThetaMax);
		assert(ix2 - kThetaMax < kThetaMax);
		
		float yc = (kThetaMax - ix1) / (ix2 - ix1) * (iy2 - iy1) + iy1;
		
		assert(areaCount < kMaxAreas);
		areas[areaCount++] = Area(speaker, ix1, iy1, kThetaMax, yc);
		
		assert(areaCount < kMaxAreas);
		areas[areaCount++] = Area(speaker, 0, yc, ix2 - kThetaMax, iy2);
	}
	else
	{
		assert(ix1 >= 0 && ix2 <= kThetaMax);
		
		assert(areaCount < kMaxAreas);
		areas[areaCount++] = Area(speaker, ix1, iy1, ix2, iy2);
	}
}
static void Integrate(float x1, float x2, const Area *areas, int areaCount, float *outFactors, float factor)
{
	assert(x1 < x2);
	assert(x1 >= 0);
	assert(x2 <= kThetaMax);
	
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
		assert(dc > 0);
		
		float v = dc * (y1+y2); // * 0.5f;
		assert(v > 0);
		
		outFactors[area.speaker] += v * factor;
	}
}

void OctogrisAudioProcessor::ProcessDataPanSpanMode(float **inputs, float **outputs, float *params, float sampleRate, unsigned int frames)
{
	const float smooth = denormalize(kSmoothMin, kSmoothMax, params[kSmooth]); // milliseconds
	const float sm_o = powf(0.01f, 1000.f / (smooth * sampleRate));
	const float sm_n = 1 - sm_o;
	
	// ramp all the parameters, except constant ones and speaker thetas
	const int sourceParameters = kNumberOfSources * kParamsPerSource;
	const int speakerParameters = kNumberOfSpeakers * kParamsPerSpeakers;
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
	for (int o = 0; o < kNumberOfSpeakers; o++)
	{
		float *output = outputs[o];
		memset(output, 0, frames * sizeof(float));
	}
	

	Area areas[kMaxAreas];
	int areaCount = 0;
	
	if (kNumberOfSpeakers > 1)
	{
		for (int o = 0; o < kNumberOfSpeakers; o++)
		{
			float t = params[ParamForSpeakerX(o)];
			int left, right;
			float dLeft, dRight;
			findSpeakers(t, params, left, right, dLeft, dRight, o);
			assert(left >= 0 && right >= 0);
			assert(dLeft > 0 && dRight > 0);
			
			AddArea(o, t - dLeft, 0, t, 1, areas, areaCount);
			AddArea(o, t, 1, t + dRight, 0, areas, areaCount);
		}
	}
	else
	{
		AddArea(0, 0, 1, kThetaMax, 1, areas, areaCount);
	}
	assert(areaCount > 0);

	// compute
	// in this context: source T, R are actually source X, Y
	for (int i = 0; i < kNumberOfSources; i++)
	{
		float *input = inputs[i];
		float *input_x = mSmoothedParametersRamps.getReference(ParamForSourceX(i)).b;
		float *input_y = mSmoothedParametersRamps.getReference(ParamForSourceY(i)).b;
		float *input_d = mSmoothedParametersRamps.getReference(ParamForSourceD(i)).b;
	
		for (unsigned int f = 0; f < frames; f++)
		{
			float s = input[f];
			float x = input_x[f];
			float y = input_y[f];
			float d = input_d[f];
			
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
			
			assert(t >= 0 && t <= kThetaMax);
			assert(angle > 0 && angle <= kHalfCircle);
			
			float outFactors[kNumberOfSpeakers];
			memset(outFactors, 0, sizeof(outFactors));
			
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
			for (int o = 0; o < kNumberOfSpeakers; o++) total += outFactors[o];
			assert(total > 0);
			
			float adj = tv / total;
			for (int o = 0; o < kNumberOfSpeakers; o++)
				if (outFactors[o])
					addToOutput(s * outFactors[o] * adj, outputs, o, f);
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
	for (int o = 0; o < kNumberOfSpeakers; o++)
	{
		float *output = outputs[o];
		float *output_x = mSmoothedParametersRamps.getReference(ParamForSpeakerX(o)).b;
		float *output_y = mSmoothedParametersRamps.getReference(ParamForSpeakerY(o)).b;
		float output_adj[kChunkSize];
		{
			float *output_m = mSmoothedParametersRamps.getReference(ParamForSpeakerM(o)).b;
			float *output_a = mSmoothedParametersRamps.getReference(ParamForSpeakerA(o)).b;
			
			for (unsigned int f = 0; f < frames; f++)
			{
				float a = dbToLinear(output_a[f]);
				float m = 1 - output_m[f];
				output_adj[f] = a * m;
			}
		}
		
		for (int i = 0; i < kNumberOfSources; i++)
		{
			float *input = inputs[i];
			float *input_x = mSmoothedParametersRamps.getReference(ParamForSourceX(i)).b;
			float *input_y = mSmoothedParametersRamps.getReference(ParamForSourceY(i)).b;
			float *input_d = mSmoothedParametersRamps.getReference(ParamForSourceD(i)).b;
			
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

static const int kDataVersion = 9;
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
	
	appendFloatData(destData, mParameters[kLinkMovement]);
	appendFloatData(destData, mParameters[kSmooth]);
	appendFloatData(destData, mParameters[kVolumeNear]);
	appendFloatData(destData, mParameters[kVolumeMid]);
	appendFloatData(destData, mParameters[kVolumeFar]);
	appendFloatData(destData, mParameters[kFilterNear]);
	appendFloatData(destData, mParameters[kFilterMid]);
	appendFloatData(destData, mParameters[kFilterFar]);
	appendFloatData(destData, mParameters[kMaxSpanVolume]);
	
	for (int i = 0; i < kNumberOfSources; i++)
	{
		appendFloatData(destData, mParameters[ParamForSourceX(i)]);
		appendFloatData(destData, mParameters[ParamForSourceY(i)]);
		appendFloatData(destData, mParameters[ParamForSourceD(i)]);
	}
	for (int i = 0; i < kNumberOfSpeakers; i++)
	{
		appendFloatData(destData, mParameters[ParamForSpeakerX(i)]);
		appendFloatData(destData, mParameters[ParamForSpeakerY(i)]);
		appendFloatData(destData, mParameters[ParamForSpeakerA(i)]);
		appendFloatData(destData, mParameters[ParamForSpeakerM(i)]);
	}
}
void OctogrisAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
	//printf("Octogris: setStateInformation\n");
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
			if (version >= 9) mParameters.set(kMaxSpanVolume, readFloatData(data, sizeInBytes, normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault)));
			for (int i = 0; i < kNumberOfSources; i++)
			{
				mParameters.set(ParamForSourceX(i), readFloatData(data, sizeInBytes, 0));
				mParameters.set(ParamForSourceY(i), readFloatData(data, sizeInBytes, 0));
				mParameters.set(ParamForSourceD(i), readFloatData(data, sizeInBytes, normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance)));
			}
			for (int i = 0; i < kNumberOfSpeakers; i++)
			{
				mParameters.set(ParamForSpeakerX(i), readFloatData(data, sizeInBytes, 0));
				mParameters.set(ParamForSpeakerY(i), readFloatData(data, sizeInBytes, 0));
				mParameters.set(ParamForSpeakerA(i), readFloatData(data, sizeInBytes, normalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, kSpeakerDefaultAttenuation)));
				mParameters.set(ParamForSpeakerM(i), readFloatData(data, sizeInBytes, 0));
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
