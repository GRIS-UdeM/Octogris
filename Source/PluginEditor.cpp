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

#if WIN32

#else

#include "HIDDelegate.h"
#include "HID_Utilities_External.h"
#endif
//==============================================================================
static const int kMargin = 10;
static const int kDefaultLabelHeight = 18;
static const int kCenterColumnWidth = 180;
static const int kRightColumnWidth = 340;
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
    mFilter(filter),
    mInited(false)
    {}
    
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
        
        if (mParamType == kParamSource)
        {
            const float newVal = 1.f - (float)getValue();
            
            if (mLink->getToggleState())
            {
                for (int i = 0; i < mFilter->getNumberOfSources(); i++)
                {
                    int paramIndex = mFilter->getParamForSourceD(i);
                    if (mFilter->getParameter(paramIndex) != newVal)
                        mFilter->setParameterNotifyingHost(paramIndex, newVal);
                }
            }
            else
            {
                if (mFilter->getParameter(mParamIndex) != newVal){
                    JUCE_COMPILER_WARNING(new string("had to comment this for panspan to work... does this break other things?"))
                    
                    //mFilter->beginParameterChangeGesture(mParamIndex);
                    mFilter->setParameterNotifyingHost(mParamIndex, newVal);
                    //mFilter->endParameterChangeGesture(mParamIndex);
                }
            }
        }
        else
        {
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
    SourceUpdateThread(OctogrisAudioProcessorEditor* p_pProcessor)
    : Thread ("SourceUpdateThread")
    ,m_iInterval(25)
    ,m_pEditor(p_pProcessor) {
        
        startThread ();
    }
    
    ~SourceUpdateThread() {
        // allow the thread 1 second to stop cleanly - should be plenty of time.
        stopThread (2 * m_iInterval);
    }
    
    void run() override {
        
        // threadShouldExit() returns true when the stopThread() method has been called
        while (! threadShouldExit()) {
            
            // sleep a bit so the threads don't all grind the CPU to a halt..
            wait (m_iInterval);
            
            m_pEditor->updateNonSelectedSourcePositions();
        }
    }
    
private:
    int m_iInterval;
    OctogrisAudioProcessorEditor* m_pEditor;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SourceUpdateThread)
};

#define STRING2(x) #x
#define STRING(x) STRING2(x)

//==================================== EDITOR ===================================================================

