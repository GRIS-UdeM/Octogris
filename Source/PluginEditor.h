/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 PluginEditor.h
 
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

#ifndef PLUGINEDITOR_H_INCLUDED
#define PLUGINEDITOR_H_INCLUDED

#include "../JuceLibraryCode/JuceHeader.h"
#include "PluginProcessor.h"
#include "LevelComponent.h"
#include "SourceMover.h"

class FieldComponent;

class Box;
enum
{
	kParamSource,
	kParamSpeaker,
	kParamSmooth,
	kParamVolumeFar,
	kParamVolumeMid,
	kParamVolumeNear,
	kParamFilterFar,
	kParamFilterMid,
	kParamFilterNear,
	kParamMaxSpanVolume
};

enum placement{
    kLeftAlternate = 1,
    kLeftClockwise,
    kLeftCounterClockWise,
    kTopClockwise,
    kTopCounterClockwise,

};

class MiniProgressBar;
class ParamSlider;
class OctTabbedComponent;

class HeartbeatComponent : public Component
{
public:
	virtual void heartbeat() {}
};

//==============================================================================
class OctogrisAudioProcessorEditor  : public AudioProcessorEditor,
									  public Button::Listener,
									  public ComboBox::Listener,
                                      public TextEditor::Listener,
									  private AudioProcessorListener,
									  private Timer
{
public:
    OctogrisAudioProcessorEditor (OctogrisAudioProcessor* ownerFilter);
    ~OctogrisAudioProcessorEditor();

    //==============================================================================
    void paint(Graphics& g);
	void resized();
	
	void buttonClicked (Button *button);
	void comboBoxChanged (ComboBox* comboBox);
    void textEditorFocusLost (TextEditor &textEditor);
    void textEditorReturnKeyPressed(TextEditor &textEditor);
	
	void timerCallback();
	void audioProcessorChanged (AudioProcessor* processor);
	void audioProcessorParameterChanged (AudioProcessor* processor,
                                                 int parameterIndex,
                                                 float newValue);
				
	void refreshSize();
	void fieldChanged() { mFieldNeedRepaint = true; }
	
	int getOscLeapSource() { return mFilter->getOscLeapSource(); }
	void setOscLeapSource(int s);
	SourceMover * getMover() { return &mMover; }
	
private:
    PluginHostType mHost;
	OctogrisAudioProcessor *mFilter;
	SourceMover mMover;
	
	// for memory management:
	OwnedArray<Component> mComponents;
	
	// for interactions:
	bool mNeedRepaint;
	bool mFieldNeedRepaint;
    bool m_bIsReturnKeyPressedCalledFromFocusLost;
    bool m_bLoadingPreset;
    uint64_t mHostChangedParameter;
	uint64_t mHostChangedProperty;
	Array<Slider*> mDistances;
	Array<Component*> mLabels;
	Array<Slider*> mAttenuations;
	Array<ToggleButton*> mMutes;
	Array<LevelComponent*> mLevels;
	ToggleButton *mShowGridLines;
	ToggleButton *mLinkDistances;
	ToggleButton *mLinkMovement;
	ToggleButton *mApplyFilter;
	ComboBox *mMovementMode;
	ComboBox *mGuiSize;
    ComboBox *mInputOutputModeCombo;
    TextButton *mApplyInputOutputModeButton;
	ComboBox *mProcessModeCombo;
	OctTabbedComponent *mTabs;
	Slider *mSmoothing;
	Slider *mVolumeFar;
	Slider *mVolumeMid;
	Slider *mVolumeNear;
	Slider *mFilterFar;
	Slider *mFilterMid;
	Slider *mFilterNear;
	Slider *mMaxSpanVolume;
	
    // sources
    TextButton *mApplySrcPlacementButton;
    TextEditor *mSrcR, *mSrcT;
    ComboBox *mSrcSelect, *mSrcPlacement;
    
    // speakers
    TextButton *mApplySpPlacementButton;
	TextEditor *mSpR, *mSpT;
	ComboBox *mSpSelect, *mSpPlacement;

	// trajectories
	ComboBox *mTrType;
	TextEditor *mTrDuration;
	ComboBox *mTrUnits;
	TextEditor *mTrRepeats;
	TextButton *mTrWriteButton;
	MiniProgressBar *mTrProgressBar;
	ComboBox *mTrSrcSelect;
	enum
	{
		kTrReady,
		kTrWriting
	};
	int mTrState;
	
	// osc, leap
	ComboBox *mOscLeapSourceCb;
    Component *mleap;
	HeartbeatComponent *mOsc;
	
	// for resizing/repaint:
	Component *mField;
    Component *mSourcesBoxLabel;
	Box *mSourcesBox;
	Component *mSpeakersBoxLabel;
	Box *mSpeakersBox;
    void updateSources(bool p_bCalledFromConstructor);
    void updateSpeakers(bool p_bCalledFromConstructor);
    void updateSourceLocationTextEditor();
    void updateSpeakerLocationTextEditor();
    void updateMovementModeCombo();
	
	Component* addLabel(const String &s, int x, int y, int w, int h, Component *into);
	ToggleButton* addCheckbox(const String &s, bool v, int x, int y, int w, int h, Component *into);
	TextButton* addButton(const String &s, int x, int y, int w, int h, Component *into);
	TextEditor* addTextEditor(const String &s, int x, int y, int w, int h, Component *into);
	Slider* addParamSlider(int paramType, int si, float v, int x, int y, int w, int h, Component *into);
};

#endif  // PLUGINEDITOR_H_INCLUDED
