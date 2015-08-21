/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 SourceMover.cpp
 Created: 8 Aug 2014 1:04:53pm
 
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

#include "SourceMover.h"


SourceMover::SourceMover(OctogrisAudioProcessor *filter)
: mFilter(filter)
 ,mMoverType(kVacant)
 ,mSelectedSrc(0)
{
    updateNumberOfSources();
}

void SourceMover::updateNumberOfSources(){
    mSourcesDownXY.clear();
    mSourcesDownRT.clear();
    mSourcesAngularOrder.clear();
    
    mSourcesDownXY.ensureStorageAllocated(mFilter->getNumberOfSources());
    mSourcesDownRT.ensureStorageAllocated(mFilter->getNumberOfSources());
    mSourcesAngularOrder.ensureStorageAllocated(mFilter->getNumberOfSources());
    
    for (int i = 0; i < mFilter->getNumberOfSources(); i++)
    {
        mSourcesDownXY.add(FPoint(0,0));
        mSourcesDownRT.add(FPoint(0,0));
        mSourcesAngularOrder.add(0);
    }
}



void SourceMover::begin(int s, MoverType mt)
{
	if (mMoverType != kVacant) return;
	mMoverType = mt;
	mSelectedSrc = s;
    mFilter->setSelectedSource(s);
	
    if (mMoverType != kSourceThread){
        mFilter->setIsRecordingAutomation(true);
        mFilter->beginParameterChangeGesture(mFilter->getParamForSourceX(mSelectedSrc));
        mFilter->beginParameterChangeGesture(mFilter->getParamForSourceY(mSelectedSrc));
    }
    
    int iNbrSrc = mFilter->getNumberOfSources();

    //if we are not in independent mode and have more than 1 source
	if (mFilter->getMovementMode() != 0 && mFilter->getNumberOfSources() > 1) {
		for (int j = 0; j < iNbrSrc; j++) {
			mSourcesDownRT.setUnchecked(j, mFilter->getSourceRT(j));
			mSourcesDownXY.setUnchecked(j, mFilter->getSourceXY(j));
            if (j == mSelectedSrc) continue;
            if (mMoverType != kSourceThread && !s_bUseOneSource){
                mFilter->beginParameterChangeGesture(mFilter->getParamForSourceX(j));
                mFilter->beginParameterChangeGesture(mFilter->getParamForSourceY(j));
            }
		}
		
        //if we are in a circular-y mode, figure out the angular order
        JUCE_COMPILER_WARNING("still necessary after instantanous change of constraint, unclear why. without it, circular same angle, move unselected source messes things up")
		if	(iNbrSrc > 1 && (mFilter->getMovementMode() == 3 || mFilter->getMovementMode() == 4)) {
            sortAngles();
		}
	}
}

void SourceMover::sortAngles(){
    
    int iNbrSrc = mFilter->getNumberOfSources();
    
    IndexedAngle * ia = new IndexedAngle[iNbrSrc];
    
    for (int j = 0; j < iNbrSrc; j++) {
        ia[j].i = j;
        ia[j].a = mFilter->getSourceRT(j).y;
    }
    
    qsort(ia, iNbrSrc, sizeof(IndexedAngle), IndexedAngleCompare);
    
    int b;
    for (b = 0; b < iNbrSrc && ia[b].i != mSelectedSrc; b++) ;
    
    if (b == iNbrSrc) {
        printf("error!\n");
        b = 0;
    }
    
    for (int j = 1; j < iNbrSrc; j++) {
        int o = (b + j) % iNbrSrc;
        o = ia[o].i;
        mSourcesAngularOrder.set(o, (M_PI * 2. * j) / iNbrSrc);
    }
    
    delete[] ia;
}

void SourceMover::setEqualRadius(){
    FPoint selSrcRT = mFilter->getSourceRT(mSelectedSrc);
    for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); iCurSrc++) {
        if (iCurSrc == mSelectedSrc){
            continue;
        }
        FPoint curSrcRT = mFilter->getSourceRT(iCurSrc);
        curSrcRT.x = selSrcRT.x;
        if (curSrcRT.y < 0) curSrcRT.y += kThetaMax;
        if (curSrcRT.y > kThetaMax) curSrcRT.y -= kThetaMax;
        mFilter->setPreventSourceLocationUpdate(true);
        mFilter->setSourceRT(iCurSrc, curSrcRT, !s_bUseOneSource);  //this call needs to NOT triger a call to processor::setSourceLocationChanged
        mFilter->mOldSrcLocRT[iCurSrc] = curSrcRT;
        mFilter->setPreventSourceLocationUpdate(false);
    }
}

