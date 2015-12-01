/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginEditor.cpp
 
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

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include "FieldComponent.h"
#include "Trajectories.h"
#include "OctoLeap.h"
#include "OscComponent.h"
#include <iomanip>

#if WIN32

#else

#include "HIDDelegate.h"
#include "HID_Utilities_External.h"
#endif

#define STRING2(x) #x
#define STRING(x) STRING2(x)
//==============================================================================
static const int kDefaultLabelHeight = 18;
static const int kParamBoxHeight = 165;
static const int kTimerDelay = 1000 / 20; // 20 fps

//==============================================================================

class MiniProgressBar : public Component
{
public:
    MiniProgressBar() : mValue(0) {}
    void paint(Graphics &g)
    {
        Rectangle<int> box = getLocalBounds();
        
        g.setColour(Colours::black);
        g.fillRect(box);
        
        g.setColour(Colour::fromRGB(0,255,0));
        box.setWidth(box.getWidth() * mValue);
        g.fillRect(box);
    }
    void setValue(float v) { mValue = v; repaint(); }
private:
    float mValue;
};

//==============================================================================
class OctTabbedComponent : public TabbedComponent
{
public:
    OctTabbedComponent(TabbedButtonBar::Orientation orientation, OctogrisAudioProcessor *filter)
    :
    TabbedComponent(orientation),
    mFilter(filter)
    ,mInited(false)
    { }
    
    void currentTabChanged (int newCurrentTabIndex, const String& newCurrentTabName)
    {
        if (!mInited) return;
        
        //printf("Octogris: currentTabChanged\n");
        mFilter->setGuiTab(newCurrentTabIndex);
    }
    
    void initDone() { mInited = true; }
    
private:
    OctogrisAudioProcessor *mFilter;
    bool mInited;
};

//======================================= BOX ===========================================================================
class Box : public Component
{
public:
    Box(bool useViewport)
    {
        if (useViewport)
        {
            
            mContent = new Component();
            mViewport = new Viewport();
            mViewport->setViewedComponent(mContent, false);
            mViewport->setScrollBarsShown(true, false);
            mViewport->setScrollBarThickness(5);
            addAndMakeVisible(mViewport);
        }
    }
    
    ~Box()
    {
        
    }
    
    Component * getContent()
    {
        return mContent.get() ? mContent.get() : this;
    }
    
    void paint(Graphics &g)
    {
        const Rectangle<int> &box = getLocalBounds();
        
        g.setColour(Colour::fromRGB(200,200,200));
        g.fillRect(box);
        g.setColour(Colours::black);
        g.drawRect(box);
    }
    
    void resized()
    {
        if (mViewport)
            mViewport->setSize(getWidth(), getHeight());
    }
    
private:
    ScopedPointer<Component> mContent;
    ScopedPointer<Viewport> mViewport;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Box)
};


//================================================== PARAMSLIDER ======================================================
JUCE_COMPILER_WARNING("this class should be in its own file")
class ParamSlider : public Slider
{
public:
    ParamSlider(int paramIndex, int paramType, ToggleButton *link, OctogrisAudioProcessor *filter)
    :
    mParamIndex(paramIndex),
    mParamType(paramType),
    mLink(link),
    mFilter(filter),
    mBeganGesture(false),
    mMouseDown(false)
    {
        jassert(mLink || mParamType != kParamSource);
    }
    
    void mouseDown (const MouseEvent &e)
    {
        mBeganGesture = false;
        mMouseDown = true;
        
        bool resetToDefault = e.mods.isAltDown();
        if (resetToDefault)
        {
            double newVal;
            switch(mParamType)
            {
                case kParamSource: newVal = normalize(kSourceMinDistance, kSourceMaxDistance, kSourceDefaultDistance); break;
                case kParamSpeaker: newVal = normalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, kSpeakerDefaultAttenuation); break;
                case kParamSmooth: newVal = normalize(kSmoothMin, kSmoothMax, kSmoothDefault); break;
                case kParamVolumeFar: newVal = normalize(kVolumeFarMin, kVolumeFarMax, kVolumeFarDefault); break;
                case kParamVolumeMid: newVal = normalize(kVolumeMidMin, kVolumeMidMax, kVolumeMidDefault); break;
                case kParamVolumeNear: newVal = normalize(kVolumeNearMin, kVolumeNearMax, kVolumeNearDefault); break;
                case kParamFilterFar: newVal = normalize(kFilterFarMin, kFilterFarMax, kFilterFarDefault); break;
                case kParamFilterMid: newVal = normalize(kFilterMidMin, kFilterMidMax, kFilterMidDefault); break;
                case kParamFilterNear: newVal = normalize(kFilterNearMin, kFilterNearMax, kFilterNearDefault); break;
                case kParamMaxSpanVolume: newVal = normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, kMaxSpanVolumeDefault); break;
				case kParamRoutingVolume: newVal = normalize(kRoutingVolumeMin, kRoutingVolumeMax, kRoutingVolumeDefault); break;
            }
            
            if (mParamType == kParamSource && mLink->getToggleState())
            {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++)
                {
                    int paramIndex = mFilter->getParamForSourceD(i);
                    if (mFilter->getParameter(paramIndex) != newVal){
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
                    }
                }
            }
            else
            {
                if (mFilter->getParameter(mParamIndex) != newVal){
                    mFilter->setParameterNotifyingHost(mParamIndex, newVal);
                }
            }
        }
        else
        {
            Slider::mouseDown(e);
        }
    }
    
    void mouseUp (const MouseEvent &e)
    {
        //fprintf(stderr, "paremslider :: mouseUp\n");
        Slider::mouseUp(e);
        
        if (mBeganGesture)
        {
            //fprintf(stderr, "paremslider :: endParameter\n");
            
            if (mParamType == kParamSource && mLink->getToggleState())
            {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++)
                {
                    int paramIndex = mFilter->getParamForSourceD(i);
                    mFilter->endParameterChangeGesture(paramIndex);
                }
            }
            else
            {
                mFilter->endParameterChangeGesture(mParamIndex);
            }
        }
        
        mMouseDown = false;
        mBeganGesture = false;
    }
    
    void valueChanged()
    {
        if (mMouseDown && !mBeganGesture)
        {
            //fprintf(stderr, "paremslider :: beginParameter\n");
            
            if (mParamType == kParamSource && mLink->getToggleState())
            {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++)
                {
                    int paramIndex = mFilter->getParamForSourceD(i);
                    mFilter->beginParameterChangeGesture(paramIndex);
                }
            }
            else
            {
                mFilter->beginParameterChangeGesture(mParamIndex);
            }
            
            mBeganGesture = true;
        }
        
        if (mParamType == kParamSource) {
            const float newVal = 1.f - (float)getValue();
            
            if (mLink->getToggleState()) {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                    int paramIndex = mFilter->getParamForSourceD(i);
                    if (mFilter->getParameter(paramIndex) != newVal)
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
                }
            } else {
                if (mFilter->getParameter(mParamIndex) != newVal){
                    mFilter->setParameterNotifyingHost(mParamIndex, newVal);
                }
            }
        } else {
            const float newVal = (float)getValue();
            if (mFilter->getParameter(mParamIndex) != newVal)
                mFilter->setParameterNotifyingHost(mParamIndex, newVal);
        }
    }
    
    String getTextFromValue (double value)
    {
        switch(mParamType)
        {
            case kParamSource: value = denormalize(kSourceMinDistance, kSourceMaxDistance, value); break;
            case kParamSpeaker: value = denormalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, value); break;
            case kParamSmooth: value = denormalize(kSmoothMin, kSmoothMax, value); break;
            case kParamVolumeFar: value = denormalize(kVolumeFarMin, kVolumeFarMax, value); break;
            case kParamVolumeMid: value = denormalize(kVolumeMidMin, kVolumeMidMax, value); break;
            case kParamVolumeNear: value = denormalize(kVolumeNearMin, kVolumeNearMax, value); break;
            case kParamFilterFar: value = denormalize(-100, 0, value); break;
            case kParamFilterMid: value = denormalize(-100, 0, value); break;
            case kParamFilterNear: value = denormalize(-100, 0, value); break;
            case kParamMaxSpanVolume: value = denormalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, value); break;
			case kParamRoutingVolume: value = denormalize(kRoutingVolumeMin, kRoutingVolumeMax, value); break;
        }
        
        if (mParamType >= kParamSmooth || mParamType <= kParamRoutingVolume) return String(roundToInt(value));
        return String(value, 1);
    }
    
    double getValueFromText (const String& text)
    {
        double value = Slider::getValueFromText(text);
        switch(mParamType)
        {
            case kParamSource: value = normalize(kSourceMinDistance, kSourceMaxDistance, value); break;
            case kParamSpeaker: value = normalize(kSpeakerMinAttenuation, kSpeakerMaxAttenuation, value); break;
            case kParamSmooth: value = normalize(kSmoothMin, kSmoothMax, value); break;
            case kParamVolumeFar: value = normalize(kVolumeFarMin, kVolumeFarMax, value); break;
            case kParamVolumeMid: value = normalize(kVolumeMidMin, kVolumeMidMax, value); break;
            case kParamVolumeNear: value = normalize(kVolumeNearMin, kVolumeNearMax, value); break;
            case kParamFilterFar: value = normalize(-100, 0, value); break;
            case kParamFilterMid: value = normalize(-100, 0, value); break;
            case kParamFilterNear: value = normalize(-100, 0, value); break;
            case kParamMaxSpanVolume: value = normalize(kMaxSpanVolumeMin, kMaxSpanVolumeMax, value); break;
			case kParamRoutingVolume: value = normalize(kRoutingVolumeMin, kRoutingVolumeMax, value); break;
        }
        return value;
    }
    
private:
    int mParamIndex, mParamType;
    ToggleButton *mLink;
    OctogrisAudioProcessor *mFilter;
    bool mBeganGesture, mMouseDown;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParamSlider)
};