OctogrisAudioProcessorEditor::OctogrisAudioProcessorEditor (OctogrisAudioProcessor* ownerFilter):
AudioProcessorEditor (ownerFilter)
,mFilter(ownerFilter)
,mMover(ownerFilter)
,m_logoImage()
{
    
    m_pSourceUpdateThread = new SourceUpdateThread(this);
    mComponents.add(m_pSourceUpdateThread);

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
    m_VersionLabel->setJustificationType(Justification(Justification::right));
    m_VersionLabel->setColour(Label::textColourId, Colours::whitesmoke);
    addAndMakeVisible(m_VersionLabel);

    mComponents.add(m_VersionLabel);

    
    // param box
    Colour tabBg = Colour::fromRGB(200,200,200);
    mTabs = new OctTabbedComponent(TabbedButtonBar::TabsAtTop, mFilter);
    mTabs->addTab("Settings", tabBg, new Component(), true);
    mTabs->addTab("Volume & Filters", tabBg, new Component(), true);
    mTabs->addTab("Sources", tabBg, new Component(), true);
   	mTabs->addTab("Speakers", tabBg, new Component(), true);
    mTabs->addTab("Trajectories", tabBg, new Component(), true);
    {
        mOsc = CreateOscComponent(mFilter, this);
        
        if (mOsc) mTabs->addTab("OSC", tabBg, mOsc, true);
    }
    mTabs->addTab("Interfaces", tabBg, new Component(), true);
	
    mTabs->setSize(kCenterColumnWidth + kMargin + kRightColumnWidth, kParamBoxHeight);
    addAndMakeVisible(mTabs);
    mComponents.add(mTabs);
    
    
    // sources
    {
        mSourcesBox = new Box(true);
        addAndMakeVisible(mSourcesBox);
        mComponents.add(mSourcesBox);
        
        mSourcesBoxLabel = addLabel("Source distance:", 0, 0, kCenterColumnWidth, kDefaultLabelHeight, this);

        Component *ct = mSourcesBox->getContent();
        
        int dh = kDefaultLabelHeight, x = 0, y = 0, w = kCenterColumnWidth;
        
        mLinkDistances = addCheckbox("Link", mFilter->getLinkDistances(), x, y, w/3, dh, ct);
        addLabel("Distance/Span", x+w/3, y, w*2/3, dh, ct);
        
        mSrcSelect = new ComboBox();
        mTabs->getTabContentComponent(2)->addAndMakeVisible(mSrcSelect);
        mComponents.add(mSrcSelect);
        mSrcSelect->addListener(this);
        
        JUCE_COMPILER_WARNING("WTF is this????")
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
        mTabs->getTabContentComponent(3)->addAndMakeVisible(mSpSelect);
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
            
            mMovementMode->setSize(w, dh);
            mMovementMode->setTopLeftPosition(x, y);
            box->addAndMakeVisible(mMovementMode);
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
        
        //-----------------------------
        // start 2nd column
        y = kMargin;
        x += w + kMargin;
        
        addLabel("Gui size:", x, y, w, dh, box);
        y += dh + 5;
        
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            cb->addItem("Small", index++);
            cb->addItem("Medium", index++);
            cb->addItem("Large", index++);
            cb->setSelectedId(mFilter->getGuiSize() + 1);
            cb->setSize(w, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            y += dh + 5;
            
            cb->addListener(this);
            mGuiSize = cb;
        }
        
        mShowGridLines = addCheckbox("Show grid lines", mFilter->getShowGridLines(), x, y, w, dh, box);
        y += dh + 5;
        
        //only using the combo box in reaper, because other hosts set the inputs and outputs automatically
		if (mFilter->getIsAllowInputOutputModeSelection()) {
            
            addLabel("Input/Output mode:", x, y, w, dh, box);
            y += dh + 5;
            
            mInputOutputModeCombo = new ComboBox();
            int index = 1;
            
            mInputOutputModeCombo->addItem("1x2", index++);
            mInputOutputModeCombo->addItem("1x4", index++);
            mInputOutputModeCombo->addItem("1x6", index++);
            mInputOutputModeCombo->addItem("1x8", index++);
            mInputOutputModeCombo->addItem("1x16", index++);
            
            mInputOutputModeCombo->addItem("2x2", index++);
            mInputOutputModeCombo->addItem("2x4", index++);
            mInputOutputModeCombo->addItem("2x6", index++);
            mInputOutputModeCombo->addItem("2x8", index++);
            mInputOutputModeCombo->addItem("2x16", index++);
            
            mInputOutputModeCombo->addItem("4x4", index++);
            mInputOutputModeCombo->addItem("4x6", index++);
            mInputOutputModeCombo->addItem("4x8", index++);
            mInputOutputModeCombo->addItem("4x16", index++);
            
            mInputOutputModeCombo->addItem("6x6", index++);
            mInputOutputModeCombo->addItem("6x8", index++);
            mInputOutputModeCombo->addItem("6x16", index++);
            
            mInputOutputModeCombo->addItem("8x8", index++);
            mInputOutputModeCombo->addItem("8x16", index++);
            
            mInputOutputModeCombo->setSelectedId(mFilter->getInputOutputMode() + 1);
            mInputOutputModeCombo->setSize(w - iButtonW, dh);
            mInputOutputModeCombo->setTopLeftPosition(x, y);
            box->addAndMakeVisible(mInputOutputModeCombo);
            mComponents.add(mInputOutputModeCombo);
            
            mApplyInputOutputModeButton = addButton("Apply", x + w - iButtonW, y, iButtonW, dh, box);
            
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
        
        int comboW = 40;
        addLabel(leapSupported ? "OSC/Leap source:" : "OSC source:", x, y, w-comboW, dh, box);
        //y += dh + 5;
        
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
            y += dh + 5;
            
            mOscLeapSourceCb->addListener(this);
        }
        
        
    }
    
    //--------------- V & F TAB ---------------- //
    box = mTabs->getTabContentComponent(1);
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
    box = mTabs->getTabContentComponent(2);
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
        int iasdf = mFilter->getSrcSelected();
        mSrcSelect->setSelectedId(iasdf);
        mSrcSelect->setSize(selectw, dh);
        mSrcSelect->setTopLeftPosition(x + w - selectw, y);
        mSrcSelect->setExplicitFocusOrder(5);
        
        int lw = 30, lwm = lw + kMargin;
        
        y += dh + 5;
        
        addLabel("R: 0 to 2, A: 0 to 360", x, y, w, dh, box);
        y += dh + 5;
        
        addLabel("R:", x, y, lw, dh, box);
        mSrcR = addTextEditor("1", x + lwm, y, w - lwm, dh, box);
        mSrcR->setExplicitFocusOrder(6);
        mSrcR->addListener(this);
        y += dh + 5;
        
        addLabel("A:", x, y, lw, dh, box);
        mSrcT = addTextEditor("0", x + lwm, y, w - lwm, dh, box);
        mSrcT->setExplicitFocusOrder(7);
        mSrcT->addListener(this);
        
    }
    //--------------- SPEAKERS TAB ---------------- //
    box = mTabs->getTabContentComponent(3);
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
        
        int lw = 30, lwm = lw + kMargin;
        
        
        y += dh + 5;
        addLabel("R: 0 to 2, A: 0 to 360", x, y, w, dh, box);
        y += dh + 5;
        addLabel("R:", x, y, lw, dh, box);
        mSpR = addTextEditor("1", x + lwm, y, w - lwm, dh, box);
        mSpR->setExplicitFocusOrder(6);
        mSpR->addListener(this);
        
        y += dh + 5;
        addLabel("A:", x, y, lw, dh, box);
        mSpT = addTextEditor("0", x + lwm, y, w - lwm, dh, box);
        mSpT->setExplicitFocusOrder(7);
        mSpT->addListener(this);
		
		//-------- column 3 --------
        y = kMargin;
        x += w + kMargin;
		
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
	}
    
    
    //--------------- TRAJECTORIES TAB ---------------- //
    box = mTabs->getTabContentComponent(4);
    {
        int x = kMargin, y = kMargin, w = (box->getWidth() - kMargin) / 3 - kMargin;
        
        int cbw = 130;
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            for (int i = 1; i < Trajectory::NumberOfTrajectories(); i++)
                cb->addItem(Trajectory::GetTrajectoryName(i), index++);
            
            cb->setSelectedId(mFilter->getTrType()+1);
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
            
            cb->setSize(cbw, dh);
            cb->setTopLeftPosition(x+cbw+cbw+10, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrReturnComboBox = cb;
            mTrReturnComboBox->addListener(this);
        }
        
        y += dh + 5;
        
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
        
        x += tew + kMargin;
        
        {
            ComboBox *cb = new ComboBox();
            int index = 1;
            cb->addItem("All sources", index++);
            for (int i = 0; i < mFilter->getNumberOfSources(); i++)
            {
                String s("Source "); s << i+1;
                cb->addItem(s, index++);
            }
            cb->setSelectedId(mFilter->getTrSrcSelect()+2);
            cb->setSize(100, dh);
            cb->setTopLeftPosition(x, y);
            box->addAndMakeVisible(cb);
            mComponents.add(cb);
            
            mTrSrcSelect = cb;
            mTrSrcSelect->addListener(this);
        }
        
        y += dh + 5;
        x = kMargin;
        
        mTrRepeats = addTextEditor(String(mFilter->getTrRepeats()), x, y, tew, dh, box);
        mTrRepeats->addListener(this);
        x += tew + kMargin;
        
        addLabel("cycle(s)", x, y, w, dh, box);
        
        y += dh + 5;
        x = kMargin;
        
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
    
    }

    //--------------- INTERFACE TAB ---------------- //
