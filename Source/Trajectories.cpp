/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 Trajectories.cpp
 Created: 3 Aug 2014 11:42:38am
 
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


#include "Trajectories.h"
#include "PluginProcessor.h"
#include "SourceMover.h"

// ==============================================================================
void Trajectory::start() {
     mMover->begin(mFilter->getSrcSelected(), kTrajectory);

    for (int i = 0; i < mFilter->getNumberOfSources(); i++){
        mSourcesInitialPositionRT.add(mFilter->getSourceRT(i));
    }
    
	mStarted = true;
}

bool Trajectory::process(float seconds, float beats) {
	if (mStopped) return true;
    if (!mStarted) {
            start();
    }
	if (mDone == mTotalDuration) {
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
    mMover->end(kTrajectory);
	mStopped = true;
}

Trajectory::Trajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source)
    :mFilter(filter)
    ,mMover(p_pMover)
	,mStarted(false)
	,mStopped(false)
	,mDone(0)
	,mDurationSingleTraj(duration)
	,mBeats(beats)
	,mSource(source)
{
	if (mDurationSingleTraj < 0.0001) mDurationSingleTraj = 0.0001;
	if (times < 0.0001) times = 0.0001;
	mTotalDuration = mDurationSingleTraj * times;
}

std::unique_ptr<vector<String>> Trajectory::getTrajectoryPossibleDirections(int p_iTrajectory){
    unique_ptr<vector<String>> vDirections (new vector<String>);
    
    switch(p_iTrajectory) {
        case Circle:
        case EllipseTr:
            vDirections->push_back("Clockwise");
            vDirections->push_back("Counter Clockwise");
            break;
        case Spiral:
            vDirections->push_back("In, Clockwise");
            vDirections->push_back("In, Counter Clockwise");
            vDirections->push_back("Out, Clockwise");
            vDirections->push_back("Out, Counter Clockwise");
            break;
        case Pendulum:
            vDirections->push_back("In");
            vDirections->push_back("Out");
            vDirections->push_back("Crossover");
            break;
        case AllTrajectoryTypes::Random:
        case RandomIndependent:
            vDirections->push_back("Slow");
            vDirections->push_back("Mid");
            vDirections->push_back("Fast");
            break;
        case RandomTarget:
        case SymXTarget:
        case SymYTarget:
        case ClosestSpeakerTarget:
            return nullptr;
            
        default:
            jassert(0);
    }
    
    return vDirections;
}

unique_ptr<AllTrajectoryDirections> Trajectory::getTrajectoryDirection(int p_iSelectedTrajectory, int p_iSelectedDirection){
    
    unique_ptr<AllTrajectoryDirections> pDirection (new AllTrajectoryDirections);
    
    switch (p_iSelectedTrajectory) {
            
        case Circle:
        case EllipseTr:
            *pDirection = static_cast<AllTrajectoryDirections>(p_iSelectedDirection);
            break;
        case Spiral:
            *pDirection = static_cast<AllTrajectoryDirections>(p_iSelectedDirection+5);
            break;
        case Pendulum:
            *pDirection = static_cast<AllTrajectoryDirections>(p_iSelectedDirection+2);
            break;
        case AllTrajectoryTypes::Random:
        case RandomIndependent:
            *pDirection = static_cast<AllTrajectoryDirections>(p_iSelectedDirection+9);
            break;
        case RandomTarget:
        case SymXTarget:
        case SymYTarget:
        case ClosestSpeakerTarget:
            *pDirection = None;
            break;
        default:
            jassert(0);
    }
    
    return pDirection;
}

std::unique_ptr<vector<String>> Trajectory::getTrajectoryPossibleReturns(int p_iTrajectory){
    
    unique_ptr<vector<String>> vReturns (new vector<String>);
    
    switch(p_iTrajectory) {
        case Circle:
        case EllipseTr:
        case AllTrajectoryTypes::Random:
        case RandomIndependent:
            return nullptr;
        case Spiral:
        case Pendulum:
            vReturns->push_back("One Way");
            vReturns->push_back("Return");
            break;
        case RandomTarget:
        case SymXTarget:
        case SymYTarget:
        case ClosestSpeakerTarget:
            return nullptr;
        default:
            jassert(0);
    }
    
    return vReturns;
}