//==================================== SourceUpdateThread ===================================================================
class SourceUpdateThread : public Thread, public Component
{
public:
    SourceUpdateThread(OctogrisAudioProcessorEditor* p_pEditor)
    : Thread ("SourceUpdateThread")
    ,m_iInterval(50)
    ,m_pEditor(p_pEditor)
    { }
    
    ~SourceUpdateThread() {
        stopThread (500);
    }
    
    void run() override {
        while (! threadShouldExit()) {
            wait (m_iInterval);
            m_pEditor->updateNonSelectedSourcePositions();
        }
    }
    
private:
    int m_iInterval;
    OctogrisAudioProcessorEditor* m_pEditor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourceUpdateThread)
};
//==================================== JoystickUpdateThread ===================================================================
class JoystickUpdateThread : public Thread, public Component {
public:
    JoystickUpdateThread(OctogrisAudioProcessorEditor* p_pEditor)
    : Thread ("JoystickUpdateThread")
    ,m_iInterval(25)
    ,m_pEditor(p_pEditor)
    {  }
    
    ~JoystickUpdateThread() {
        stopThread (500);
    }
    
    void run() override {
        while (! threadShouldExit()) {
            wait (m_iInterval);
            m_pEditor->readAndUseJoystickValues();
        }
    }
private:
    int m_iInterval;
    OctogrisAudioProcessorEditor* m_pEditor;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (JoystickUpdateThread)
};



//==================================== EDITOR ===================================================================