#if WIN32
    
#else
    //changements lié a l'ajout de joystick à l'onglet leap
    box = mTabs->getTabContentComponent(6);
    {
        int x = kMargin, y = kMargin;
        const int m = 10, dh = 18, cw = 300;
        
        mEnableLeap = new ToggleButton();
        mEnableLeap->setButtonText("Enable Leap");
        mEnableLeap->setSize(cw-100, dh);
        mEnableLeap->setTopLeftPosition(x, y);
        mEnableLeap->addListener(this);
        mEnableLeap->setToggleState(false, dontSendNotification);
        box->addAndMakeVisible(mEnableLeap);
        mComponents.add(mEnableLeap);
        
        y += dh + 5;
        
        mEnableJoystick = new ToggleButton();
        mEnableJoystick->setButtonText("Enable Joystick");
        mEnableJoystick->setSize(cw-150, dh);
        mEnableJoystick->setTopLeftPosition(x, y);
        mEnableJoystick->addListener(this);
        mEnableJoystick->setToggleState(false, dontSendNotification);
        box->addAndMakeVisible(mEnableJoystick);
        mComponents.add(mEnableJoystick);
        
        x += cw-150 + m;
        
        mStateJoystick = new Label();
        mStateJoystick->setText("", dontSendNotification);
        mStateJoystick->setSize(cw, dh);
        mStateJoystick->setJustificationType(Justification::left);
        mStateJoystick->setMinimumHorizontalScale(1);
        mStateJoystick->setTopLeftPosition(x, y);
        box->addAndMakeVisible(mStateJoystick);
        mComponents.add(mStateJoystick);
        
        y -= dh + 5;
        
        mStateLeap = new Label();
        mStateLeap->setText("", dontSendNotification);
        mStateLeap->setSize(cw, dh);
        mStateLeap->setJustificationType(Justification::left);
        mStateLeap->setMinimumHorizontalScale(1);
        mStateLeap->setTopLeftPosition(x, y);
        box->addAndMakeVisible(mStateLeap);
        mComponents.add(mStateLeap);
        //fin de changements lié a l'ajout de joystick à l'onglet leap
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
    
    refreshSize();
}

