/*
  ==============================================================================

    Trajectories.cpp
    Created: 3 Aug 2014 11:42:38am
    Author:  makira

  ==============================================================================
*/

#include "Trajectories.h"
#include "PluginProcessor.h"

// ==============================================================================
void Trajectory::start()
{
	if (mSource < 0)
	{
		for (int j = 0; j < mFilter->getNumberOfSources(); j++)
		{
			mFilter->beginParameterChangeGesture(mFilter->getParamForSourceX(j));
			mFilter->beginParameterChangeGesture(mFilter->getParamForSourceY(j));
		}
	}
	else
	{
		mFilter->beginParameterChangeGesture(mFilter->getParamForSourceX(mSource));
		mFilter->beginParameterChangeGesture(mFilter->getParamForSourceY(mSource));
	}
	spInit();
	mStarted = true;
}

bool Trajectory::process(float seconds, float beats)
{
	if (mStopped) return true;
	if (!mStarted) start();
	if (mDone == mTotalDuration)
	{
		spProcess(0, 0);
		stop();
		return true;
	}

	float duration = mBeats ? beats : seconds;
	spProcess(duration, seconds);
	
	mDone += duration;
	if (mDone > mTotalDuration)
		mDone = mTotalDuration;
	
	return false;
}

float Trajectory::progress()
{
	return mDone / mTotalDuration;
}

void Trajectory::stop()
{
	if (!mStarted || mStopped) return;
	if (mSource < 0)
	{
		for (int j = 0; j < mFilter->getNumberOfSources(); j++)
		{
			mFilter->endParameterChangeGesture(mFilter->getParamForSourceX(j));
			mFilter->endParameterChangeGesture(mFilter->getParamForSourceY(j));
		}
	}
	else
	{
		mFilter->endParameterChangeGesture(mFilter->getParamForSourceX(mSource));
		mFilter->endParameterChangeGesture(mFilter->getParamForSourceY(mSource));
	}
	mStopped = true;
}

Trajectory::Trajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source)
:
	mFilter(filter),
	mStarted(false),
	mStopped(false),
	mDone(0),
	mDuration(duration),
	mBeats(beats),
	mSource(source)
{
	if (mDuration < 0.0001) mDuration = 0.0001;
	if (times < 0.0001) times = 0.0001;
	mTotalDuration = mDuration * times;
}

// ==============================================================================
class CircleTrajectory : public Trajectory
{
public:
	CircleTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source, bool ccw)
	: Trajectory(filter, duration, beats, times, source), mCCW(ccw) {}
	
protected:
	void spInit()
	{
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
			mSourcesInitRT.add(mFilter->getSourceRT(i));
	}
	void spProcess(float duration, float seconds)
	{
		float da = mDone / mDuration * (2 * M_PI);
		if (!mCCW) da = -da;
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
		if (mSource < 0 || mSource == i)
		{
			FPoint p = mSourcesInitRT.getUnchecked(i);
			mFilter->setSourceRT(i, FPoint(p.x, p.y + da));
		}
	}
	
private:
	Array<FPoint> mSourcesInitRT;
	bool mCCW;
};

// ==============================================================================
class SpiralTrajectory : public Trajectory
{
public:
	SpiralTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source, bool ccw, bool in, bool rt)
	: Trajectory(filter, duration, beats, times, source), mCCW(ccw), mIn(in), mRT(rt) {}
	
protected:
	void spInit()
	{
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
			mSourcesInitRT.add(mFilter->getSourceRT(i));
	}
	void spProcess(float duration, float seconds)
	{
		float da;
		if (mRT)
			da = mDone / mDuration * (2 * M_PI);
		else
		{
			if (mDone < mTotalDuration) da = fmodf(mDone / mDuration * M_PI, M_PI);
			else da = M_PI;
		}
		if (!mCCW) da = -da;
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
		if (mSource < 0 || mSource == i)
		{
			FPoint p = mSourcesInitRT.getUnchecked(i);
			float l = (cos(da)+1)*0.5;
			float r = mIn ? (p.x * l) : (p.x + (2 - p.x) * (1 - l));
			float t = p.y + da;
			mFilter->setSourceRT(i, FPoint(r, t));
		}
	}
	