// ==============================================================================
class CircleTrajectory : public Trajectory
{
public:
    CircleTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source, bool ccw, float p_fTurns)
    : Trajectory(filter, p_pMover, duration, beats, times, source)
    , mCCW(ccw)
    , m_fTurns(p_fTurns)
    {}
    
protected:
    void spProcess(float duration, float seconds) {
        float integralPart;
        float da = m_fTurns * mDone / mDurationSingleTraj;
        da = modf(da/m_fTurns, & integralPart) * m_fTurns;      //the modf makes da cycle back to 0 when it reaches m_fTurn, then we multiply it back by m_fTurn to undo the modification
        if (!mCCW) da = -da;
    
        FPoint p = mSourcesInitialPositionRT.getUnchecked(mFilter->getSrcSelected());
        mMover->move(mFilter->convertRt2Xy01(p.x, p.y+da*2*M_PI), kTrajectory);
    }
	
private:
	bool mCCW;
    float m_fTurns;
};

// ==============================================================================







// =================================================== FROM ZIRKOSC =============================================
//class SpiralTrajectory : public Trajectory
//{
//public:
//    SpiralTrajectory(ZirkOscAudioProcessor *filter, float duration, bool beats, float times, int source, bool ccw, bool rt, const std::pair<int, int> &endPoint, float fTurns)
//    : Trajectory(filter, duration, beats, times, source)
//    , mCCW(ccw)
//    , m_bRT(rt)
//    , m_fEndPair(endPoint)
//    , m_fTurns(fTurns)
//    { }
//    
//protected:
//    void spInit(){
//    }
//    void spProcess(float duration, float seconds){
//        float newAzimuth01, theta, integralPart; //integralPart is only a temp buffer
//        float newElevation01 = mDone / mDurationSingleTrajectory;
//        theta = modf(newElevation01, &integralPart);                                          //result from this modf is theta [0,1]
//        float fTranslationFactor = theta;
//        
//        //what we need is fCurStartElev01 (and Azim01) to transition from m_fTrajectoryInitialElevation01 to m_fTransposedStartElev01 as we go up the spiral, then back to start values as we go down the spiral
//        float fCurStartAzim01 = m_fTrajectoryInitialAzimuth01   ;//+ theta * (m_fTransposedStartAzim01 - m_fTrajectoryInitialAzimuth01);
//        float fCurStartElev01 = m_fTrajectoryInitialElevation01 ;//+ theta * (m_fTransposedStartElev01 - m_fTrajectoryInitialElevation01);
//        
//        //UP AND DOWN SPIRAL
//        if (m_bRT){
//            if (mIn){
//                newElevation01 = abs( (1 - fCurStartElev01) * sin(newElevation01 * M_PI) ) + fCurStartElev01;
//            } else {
//                JUCE_COMPILER_WARNING("mIn is always true; so either delete this or create another trajectory/mode for it")
//                newElevation01 = abs( fCurStartElev01 * cos(newElevation01 * M_PI) );  //only positive cos wave with phase _TrajectoriesPhi
//            }
//            if (theta > .5){
//                fTranslationFactor = 1 - theta;
//            }
//            theta *= 2;
//        } else {
//            //***** kinda like archimedian spiral r = a + b * theta , but azimuth does not reset at the top
//            newElevation01 = theta * (1 - fCurStartElev01) + fCurStartElev01;                     //newElevation is a mapping of theta[0,1] to [fCurStartElev01, 1]
//            if (!mIn){
//                newElevation01 = fCurStartElev01 * (1 - newElevation01) / (1-fCurStartElev01);    //map newElevation from [fCurStartElev01, 1] to [fCurStartElev01, 0]
//            }
//        }
//        
//        if (!mCCW){
//            theta = -theta;
//        }
//        
//        newAzimuth01 = modf(fCurStartAzim01 + m_fTurns * theta, &integralPart);                    //this is like adding a to theta
//        //convert newAzim+Elev to XY
//        float fNewX, fNewY;
//        SoundSource::azimElev01toXY(newAzimuth01, newElevation01, fNewX, fNewY);
//        //when return, theta goes from 0 to -2. otherwise 0 to -1, and cycles for every trajectory (not the sum of trajectories)
//        fNewX += modf(fTranslationFactor, &integralPart) * 2 * m_fEndPair.first;
//        fNewY += modf(fTranslationFactor, &integralPart) * 2 * m_fEndPair.second;
//        moveXY(fNewX, fNewY);
//    }
//    
//private:
//    bool mCCW, mIn = true;
//    bool m_bRT = false;
//    std::pair<float, float> m_fEndPair;
//    //    float m_fTransposedStartAzim01, m_fTransposedStartElev01;
//    float m_fTurns;
//};

