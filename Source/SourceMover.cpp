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
:
	mFilter(filter),
	mMoverType(kVacant)
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
	
	mFilter->beginParameterChangeGesture(mFilter->getParamForSourceX(mSelectedSrc));
	mFilter->beginParameterChangeGesture(mFilter->getParamForSourceY(mSelectedSrc));
    
//    if (mMoverType != kSourceThread){
//        return;
//    }
    
    int iNbrSrc = mFilter->getNumberOfSources();

	if (mFilter->getMovementMode() != 0 && mFilter->getNumberOfSources() > 1 && mFilter->getLinkMovement())
	{
		for (int j = 0; j < iNbrSrc; j++)
		{
			mSourcesDownRT.setUnchecked(j, mFilter->getSourceRT(j));
			mSourcesDownXY.setUnchecked(j, mFilter->getSourceXY(j));
			if (j == mSelectedSrc) continue;
			mFilter->beginParameterChangeGesture(mFilter->getParamForSourceX(j));
			mFilter->beginParameterChangeGesture(mFilter->getParamForSourceY(j));
		}
		
		if	(	(iNbrSrc == 2 && (mFilter->getMovementMode() == 6 || mFilter->getMovementMode() == 7))
			 ||	(iNbrSrc >  2 && (mFilter->getMovementMode() == 3 || mFilter->getMovementMode() == 4))
			 )
		{
			// need to calculate angular order
			
			//IndexedAngle ia[iNbrSrc];
			IndexedAngle * ia = new IndexedAngle[iNbrSrc];
			
			for (int j = 0; j < iNbrSrc; j++)
			{
				ia[j].i = j;
				ia[j].a = mFilter->getSourceRT(j).y;
			}
			
			//printf("==============\nbefore sort:\n");
			//for (int j = 0; j < mNumberOfSources; j++) printf("ia[%i] = { %i, %.3f }\n", j+1, ia[j].i+1, ia[j].a);
			
			qsort(ia, iNbrSrc, sizeof(IndexedAngle), IndexedAngleCompare);
			
			//printf("after sort:\n");
			//for (int j = 0; j < mNumberOfSources; j++) printf("ia[%i] = { %i, %.3f }\n", j+1, ia[j].i+1, ia[j].a);
			
			int b;
			for (b = 0; b < iNbrSrc && ia[b].i != mSelectedSrc; b++) ;
			
			if (b == iNbrSrc)
			{
				printf("error!\n");
				b = 0;
			}
			
			//printf("mSelectedItem: %i base: %i step: %.3f\n", mSelectedItem+1, b+1, (M_PI * 2.) / mNumberOfSources);
			
			for (int j = 1; j < iNbrSrc; j++)
			{
				int o = (b + j) % iNbrSrc;
				o = ia[o].i;
				mSourcesAngularOrder.set(o, (M_PI * 2. * j) / iNbrSrc);
			}
			
			//for (int j = 0; j < mNumberOfSources; j++)  printf("mSourceAngularOrder[%i] = %.3f\n", j+1, mSourceAngularOrder[j]);

			delete[] ia;
		}
	}
	
}