void SourceMover::setEqualAngles(){
    //first figure out the correct angles
    sortAngles();
    
    //then set them
    FPoint selSrcRT = mFilter->getSourceRT(mSelectedSrc);
    for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); iCurSrc++) {
        if (iCurSrc == mSelectedSrc){
            continue;
        }
        FPoint curSrcRT = mFilter->getSourceRT(iCurSrc);
        curSrcRT.y = selSrcRT.y + mSourcesAngularOrder[iCurSrc];
        if (curSrcRT.x < 0) curSrcRT.x = 0;
        if (curSrcRT.x > kRadiusMax) curSrcRT.x = kRadiusMax;
        if (curSrcRT.y < 0) curSrcRT.y += kThetaMax;
        if (curSrcRT.y > kThetaMax) curSrcRT.y -= kThetaMax;
        mFilter->setPreventSourceLocationUpdate(true);
        mFilter->setSourceRT(iCurSrc, curSrcRT, !s_bUseOneSource);
        mFilter->mOldSrcLocRT[iCurSrc] = curSrcRT;
        mFilter->setPreventSourceLocationUpdate(false);

    }
}

void SourceMover::setEqualRadiusAndAngles(){
    //first figure out the correct angles
    sortAngles();
    
    //then set them
    FPoint selSrcRT = mFilter->getSourceRT(mSelectedSrc);
    for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); iCurSrc++) {
        if (iCurSrc == mSelectedSrc){
            continue;
        }
        FPoint curSrcRT = mFilter->getSourceRT(iCurSrc);

        curSrcRT.x = selSrcRT.x;
        curSrcRT.y = selSrcRT.y + mSourcesAngularOrder[iCurSrc];
        if (curSrcRT.y < 0) curSrcRT.y += kThetaMax;
        if (curSrcRT.y > kThetaMax) curSrcRT.y -= kThetaMax;
        mFilter->setPreventSourceLocationUpdate(true);
        mFilter->setSourceRT(iCurSrc, curSrcRT, !s_bUseOneSource);
        mFilter->mOldSrcLocRT[iCurSrc] = curSrcRT;
        mFilter->setPreventSourceLocationUpdate(false);
    }
}