class SpiralTrajectory : public Trajectory
{
public:
	SpiralTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source, bool ccw, bool in, bool rt, float p_fTurns)
	: Trajectory(filter, p_pMover, duration, beats, times, source)
    , mCCW(ccw)
    , mIn(in)
    , mRT(rt)
    , m_fTurns(p_fTurns)
    {
    }
	
protected:
    void spProcess(float duration, float seconds) {
        float da;
        if (mRT) {
            da = mDone / mDurationSingleTraj * (2 * M_PI);
        } else {
            if (mDone < mTotalDuration) da = fmodf(mDone / mDurationSingleTraj * M_PI, M_PI);
            else da = M_PI;
        }
        if (!mCCW) da = -da;
        
        FPoint p = mSourcesInitialPositionRT.getUnchecked(mFilter->getSrcSelected());
        float l = (cos(da)+1)*0.5;
        float r = mIn ? (p.x * l) : (p.x + (2 - p.x) * (1 - l));
        float t = p.y + m_fTurns*2*da;
        mMover->move(mFilter->convertRt2Xy01(r, t), kTrajectory);
    }
	
private:
	bool mCCW, mIn, mRT;
    float m_fTurns;
};

// =================================================== FROM ZIRKOSC =============================================
//class PendulumTrajectory2 : public Trajectory
//{
//public:
//    PendulumTrajectory2(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source, bool ccw,
//                       bool rt,  const std::pair<float, float> &endPoint, float fDeviation, float p_fDampening)
//    :Trajectory(filter, p_pMover, duration, beats, times, source)
//    ,mCCW(ccw)
//    ,m_bRT(rt)
//    ,m_fEndPair(endPoint)
//    ,m_fDeviation(fDeviation/360)
//    ,m_fTotalDampening(p_fDampening)
//    {
//    
//        m_fStartPair.first = filter->getParameter(ZirkOscAudioProcessor::ZirkOSC_X_ParamId + m_iSelectedSourceForTrajectory*5);
//        m_fStartPair.first = m_fStartPair.first*2*ZirkOscAudioProcessor::s_iDomeRadius - ZirkOscAudioProcessor::s_iDomeRadius;
//        m_fStartPair.second = filter->getPa rameter(ZirkOscAudioProcessor::ZirkOSC_Y_ParamId + m_iSelectedSourceForTrajectory*5);
//        m_fStartPair.second = m_fStartPair.second*2*ZirkOscAudioProcessor::s_iDomeRadius - ZirkOscAudioProcessor::s_iDomeRadius;
//    }
//    
//protected:
//    void spInit() {
//        if (m_fEndPair.first != m_fStartPair.first){
//            m_bYisDependent = true;
//            m_fM = (m_fEndPair.second - m_fStartPair.second) / (m_fEndPair.first - m_fStartPair.first);
//            m_fB = m_fStartPair.second - m_fM * m_fStartPair.first;
//        } else {
//            m_bYisDependent = false;
//            m_fM = 0;
//            m_fB = m_fStartPair.first;
//        }
//    }
//    void spProcess(float duration, float seconds) {
//        
//        int iReturn = m_bRT ? 2:1;
//        float fCurDampening = m_fTotalDampening * mDone / (mDurationSingleTrajectory * m_dTrajectoryCount);
//        //pendulum part
//        float newX, newY, temp, fCurrentProgress = modf((mDone / mDurationSingleTrajectory), &temp);
//        
//        if (m_bYisDependent){
//            fCurrentProgress = (m_fEndPair.first - m_fStartPair.first) * (1-cos(fCurrentProgress * iReturn * M_PI)) / 2;
//            newX = m_fStartPair.first + fCurrentProgress;
//            newY = m_fM * newX + m_fB;
//        } else {
//            fCurrentProgress = (m_fEndPair.second - m_fStartPair.second) * (1-cos(fCurrentProgress * iReturn * M_PI)) / 2;
//            newX = m_fStartPair.first;
//            newY = m_fStartPair.second + fCurrentProgress;
//        }
//        newX = newX - newX*fCurDampening;
//        newY = newY - newY*fCurDampening;
//        float fPendulumAzim = SoundSource::XYtoAzim01(newX, newY);
//        float fPendulumElev = SoundSource::XYtoElev01(newX, newY);
//        
//        //circle part
//        float newAzimuth, integralPart;
//        newAzimuth = mDone / (mDurationSingleTrajectory * m_dTrajectoryCount);
//        newAzimuth = modf(newAzimuth, &integralPart);
//        if (!mCCW) {
//            newAzimuth = - newAzimuth;
//        }
//        
//        newAzimuth *= m_fDeviation;
//        move(fPendulumAzim + newAzimuth, fPendulumElev);
//    }
//private:
//    bool mCCW, m_bRT, m_bYisDependent;
//    std::pair<float, float> m_fStartPair;
//    std::pair<float, float> m_fEndPair;
//    float m_fM;
//    float m_fB;
//    float m_fDeviation;
//    float m_fTotalDampening;
//};