private:
	Array<FPoint> mSourcesInitRT;
	bool mCCW, mIn, mRT;
};

// ==============================================================================
class PendulumTrajectory : public Trajectory
{
public:
	PendulumTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source, bool in, bool rt)
	: Trajectory(filter, duration, beats, times, source), mIn(in), mRT(rt) {}
	
protected:
	void spInit()
	{
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
			mSourcesInitRT.add(mFilter->getSourceRT(i));
	}
	void spProcess(float duration, float seconds)
	{
		float da;
		if (mRT)
			da = mDone / mDuration * (2 * M_PI);
		else
		{
			if (mDone < mTotalDuration) da = fmodf(mDone / mDuration * M_PI, M_PI);
			else da = M_PI;
		}
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
		if (mSource < 0 || mSource == i)
		{
			FPoint p = mSourcesInitRT.getUnchecked(i);
			float l = (mRT && mIn) ? cos(da) : (cos(da)+1)*0.5;
			float r = mIn ? (p.x * l) : (p.x + (2 - p.x) * (1 - l));
			mFilter->setSourceRT(i, FPoint(r, p.y));
		}
	}
	
private:
	Array<FPoint> mSourcesInitRT;
	bool mIn, mRT;
};

// ==============================================================================
class EllipseTrajectory : public Trajectory
{
public:
	EllipseTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source, bool ccw)
	: Trajectory(filter, duration, beats, times, source), mCCW(ccw) {}
	
protected:
	void spInit()
	{
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
			mSourcesInitRT.add(mFilter->getSourceRT(i));
	}
	void spProcess(float duration, float seconds)
	{
		float da = mDone / mDuration * (2 * M_PI);
		if (!mCCW) da = -da;
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
		if (mSource < 0 || mSource == i)
		{
			FPoint p = mSourcesInitRT.getUnchecked(i);
			
			// http://www.edmath.org/MATtours/ellipses/ellipses1.07.3.html
			float a = 1;
			float b = 0.5;
			float cosDa = cos(da);
			float a2 = a*a;
			float b2 = b*b;
			float cosDa2 = cosDa*cosDa;
			float r2 = (a2*b2)/((b2-a2)*cosDa2+a2);
			float r = sqrt(r2);
			
			mFilter->setSourceRT(i, FPoint(p.x * r, p.y + da));
		}
	}
	
private:
	Array<FPoint> mSourcesInitRT;
	bool mCCW;
};

// ==============================================================================
// Mersenne Twister random number generator, this is now included in c++11, see here: http://en.cppreference.com/w/cpp/numeric/random
class MTRand_int32
{
public:
	MTRand_int32()
	{
		seed(rand());
	}
	uint32_t rand_uint32()
	{
		if (p == n) gen_state();
		unsigned long x = state[p++];
		x ^= (x >> 11);
		x ^= (x << 7) & 0x9D2C5680;
		x ^= (x << 15) & 0xEFC60000;
		return x ^ (x >> 18);
	}
	void seed(uint32_t s)
	{
		state[0] = s;
		for (int i = 1; i < n; ++i)
			state[i] = 1812433253 * (state[i - 1] ^ (state[i - 1] >> 30)) + i;

		p = n; // force gen_state() to be called for next random number
	}

private:
	static const int n = 624, m = 397;
	int p;
	unsigned long state[n];
	unsigned long twiddle(uint32_t u, uint32_t v)
	{
		return (((u & 0x80000000) | (v & 0x7FFFFFFF)) >> 1) ^ ((v & 1) * 0x9908B0DF);
	}
	void gen_state()
	{
		for (int i = 0; i < (n - m); ++i)
			state[i] = state[i + m] ^ twiddle(state[i], state[i + 1]);
		for (int i = n - m; i < (n - 1); ++i)
			state[i] = state[i + m - n] ^ twiddle(state[i], state[i + 1]);
		state[n - 1] = state[m - 1] ^ twiddle(state[n - 1], state[0]);
		
		p = 0; // reset position
	}
	// make copy constructor and assignment operator unavailable, they don't make sense
	MTRand_int32(const MTRand_int32&);
	void operator=(const MTRand_int32&);
};
class RandomTrajectory : public Trajectory
{
public:
	RandomTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source, float speed)
	: Trajectory(filter, duration, beats, times, source), mClock(0), mSpeed(speed) {}
	