OctogrisAudioProcessorEditor::OctogrisAudioProcessorEditor (OctogrisAudioProcessor* ownerFilter):
AudioProcessorEditor (ownerFilter)
,mFilter(ownerFilter)
,mMover(ownerFilter)
,m_logoImage()
{
    
    m_pSourceUpdateThread = new SourceUpdateThread(this);
    mComponents.add(m_pSourceUpdateThread);

    m_pJoystickUpdateThread = new JoystickUpdateThread(this);
    mComponents.add(m_pJoystickUpdateThread);
    
    mHostChangedParameter = mFilter->getHostChangedParameter();
    mHostChangedProperty = mFilter->getHostChangedProperty();
    
    mNeedRepaint = false;
    mFieldNeedRepaint = false;
	m_bLoadingPreset = false;
    bool leapSupported = true;
    
    startTimer(kTimerDelay);
    mFilter->addListener(this);
    
    // main field
    mField = new FieldComponent(mFilter, &mMover);
    addAndMakeVisible(mField);
    mComponents.add(mField);

    //GRIS logo
    m_logoImage.setImage(ImageFileFormat::loadFrom (BinaryData::logoGris_png, (size_t) BinaryData::logoGris_pngSize));
    addAndMakeVisible(&m_logoImage);
    
    //version label
    m_VersionLabel = new Label();
    String version = STRING(JUCE_APP_VERSION);
#ifdef JUCE_DEBUG
    version += " ";
    version += STRING(__TIME__);
#endif
    
    m_VersionLabel->setText("Octogris" + version,  dontSendNotification);
    m_VersionLabel->setFont (Font (Font::getDefaultMonospacedFontName(), 12.0f, Font::plain));
    m_VersionLabel->setJustificationType(Justification(Justification::right));
    m_VersionLabel->setColour(Label::textColourId, Colours::whitesmoke);
    addAndMakeVisible(m_VersionLabel);

    mComponents.add(m_VersionLabel);

    
    // param box
    Colour tabBg = Colour::fromRGB(200,200,200);
    mTabs = new OctTabbedComponent(TabbedButtonBar::TabsAtTop, mFilter);
    mTabs->addTab("Settings",           tabBg, new Component(), true);
    mTabs->addTab("Trajectories",       tabBg, new Component(), true);
    mTabs->addTab("Volume & Filters",   tabBg, new Component(), true);
    mTabs->addTab("Sources",            tabBg, new Component(), true);
   	mTabs->addTab("Speakers",           tabBg, new Component(), true);
//    {
//        mOsc = CreateOscComponent(mFilter, this);
//        if (mOsc) mTabs->addTab("OSC",  tabBg, mOsc, true);
//    }
    mTabs->addTab("Interfaces",         tabBg, new Component(), true);

    mTabs->setSize(kCenterColumnWidth + kMargin + kRightColumnWidth, kParamBoxHeight);
    addAndMakeVisible(mTabs);
    mComponents.add(mTabs);
    
    
    // sources
    {
        mSourcesBox = new Box(true);
        addAndMakeVisible(mSourcesBox);
        mComponents.add(mSourcesBox);
        
        mSourcesBoxLabel = addLabel("Surface:", 0, 0, kCenterColumnWidth, kDefaultLabelHeight, this);

        Component *ct = mSourcesBox->getContent();
        
        int dh = kDefaultLabelHeight, x = 0, y = 0, w = kCenterColumnWidth;
        
        mLinkDistances = addCheckbox("Link", mFilter->getLinkDistances(), x, y, w/3, dh, ct);
        addLabel("Distance/Span", x+w/3, y, w*2/3, dh, ct);
        
        mSrcSelect = new ComboBox();
        mTabs->getTabContentComponent(3)->addAndMakeVisible(mSrcSelect);
        mComponents.add(mSrcSelect);
        mSrcSelect->addListener(this);
        
        if (mFilter->getIsAllowInputOutputModeSelection()){
            mFilter->setInputOutputMode(mFilter->getInputOutputMode());
        }
        updateSources(true);
    }
    
    // speakers
    {
        mSpeakersBox = new Box(true);
        addAndMakeVisible(mSpeakersBox);
        mComponents.add(mSpeakersBox);
        
        int dh = kDefaultLabelHeight;
        
        int x = 0, y = 0, w = kRightColumnWidth;
        
        mSpeakersBoxLabel = addLabel("Speaker attenuation:", 0, 0, kRightColumnWidth, kDefaultLabelHeight, this);

        Component *ct = mSpeakersBox->getContent();
        const int muteWidth = 50;
        addLabel("Mute", x, y, muteWidth, dh, ct);
        addLabel("Attenuation (dB)", x+muteWidth, y, w*2/3 - muteWidth, dh, ct);
        addLabel("Level", x+w*2/3, y, w/3, dh, ct);
        
        mSpSelect = new ComboBox();
        mTabs->getTabContentComponent(4)->addAndMakeVisible(mSpSelect);
        mComponents.add(mSpSelect);
        mSpSelect->addListener(this);

        updateSpeakers(true);
    }

    int dh = kDefaultLabelHeight;
    int iButtonW = 50;
    
    //--------------- SETTINGS TAB ---------------- //
    Component *box = mTabs->getTabContentComponent(0);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
        //-----------------------------
        // start 1st column
        
        addLabel("Movements:", x, y, w, dh, box);
        y += dh + 5;
        
        {
            mMovementMode = new ComboBox();
            updateMovementModeCombo();
            //mMovementMode->setBounds(x, y, w, dh);
            //box->addAndMakeVisible(mMovementMode);
            mComponents.add(mMovementMode);
            y += dh + 5;
            mMovementMode->addListener(this);
        }
        
        {
            addLabel("Param smoothing (ms):", x, y, w, dh, box);
            y += dh + 5;
            
            Slider *ds = addParamSlider(kParamSmooth, kSmooth, mFilter->getParameter(kSmooth), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mSmoothing = ds;
            y += dh + 5;
        }
        mShowGridLines = addCheckbox("Show grid lines", mFilter->getShowGridLines(), x, y, w, dh, box);
        
        //-----------------------------
        // start 2nd column
        y = kMargin;
        x += w + kMargin;
    
        //only using the combo box in reaper, because other hosts set the inputs and outputs automatically
		if (mFilter->getIsAllowInputOutputModeSelection()) {
            
            
            int iMaxSources = mFilter->getNumInputChannels();
            int iMaxSpeakers = mFilter->getNumOutputChannels();

            addLabel("Input/Output mode:", x, y, w, dh, box);
            y += dh + 5;
            
            mInputOutputModeCombo = new ComboBox();
            if (iMaxSpeakers >=2)  { mInputOutputModeCombo->addItem("1x2",  i1o2);  }
            if (iMaxSpeakers >=4)  { mInputOutputModeCombo->addItem("1x4",  i1o4);  }
            if (iMaxSpeakers >=6)  { mInputOutputModeCombo->addItem("1x6",  i1o6);  }
            if (iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("1x8",  i1o8);  }
            if (iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("1x16", i1o16); }
            
            if (iMaxSources >=2 && iMaxSpeakers >=2)  { mInputOutputModeCombo->addItem("2x2",  i2o2);  }
            if (iMaxSources >=2 && iMaxSpeakers >=4)  { mInputOutputModeCombo->addItem("2x4",  i2o4);  }
            if (iMaxSources >=2 && iMaxSpeakers >=6)  { mInputOutputModeCombo->addItem("2x6",  i2o6);  }
            if (iMaxSources >=2 && iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("2x8",  i2o8);  }
            if (iMaxSources >=2 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("2x16", i2o16); }

            if (iMaxSources >=4 && iMaxSpeakers >=4)  { mInputOutputModeCombo->addItem("4x4",  i4o4);  }
            if (iMaxSources >=4 && iMaxSpeakers >=6)  { mInputOutputModeCombo->addItem("4x6",  i4o6);  }
            if (iMaxSources >=4 && iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("4x8",  i4o8);  }
            if (iMaxSources >=4 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("4x16", i4o16); }

            if (iMaxSources >=6 && iMaxSpeakers >=6)  { mInputOutputModeCombo->addItem("6x6",  i6o6);  }
            if (iMaxSources >=6 && iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("6x8",  i6o8);  }
            if (iMaxSources >=6 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("6x16", i6o16); }

            if (iMaxSources >=8 && iMaxSpeakers >=8)  { mInputOutputModeCombo->addItem("8x8",  i8o8);  }
            if (iMaxSources >=8 && iMaxSpeakers >=16) { mInputOutputModeCombo->addItem("8x16", i8o16); }
            
            mInputOutputModeCombo->setSelectedId(mFilter->getInputOutputMode());
            mInputOutputModeCombo->setSize(w - iButtonW, dh);
            mInputOutputModeCombo->setTopLeftPosition(x, y);
            box->addAndMakeVisible(mInputOutputModeCombo);
            mComponents.add(mInputOutputModeCombo);
            
            mApplyInputOutputModeButton = addButton("Apply", x + w - iButtonW, y, iButtonW, dh, box);
            y += dh + 5;
        }
        addLabel("Routing mode:", x, y, w, dh, box);
        y += dh + 5;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            cb->addItem("Normal", index++);
            cb->addItem("Internal write", index++);
            cb->addItem("Internal read 1-2", index++);
            cb->addItem("Internal read 3-4", index++);
            cb->addItem("Internal read 5-6", index++);
            cb->addItem("Internal read 7-8", index++);
            cb->addItem("Internal read 9-10", index++);
            cb->addItem("Internal read 11-12", index++);
            cb->addItem("Internal read 13-14", index++);
            cb->addItem("Internal read 15-16", index++);
            cb->setSelectedId(mFilter->getRoutingMode() + 1);
            cb->setSize(w, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            y += dh + 5;
            
            cb->addListener(this);
            mRoutingMode = cb;
        }
        
        addLabel("Routing volume (dB):", x, y, w, dh, box);
        y += dh + 5;
        {
            Slider *ds = addParamSlider(kParamRoutingVolume, kRoutingVolume, mFilter->getParameter(kRoutingVolume), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mRoutingVolume = ds;
            y += dh + 5;
        }
        //-----------------------------
        // start 3rd column
        y = kMargin;
        x += w + kMargin;
        
        addLabel("Process mode:", x, y, w, dh, box);
        y += dh + 5;
        
        {
            mProcessModeCombo = new ComboBox();
            int index = 1;
            mProcessModeCombo->addItem("Free volume", index++);
            mProcessModeCombo->addItem("Pan volume", index++);
            mProcessModeCombo->addItem("Pan span", index++);
            mProcessModeCombo->setSelectedId(mFilter->getProcessMode() + 1);
            mProcessModeCombo->setSize(w, dh);
            mProcessModeCombo->setTopLeftPosition(x, y);
            box->addAndMakeVisible(mProcessModeCombo);
            mComponents.add(mProcessModeCombo);
            y += dh + 5;
            
            mProcessModeCombo->addListener(this);
        }
        
        {
            addLabel("Max span volume (dB):", x, y, w, dh, box);
            y += dh + 5;
            
            Slider *ds = addParamSlider(kParamMaxSpanVolume, kMaxSpanVolume, mFilter->getParameter(kMaxSpanVolume), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mMaxSpanVolume = ds;
            y += dh + 5;
        }
    }
    
    //--------------- TRAJECTORIES TAB ---------------- //
    box = mTabs->getTabContentComponent(1);
    {
        //---------- ROW 1 -------------
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
        int cbw = 130;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            for (int i = 1; i < Trajectory::NumberOfTrajectories(); i++){
                cb->addItem(Trajectory::GetTrajectoryName(i), index++);
            }
            cb->setSelectedId(mFilter->getTrType());
            cb->setSize(cbw, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrTypeComboBox = cb;
            mTrTypeComboBox->addListener(this);
        }
        
        {
            ComboBox *cb = new ComboBox();
            cb->setSize(cbw, dh);
            cb->setTopLeftPosition(x+cbw+5, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrDirectionComboBox = cb;
            mTrDirectionComboBox->addListener(this);
        }
        
        {
            ComboBox *cb = new ComboBox();
            cb->setSize(cbw-40, dh);
            cb->setTopLeftPosition(x+2*(cbw+5), y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrReturnComboBox = cb;
            mTrReturnComboBox->addListener(this);
        }
        
        
        mTrSeparateAutomationMode = addCheckbox("Force separate automation", mFilter->getShowGridLines(), x+3*(cbw+5)-40, y, cbw+20, dh, box);
        
        int tewShort = 30;
        int x2 = x+3*(cbw+5);
        mTrDampeningTextEditor = addTextEditor(String(mFilter->getTrDampening()), x2, y, tewShort, dh, box);
        mTrDampeningTextEditor->addListener(this);
        mTrDampeningLabel = addLabel("dampening", x2 + tewShort, y, w, dh, box);
        
        mTrTurnsTextEditor = addTextEditor(String(mFilter->getTrTurns()), x2, y, tewShort, dh, box);
        mTrTurnsTextEditor->addListener(this);
        mTrTurnsLabel = addLabel("turn(s)", x2 + tewShort, y, w, dh, box);

        
        //---------- ROW 2 -------------
        y += dh + 5;
        x = kMargin;
        int tew = 80;
        
        mTrDuration = addTextEditor(String(mFilter->getTrDuration()), x, y, tew, dh, box);
        mTrDuration->addListener(this);
        x += tew + kMargin;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            cb->addItem("Beat(s)", index++);
            cb->addItem("Second(s)", index++);
            
            cb->setSelectedId(mFilter->getTrUnits());
            cb->setSize(tew, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrUnits = cb;
            mTrUnits->addListener(this);
        }
        x += tew + kMargin;
        addLabel("per cycle", x, y, w, dh, box);
        
        mTrDeviationTextEditor = addTextEditor(String(mFilter->getTrDeviation()), x2, y, tewShort, dh, box);
        mTrDeviationTextEditor->addListener(this);
        mTrDeviationLabel = addLabel("deviation", x2 + tewShort, y, w, dh, box);
        
        //---------- ROW 3 -------------
        y += dh + 5;
        x = kMargin;
        
        mTrRepeats = addTextEditor(String(mFilter->getTrRepeats()), x, y, tew, dh, box);
        mTrRepeats->addListener(this);
        x += tew + kMargin;
        
        addLabel("cycle(s)", x, y, w, dh, box);
        
        //---------- ROW 4 -------------
        y += dh + 5;
        x = kMargin;
        
        mTrEndPointButton = addButton("Set end point", x, y, cbw, dh, box);
        mTrEndPointButton->setClickingTogglesState(true);
        
        x += cbw + kMargin;
        m_pTrEndRayTextEditor = addTextEditor("", x, y, cbw/2, dh, box);
        m_pTrEndRayTextEditor->setTextToShowWhenEmpty("Ray", juce::Colour::greyLevel(.6));
        m_pTrEndRayTextEditor->setColour(TextEditor::textColourId, juce::Colour::greyLevel(.6));
        m_pTrEndRayTextEditor->setReadOnly(true);
        m_pTrEndRayTextEditor->setCaretVisible(false);
        
        x += cbw/2 + kMargin;
        m_pTrEndAngleTextEditor = addTextEditor("", x, y, cbw/2, dh, box);
        m_pTrEndAngleTextEditor->setTextToShowWhenEmpty("Angle", juce::Colour::greyLevel(.6));
        m_pTrEndAngleTextEditor->setColour(TextEditor::textColourId, juce::Colour::greyLevel(.6));
        m_pTrEndAngleTextEditor->setReadOnly(true);
        m_pTrEndAngleTextEditor->setCaretVisible(false);
        updateEndLocationTextEditors();

        x += cbw/2 + kMargin;
        m_pTrResetEndButton = addButton("Reset end point", x, y, cbw, dh, box);
        
        x = kMargin;
        y += dh + 5;
        
        mTrWriteButton = addButton("Ready", x, y, cbw, dh, box);
        mTrWriteButton->setClickingTogglesState(true);
        y += dh + 5;
        
        mTrProgressBar = new MiniProgressBar();
        mTrProgressBar->setSize(tew, dh);
        mTrProgressBar->setTopLeftPosition(x, y);
        
        Trajectory::Ptr t = mFilter->getTrajectory();
        if (t){
            mTrProgressBar->setVisible(true);
        } else {
            mTrProgressBar->setVisible(false);
        }
        
        box->addChildComponent(mTrProgressBar);
        mComponents.add(mTrProgressBar);
        
        mTrStateEditor = mFilter->getTrState();
        if (mTrStateEditor == kTrWriting){
            mTrWriteButton->setToggleState(true, dontSendNotification);
            mTrWriteButton->setButtonText("Cancel");
        }
        
        x = 2*cbw + 2*kMargin;
        y = kMargin + dh + 5;
        addLabel("Movements:", x, y, w-50, dh, box);
        
    }
    
    //--------------- V & F TAB ---------------- //
    box = mTabs->getTabContentComponent(2);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
        //-----------------------------
        // start 1st column
        
        {
            addLabel("Volume center (dB):", x, y, w, dh, box);
            y += dh + 5;
            
            Slider *ds = addParamSlider(kParamVolumeNear, kVolumeNear, mFilter->getParameter(kVolumeNear), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mVolumeNear = ds;
            y += dh + 5;
        }
        
        {
            addLabel("Filter center:", x, y, w, dh, box);
            y += dh + 5;
            
            Slider *ds = addParamSlider(kParamFilterNear, kFilterNear, mFilter->getParameter(kFilterNear), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mFilterNear = ds;
            y += dh + 5;
        }
        
        mApplyFilter = addCheckbox("Apply Filter", mFilter->getApplyFilter(),
                                   x, y, w, dh, box);
        y += dh + 5;
        
        //-----------------------------
        // start 2nd column
        y = kMargin;
        x += w + kMargin;
        
        {
            addLabel("Volume speakers (dB):", x, y, w, dh, box);
            y += dh + 5;
            
            Slider *ds = addParamSlider(kParamVolumeMid, kVolumeMid, mFilter->getParameter(kVolumeMid), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mVolumeMid = ds;
            y += dh + 5;
        }
        
        {
            addLabel("Filter speakers:", x, y, w, dh, box);
            y += dh + 5;
            
            Slider *ds = addParamSlider(kParamFilterMid, kFilterMid, mFilter->getParameter(kFilterMid), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mFilterMid = ds;
            y += dh + 5;
        }
        
        //-----------------------------
        // start 3rd column
        y = kMargin;
        x += w + kMargin;
        
        {
            addLabel("Volume far (dB):", x, y, w, dh, box);
            y += dh + 5;
            
            Slider *ds = addParamSlider(kParamVolumeFar, kVolumeFar, mFilter->getParameter(kVolumeFar), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mVolumeFar = ds;
            y += dh + 5;
        }
        
        {
            addLabel("Filter far:", x, y, w, dh, box);
            y += dh + 5;
            
            Slider *ds = addParamSlider(kParamFilterFar, kFilterFar, mFilter->getParameter(kFilterFar), x, y, w, dh, box);
            ds->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
            mFilterFar = ds;
            y += dh + 5;
        }
    }
    
    //--------------- SOURCES TAB ---------------- //
    box = mTabs->getTabContentComponent(3);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        int selectw = 50;
        
        // column 1
        addLabel("Source placement:", x, y, w, dh, box);
        y += dh + 5;
        
        mSrcPlacement = new ComboBox();
        mSrcPlacement->addItem("Left Alternate", kLeftAlternate);
        mSrcPlacement->addItem("Left Clockwise", kLeftClockwise);
        mSrcPlacement->addItem("Left Counter Clockwise", kLeftCounterClockWise);
        mSrcPlacement->addItem("Top Clockwise", kTopClockwise);
        mSrcPlacement->addItem("Top Counter Clockwise", kTopCounterClockwise);
        
        mSrcPlacement->setSelectedId(mFilter->getSrcPlacementMode());
        box->addAndMakeVisible(mSrcPlacement);
        mComponents.add(mSrcPlacement);
        mSrcPlacement->setSize(w, dh);
        mSrcPlacement->setTopLeftPosition(x, y);
        mSrcPlacement->setExplicitFocusOrder(5);
        //mSrcPlacement->addListener(this);
        y += dh + 5;
        mApplySrcPlacementButton = addButton("Apply", x, y, iButtonW, dh, box);
        
        // column 2
        y = kMargin;
        x += w + kMargin;
        
        addLabel("Set RA position:", x, y, w - selectw, dh, box);
        mSrcSelect->setSelectedId(mFilter->getSrcSelected()+1);
        mSrcSelect->setSize(selectw, dh);
        mSrcSelect->setTopLeftPosition(x + w - selectw, y);
        mSrcSelect->setExplicitFocusOrder(5);
        
        int lw = 60, lwm = lw + kMargin;
        
        y += dh + 5;
        
        addLabel("R: 0 to 2, A: 0 to 360", x, y, w, dh, box);
        y += dh + 5;
        
        addLabel("Ray:", x, y, lw, dh, box);
        mSrcR = addTextEditor("1", x + lwm, y, w - lwm, dh, box);
        mSrcR->setExplicitFocusOrder(6);
        mSrcR->addListener(this);
        y += dh + 5;
        
        addLabel("Angle:", x, y, lw, dh, box);
        mSrcT = addTextEditor("0", x + lwm, y, w - lwm, dh, box);
        mSrcT->setExplicitFocusOrder(7);
        mSrcT->addListener(this);
        
    }
    //--------------- SPEAKERS TAB ---------------- //
    box = mTabs->getTabContentComponent(4);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        int selectw = 50;
        
        //-------- column 1 --------
        addLabel("Speaker placement:", x, y, w, dh, box);
        y += dh + 5;
        
        mSpPlacement = new ComboBox();
        mSpPlacement->addItem("Left Alternate", kLeftAlternate);
        mSpPlacement->addItem("Left Clockwise", kLeftClockwise);
        mSpPlacement->addItem("Left Counter Clockwise", kLeftCounterClockWise);
        mSpPlacement->addItem("Top Clockwise", kTopClockwise);
        mSpPlacement->addItem("Top Counter Clockwise", kTopCounterClockwise);
        
        mSpPlacement->setSelectedId(mFilter->getSpPlacementMode());
        
        box->addAndMakeVisible(mSpPlacement);
        mComponents.add(mSpPlacement);
        mSpPlacement->setSize(w, dh);
        mSpPlacement->setTopLeftPosition(x, y);
        mSpPlacement->setExplicitFocusOrder(5);
        //mSpPlacement->addListener(this);
        y += dh + 5;
        mApplySpPlacementButton = addButton("Apply", x, y, iButtonW, dh, box);
        
        
        //-------- column 2 --------
        y = kMargin;
        x += w + kMargin;
        
        addLabel("Set RA position:", x, y, w - selectw, dh, box);
        mSpSelect->setSelectedId(mFilter->getSpSelected());
        mSpSelect->setSize(selectw, dh);
        mSpSelect->setTopLeftPosition(x + w - selectw, y);
        mSpSelect->setExplicitFocusOrder(5);
        
        int lw = 60, lwm = lw + kMargin;
        
        
        y += dh + 5;
        addLabel("R: 0 to 2, A: 0 to 360", x, y, w, dh, box);
        y += dh + 5;
        addLabel("Ray:", x, y, lw, dh, box);
        mSpR = addTextEditor("1", x + lwm, y, w - lwm, dh, box);
        mSpR->setExplicitFocusOrder(6);
        mSpR->addListener(this);
        
        y += dh + 5;
        addLabel("Angle:", x, y, lw, dh, box);
        mSpT = addTextEditor("0", x + lwm, y, w - lwm, dh, box);
        mSpT->setExplicitFocusOrder(7);
        mSpT->addListener(this);
	}
    
    

    //--------------- INTERFACE TAB ---------------- //
#if WIN32
    
#else
    box = mTabs->getTabContentComponent(5);
    {
        int x = kMargin, y = kMargin;
        const int m = 10, dh = 18, cw = 300;
        int comboW = 40, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
        addLabel(leapSupported ? "OSC/Leap source:" : "OSC source:", x, y, w-comboW, dh, box);
        {
            mOscLeapSourceCb = new ComboBox();
            int index = 1;
            for (int i = 0; i < mFilter->getNumberOfSources(); i++)
            {
                String s; s << i+1;
                mOscLeapSourceCb->addItem(s, index++);
            }
            
            mOscLeapSourceCb->setSelectedId(mFilter->getOscLeapSource() + 1);
            mOscLeapSourceCb->setSize(comboW, dh);
            mOscLeapSourceCb->setTopLeftPosition(x+w-comboW, y);
            box->addAndMakeVisible(mOscLeapSourceCb);
            mComponents.add(mOscLeapSourceCb);            
            mOscLeapSourceCb->addListener(this);
        }
        
        y += dh + 5;
        
        mEnableLeap = new ToggleButton();
        mEnableLeap->setButtonText("Enable Leap");
        mEnableLeap->setSize(cw-100, dh);
        mEnableLeap->setTopLeftPosition(x, y);
        mEnableLeap->addListener(this);
        mEnableLeap->setToggleState(false, dontSendNotification);
        box->addAndMakeVisible(mEnableLeap);
        mComponents.add(mEnableLeap);
        
        mStateLeap = new Label();
        mStateLeap->setText("", dontSendNotification);
        mStateLeap->setSize(cw, dh);
        mStateLeap->setJustificationType(Justification::left);
        mStateLeap->setMinimumHorizontalScale(1);
        mStateLeap->setTopLeftPosition(x+cw-150+ m, y);
        box->addAndMakeVisible(mStateLeap);
        mComponents.add(mStateLeap);
        
        y += dh + 10;
        
        mEnableJoystick = new ToggleButton();
        mEnableJoystick->setButtonText("Enable Joystick");
        mEnableJoystick->setSize(cw-150, dh);
        mEnableJoystick->setTopLeftPosition(x, y);
        mEnableJoystick->addListener(this);
        mEnableJoystick->setToggleState(false, dontSendNotification);
        box->addAndMakeVisible(mEnableJoystick);
        mComponents.add(mEnableJoystick);
        
        mStateJoystick = new Label();
        mStateJoystick->setText("", dontSendNotification);
        mStateJoystick->setSize(cw, dh);
        mStateJoystick->setJustificationType(Justification::left);
        mStateJoystick->setMinimumHorizontalScale(1);
        mStateJoystick->setTopLeftPosition(x+cw-150+ m, y);
        box->addAndMakeVisible(mStateJoystick);
        mComponents.add(mStateJoystick);
        
        y += dh;
        
        mOsc = CreateOscComponent(mFilter, this);
        if (mOsc) {
            mOsc->setTopLeftPosition(0, y);
            mOsc->setSize(box->getWidth(), box->getHeight()-y);
            box->addAndMakeVisible(mOsc);
            mComponents.add(mOsc);

            
            
//            mTabs->addTab("OSC",  tabBg, mOsc, true);
        }
    }
#endif
    
    int selectedTab = mFilter->getGuiTab();
    if (selectedTab >= 0 && selectedTab < mTabs->getNumTabs())
    {
        bool sendChangeMessage = false;
        mTabs->setCurrentTabIndex(selectedTab, sendChangeMessage);
    }
    
    mTabs->initDone();
    
    mFilter->setCalculateLevels(true);
    
    //resizable corner
    m_oResizeLimits.setSizeLimits (960-150, 420-150, 1560, 1020);
    addAndMakeVisible (m_pResizer = new ResizableCornerComponent (this, &m_oResizeLimits));
    setSize (mFilter->getGuiWidth(), mFilter->getGuiHeight());
    //refreshSize();
}

void OctogrisAudioProcessorEditor::updateEndLocationTextEditors(){
    std::pair<float, float> endLocation = mFilter->getEndLocationXY();
    FPoint pointRT = mFilter->convertXy012Rt(FPoint(endLocation.first, 1-endLocation.second), false);
    pointRT.y *= 360/(2*M_PI);
    {
        ostringstream oss;
        oss << std::fixed << std::right << std::setw( 4 ) << setprecision(2) << std::setfill( ' ' ) << "" <<  pointRT.x;
        m_pTrEndRayTextEditor->setText(oss.str());
    }
    {
        ostringstream oss;
        oss << std::fixed << std::right << std::setw( 4 ) << setprecision(2) << std::setfill( ' ' ) << "" << pointRT.y;
        m_pTrEndAngleTextEditor->setText(oss.str());
    }
}

void OctogrisAudioProcessorEditor::updateNonSelectedSourcePositions(){
    int iSourceChanged = mFilter->getSourceLocationChanged();
//    if (!mFilter->getIsRecordingAutomation() && mFilter->getMovementMode() != 0 && iSourceChanged != -1) {
    if (iSourceChanged != -1){
        mMover.begin(iSourceChanged, kSourceThread);
        mMover.move(mFilter->getSourceXY01(iSourceChanged), kSourceThread);
        mMover.end(kSourceThread);
        mFilter->setSourceLocationChanged(-1);
    }
}

void OctogrisAudioProcessorEditor::updateTrajectoryComponents(){
    int iSelectedTrajectory = mFilter->getTrType();
    //if pendulum is selected

    if (iSelectedTrajectory == Pendulum){
        setDefaultPendulumEndpoint();
        updateEndLocationTextEditors();
        
        mTrDampeningTextEditor->setVisible(true);
        mTrDampeningLabel->setVisible(true);
        mTrDeviationTextEditor->setVisible(true);
        mTrDeviationLabel->setVisible(true);
    } else {
        mTrDampeningTextEditor->setVisible(false);
        mTrDampeningLabel->setVisible(false);
        mTrDeviationTextEditor->setVisible(false);
        mTrDeviationLabel->setVisible(false);
    }
    
    if (iSelectedTrajectory >= Circle & iSelectedTrajectory <= Spiral){
        mTrTurnsTextEditor->setVisible(true);
        mTrTurnsLabel->setVisible(true);
    } else {
        mTrTurnsTextEditor->setVisible(false);
        mTrTurnsLabel->setVisible(false);
    }
    
    if (iSelectedTrajectory == Spiral || iSelectedTrajectory == Pendulum){
        mTrEndPointButton->setVisible(true);
        m_pTrEndRayTextEditor->setVisible(true);
        m_pTrEndAngleTextEditor->setVisible(true);
        m_pTrResetEndButton->setVisible(true);
    } else {
        mTrEndPointButton->setVisible(false);
        m_pTrEndRayTextEditor->setVisible(false);
        m_pTrEndAngleTextEditor->setVisible(false);
        m_pTrResetEndButton->setVisible(false);
    }

    if (iSelectedTrajectory == RandomTarget || iSelectedTrajectory == RandomTrajectory){
        mTrSeparateAutomationMode->setVisible(true);
    } else {
        mTrSeparateAutomationMode->setVisible(false);
    }
    
    unique_ptr<vector<String>> allDirections = Trajectory::getTrajectoryPossibleDirections(iSelectedTrajectory);
    if (allDirections != nullptr){
        mTrDirectionComboBox->clear();
        for(auto it = allDirections->begin(); it != allDirections->end(); ++it){
            mTrDirectionComboBox->addItem(*it, it - allDirections->begin()+1);
        }
        mTrDirectionComboBox->setVisible(true);
        
        int iSelectedDirection = mFilter->getTrDirection()+1;
        
        if (iSelectedDirection > allDirections->size()){
            iSelectedDirection = 1;
            mFilter->setTrDirection(iSelectedDirection);
        }
        mTrDirectionComboBox->setSelectedId(iSelectedDirection);
        
    } else {
        mTrDirectionComboBox->setVisible(false);
    }
    
    unique_ptr<vector<String>> allReturns = Trajectory::getTrajectoryPossibleReturns(iSelectedTrajectory);
    if (allReturns != nullptr){
        mTrReturnComboBox->clear();
        for(auto it = allReturns->begin(); it != allReturns->end(); ++it){
            mTrReturnComboBox->addItem(*it, it - allReturns->begin()+1);
        }
        mTrReturnComboBox->setVisible(true);
        mTrReturnComboBox->setSelectedId(mFilter->getTrReturn()+1);
    } else {
        mTrReturnComboBox->setVisible(false);
    }
}

OctogrisAudioProcessorEditor::~OctogrisAudioProcessorEditor()
{
    mFilter->setCalculateLevels(false);
    mFilter->removeListener(this);
#if WIN32
    
#else
    if(mEnableJoystick->getToggleState())
    {
        IOHIDManagerUnscheduleFromRunLoop(gIOHIDManagerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
        IOHIDManagerRegisterInputValueCallback(gIOHIDManagerRef, NULL,this);
        IOHIDManagerClose(gIOHIDManagerRef, kIOHIDOptionsTypeNone);
        gIOHIDManagerRef = NULL;
        gDeviceCFArrayRef = NULL;
        gElementCFArrayRef = NULL;
    }
    mJoystick = NULL;
    if(mController)
    {
        mController->enableGesture(Leap::Gesture::TYPE_INVALID);
        mController=NULL;
        gIsLeapConnected = 0;
    }
    getMover()->end(kLeap);
    getMover()->end(kHID);
#endif
}

void OctogrisAudioProcessorEditor::resized()
{
    int w = getWidth();
    int h = getHeight();
    
    mFilter->setGuiWidth(w);
    mFilter->setGuiHeight(h);
    
    m_pResizer->setBounds (w - 16, h - 16, 16, 16);
    
    int fieldWidth = w - (kMargin + kMargin + kCenterColumnWidth + kMargin + kRightColumnWidth + kMargin);
    int fieldHeight = h - (kMargin + kMargin);
    int fieldSize = jmin(fieldWidth, fieldHeight);
    
    mField->setBounds(kMargin, kMargin, fieldSize, fieldSize);

    m_logoImage.setBounds(15, 15, (float)fieldSize/7, (float)fieldSize/7);
    
    int iLabelX = 2*(float)fieldSize/3;
    m_VersionLabel->setBounds(iLabelX,5,fieldSize-iLabelX,25);
    
    int x = kMargin + fieldSize  + kMargin;
    int y = kMargin;
    int iExtraSpace = 10;
    mSourcesBoxLabel->setTopLeftPosition(x, y);
    
    int lh = mSourcesBoxLabel->getHeight() + 2;
    mSourcesBox->setBounds(x, y + lh, kCenterColumnWidth, h - (kMargin + kParamBoxHeight + kMargin + y + lh + iExtraSpace));
    
    mTabs->setBounds(x, h - (kParamBoxHeight + kMargin + iExtraSpace), kCenterColumnWidth + kMargin + kRightColumnWidth, kParamBoxHeight + iExtraSpace);
    
    x += kCenterColumnWidth + kMargin;
    mSpeakersBoxLabel->setTopLeftPosition(x, y);
    mSpeakersBox->setBounds(x, y + lh, kRightColumnWidth, h - (kMargin + kParamBoxHeight + kMargin + y + lh + iExtraSpace));
}


void OctogrisAudioProcessorEditor::updateSources(bool p_bCalledFromConstructor){
    
    int dh = kDefaultLabelHeight, x = 0, y = 0, w = kCenterColumnWidth;
    
    Component *ct = mSourcesBox->getContent();
    
    //remove old stuff
    for (int iCurLevelComponent = 0; iCurLevelComponent < mDistances.size(); ++iCurLevelComponent){
        ct->removeChildComponent(mDistances.getUnchecked(iCurLevelComponent));
        ct->removeChildComponent(mLabels.getUnchecked(iCurLevelComponent));
    }
    mDistances.clear();
    mLabels.clear();

    if (!p_bCalledFromConstructor){
        mSrcSelect->clear(dontSendNotification);
        mMovementMode->clear(dontSendNotification);
        updateMovementModeCombo();
    }

    //put new stuff
    int iCurSources = mFilter->getNumberOfSources();
    
    bool bIsFreeVolumeMode = mFilter->getProcessMode() == kPanVolumeMode;
    y += dh + 5;
    for (int i = 0; i < iCurSources; i++){
        String s; s << i+1; s << ":";
        Component *label = addLabel(s, x, y, w/3, dh, ct);
        mLabels.add(label);
        
        float distance = mFilter->getSourceD(i);
        Slider *slider = addParamSlider(kParamSource, i, distance, x + w/3, y, w*2/3, dh, ct);
        
        if (bIsFreeVolumeMode){
            slider->setEnabled(false);
        }
        mDistances.add(slider);
        
        y += dh + 5;
    }
    
    ct->setSize(w, y);
    
    mMover.updateNumberOfSources();
    
    if (!p_bCalledFromConstructor){
        buttonClicked(mApplySrcPlacementButton);
        //comboBoxChanged(mSrcPlacement);
    }
    
    //source position combobox in source tab
    int index = 1;
    for (int i = 0; i < iCurSources; i++){
        String s; s << i+1;
        mSrcSelect->addItem(s, index++);
    }
    mSrcSelect->setSelectedId(mFilter->getSrcSelected()+1);
}


void OctogrisAudioProcessorEditor::updateSpeakers(bool p_bCalledFromConstructor){

    //remove old stuff
    Component *ct = mSpeakersBox->getContent();
    for (int iCurLevelComponent = 0; iCurLevelComponent < mLevels.size(); ++iCurLevelComponent){
        ct->removeChildComponent(mMutes.getUnchecked(iCurLevelComponent));
        ct->removeChildComponent(mAttenuations.getUnchecked(iCurLevelComponent));
        ct->removeChildComponent(mLevels.getUnchecked(iCurLevelComponent));
        
        mComponents.removeObject(mLevels.getUnchecked(iCurLevelComponent));
    }
    mMutes.clear();
    mAttenuations.clear();
    mLevels.clear();
    mSpSelect->clear();
    
    
    //put new stuff
    int iCurSpeakers = mFilter->getNumberOfSpeakers();
   	int dh = kDefaultLabelHeight, x = 0, y = 0, w = kRightColumnWidth;
    
    const int muteWidth = 50;
    y += dh + 5;

    for (int i = 0; i < iCurSpeakers; i++){
        String s; s << i+1; s << ":";
        
		float fMute = mFilter->getSpeakerM(i);
		ToggleButton *mute = addCheckbox(s, fMute, x, y, muteWidth, dh, ct);
        mMutes.add(mute);
        
        float att = mFilter->getSpeakerA(i);
        Slider *slider = addParamSlider(kParamSpeaker, i, att, x+muteWidth, y, w*2/3 - muteWidth, dh, ct);
        
        slider->setTextBoxStyle(Slider::TextBoxLeft, false, 40, dh);
        mAttenuations.add(slider);
        
        Rectangle<int> level(x+w*2/3, y + 3, w/3 - 10, dh - 6);
        
        LevelComponent *lc = new LevelComponent(mFilter, i);
        lc->setBounds(level);
        ct->addAndMakeVisible(lc);
        mComponents.add(lc);
        mLevels.add(lc);
        
        y += dh + 5;
    }
    
    ct->setSize(w, y);
    
    if (!p_bCalledFromConstructor){
        buttonClicked(mApplySpPlacementButton); 
    }
    
    
    //speaker position combo box in speakers tab
    int index = 1;
    for (int i = 0; i < iCurSpeakers; i++)
    {
        String s; s << i+1;
        mSpSelect->addItem(s, index++);
    }
    mSpSelect->setSelectedId(mFilter->getSpSelected());
}

void OctogrisAudioProcessorEditor::updateMovementModeCombo(){
    int index = 1;
    mMovementMode->addItem("Independent", index++);
    if (mFilter->getNumberOfSources() > 1){
        mMovementMode->addItem("Circular", index++);
        mMovementMode->addItem("Circular Fixed Radius", index++);
        mMovementMode->addItem("Circular Fixed Angle", index++);
        mMovementMode->addItem("Circular Fully Fixed", index++);
        mMovementMode->addItem("Delta Lock", index++);
        if (mFilter->getNumberOfSources() == 2){
            mMovementMode->addItem("Symmetric X", index++);
            mMovementMode->addItem("Symmetric Y", index++);
            //mMovementMode->addItem("Symmetric X & Y", index++);
        }
    }
    int iCurMode = mFilter->getMovementMode() + 1;
    //iCurMode > mMovementMode->getNumItems() ? mMovementMode->setSelectedId(1) : mMovementMode->setSelectedId(iCurMode);
    mMovementMode->setSelectedId(iCurMode);
    
}


void OctogrisAudioProcessorEditor::setOscLeapSource(int s)
{
    if (s < 0) s = 0;
    if (s >= mFilter->getNumberOfSources()) s = mFilter->getNumberOfSources() - 1;
    mFilter->setOscLeapSource(s);
    
    const MessageManagerLock mmLock;
    mOscLeapSourceCb->setSelectedId(s + 1);
}



//==============================================================================
Component* OctogrisAudioProcessorEditor::addLabel(const String &s, int x, int y, int w, int h, Component *into)
{
    Label *label = new Label();
    label->setText(s, dontSendNotification);
    label->setSize(w, h);
    label->setJustificationType(Justification::left);
    label->setMinimumHorizontalScale(1);
    label->setTopLeftPosition(x, y);
    into->addAndMakeVisible(label);
    mComponents.add(label);
    return label;
}

ToggleButton* OctogrisAudioProcessorEditor::addCheckbox(const String &s, bool v, int x, int y, int w, int h, Component *into)
{
    ToggleButton *tb = new ToggleButton();
    tb->setButtonText(s);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    tb->setToggleState(v, dontSendNotification);
    into->addAndMakeVisible(tb);
    mComponents.add(tb);
    return tb;
}

TextButton* OctogrisAudioProcessorEditor::addButton(const String &s, int x, int y, int w, int h, Component *into)
{
    TextButton *tb = new TextButton();
    tb->setButtonText(s);
    tb->setSize(w, h);
    tb->setTopLeftPosition(x, y);
    tb->addListener(this);
    into->addAndMakeVisible(tb);
    mComponents.add(tb);
    return tb;
}


TextEditor* OctogrisAudioProcessorEditor::addTextEditor(const String &s, int x, int y, int w, int h, Component *into)
{
    TextEditor *te = new TextEditor();
    te->setText(s);
    te->setSize(w, h);
    te->setTopLeftPosition(x, y);
    into->addAndMakeVisible(te);
    mComponents.add(te);
    return te;
}

Slider* OctogrisAudioProcessorEditor::addParamSlider(int paramType, int si, float v, int x, int y, int w, int h, Component *into)
{
    int index ;
    if (paramType == kParamSource) index = mFilter->getParamForSourceD(si);
    else if (paramType == kParamSpeaker) index = mFilter->getParamForSpeakerA(si);
    else index = si;
    
    if (paramType == kParamSource)
        v = 1.f - v;
    
    ParamSlider *ds = new ParamSlider(index, paramType, (paramType == kParamSource) ? mLinkDistances : NULL, mFilter);
    ds->setRange(0, 1);
    ds->setValue(v);
    ds->setTextBoxStyle(Slider::NoTextBox, true, 0, 0);
    ds->setSize(w, h);
    ds->setTopLeftPosition(x, y);
    into->addAndMakeVisible(ds);
    mComponents.add(ds);
    return ds;
}


//==============================================================================
void OctogrisAudioProcessorEditor::textEditorReturnKeyPressed(TextEditor & textEditor){
    if (&textEditor == mSrcR || &textEditor == mSrcT){
        int src = mSrcSelect->getSelectedId() - 1;
        float r = mSrcR->getText().getFloatValue();
        float t = mSrcT->getText().getFloatValue();
        if (r < 0) r = 0; else if (r > kRadiusMax) r = kRadiusMax;

        mMover.begin(src, kField);
        mMover.move(mFilter->convertRt2Xy01(r, t * M_PI / 180.), kField);
        mMover.end(kField);
    }
    else if (&textEditor == mSpR || &textEditor == mSpT) {
        int sp = mSpSelect->getSelectedId() - 1;
        float r = mSpR->getText().getFloatValue();
        float t = mSpT->getText().getFloatValue();
        if (r < 0) r = 0; else if (r > kRadiusMax) r = kRadiusMax;
        mFilter->setSpeakerRT(sp, FPoint(r, t * M_PI / 180.));
    }
    else if (&textEditor == mTrDuration){
        float duration = mTrDuration->getText().getFloatValue();
        if (duration >= 0 && duration <= 10000){
            mFilter->setTrDuration(duration);
        }
        mTrDuration->setText(String(mFilter->getTrDuration()));
    }
    else if (&textEditor == mTrRepeats){
        float repeats = mTrRepeats->getText().getFloatValue();
        if (repeats >= 0 && repeats <= 10000){
            mFilter->setTrRepeats(repeats);
        }
        mTrRepeats->setText(String(mFilter->getTrRepeats()));
    }
    else if (&textEditor == mTrDampeningTextEditor){
        float dampening = mTrDampeningTextEditor->getText().getFloatValue();
        if (dampening >= 0 && dampening <= 1){
            mFilter->setTrDampening(dampening);
        }
        mTrDampeningTextEditor->setText(String(mFilter->getTrDampening()));
    }
    else if (&textEditor == mTrDeviationTextEditor){
        float deviation = mTrDeviationTextEditor->getText().getFloatValue()/360;
        if (deviation >= 0 && deviation <= 1){
            mFilter->setTrDeviation(deviation);
        }
        mTrDeviationTextEditor->setText(String(mFilter->getTrDeviation()*360));
    }
    else if (&textEditor == mTrTurnsTextEditor){
        float Turns = mTrTurnsTextEditor->getText().getFloatValue();
        int iSelectedTrajectory = mFilter->getTrType();
        int iUpperLimit = 1;
        if (iSelectedTrajectory == 3){
            iUpperLimit = 10;
        }
        if (Turns >= 0 && Turns <= iUpperLimit){
            mFilter->setTrTurns(Turns);
        }
        mTrTurnsTextEditor->setText(String(mFilter->getTrTurns()));
    }
    else {
        printf("unknown TextEditor clicked...\n");
    }
    
    //if called from actually pressing enter, put focus on something else
    if (!m_bIsReturnKeyPressedCalledFromFocusLost){
        mLinkDistances->grabKeyboardFocus();
    }
    
}

void OctogrisAudioProcessorEditor::buttonClicked (Button *button){
    for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++) {
        if (button == mMutes[i]) {
            float v = button->getToggleState() ? 1.f : 0.f;
            mFilter->setParameterNotifyingHost(mFilter->getParamForSpeakerM(i), v);
            mField->repaint();
            return;
        }
    }
    
    if (button == mTrWriteButton) {
        Trajectory::Ptr t = mFilter->getTrajectory();
        //a trajectory exists, so we want to cancel it
        if (t) {
            mFilter->setTrajectory(NULL);
            mFilter->setIsRecordingAutomation(false);
            mFilter->restoreCurrentLocations();
            mTrWriteButton->setButtonText("Ready");
            mTrProgressBar->setVisible(false);
            mTrStateEditor = kTrReady;
            mFilter->setTrState(mTrStateEditor);
            t->stop();
            mNeedRepaint = true;
        }
        //a trajectory does not exist, create one
        else {
            float   duration        = mTrDuration->getText().getFloatValue();
            bool    beats           = mTrUnits->getSelectedId() == 1;
            float   repeats         = mTrRepeats->getText().getFloatValue();
            int     type            = mTrTypeComboBox->getSelectedId();
            bool    bReturn         = (mTrReturnComboBox->getSelectedId() == 2);
            float   p_fDampening    = mTrDampeningTextEditor->getText().getFloatValue();
            float   p_fDeviation    = mTrDeviationTextEditor->getText().getFloatValue()/360;
            float   p_fTurns        = mTrTurnsTextEditor->getText().getFloatValue();
            unique_ptr<AllTrajectoryDirections> direction = Trajectory::getTrajectoryDirection(type, mTrDirectionComboBox->getSelectedId());

            mFilter->setIsRecordingAutomation(true);
            mFilter->storeCurrentLocations();
            mFilter->setTrajectory(Trajectory::CreateTrajectory(type, mFilter, &mMover, duration, beats, *direction, bReturn, repeats, p_fDampening, p_fDeviation, p_fTurns, mFilter->getEndLocationXY()));
            mTrWriteButton->setButtonText("Cancel");
            mTrStateEditor = kTrWriting;
            mFilter->setTrState(mTrStateEditor);
            
            mTrProgressBar->setValue(0);
            mTrProgressBar->setVisible(true);
        }
    }
    else if (button == mTrEndPointButton) {
        if (mTrEndPointButton->getToggleState()){
            mTrEndPointButton->setButtonText("Cancel");
            mFilter->setIsSettingEndPoint(true);
//            m_oEndPointLabel.setVisible(true);
            m_pTrEndRayTextEditor->setText("");
            m_pTrEndAngleTextEditor->setText("");
        } else {
            mTrEndPointButton->setButtonText("Set end point");
            mFilter->setIsSettingEndPoint(false);
//            m_oEndPointLabel.setVisible(false);
            updateEndLocationTextEditors();
        }
    }
    else if (button == m_pTrResetEndButton) {
        if (mFilter->getTrType() == Pendulum){
            setDefaultPendulumEndpoint();
        } else {
            pair<float, float> pair = make_pair(.5, .5);
            mFilter->setEndLocationXY(pair);
        }
        updateEndLocationTextEditors();
    }
    else if (mFilter->getIsAllowInputOutputModeSelection() && button == mApplyInputOutputModeButton) {
        int iSelectedMode = mInputOutputModeCombo->getSelectedId();
        mFilter->setInputOutputMode(iSelectedMode);
        
		updateSources(false);
		updateSpeakers(false);
        if (m_bLoadingPreset){
            mFilter->restoreCurrentLocations();
            m_bLoadingPreset = false;
        }
        mField->repaint();
        if (iSelectedMode == i1o2 || iSelectedMode == i1o4 || iSelectedMode == i1o6 || iSelectedMode == i1o8 || iSelectedMode == i1o16){
            mMovementMode->setSelectedId(1);
        }
    }
    else if (button == mApplySrcPlacementButton) {
        
        if (mFilter->getNumberOfSources() == 1){
            mFilter->setSourceRT(0, FPoint(0, 0));
            return;
        }
        
        bool alternate = false;
        bool startAtTop = false;
        bool clockwise = false;
        
        switch (mSrcPlacement->getSelectedId()){
            case kLeftAlternate:
                alternate = true;
                break;
            case kTopClockwise:
                startAtTop = true;
                clockwise = true;
                break;
            case kTopCounterClockwise:
                startAtTop = true;
                break;
            case kLeftClockwise:
                clockwise = true;
                break;
            case kLeftCounterClockWise:
                break;
                
        }
        
        float anglePerSp = kThetaMax / mFilter->getNumberOfSources();
        JUCE_COMPILER_WARNING("this stuff is kind of a replication of processor::setNumberOfSources, although setNumberOfSources is only for default placement")
        if (alternate)
        {
            float offset = startAtTop
            ? (clockwise ? kQuarterCircle : (kQuarterCircle - anglePerSp))
            : (kQuarterCircle - anglePerSp/2);
            float start = offset;
            for (int i = clockwise ? 0 : 1; i < mFilter->getNumberOfSources(); i += 2)
            {
                mFilter->setSourceRT(i, FPoint(kSourceDefaultRadius, offset));
                offset -= anglePerSp;
            }
            
            offset = start + anglePerSp;
            for (int i = clockwise ? 1 : 0; i < mFilter->getNumberOfSources(); i += 2)
            {
                mFilter->setSourceRT(i, FPoint(kSourceDefaultRadius, offset));
                offset += anglePerSp;
            }
        }
        else
        {
            float offset = startAtTop ? kQuarterCircle : kQuarterCircle + anglePerSp/2;
            float delta = clockwise ? -anglePerSp : anglePerSp;
            for (int i = 0; i < mFilter->getNumberOfSources(); i++)
            {
                mFilter->setSourceRT(i, FPoint(1, offset));
                offset += delta;
            }
        }
        updateSourceLocationTextEditor(false);
        mFilter->setSrcPlacementMode(mSrcPlacement->getSelectedId());
    }
    else if (button == mApplySpPlacementButton) {
        
        bool alternate = false;
        bool startAtTop = false;
        bool clockwise = false;
        
        switch (mSpPlacement->getSelectedId()){
            case kLeftAlternate:
                alternate = true;
                break;
            case kTopClockwise:
                startAtTop = true;
                clockwise = true;
                break;
            case kTopCounterClockwise:
                startAtTop = true;
                break;
            case kLeftClockwise:
                clockwise = true;
                break;
            case kLeftCounterClockWise:
                break;
        }
        
        mFilter->updateSpeakerLocation(alternate, startAtTop, clockwise);

        updateSpeakerLocationTextEditor();
        mFilter->setSpPlacementMode(mSpPlacement->getSelectedId());
    }
    else if (button == mShowGridLines) {
        mFilter->setShowGridLines(button->getToggleState());
        mField->repaint();
    }
    else if (button == mTrSeparateAutomationMode) {
        mFilter->setIndependentMode(button->getToggleState());
    }
    else if (button == mLinkDistances) {
        mFilter->setLinkDistances(button->getToggleState());
    }
    else if (button == mApplyFilter) {
        mFilter->setApplyFilter(button->getToggleState());
    }
#if WIN32
    
#else
    //Changements lié a l'ajout de joystick à l'onglet interface
    else if(button == mEnableJoystick) {

        bool bJoystickEnabled = mEnableJoystick->getToggleState();
        mFilter->setIsJoystickEnabled(bJoystickEnabled);
        if (bJoystickEnabled) {
            if (!gIOHIDManagerRef) {
                mStateJoystick->setText("Joystick not connected", dontSendNotification);
                gIOHIDManagerRef = IOHIDManagerCreate(CFAllocatorGetDefault(),kIOHIDOptionsTypeNone);
                if(!gIOHIDManagerRef) {
                    printf("Could not create IOHIDManager");
                } else {
                    mJoystick = HIDDelegate::CreateHIDDelegate(mFilter, this);
                    mJoystick->Initialize_HID(this);
                    if(mJoystick->getDeviceSetRef()) {
                        mStateJoystick->setText("Joystick connected", dontSendNotification);
                    } else {
                        mStateJoystick->setText("Joystick not connected", dontSendNotification);
                        mEnableJoystick->setToggleState(false, dontSendNotification);
                        gIOHIDManagerRef = NULL;
                    }
                }
            } else {
                mEnableJoystick->setToggleState(false, dontSendNotification);
                mStateJoystick->setText("Joystick connected to another Octogris", dontSendNotification);
            }
        } else {
            if(gIOHIDManagerRef) {
                IOHIDManagerUnscheduleFromRunLoop(gIOHIDManagerRef, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
                IOHIDManagerRegisterInputValueCallback(gIOHIDManagerRef, NULL,this);
                IOHIDManagerClose(gIOHIDManagerRef, kIOHIDOptionsTypeNone);
                gIOHIDManagerRef = NULL;
                gDeviceCFArrayRef = NULL;
                gElementCFArrayRef = NULL;
                mJoystick = NULL;
                mStateJoystick->setText("", dontSendNotification);
            }
        }
    }
    
    else if(button == mEnableLeap) {
        bool state = mEnableLeap->getToggleState();
        
        if (state) {
            if (!gIsLeapConnected) {
                mStateLeap->setText("Leap not connected", dontSendNotification);
                mController = new Leap::Controller();
                if(!mController) {
                    printf("Could not create leap controler");
                } else {
                    mleap = OctoLeap::CreateLeapComponent(mFilter, this);
                    if(mleap) {
                        gIsLeapConnected = 1;
                        mController->addListener(*mleap);
                    } else {
                        mStateLeap->setText("Leap not connected", dontSendNotification);
                    }
                }
            } else {
                mStateLeap->setText("Leap used in another Octogris", dontSendNotification);
                mEnableLeap->setToggleState(false, dontSendNotification);
               
            }
        } else {
            if(gIsLeapConnected) {
                mController->enableGesture(Leap::Gesture::TYPE_INVALID);
                mController->removeListener(*mleap);
                mController = NULL;
                gIsLeapConnected = 0;
                mStateLeap->setText("", dontSendNotification);
            }
        }
    }
	//fin de changements lié a l'ajout de joystick à l'onglet leap
    
#endif
    
 else {
		printf("unknown button clicked...\n");
	}
}

void OctogrisAudioProcessorEditor::setDefaultPendulumEndpoint(){
    int iSelectedSrc    = mFilter->getSrcSelected();
    FPoint pointRT      = mFilter->getSourceRT(iSelectedSrc);
    pointRT.y += M_PI;
    JUCE_COMPILER_WARNING("throughout the code, need to check conversions, especially pertaining to the end location of trajectories. Also need to make the y consistent so that we don't revert it in only some cases")
    FPoint pointXY = mFilter->convertRt2Xy01(pointRT.x, pointRT.y);
    mFilter->setEndLocationXY(make_pair(pointXY.x, 1-pointXY.y));
}


void OctogrisAudioProcessorEditor::textEditorFocusLost (TextEditor &textEditor){
    m_bIsReturnKeyPressedCalledFromFocusLost = true;
    textEditorReturnKeyPressed(textEditor);
    m_bIsReturnKeyPressedCalledFromFocusLost = false;
}

void OctogrisAudioProcessorEditor::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == mMovementMode) {
        int iSelectedMode = comboBox->getSelectedId() - 1;
        mFilter->setMovementMode(iSelectedMode);
        if(mFilter->getNumberOfSources() > 1){
            m_pSourceUpdateThread->stopThread(500);
            switch (iSelectedMode) {
                case 2:
                    mMover.setEqualRadius();
                    break;
                case 3:
                    mMover.setEqualAngles();
                    break;
                case 4:
                    mMover.setEqualRadiusAndAngles();
                    break;
                case 6:
                    mMover.setSymmetricX();
                    break;
                case 7:
                    mMover.setSymmetricY();
                    break;
                default:
                    break;
            }
            m_pSourceUpdateThread->startThread();
        }
    }
    else if (comboBox == mRoutingMode) {
		mFilter->setRoutingMode(comboBox->getSelectedId() - 1);
	}
    else if (comboBox == mProcessModeCombo) {
        int iSelectedMode = comboBox->getSelectedId() - 1;
        mFilter->setProcessMode(iSelectedMode);
        if (iSelectedMode == kPanVolumeMode){
            for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                mDistances.getUnchecked(i)->setEnabled(false);
            }
        } else {
            for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
                mDistances.getUnchecked(i)->setEnabled(true);
                mDistances.getUnchecked(i)->valueChanged();
            }
        }
		repaint();
	}
	else if (comboBox == mOscLeapSourceCb)
	{
		mFilter->setOscLeapSource(comboBox->getSelectedId() - 1);
	}

    else if (comboBox == mSrcSelect){
        updateSourceLocationTextEditor(true);
    }
    else if (comboBox == mSpSelect){
        updateSpeakerLocationTextEditor();
    }
    else if (comboBox == mTrUnits)
    {
        mFilter->setTrUnits(mTrUnits->getSelectedId());
    }
    else if (comboBox == mTrTypeComboBox)
    {
        int type = mTrTypeComboBox->getSelectedId();
        mFilter->setTrType(type);
        updateTrajectoryComponents();
    }
    else if (comboBox ==  mTrDirectionComboBox)
    {
        int direction = mTrDirectionComboBox->getSelectedId()-1;
        mFilter->setTrDirection(direction);
    }
    else if (comboBox == mTrReturnComboBox)
    {
        int iReturn = mTrReturnComboBox->getSelectedId()-1;
        mFilter->setTrReturn(iReturn);
    }
    else
    {
        printf("unknown combobox clicked...\n");
    }
}


void OctogrisAudioProcessorEditor::updateSourceLocationTextEditor(bool p_bUpdateFilter){
    int iSelectedSrc = mSrcSelect->getSelectedId();
    iSelectedSrc = (iSelectedSrc <= 0) ? 1: iSelectedSrc;
    if (p_bUpdateFilter){
        mFilter->setSrcSelected(iSelectedSrc-1);
    }
    FPoint curPosition = mFilter->getSourceRT(iSelectedSrc-1);
    mSrcR->setText(String(curPosition.x));
    mSrcT->setText(String(curPosition.y * 180. / M_PI));
}

void OctogrisAudioProcessorEditor::updateSpeakerLocationTextEditor(){
    FPoint curPosition = mFilter->getSpeakerRT(mSpSelect->getSelectedId()-1);
    mSpR->setText(String(curPosition.x));
    mSpT->setText(String(curPosition.y * 180. / M_PI));
}


//==============================================================================
void OctogrisAudioProcessorEditor::timerCallback()
{
	switch(mTrStateEditor)	{
		case kTrWriting: {
			Trajectory::Ptr t = mFilter->getTrajectory();
			if (t) {
				mTrProgressBar->setValue(t->progress());
			} else {
				mTrWriteButton->setButtonText("Ready");
                mTrWriteButton->setToggleState(false, dontSendNotification);
                mFilter->restoreCurrentLocations(mFilter->getSrcSelected());
				mTrProgressBar->setVisible(false);
                mTrStateEditor = kTrReady;
				mFilter->setTrState(mTrStateEditor);
                mFilter->setIsRecordingAutomation(false);
                //this is to erase the trajectory path
                fieldChanged();
			}
		}
		break;
	}
    
    if (mFilter->justSelectedEndPoint()){
        updateEndLocationTextEditors();
        mTrEndPointButton->setToggleState(false, dontSendNotification);
        mTrEndPointButton->setButtonText("Set end point");
//        m_oEndPointLabel.setVisible(false);
        mFilter->setJustSelectedEndPoint(false);
    }
		
	uint64_t hcp = mFilter->getHostChangedProperty();
	if (hcp != mHostChangedProperty) {
		mHostChangedProperty = hcp;

		mMovementMode->setSelectedId(mFilter->getMovementMode() + 1);
		mProcessModeCombo->setSelectedId(mFilter->getProcessMode() + 1);
        mOscLeapSourceCb->setSelectedId(mFilter->getOscLeapSource() + 1);
        
		if (mFilter->getIsAllowInputOutputModeSelection()){
            int iCurMode = mInputOutputModeCombo->getSelectedId();
            int iNewMode = mFilter->getInputOutputMode();
            if (iNewMode != iCurMode){
                mFilter->storeCurrentLocations();
                m_bLoadingPreset = true;
                mInputOutputModeCombo->setSelectedId(iNewMode);
                buttonClicked(mApplyInputOutputModeButton);
            }
        }
        
        mSrcSelect->setSelectedId(mFilter->getSrcSelected()+1);
        mSpSelect->setSelectedId(mFilter->getSpSelected());
        
        mSrcPlacement->setSelectedId(mFilter->getSrcPlacementMode(), dontSendNotification);
        updateSourceLocationTextEditor(false);
        mSpPlacement->setSelectedId(mFilter->getSpPlacementMode(), dontSendNotification);
        updateSpeakerLocationTextEditor();
        
        mTrTypeComboBox->setSelectedId(mFilter->getTrType());
        mTrDuration->setText(String(mFilter->getTrDuration()));
        mTrUnits->setSelectedId(mFilter->getTrUnits());
        mTrRepeats->setText(String(mFilter->getTrRepeats()));
        mTrDampeningTextEditor->setText(String(mFilter->getTrDampening()));
        mTrDeviationTextEditor->setText(String(mFilter->getTrDeviation()*360));
        mTrTurnsTextEditor->setText(String(mFilter->getTrDampening()));
        
        updateOscComponent(mOsc);
        
/*#if JUCE_MAC
        updateLeapComponent(mleap);
#endif*/
        mShowGridLines->setToggleState(mFilter->getShowGridLines(), dontSendNotification);
        mTrSeparateAutomationMode->setToggleState(mFilter->getIndependentMode(), dontSendNotification);
        mLinkDistances->setToggleState(mFilter->getLinkDistances(), dontSendNotification);
        mApplyFilter->setToggleState(mFilter->getApplyFilter(), dontSendNotification);
    }
    
    hcp = mFilter->getHostChangedParameter();
    if (hcp != mHostChangedParameter) {
        mHostChangedParameter = hcp;
        mNeedRepaint = true;
    }
    
    if (mFieldNeedRepaint || mNeedRepaint){
        mField->repaint();
    }
    
    for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++)
        mLevels.getUnchecked(i)->refreshIfNeeded();
    
    if (mNeedRepaint){
        if(mFilter->getGuiTab() == 0){
            int w = (mTabs->getTabContentComponent(0)->getWidth() - kMargin) / 3 - kMargin;
            mMovementMode->setBounds(kMargin, kMargin+kDefaultLabelHeight+5, w, kDefaultLabelHeight);
            mTabs->getTabContentComponent(0)->addAndMakeVisible(mMovementMode);
        } else if(mFilter->getGuiTab() == 1){
            
            int cbw = 130;
            int x = 2*cbw + 2*kMargin;
            int y = kMargin + 2 * (kDefaultLabelHeight + 5);
            int w = (mTabs->getTabContentComponent(1)->getWidth() - kMargin) / 3 - kMargin;
            mMovementMode->setBounds(x, y, w, kDefaultLabelHeight);
            mTabs->getTabContentComponent(1)->addAndMakeVisible(mMovementMode);
        }
        
        mSmoothing->setValue(mFilter->getParameter(kSmooth));
        mVolumeFar->setValue(mFilter->getParameter(kVolumeFar));
        mVolumeMid->setValue(mFilter->getParameter(kVolumeMid));
        mVolumeNear->setValue(mFilter->getParameter(kVolumeNear));
        mMaxSpanVolume->setValue(mFilter->getParameter(kMaxSpanVolume));
        
        mFilterNear->setValue(mFilter->getParameter(kFilterNear));
        mFilterMid->setValue(mFilter->getParameter(kFilterMid));
        mFilterFar->setValue(mFilter->getParameter(kFilterFar));
        
        updateSourceLocationTextEditor(false);
        updateSpeakerLocationTextEditor();
        
        for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
			mDistances.getUnchecked(i)->setValue(1.f - mFilter->getSourceD(i), dontSendNotification);
        }
        
        for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++){
			mAttenuations.getUnchecked(i)->setValue(mFilter->getSpeakerA(i), dontSendNotification);
			mMutes.getUnchecked(i)->setToggleState(mFilter->getSpeakerM(i), dontSendNotification);
        }
    }

    if (!mFilter->getIsRecordingAutomation() && mFilter->getMovementMode() != 0 && mFilter->getSourceLocationChanged() != -1) {
        if(!m_pSourceUpdateThread->isThreadRunning()){
            m_pSourceUpdateThread->startThread();
        }
    } else if (m_pSourceUpdateThread->isThreadRunning()){
            m_pSourceUpdateThread->stopThread(500);
    }
    
    if(mEnableJoystick->getToggleState()) {
        if(!m_pJoystickUpdateThread->isThreadRunning()){
            m_pJoystickUpdateThread->startThread();
        }
    } else if (m_pJoystickUpdateThread->isThreadRunning()){
        m_pJoystickUpdateThread->stopThread(500);

    }
    mNeedRepaint = false;
    mFieldNeedRepaint = false;
    
    if (mOsc) mOsc->heartbeat();
    
    startTimer(kTimerDelay);
}

void OctogrisAudioProcessorEditor::audioProcessorChanged (AudioProcessor* processor){
    mNeedRepaint = true;
}

void OctogrisAudioProcessorEditor::readAndUseJoystickValues(){
    mJoystick->readAndUseJoystickValues();
}

void OctogrisAudioProcessorEditor::audioProcessorParameterChanged(AudioProcessor* processor, int parameterIndex, float newValue){
    mNeedRepaint = true;
}

//==============================================================================
void OctogrisAudioProcessorEditor::paint (Graphics& g)
{
    g.fillAll (Colours::white);
}
#if WIN32

#else
void OctogrisAudioProcessorEditor::uncheckJoystickButton()
{
    mEnableJoystick->setToggleState(false, dontSendNotification);
    buttonClicked(mEnableJoystick);
}
int OctogrisAudioProcessorEditor::getNbSources()
{
    return mFilter->getNumberOfSources();
}
#endif