class PendulumTrajectory : public Trajectory
{
public:
	PendulumTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source, bool in, bool rt, bool cross, float p_fDampening, float p_fDeviation)
	: Trajectory(filter, p_pMover, duration, beats, times, source)
    , mIn(in)
    , mRT(rt)
    , mCross(cross)
    , m_fTotalDampening(p_fDampening)
    , m_fTotalDeviation(p_fDeviation)
    {
    }
	
protected:
	void spProcess(float duration, float seconds){
        float da;
        float fCurDampening = m_fTotalDampening * mDone / mTotalDuration;
        if (mRT){
            da = mDone / mDurationSingleTraj * (2 * M_PI);
        } else {
            if (mDone < mTotalDuration){
                da = fmodf(mDone / mDurationSingleTraj * M_PI, M_PI);
            } else {
                da = M_PI;
            }
        }
        
        FPoint p = mSourcesInitialPositionRT.getUnchecked(mFilter->getSrcSelected());
        float l = mCross ? cos(da) : (cos(da)+1)*0.5;
        float r = (mCross || mIn) ? (p.x * l) : (p.x + (2 - p.x) * (1 - l));
        
        //r is the ray, unclear what l is
        r -= fCurDampening * r;
        
        //circle/deviation part
        float deviationAngle, integralPart;
        deviationAngle = mDone / mTotalDuration;
        deviationAngle = modf(deviationAngle, &integralPart);
        if (!mCCW) {
            deviationAngle = - deviationAngle;
        }
        deviationAngle *= 2 * M_PI * m_fTotalDeviation;
        
        mMover->move(mFilter->convertRt2Xy01(r, p.y + deviationAngle), kTrajectory);
}
	
private:
	bool mIn, mRT, mCross, mCCW = true;
    float m_fTotalDampening, m_fTotalDeviation;
};

// ==============================================================================
class EllipseTrajectory : public Trajectory
{
public:
	EllipseTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source, bool ccw, float p_fTurns)
	: Trajectory(filter, p_pMover, duration, beats, times, source)
    , mCCW(ccw)
    , m_fTurns(p_fTurns)
    {}
	
protected:
	void spProcess(float duration, float seconds)
	{
        float integralPart;
        float da = m_fTurns * mDone / mDurationSingleTraj;
        da = modf(da/m_fTurns, & integralPart) * m_fTurns*2*M_PI;      //the modf makes da cycle back to 0 when it reaches m_fTurn, then we multiply it back by m_fTurn to undo the modification

        if (!mCCW) da = -da;
        // http://www.edmath.org/MATtours/ellipses/ellipses1.07.3.html
        FPoint p = mSourcesInitialPositionRT.getUnchecked(mFilter->getSrcSelected());
        float a = 1;
        float b = 0.5;
        float cosDa = cos(da);
        float a2 = a*a;
        float b2 = b*b;
        float cosDa2 = cosDa*cosDa;
        float r2 = (a2*b2)/((b2-a2)*cosDa2+a2);
        float r = sqrt(r2);
        mMover->move(mFilter->convertRt2Xy01(p.x * r, p.y + da), kTrajectory);
    }
	