//in kSourceThread, FPoint p is the current location of the selected source, as read on the automation
void SourceMover::move(FPoint pointXY01, MoverType mt)
{
    if (mMoverType != mt){
        return;
    }
    
    //move selected item only if not kSourceThread, since in kSourceThread item is already moved by automation
    if (mMoverType != kSourceThread){
        mFilter->setSourceXY01(mSelectedSrc, pointXY01);
        mFilter->mOldSrcLocRT[mSelectedSrc] = mFilter->convertXy012Rt(pointXY01);
    }
    
    int iMovementMode = mFilter->getMovementMode();
    if (iMovementMode == 0){
        return; //independent, so no need to move unselected sources
    }
    
    if (mFilter->getNumberOfSources() > 1) {
        
        //calculate delta for selected source
        FPoint oldSelSrcPosRT = (mMoverType == kSourceThread) ? mFilter->mOldSrcLocRT[mSelectedSrc] : mSourcesDownRT[mSelectedSrc];
        FPoint newSelSrcPosRT = mFilter->getSourceRT(mSelectedSrc); //in kSourceThread, this will be the same as mFilter->convertXy012Rt(pointXY01)
        FPoint delSelSrcPosRT = newSelSrcPosRT - oldSelSrcPosRT;
        
        if (delSelSrcPosRT.isOrigin()){
            return;     //return if delta is null
        }

        float vxo = pointXY01.x, vyo = pointXY01.y;
        
        if (kSourceThread){
            mFilter->setPreventSourceLocationUpdate(true);
        }
        
        for (int iCurSrc = 0; iCurSrc < mFilter->getNumberOfSources(); iCurSrc++) {

            if (iCurSrc == mSelectedSrc) {
                mFilter->mOldSrcLocRT[iCurSrc] = newSelSrcPosRT  ;
                continue;
            }
            
            //calculate new position for curSrc using delta for selected source
            FPoint oldCurSrcPosRT = (mMoverType == kSourceThread) ? mFilter->mOldSrcLocRT[iCurSrc] : mSourcesDownRT[iCurSrc];
            FPoint newCurSrcPosRT = oldCurSrcPosRT + delSelSrcPosRT;
            
            //all x's and y's here are actually r's and t's
            
            switch(mFilter->getMovementMode()) {
                case 1:     // circular
                case 2:     // circular, fixed radius
                case 3:     // circular, fixed angle
                case 4:     // circular, fully fixed
                    if (newCurSrcPosRT.x < 0) newCurSrcPosRT.x = 0;
                    if (newCurSrcPosRT.x > kRadiusMax) newCurSrcPosRT.x = kRadiusMax;
                    if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                    if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                    mFilter->setSourceRT(iCurSrc, newCurSrcPosRT, !s_bUseOneSource);
                    mFilter->mOldSrcLocRT[iCurSrc] = newCurSrcPosRT;
                    break;
//                case 2:     // circular, fixed radius
//                    newCurSrcPosRT.x = newSelSrcPosRT.x;
//                    if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
//                    if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
//                    mFilter->setSourceRT(iCurSrc, newCurSrcPosRT, !s_bUseOneSource);     //when kSourceThread, this call needs to NOT triger a call to processor::setSourceLocationChanged
//                    mFilter->mOldSrcLocRT[iCurSrc] = newCurSrcPosRT;
//                    break;
//                case 3:     // circular, fixed angle
//                    newCurSrcPosRT.y = newSelSrcPosRT.y + mSourcesAngularOrder[iCurSrc];
//                    if (newCurSrcPosRT.x < 0) newCurSrcPosRT.x = 0;
//                    if (newCurSrcPosRT.x > kRadiusMax) newCurSrcPosRT.x = kRadiusMax;
//                    if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
//                    if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
//                    mFilter->setSourceRT(iCurSrc, newCurSrcPosRT, !s_bUseOneSource);
//                    mFilter->mOldSrcLocRT[iCurSrc] = newCurSrcPosRT;
//                    break;
//                case 4:     // circular, fully fixed
//                    newCurSrcPosRT.x = newSelSrcPosRT.x;
//                    newCurSrcPosRT.y = newSelSrcPosRT.y + mSourcesAngularOrder[iCurSrc];
//                    if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
//                    if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
//                    mFilter->setSourceRT(iCurSrc, newCurSrcPosRT, !s_bUseOneSource);
//                    mFilter->mOldSrcLocRT[iCurSrc] = newCurSrcPosRT;
//                    break;
                case 5:{      // delta lock
                    FPoint d = mFilter->getSourceXY(mSelectedSrc) - mSourcesDownXY[mSelectedSrc];
                    mFilter->setSourceXY(iCurSrc, mSourcesDownXY[iCurSrc] + d, !s_bUseOneSource);
                    mFilter->mOldSrcLocRT[iCurSrc] = mSourcesDownXY[iCurSrc] + d;
                    break;
                }
                case 6:  // sym x
                    vyo = 1 - vyo;
                    mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
                    break;
                case 7: // sym y
                    vxo = 1 - vxo;
                    mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
                    break;
                    
                case 8: // sym x/y
                    vxo = 1 - vxo;
                    vyo = 1 - vyo;
                    mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
                    break;
            }
        }
        if (kSourceThread){
            mFilter->setPreventSourceLocationUpdate(false);
        }
    }
}

void SourceMover::end(MoverType mt)
{
	if (mMoverType != mt) return;

    if (mMoverType != kSourceThread){
        mFilter->endParameterChangeGesture(mFilter->getParamForSourceX(mSelectedSrc));
        mFilter->endParameterChangeGesture(mFilter->getParamForSourceY(mSelectedSrc));
        
        if (mFilter->getMovementMode() != 0 && mFilter->getNumberOfSources() > 1) {
            for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                if (i == mSelectedSrc) continue;
                if (!s_bUseOneSource){
                    mFilter->endParameterChangeGesture(mFilter->getParamForSourceX(i));
                    mFilter->endParameterChangeGesture(mFilter->getParamForSourceY(i));
                }
            }
        }
        mFilter->setIsRecordingAutomation(false);
    }
    
    mMoverType = kVacant;
}