void OctogrisAudioProcessorEditor::updateNonSelectedSourcePositions(){
    int iSourceChanged = mFilter->getSourceLocationChanged();
    if (s_bUseOneSource && !mFilter->getIsRecordingAutomation() && mFilter->getMovementMode() != 0 && iSourceChanged != -1) {
        
        //cout << "THREAD: " << s_bUseOneSource << ", " << !mFilter->getIsRecordingAutomation() << ", " << mFilter->getMovementMode() << ", " << iSourceChanged << "----------";
        
        mMover.begin(iSourceChanged, kSourceThread);
        mMover.move(mFilter->getSourceXY01(iSourceChanged), kSourceThread);
        mMover.end(kSourceThread);
        
        mFilter->setSourceLocationChanged(-1);
    }
}

void OctogrisAudioProcessorEditor::updateTrajectoryComboboxes(){
    int iSelectedTrajectory = mFilter->getTrType()+1;
    
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
    mHIDDel = NULL;
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

//==============================================================================
void OctogrisAudioProcessorEditor::refreshSize()
{
    int fieldSize = 500;
    
    int guiSize = mFilter->getGuiSize();
    fieldSize += (guiSize - 1) * 100;
    
    setSize(kMargin + fieldSize + kMargin + kCenterColumnWidth + kMargin + kRightColumnWidth + kMargin,
            kMargin + fieldSize + kMargin);
}

void OctogrisAudioProcessorEditor::resized()
{
    int w = getWidth();
    int h = getHeight();
    
    int fieldWidth = w - (kMargin + kMargin + kCenterColumnWidth + kMargin + kRightColumnWidth + kMargin);
    int fieldHeight = h - (kMargin + kMargin);
    int fieldSize = jmin(fieldWidth, fieldHeight);
    
    mField->setBounds(kMargin, kMargin, fieldSize, fieldSize);

    //m_logoImage.setBounds(15, 15, 80, 80);
    m_logoImage.setBounds(15, 15, (float)fieldSize/7, (float)fieldSize/7);
    
    int iLabelX = 2*(float)fieldSize/3;
    m_VersionLabel->setBounds(iLabelX,5,fieldSize-iLabelX,25);
    
    int x = kMargin + fieldSize  + kMargin;
    int y = kMargin;
    mSourcesBoxLabel->setTopLeftPosition(x, y);
    
    int lh = mSourcesBoxLabel->getHeight() + 2;
    mSourcesBox->setBounds(x, y + lh, kCenterColumnWidth, h - (kMargin + kParamBoxHeight + kMargin + y + lh));
    
    mTabs->setBounds(x, h - (kParamBoxHeight + kMargin), kCenterColumnWidth + kMargin + kRightColumnWidth, kParamBoxHeight);
    
    x += kCenterColumnWidth + kMargin;
    mSpeakersBoxLabel->setTopLeftPosition(x, y);
    mSpeakersBox->setBounds(x, y + lh, kRightColumnWidth, h - (kMargin + kParamBoxHeight + kMargin + y + lh));
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
        mTrSrcSelect->clear(dontSendNotification);
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
    mSrcSelect->setSelectedId(mFilter->getSrcSelected());
    
    
    //source selection combo in trajectory tab
    if (!p_bCalledFromConstructor){
        int index = 1;
        mTrSrcSelect->addItem("All sources", index++);
        for (int i = 0; i < mFilter->getNumberOfSources(); i++)
        {
            String s("Source "); s << i+1;
            mTrSrcSelect->addItem(s, index++);
        }
        mTrSrcSelect->setSelectedId(mFilter->getTrSrcSelect()+2);
    }
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
    if (mFilter->getNumberOfSources() == 2)
    {
        mMovementMode->addItem("Symmetric X", index++);
        mMovementMode->addItem("Symmetric Y", index++);
        mMovementMode->addItem("Symmetric X & Y", index++);
    }
    if (mFilter->getNumberOfSources() >= 2)
    {
        mMovementMode->addItem("Circular", index++);
        mMovementMode->addItem("Circular Fixed Radius", index++);
        mMovementMode->addItem("Circular Fixed Angle", index++);
        mMovementMode->addItem("Circular Fully Fixed", index++);
        mMovementMode->addItem("Delta Lock", index++);
    }
    int iCurMode = mFilter->getMovementMode() + 1;
    iCurMode > mMovementMode->getNumItems() ? mMovementMode->setSelectedId(1) : mMovementMode->setSelectedId(iCurMode);
    
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
    
    if (&textEditor == mSrcR || &textEditor == mSrcT)
    {
        int sp = mSrcSelect->getSelectedId() - 1;
        float r = mSrcR->getText().getFloatValue();
        float t = mSrcT->getText().getFloatValue();
        if (r < 0) r = 0; else if (r > kRadiusMax) r = kRadiusMax;
        mFilter->setSourceRT(sp, FPoint(r, t * M_PI / 180.));
    }
    else if (&textEditor == mSpR || &textEditor == mSpT)
    {
        int sp = mSpSelect->getSelectedId() - 1;
        float r = mSpR->getText().getFloatValue();
        float t = mSpT->getText().getFloatValue();
        if (r < 0) r = 0; else if (r > kRadiusMax) r = kRadiusMax;
        mFilter->setSpeakerRT(sp, FPoint(r, t * M_PI / 180.));
    }
    else if (&textEditor == mTrDuration){
        float duration = mTrDuration->getText().getFloatValue();
        mFilter->setTrDuration(duration);
    }
    else if (&textEditor == mTrRepeats){
        float repeats = mTrRepeats->getText().getFloatValue();
        mFilter->setTrRepeats(repeats);
    }
    
    else
    {
        printf("unknown TextEditor clicked...\n");
    }
    
    //if called from actually pressing enter, put focus on something else
    if (!m_bIsReturnKeyPressedCalledFromFocusLost){
        mLinkDistances->grabKeyboardFocus();
    }
    
}

void OctogrisAudioProcessorEditor::buttonClicked (Button *button)
{
    
    for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++)
    {
        if (button == mMutes[i])
        {
            float v = button->getToggleState() ? 1.f : 0.f;
            mFilter->setParameterNotifyingHost(mFilter->getParamForSpeakerM(i), v);
            mField->repaint();
            return;
        }
    }
    
	if (mFilter->getIsAllowInputOutputModeSelection() && button == mApplyInputOutputModeButton)
    {
        mFilter->setInputOutputMode(mInputOutputModeCombo->getSelectedItemIndex());
        
		updateSources(false);
		updateSpeakers(false);
        if (m_bLoadingPreset){
            mFilter->restoreCurrentLocations();
            m_bLoadingPreset = false;
        }
        mField->repaint();
        
    }
    else if (button == mApplySrcPlacementButton)
    {
        
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
        updateSourceLocationTextEditor();
        mFilter->setSrcPlacementMode(mSrcPlacement->getSelectedId());
    }
    else if (button == mApplySpPlacementButton)
    {
        
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
    
    else if (button == mShowGridLines)
    {
        mFilter->setShowGridLines(button->getToggleState());
        mField->repaint();
    }
    else if (button == mLinkDistances)
    {
        mFilter->setLinkDistances(button->getToggleState());
    }
    else if (button == mApplyFilter)
    {
        mFilter->setApplyFilter(button->getToggleState());
    }
#if WIN32
    
#else
    //Changements lié a l'ajout de joystick à l'onglet leap qui est devenu interface
    else if(button == mEnableJoystick)
    {

        bool state = mEnableJoystick->getToggleState();
        mFilter->setIsJoystickEnabled(state);
        if (state) {
            if (!gIOHIDManagerRef) {
                mStateJoystick->setText("Joystick not connected", dontSendNotification);
                gIOHIDManagerRef = IOHIDManagerCreate(CFAllocatorGetDefault(),kIOHIDOptionsTypeNone);
                if(!gIOHIDManagerRef) {
                    printf("Could not create IOHIDManager");
                } else {
                    mHIDDel = HIDDelegate::CreateHIDDelegate(mFilter, this);
                    mHIDDel->Initialize_HID(this);
                    if(mHIDDel->getDeviceSetRef()) {
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
                mHIDDel = NULL;
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
    
    else if (button == mTrWriteButton) {
        Trajectory::Ptr t = mFilter->getTrajectory();
        if (t) {
            mFilter->setTrajectory(NULL);
            mFilter->restoreCurrentLocations();
            mTrWriteButton->setButtonText("Ready");
            mTrProgressBar->setVisible(false);
            mTrStateEditor = kTrReady;
            mFilter->setTrState(mTrStateEditor);
            t->stop();
            mFilter->setIsRecordingAutomation(false);
            mNeedRepaint = true;
        } else {
            float duration = mTrDuration->getText().getFloatValue();
            bool beats = mTrUnits->getSelectedId() == 1;
            float repeats = mTrRepeats->getText().getFloatValue();
            int type = mTrTypeComboBox->getSelectedId();
            
          
            unique_ptr<AllTrajectoryDirections> direction = Trajectory::getTrajectoryDirection(type, mTrDirectionComboBox->getSelectedId());
            
            bool bReturn = mTrReturnComboBox->getSelectedId() == 2;
            
            int source = /*s_bUseOneSource ? 0 :*/ mTrSrcSelect->getSelectedId()-2;
            
            mFilter->setTrDuration(duration);
            JUCE_COMPILER_WARNING("this operation was already done by event funct, clean this up")
            mFilter->setTrUnits(mTrUnits->getSelectedId());
            mFilter->setTrRepeats(repeats);
            mFilter->setTrType(type);
            mFilter->setTrSrcSelect(source);
            
            bool bUniqueTarget = true;
            if  (mFilter->getMovementMode() == 0){
                bUniqueTarget = false;
            }

            mFilter->setIsRecordingAutomation(true);
			mFilter->storeCurrentLocations();
			mFilter->setTrajectory(Trajectory::CreateTrajectory(type, mFilter, duration, beats, *direction, bReturn, repeats, source, bUniqueTarget));
			mTrWriteButton->setButtonText("Cancel");
            mTrStateEditor = kTrWriting;
            mFilter->setTrState(mTrStateEditor);
			
			mTrProgressBar->setValue(0);
			mTrProgressBar->setVisible(true);
		}
	} else {
		printf("unknown button clicked...\n");
	}
}

void OctogrisAudioProcessorEditor::textEditorFocusLost (TextEditor &textEditor){
    m_bIsReturnKeyPressedCalledFromFocusLost = true;
    textEditorReturnKeyPressed(textEditor);
    m_bIsReturnKeyPressedCalledFromFocusLost = false;
}

void OctogrisAudioProcessorEditor::comboBoxChanged (ComboBox* comboBox)
{
    if (comboBox == mMovementMode)
    {
        mFilter->setMovementMode(comboBox->getSelectedId() - 1);
    }
	else if (comboBox == mRoutingMode)
	{
		mFilter->setRoutingMode(comboBox->getSelectedId() - 1);
	}
    else if (comboBox == mGuiSize)
    {
        mFilter->setGuiSize(comboBox->getSelectedId() - 1);
        refreshSize();
    }
    else if (comboBox == mProcessModeCombo)
    {
        int iSelectedMode = comboBox->getSelectedId() - 1;
        mFilter->setProcessMode(iSelectedMode);
        if (iSelectedMode == kPanVolumeMode){
            for (int i = 0; i < mFilter->getNumberOfSources(); i++) { mDistances.getUnchecked(i)->setEnabled(false);  }
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
        updateSourceLocationTextEditor();
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
        int type = mTrTypeComboBox->getSelectedId()-1;
        mFilter->setTrType(type);
        updateTrajectoryComboboxes();
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
    else if (comboBox == mTrSrcSelect)
    {
        int source = mTrSrcSelect->getSelectedId()-2;
        mFilter->setTrSrcSelect(source);
    }
    else
    {
        printf("unknown combobox clicked...\n");
    }
}


void OctogrisAudioProcessorEditor::updateSourceLocationTextEditor(){
    int i = mSrcSelect->getSelectedId();
    i = (i <= 0) ? 1: i;
    FPoint curPosition = mFilter->getSourceRT(i-1);
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
                mFilter->restoreCurrentLocations();
				mTrProgressBar->setVisible(false);
                mTrStateEditor = kTrReady;
				mFilter->setTrState(mTrStateEditor);
			}
		}
		break;
	}
		
	uint64_t hcp = mFilter->getHostChangedProperty();
	if (hcp != mHostChangedProperty) {
		mHostChangedProperty = hcp;

		mMovementMode->setSelectedId(mFilter->getMovementMode() + 1);
		mProcessModeCombo->setSelectedId(mFilter->getProcessMode() + 1);
		mGuiSize->setSelectedId(mFilter->getGuiSize() + 1);
        mOscLeapSourceCb->setSelectedId(mFilter->getOscLeapSource() + 1);
        
		if (mFilter->getIsAllowInputOutputModeSelection()){
            int iCurMode = mInputOutputModeCombo->getSelectedId();
            int iNewMode = mFilter->getInputOutputMode()+1;
            if (iNewMode != iCurMode){
                mFilter->storeCurrentLocations();
                m_bLoadingPreset = true;
                mInputOutputModeCombo->setSelectedId(iNewMode);
                buttonClicked(mApplyInputOutputModeButton);
            }
        }
        
        
        mSrcSelect->setSelectedId(mFilter->getSrcSelected());
        mSpSelect->setSelectedId(mFilter->getSpSelected());
        
        mSrcPlacement->setSelectedId(mFilter->getSrcPlacementMode(), dontSendNotification);
        updateSourceLocationTextEditor();
        mSpPlacement->setSelectedId(mFilter->getSpPlacementMode(), dontSendNotification);
        updateSpeakerLocationTextEditor();
        
        mTrTypeComboBox->setSelectedId(mFilter->getTrType()+1);
        
        mTrSrcSelect->setSelectedId(mFilter->getTrSrcSelect()+2);
        mTrDuration->setText(String(mFilter->getTrDuration()));
        mTrUnits->setSelectedId(mFilter->getTrUnits());
        mTrRepeats->setText(String(mFilter->getTrRepeats()));
        
        updateOscComponent(mOsc);
        
/*#if JUCE_MAC
        updateLeapComponent(mleap);
#endif*/
        mShowGridLines->setToggleState(mFilter->getShowGridLines(), dontSendNotification);
        mLinkDistances->setToggleState(mFilter->getLinkDistances(), dontSendNotification);
        mApplyFilter->setToggleState(mFilter->getApplyFilter(), dontSendNotification);
        
        refreshSize();
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
        mSmoothing->setValue(mFilter->getParameter(kSmooth));
        mVolumeFar->setValue(mFilter->getParameter(kVolumeFar));
        mVolumeMid->setValue(mFilter->getParameter(kVolumeMid));
        mVolumeNear->setValue(mFilter->getParameter(kVolumeNear));
        mMaxSpanVolume->setValue(mFilter->getParameter(kMaxSpanVolume));
        
        mFilterNear->setValue(mFilter->getParameter(kFilterNear));
        mFilterMid->setValue(mFilter->getParameter(kFilterMid));
        mFilterFar->setValue(mFilter->getParameter(kFilterFar));
        
        updateSourceLocationTextEditor();
        updateSpeakerLocationTextEditor();
        
        for (int i = 0; i < mFilter->getNumberOfSources(); i++) {
			mDistances.getUnchecked(i)->setValue(1.f - mFilter->getSourceD(i), dontSendNotification);
        }
        
        for (int i = 0; i < mFilter->getNumberOfSpeakers(); i++){
			mAttenuations.getUnchecked(i)->setValue(mFilter->getSpeakerA(i), dontSendNotification);
			mMutes.getUnchecked(i)->setToggleState(mFilter->getSpeakerM(i), dontSendNotification);
        }
    }
    if(mEnableJoystick->getToggleState())
    {
        mHIDDel->readJoystickValuesAndUsingThem();
    }
    mNeedRepaint = false;
    mFieldNeedRepaint = false;
    
    if (mOsc) mOsc->heartbeat();
    
    startTimer(kTimerDelay);
}

void OctogrisAudioProcessorEditor::audioProcessorChanged (AudioProcessor* processor)
{
    mNeedRepaint = true;
}

void OctogrisAudioProcessorEditor::audioProcessorParameterChanged(AudioProcessor* processor,
                                                                  int parameterIndex,
                                                                  float newValue)
{
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