private:
	bool mCCW;
    float m_fTurns;
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
	RandomTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source, float speed, bool bUniqueTarget)
	: Trajectory(filter, p_pMover, duration, beats, times, source)
    , mClock(0)
    , mSpeed(speed)
    , mUniqueTarget(bUniqueTarget)
    {}
	
protected:
    void spProcess(float duration, float seconds){
        if (fmodf(mDone, mDurationSingleTraj) < 0.01){
            mFilter->restoreCurrentLocations(mFilter->getSrcSelected());
        }
        mClock += seconds;
        while(mClock > 0.01){
            mClock -= 0.01;
            
            float rand1 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
            float rand2 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
            
            FPoint p = mFilter->getSourceXY(mFilter->getSrcSelected());

            p.x += (rand1 - 0.5) * mSpeed;
            p.y += (rand2 - 0.5) * mSpeed;
            //convert ±radius range to 01 range
            p.x = (p.x + kRadiusMax) / (2*kRadiusMax);
            p.y = (p.y + kRadiusMax) / (2*kRadiusMax);
            mMover->move(p, kTrajectory);
        }
    }
    
private:
    MTRand_int32 mRNG;
    float mClock;
    float mSpeed;
    bool mUniqueTarget;
};

class RandomIndependentTrajectory : public Trajectory
{
public:
    RandomIndependentTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source, float speed)
    : Trajectory(filter, p_pMover, duration, beats, times, source)
    , mClock(0)
    , mSpeed(speed)
    {}
    
protected:
    void spProcess(float duration, float seconds){

        for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); ++iCurSrc){
            if (fmodf(mDone, mDurationSingleTraj) < 0.01){
                mFilter->restoreCurrentLocations(iCurSrc);
            }
            mClock += seconds;
            while(mClock > 0.01){
                mClock -= 0.01;
                
                float rand1 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
                float rand2 = mRNG.rand_uint32() / (float)0xFFFFFFFF;
                
                FPoint p = mFilter->getSourceXY(iCurSrc);
                
                p.x += (rand1 - 0.5) * mSpeed;
                p.y += (rand2 - 0.5) * mSpeed;
                //convert ±radius range to 01 range
                p.x = (p.x + kRadiusMax) / (2*kRadiusMax);
                p.y = (p.y + kRadiusMax) / (2*kRadiusMax);
                
                mFilter->setSourceXY01(iCurSrc, p);
                mFilter->mOldSrcLocRT[iCurSrc] = mFilter->convertXy012Rt(p);
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
	TargetTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source)
	: Trajectory(filter, p_pMover, duration, beats, times, source), mCycle(-1) {}
	
protected:
	virtual FPoint destinationForSource(int s, FPoint o) = 0;
    virtual void resetIfRandomTarget(){};

	void spProcess(float duration, float seconds)
	{
		float p = mDone / mDurationSingleTraj;
		
		int cycle = (int)p;
		if (mCycle != cycle) {
            resetIfRandomTarget();
			mCycle = cycle;
			mSourcesOrigins.clearQuick();
			mSourcesDestinations.clearQuick();
            
			for (int i = 0; i < mFilter->getNumberOfSources(); i++)
			if (mSource < 0 || mSource == i)
			{
                FPoint o = mFilter->getSourceXY(i);
				mSourcesOrigins.add(o);
                mSourcesDestinations.add(destinationForSource(i, o));
                //mSourcesDestinations.add(uniqueDestination);
			}
		}

		float d = fmodf(p, 1);
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
		if (mSource < 0 || mSource == i)
		{
			FPoint a = mSourcesOrigins.getUnchecked(i);
			FPoint b = mSourcesDestinations.getUnchecked(i);
			FPoint p = a + (b - a) * d;
            JUCE_COMPILER_WARNING("i don't understand what this means, how this works, and why we're not using the mover in this trajectory")
            bool bWriteAutomation = (i == 0) ? true : false;
			mFilter->setSourceXY(i, p, bWriteAutomation);
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
	RandomTargetTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source)
	: TargetTrajectory(filter, p_pMover, duration, beats, times, source) {}
	
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
    
    void resetIfRandomTarget(){
        mFilter->restoreCurrentLocations(mFilter->getSrcSelected());
    }
	
private:
	MTRand_int32 mRNG;
};

// ==============================================================================
class SymXTargetTrajectory : public TargetTrajectory
{
public:
	SymXTargetTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source)
	: TargetTrajectory(filter, p_pMover, duration, beats, times, source) {}
	
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
	SymYTargetTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source)
	: TargetTrajectory(filter, p_pMover, duration, beats, times, source) {}
	
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
	ClosestSpeakerTargetTrajectory(OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats, float times, int source)
	: TargetTrajectory(filter, p_pMover, duration, beats, times, source) {}
	
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