protected:
	void spProcess(float duration, float seconds)
	{
		mClock += seconds;
		while(mClock > 0.01)
		{
			mClock -= 0.01;
			for (int i = 0; i < mFilter->getNumberOfSources(); i++)
			if (mSource < 0 || mSource == i)
			{
				FPoint p = mFilter->getSourceXY(i);
				float r1 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
				float r2 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
				p.x += (r1 - 0.5) * mSpeed;
				p.y += (r2 - 0.5) * mSpeed;
				mFilter->setSourceXY(i, p);
			}
		}
	}
	
private:
	MTRand_int32 mRNG;
	float mClock;
	float mSpeed;
};

// ==============================================================================
class TargetTrajectory : public Trajectory
{
public:
	TargetTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source)
	: Trajectory(filter, duration, beats, times, source), mCycle(-1) {}
	
protected:
	virtual FPoint destinationForSource(int s, FPoint o) = 0;

	void spProcess(float duration, float seconds)
	{
		float p = mDone / mDuration;
		
		int cycle = (int)p;
		if (mCycle != cycle)
		{
			mCycle = cycle;
			mSourcesOrigins.clearQuick();
			mSourcesDestinations.clearQuick();
			
			for (int i = 0; i < mFilter->getNumberOfSources(); i++)
			if (mSource < 0 || mSource == i)
			{
				FPoint o = mFilter->getSourceXY(i);
				mSourcesOrigins.add(o);
				mSourcesDestinations.add(destinationForSource(i, o));
			}
		}

		float d = fmodf(p, 1);
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
		if (mSource < 0 || mSource == i)
		{
			FPoint a = mSourcesOrigins.getUnchecked(i);
			FPoint b = mSourcesDestinations.getUnchecked(i);
			FPoint p = a + (b - a) * d;
			mFilter->setSourceXY(i, p);
		}
	}
	
private:
	Array<FPoint> mSourcesOrigins;
	Array<FPoint> mSourcesDestinations;
	int mCycle;
};


// ==============================================================================
class RandomTargetTrajectory : public TargetTrajectory
{
public:
	RandomTargetTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source)
	: TargetTrajectory(filter, duration, beats, times, source) {}
	
protected:
	FPoint destinationForSource(int s, FPoint o)
	{
		float r1 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
		float r2 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
		float x = r1 * (kRadiusMax*2) - kRadiusMax;
		float y = r2 * (kRadiusMax*2) - kRadiusMax;
		float r = hypotf(x, y);
		if (r > kRadiusMax)
		{
			float c = kRadiusMax/r;
			x *= c;
			y *= c;
		}
		return FPoint(x,y);
	}
	
private:
	MTRand_int32 mRNG;
};

// ==============================================================================
class SymXTargetTrajectory : public TargetTrajectory
{
public:
	SymXTargetTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source)
	: TargetTrajectory(filter, duration, beats, times, source) {}
	
protected:
	FPoint destinationForSource(int s, FPoint o)
	{
		return FPoint(o.x,-o.y);
	}
};

// ==============================================================================
class SymYTargetTrajectory : public TargetTrajectory
{
public:
	SymYTargetTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source)
	: TargetTrajectory(filter, duration, beats, times, source) {}
	
protected:
	FPoint destinationForSource(int s, FPoint o)
	{
		return FPoint(-o.x,o.y);
	}
};

