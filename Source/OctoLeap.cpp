/*
  ==============================================================================

    OctoLeap.cpp
    Created: 4 Aug 2014 1:23:01pm
    Author:  makira

  ==============================================================================
*/

#include <iostream>
#include "OctoLeap.h"

#if JUCE_WINDOWS
Component * CreateLeapComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
{
	// not implemented yet on windows
	return NULL;
}
#else

#include "Leap.h"

class OctoLeap : public Component, public Button::Listener, public Leap::Listener
{
public:
	OctoLeap(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
	:
		mFilter(filter),
		mEditor(editor),
		mController(NULL),
		mPointableId(-1),
		mLastPositionValid(0)
	{
		//mController.addListener(*this);
		
		const int m = 10, dh = 18, cw = 200;
		int x = m, y = m;
		
		mEnable = new ToggleButton();
		mEnable->setButtonText("Enable Leap");
		mEnable->setSize(cw, dh);
		mEnable->setTopLeftPosition(x, y);
		mEnable->addListener(this);
		mEnable->setToggleState(false, dontSendNotification);
		addAndMakeVisible(mEnable);
		
		y += dh + m;
		
		mState = new Label();
		mState->setText("", dontSendNotification);
		mState->setSize(cw, dh);
		mState->setJustificationType(Justification::left);
		mState->setMinimumHorizontalScale(1);
		mState->setTopLeftPosition(x, y);
		addAndMakeVisible(mState);
	}
	
	void buttonClicked (Button *button)
	{
		if (button == mEnable)
		{
			if (mEnable->getToggleState())
			{
				if (!mController)
				{
					mState->setText("Leap not connected", dontSendNotification);
					mController = new Leap::Controller();
					mController->addListener(*this);
				}
			}
			else
			{
				mController = NULL;
				mState->setText("", dontSendNotification);
			}
		}
		else
		{
			printf("unknown button clicked...\n");
		}
	}
	
	void onConnect(const Leap::Controller& controller)
	{
		const MessageManagerLock mmLock;
		mState->setText("Leap connected", dontSendNotification);
	}

	//Not dispatched when running in a debugger
	void onDisconnect(const Leap::Controller& controller)
	{
		const MessageManagerLock mmLock;
		mState->setText("Leap disconnected", dontSendNotification);
	}

	void onFrame(const Leap::Controller& controller)
	{
		//std::cout << "New frame available" << std::endl;
		Leap::Frame frame = controller.frame();
		if (mPointableId >= 0)
		{
			Leap::Pointable p = frame.pointable(mPointableId);
			if (!p.isValid() || !p.isExtended())
			{
				mPointableId = -1;
				mLastPositionValid = false;
				//std::cout << "pointable not valid or not extended" << std::endl;
				
				mEditor->getMover()->end(kLeap);
			}
			else
			{
				Leap::Vector pos = p.tipPosition();
				//std::cout << "x: " << pos.x << " y: " << pos.y << " z: " << pos.z << std::endl;
					
				const float zPlane1 = 50;	// 5 cm
				const float zPlane2 = 100;	// 10 cm
				
				if (pos.z < zPlane2)
				{
					if (mLastPositionValid)
					{
						Leap::Vector delta = pos - mLastPosition;
						//std::cout << "dx: " << delta.x << " dy: " << delta.y << " dz: " << delta.z << std::endl;
						
						float scale = 1 / 100.;
						if (pos.z > zPlane1)
						{
							float s = 1 - (pos.z - zPlane1) / (zPlane2 - zPlane1);
							scale *= s;
						
						}
						//std::cout << "scale: " << scale << std::endl;
						
						int src = mEditor->getOscLeapSource();
						
						FPoint sp = mFilter->getSourceXY01(src);
						sp.x += delta.x * scale;
						sp.y += delta.y * scale;
						mEditor->getMover()->move(sp, kLeap);
						
						mEditor->fieldChanged();
					}
					else
					{
						//std::cout << "pointable last pos not valid" << std::endl;
						mEditor->getMover()->begin(mEditor->getOscLeapSource(), kLeap);
						
					}
					
					mLastPosition = pos;
					mLastPositionValid = true;
				}
				else
				{
					//std::cout << "pointable not touching plane" << std::endl;
					mLastPositionValid = false;
					
					mEditor->getMover()->end(kLeap);
				}
			}
		}
		if (mPointableId < 0)
		{
			Leap::PointableList pl = frame.pointables().extended();
			if (pl.count() > 0)
			{
				mPointableId = pl[0].id();
				//std::cout << "got new pointable: " << mPointableId << std::endl;
			}
		}
	}

private:
	OctogrisAudioProcessor *mFilter;
	OctogrisAudioProcessorEditor *mEditor;
	
	ScopedPointer<ToggleButton> mEnable;
	ScopedPointer<Label> mState;
	
	ScopedPointer<Leap::Controller> mController;

	int32_t mPointableId;
	bool mLastPositionValid;
	Leap::Vector mLastPosition;
	
	JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OctoLeap)
};


Component * CreateLeapComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor)
{
	return new OctoLeap(filter, editor);
}

#endif