void SourceMover::move(FPoint p, MoverType mt)
{
    if (mMoverType != mt){
        return;
    }
    
    //move selected item
    float fCurX01 = p.x, fCurY01 = p.y;;
    
    // in non-thread case, just update selectedSrc
//    if (mMoverType != kSourceThread){
        mFilter->setSourceXY01(mSelectedSrc, FPoint(fCurX01, fCurY01));
//        mFilter->mOldSrcLocRT[mSelectedSrc] = FPoint(fCurX01, fCurY01);
//        return;
//    }
    
    int iMovementMode = mFilter->getMovementMode();
    if (iMovementMode == 0 || !mFilter->getLinkMovement()){
        return;
    }
    
    if (mFilter->getNumberOfSources() > 2) {
        for (int iCurItem = 0; iCurItem < mFilter->getNumberOfSources(); iCurItem++) {
            
            if (iCurItem == mSelectedSrc) {
                //mFilter->mOldSrcLocRT[iCurItem] = FPoint(fCurX01, fCurY01);
                continue;
            }
            
            FPoint oldCurSrcPosRT = mSourcesDownRT[iCurItem];
            FPoint oldSelSrcPosRT = mSourcesDownRT[mSelectedSrc];
            FPoint newSelSrcPosRT = mFilter->getSourceRT(mSelectedSrc);
            FPoint delSelSrcPosRT = newSelSrcPosRT - oldSelSrcPosRT;
            
            if (delSelSrcPosRT.isOrigin()){
                return;     //return if delta is null
            }
            
            FPoint newCurSrcPosRT = oldCurSrcPosRT + delSelSrcPosRT;
            
            //all x's and y's here are actually r's and t's
            switch(mFilter->getMovementMode()) {
                case 1:     // circular
                    if (newCurSrcPosRT.x < 0) newCurSrcPosRT.x = 0;
                    if (newCurSrcPosRT.x > kRadiusMax) newCurSrcPosRT.x = kRadiusMax;
                    if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                    if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                    mFilter->setSourceRT(iCurItem, newCurSrcPosRT);
                    break;
                case 2:     // circular, fixed radius
                    newCurSrcPosRT.x = newSelSrcPosRT.x;
                    if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                    if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                    mFilter->setSourceRT(iCurItem, newCurSrcPosRT);
                    break;
                case 3:     // circular, fixed angle
                    newCurSrcPosRT.y = newSelSrcPosRT.y + mSourcesAngularOrder[iCurItem];
                    if (newCurSrcPosRT.x < 0) newCurSrcPosRT.x = 0;
                    if (newCurSrcPosRT.x > kRadiusMax) newCurSrcPosRT.x = kRadiusMax;
                    if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                    if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                    mFilter->setSourceRT(iCurItem, newCurSrcPosRT);
                    break;
                case 4:     // circular, fully fixed
                    newCurSrcPosRT.x = newSelSrcPosRT.x;
                    newCurSrcPosRT.y = newSelSrcPosRT.y + mSourcesAngularOrder[iCurItem];
                    if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                    if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                    mFilter->setSourceRT(iCurItem, newCurSrcPosRT);
                    break;
                case 5:      // delta lock
                    FPoint d = mFilter->getSourceXY(mSelectedSrc) - mSourcesDownXY[mSelectedSrc];
                    mFilter->setSourceXY(iCurItem, mSourcesDownXY[iCurItem] + d);
                    break;
            }
        }
    }
    
    //we need to have a whole different case for when we have 2 sources because the getMovementModes() are not the same!
    else if (mFilter->getNumberOfSources() == 2)
    {
        int iCurSrc = 1 - mSelectedSrc;
        float vxo = fCurX01, vyo = fCurY01;
        
        FPoint oldSelSrcPosRT = mSourcesDownRT[mSelectedSrc];
        FPoint oldCurSrcPosRT = mSourcesDownRT[iCurSrc];
        FPoint newSelSrcPosRT = mFilter->getSourceRT(mSelectedSrc);
        FPoint newCurSrcPosRT = oldCurSrcPosRT + newSelSrcPosRT - oldSelSrcPosRT;
        
        switch(mFilter->getMovementMode())
        {
            case 1: // sym x
                vyo = 1 - vyo;
                mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
                break;
                
            case 2: // sym y
                vxo = 1 - vxo;
                mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
                break;
                
            case 3: // sym x/y
                vxo = 1 - vxo;
                vyo = 1 - vyo;
                mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
                break;
                
            case 4: // circular
                if (newCurSrcPosRT.x < 0) newCurSrcPosRT.x = 0;
                if (newCurSrcPosRT.x > kRadiusMax) newCurSrcPosRT.x = kRadiusMax;
                if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                mFilter->setSourceRT(iCurSrc, newCurSrcPosRT);
                break;
                
            case 5: // circular, fixed radius
                newCurSrcPosRT.x = newSelSrcPosRT.x;
                if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                mFilter->setSourceRT(iCurSrc, newCurSrcPosRT);
                break;
                
            case 6: // circular, fixed angle
                newCurSrcPosRT.y = newSelSrcPosRT.y + mSourcesAngularOrder[iCurSrc];
                if (newCurSrcPosRT.x < 0) newCurSrcPosRT.x = 0;
                if (newCurSrcPosRT.x > kRadiusMax) newCurSrcPosRT.x = kRadiusMax;
                if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                mFilter->setSourceRT(iCurSrc, newCurSrcPosRT);
                break;
                
            case 7: // circular, fully fixed
                newCurSrcPosRT.x = newSelSrcPosRT.x;
                newCurSrcPosRT.y = newSelSrcPosRT.y + mSourcesAngularOrder[iCurSrc];
                if (newCurSrcPosRT.y < 0) newCurSrcPosRT.y += kThetaMax;
                if (newCurSrcPosRT.y > kThetaMax) newCurSrcPosRT.y -= kThetaMax;
                mFilter->setSourceRT(iCurSrc, newCurSrcPosRT);
                break;
                
            case 8: // delta lock
                FPoint d = mFilter->getSourceXY(mSelectedSrc) - mSourcesDownXY[mSelectedSrc];
                mFilter->setSourceXY(iCurSrc, mSourcesDownXY[iCurSrc] + d);
                break;
        }
    }
}