int Trajectory::NumberOfTrajectories() { return TotalNumberTrajectories-1; }

String Trajectory::GetTrajectoryName(int i)
{
    switch(i)
    {
        case Circle: return "Circle";
        case EllipseTr: return "Ellipse";
        case Spiral: return "Spiral";
        case Pendulum: return "Pendulum";
        case AllTrajectoryTypes::Random: return "Random";
        case RandomIndependent: return "Random(Independent)";
        case RandomTarget: return "Random Target";
        case SymXTarget: return "Sym X Target";
        case SymYTarget: return "Sym Y Target";
        case ClosestSpeakerTarget: return "Closest Speaker Target";
    }
    jassert(0);
    return "";
}

Trajectory::Ptr Trajectory::CreateTrajectory(int type, OctogrisAudioProcessor *filter, SourceMover *p_pMover, float duration, bool beats,
                                             AllTrajectoryDirections direction, bool bReturn, float times, int source, bool bUniqueTarget, float p_fDampening, float p_fDeviation, float p_fTurns)
{
    
    bool ccw, in, cross;
    float speed;
    
    if (direction != None)
    switch (direction) {
        case CW:
            ccw = false;
            break;
        case CCW:
            ccw = true;
            break;
        case In:
            in = true;
            cross = false;
            break;
        case Out:
            in = false;
            cross = false;
            break;
        case Crossover:
            in = true;
            cross = true;
            break;
        case InCW:
            in = true;
            ccw = false;
            break;
        case InCCW:
            in = true;
            ccw = true;
            break;
        case OutCW:
            in = false;
            ccw = false;
            break;
        case OutCCW:
            in = false;
            ccw = true;
            break;
        case Slow:
            speed = .02;
            break;
        case Mid:
            speed = .06;
            break;
        case Fast:
            speed = .1;
            break;
        default:
            break;
    }
    
    
    switch(type)
    {
        case Circle:                     return new CircleTrajectory(filter, p_pMover, duration, beats, times, source, ccw, p_fTurns);
        case EllipseTr:                  return new EllipseTrajectory(filter, p_pMover, duration, beats, times, source, ccw, p_fTurns);
        case Spiral:                     return new SpiralTrajectory(filter, p_pMover, duration, beats, times, source, ccw, in, bReturn, p_fTurns);
        case Pendulum:                   return new PendulumTrajectory(filter, p_pMover, duration, beats, times, source, in, bReturn, cross, p_fDampening, p_fDeviation);
        case AllTrajectoryTypes::Random: return new RandomTrajectory(filter, p_pMover, duration, beats, times, source, speed, bUniqueTarget);
        case RandomIndependent:          return new RandomIndependentTrajectory(filter, p_pMover, duration, beats, times, source, speed);
        case RandomTarget:               return new RandomTargetTrajectory(filter, p_pMover, duration, beats, times, source);
        case SymXTarget:                 return new SymXTargetTrajectory(filter, p_pMover, duration, beats, times, source);
        case SymYTarget:                 return new SymYTargetTrajectory(filter, p_pMover, duration, beats, times, source);
        case ClosestSpeakerTarget:       return new ClosestSpeakerTargetTrajectory(filter, p_pMover, duration, beats, times, source);
    }
    jassert(0);
    return NULL;
}