// ==============================================================================
class ClosestSpeakerTargetTrajectory : public TargetTrajectory
{
public:
	ClosestSpeakerTargetTrajectory(OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source)
	: TargetTrajectory(filter, duration, beats, times, source) {}
	
protected:
	FPoint destinationForSource(int s, FPoint o)
	{
		int bestSpeaker = 0;
		float bestDistance = o.getDistanceFrom(mFilter->getSpeakerXY(0));
		
		for (int i = 1; i < mFilter->getNumberOfSpeakers(); i++)
		{
			float d = o.getDistanceFrom(mFilter->getSpeakerXY(i));
			if (d < bestDistance)
			{
				bestSpeaker = i;
				bestDistance = d;
			}
		}
		
		return mFilter->getSpeakerXY(bestSpeaker);
	}
};

// ==============================================================================
int Trajectory::NumberOfTrajectories() { return 23; }
String Trajectory::GetTrajectoryName(int i)
{
	switch(i)
	{
		case 0: return "Circle (CW)";
		case 1: return "Circle (CCW)";
		case 2: return "Ellipse (CW)";
		case 3: return "Ellipse (CCW)";
		case 4: return "Spiral (In, RT, CW)";
		case 5: return "Spiral (In, RT, CCW)";
		case 6: return "Spiral (Out, RT, CW)";
		case 7: return "Spiral (Out, RT, CCW)";
		case 8: return "Spiral (In, OW, CW)";
		case 9: return "Spiral (In, OW, CCW)";
		case 10: return "Spiral (Out, OW, CW)";
		case 11: return "Spiral (Out, OW, CCW)";
		case 12: return "Pendulum (In, RT)";
		case 13: return "Pendulum (Out, RT)";
		case 14: return "Pendulum (In, OW)";
		case 15: return "Pendulum (Out, OW)";
		case 16: return "Random Target";
		case 17: return "Random (Slow)";
		case 18: return "Random (Mid)";
		case 19: return "Random (Fast)";
		case 20: return "Sym X Target";
		case 21: return "Sym Y Target";
		case 22: return "Closest Speaker Target";
	}
	jassert(0);
	return "";
}
Trajectory::Ptr Trajectory::CreateTrajectory(int i, OctogrisAudioProcessor *filter, float duration, bool beats, float times, int source)
{
	switch(i)
	{
		case 0: return new CircleTrajectory(filter, duration, beats, times, source, false);
		case 1: return new CircleTrajectory(filter, duration, beats, times, source, true);
		case 2: return new EllipseTrajectory(filter, duration, beats, times, source, false);
		case 3: return new EllipseTrajectory(filter, duration, beats, times, source, true);
		case 4: return new SpiralTrajectory(filter, duration, beats, times, source, false, true, true);
		case 5: return new SpiralTrajectory(filter, duration, beats, times, source, true, true, true);
		case 6: return new SpiralTrajectory(filter, duration, beats, times, source, false, false, true);
		case 7: return new SpiralTrajectory(filter, duration, beats, times, source, true, false, true);
		case 8: return new SpiralTrajectory(filter, duration, beats, times, source, false, true, false);
		case 9: return new SpiralTrajectory(filter, duration, beats, times, source, true, true, false);
		case 10: return new SpiralTrajectory(filter, duration, beats, times, source, false, false, false);
		case 11: return new SpiralTrajectory(filter, duration, beats, times, source, true, false, false);
		case 12: return new PendulumTrajectory(filter, duration, beats, times, source, true, true);
		case 13: return new PendulumTrajectory(filter, duration, beats, times, source, false, true);
		case 14: return new PendulumTrajectory(filter, duration, beats, times, source, true, false);
		case 15: return new PendulumTrajectory(filter, duration, beats, times, source, false, false);
		case 16: return new RandomTargetTrajectory(filter, duration, beats, times, source);
		case 17: return new RandomTrajectory(filter, duration, beats, times, source, 0.02);
		case 18: return new RandomTrajectory(filter, duration, beats, times, source, 0.06);
		case 19: return new RandomTrajectory(filter, duration, beats, times, source, 0.1);
		case 20: return new SymXTargetTrajectory(filter, duration, beats, times, source);
		case 21: return new SymYTargetTrajectory(filter, duration, beats, times, source);
		case 22: return new ClosestSpeakerTargetTrajectory(filter, duration, beats, times, source);
	}
	jassert(0);
	return NULL;
}