//ORIGINAL SOURCEMOVER
//void SourceMover::move(FPoint p, MoverType mt)
//{
//    if (mMoverType != mt){
//        return;
//    }
//    
//    //move selected item
//    float fCurX01 = p.x, fCurY01 = p.y;
//    mFilter->setSourceXY01(mSelectedSrc, FPoint(fCurX01, fCurY01));
//    
//    int iMovementMode = mFilter->getMovementMode();
//    if (iMovementMode == 0 || !mFilter->getLinkMovement()){
//        return;
//    }
//    
//    if (mFilter->getNumberOfSources() > 2) {
//        for (int iCurItem = 0; iCurItem < mFilter->getNumberOfSources(); iCurItem++) {
//            if (iCurItem == mSelectedSrc) {
//                continue;
//            }
//            
//            switch(mFilter->getMovementMode()) {
//                case 1: // circular
//                {
//                    FPoint s = mSourcesDownRT[mSelectedSrc];
//                    FPoint o = mSourcesDownRT[iCurItem];
//                    FPoint sn = mFilter->getSourceRT(mSelectedSrc);
//                    FPoint n = o + sn - s;
//                    if (n.x < 0) n.x = 0;
//                    if (n.x > kRadiusMax) n.x = kRadiusMax;
//                    if (n.y < 0) n.y += kThetaMax;
//                    if (n.y > kThetaMax) n.y -= kThetaMax;
//                    mFilter->setSourceRT(iCurItem, n);
//                }
//                    break;
//                    
//                case 2: // circular, fixed radius
//                {
//                    FPoint s = mSourcesDownRT[mSelectedSrc];
//                    FPoint o = mSourcesDownRT[iCurItem];
//                    FPoint sn = mFilter->getSourceRT(mSelectedSrc);
//                    FPoint n = o + sn - s;
//                    n.x = sn.x;
//                    if (n.y < 0) n.y += kThetaMax;
//                    if (n.y > kThetaMax) n.y -= kThetaMax;
//                    mFilter->setSourceRT(iCurItem, n);
//                }
//                    break;
//                    
//                case 3: // circular, fixed angle
//                {
//                    FPoint s = mSourcesDownRT[mSelectedSrc];
//                    FPoint o = mSourcesDownRT[iCurItem];
//                    FPoint sn = mFilter->getSourceRT(mSelectedSrc);
//                    FPoint n = o + sn - s;
//                    n.y = sn.y + mSourcesAngularOrder[iCurItem];
//                    if (n.x < 0) n.x = 0;
//                    if (n.x > kRadiusMax) n.x = kRadiusMax;
//                    if (n.y < 0) n.y += kThetaMax;
//                    if (n.y > kThetaMax) n.y -= kThetaMax;
//                    mFilter->setSourceRT(iCurItem, n);
//                }
//                    break;
//                    
//                case 4: // circular, fully fixed
//                {
//                    FPoint s = mSourcesDownRT[mSelectedSrc];
//                    FPoint o = mSourcesDownRT[iCurItem];
//                    FPoint sn = mFilter->getSourceRT(mSelectedSrc);
//                    FPoint n = o + sn - s;
//                    n.x = sn.x;
//                    n.y = sn.y + mSourcesAngularOrder[iCurItem];
//                    if (n.y < 0) n.y += kThetaMax;
//                    if (n.y > kThetaMax) n.y -= kThetaMax;
//                    mFilter->setSourceRT(iCurItem, n);
//                }
//                    break;
//                    
//                case 5: // delta lock
//                {
//                    FPoint d = mFilter->getSourceXY(mSelectedSrc) - mSourcesDownXY[mSelectedSrc];
//                    mFilter->setSourceXY(iCurItem, mSourcesDownXY[iCurItem] + d);
//                }
//                    break;
//            }
//        }
//    }
//    
//    else if (mFilter->getNumberOfSources() == 2)
//    {
//        int iCurSrc = 1 - mSelectedSrc;
//        float vxo = fCurX01, vyo = fCurY01;
//        switch(mFilter->getMovementMode())
//        {
//            case 1: // sym x
//                vyo = 1 - vyo;
//                mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
//                break;
//                
//            case 2: // sym y
//                vxo = 1 - vxo;
//                mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
//                break;
//                
//            case 3: // sym x/y
//                vxo = 1 - vxo;
//                vyo = 1 - vyo;
//                mFilter->setSourceXY01(iCurSrc, FPoint(vxo, vyo));
//                break;
//                
//            case 4: // circular
//            {
//                FPoint s = mSourcesDownRT[mSelectedSrc];
//                FPoint o = mSourcesDownRT[iCurSrc];
//                FPoint n = o + mFilter->getSourceRT(mSelectedSrc) - s;
//                if (n.x < 0) n.x = 0;
//                if (n.x > kRadiusMax) n.x = kRadiusMax;
//                if (n.y < 0) n.y += kThetaMax;
//                if (n.y > kThetaMax) n.y -= kThetaMax;
//                mFilter->setSourceRT(iCurSrc, n);
//            }
//                break;
//                
//            case 5: // circular, fixed radius
//            {
//                FPoint s = mSourcesDownRT[mSelectedSrc];
//                FPoint o = mSourcesDownRT[iCurSrc];
//                FPoint sn = mFilter->getSourceRT(mSelectedSrc);
//                FPoint n = o + sn - s;
//                n.x = sn.x;
//                if (n.y < 0) n.y += kThetaMax;
//                if (n.y > kThetaMax) n.y -= kThetaMax;
//                mFilter->setSourceRT(iCurSrc, n);
//            }
//                break;
//                
//            case 6: // circular, fixed angle
//            {
//                FPoint s = mSourcesDownRT[mSelectedSrc];
//                FPoint o = mSourcesDownRT[iCurSrc];
//                FPoint sn = mFilter->getSourceRT(mSelectedSrc);
//                FPoint n = o + sn - s;
//                n.y = sn.y + mSourcesAngularOrder[iCurSrc];
//                if (n.x < 0) n.x = 0;
//                if (n.x > kRadiusMax) n.x = kRadiusMax;
//                if (n.y < 0) n.y += kThetaMax;
//                if (n.y > kThetaMax) n.y -= kThetaMax;
//                mFilter->setSourceRT(iCurSrc, n);
//            }
//                break;
//                
//            case 7: // circular, fully fixed
//            {
//                FPoint s = mSourcesDownRT[mSelectedSrc];
//                FPoint o = mSourcesDownRT[iCurSrc];
//                FPoint sn = mFilter->getSourceRT(mSelectedSrc);
//                FPoint n = o + sn - s;
//                n.x = sn.x;
//                n.y = sn.y + mSourcesAngularOrder[iCurSrc];
//                if (n.y < 0) n.y += kThetaMax;
//                if (n.y > kThetaMax) n.y -= kThetaMax;
//                mFilter->setSourceRT(iCurSrc, n);
//            }
//                break;
//                
//            case 8: // delta lock
//            {
//                FPoint d = mFilter->getSourceXY(mSelectedSrc) - mSourcesDownXY[mSelectedSrc];
//                mFilter->setSourceXY(iCurSrc, mSourcesDownXY[iCurSrc] + d);
//            }
//                break;
//        }
//        
//    }
//}

void SourceMover::end(MoverType mt)
{
	if (mMoverType != mt) return;
	
	mFilter->endParameterChangeGesture(mFilter->getParamForSourceX(mSelectedSrc));
	mFilter->endParameterChangeGesture(mFilter->getParamForSourceY(mSelectedSrc));
    
//    if (mMoverType != kSourceThread){
//        return;
//    }

    
	if (mFilter->getMovementMode() != 0 && mFilter->getNumberOfSources() > 1 && mFilter->getLinkMovement())
	{
		for (int i = 0; i < mFilter->getNumberOfSources(); i++)
		{
			if (i == mSelectedSrc) continue;
			mFilter->endParameterChangeGesture(mFilter->getParamForSourceX(i));
			mFilter->endParameterChangeGesture(mFilter->getParamForSourceY(i));
		}
	}
	
	mMoverType = kVacant;
}
