/*
 ==============================================================================
 Octogris2: multichannel sound spatialization plug-in.
 
 Copyright (C) 2015  GRIS-UdeM
 
 OscComponent.cpp
 Created: 8 Aug 2014 9:27:08am
 
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

#include "OscComponent.h"
const String kOscPathSourceXY = "/Octo/SourceXY";
const String kOscPathSelectSource = "/Octo/Source";

static void osc_err_handler(int num, const char *msg, const char *path){
	fprintf(stderr, "osc_err_handler num: %d msg: %s path: %s\n",
					num,
					msg ? msg : "(null)",
					path ? path : "(null)");

}

static String getLocalIPAddress(){
//    Array<IPAddress> addresses;
//    IPAddress::findAllAddresses (addresses);
//    
//    String addressList;
//    
//    for (int i = 0; i < addresses.size(); ++i)
//        addressList << "   " << addresses[i].toString() << newLine;
//    
//    return addressList;
    
    Array<IPAddress> addresses;
    IPAddress::findAllAddresses (addresses);
    return addresses[1].toString();
}


class OscComponent : public HeartbeatComponent, public Button::Listener, public TextEditor::Listener, private OSCReceiver,
	private OSCReceiver::ListenerWithOSCAddress<OSCReceiver::MessageLoopCallback>
{
public:
    OscComponent(OctogrisAudioProcessor* filter, OctogrisAudioProcessorEditor *editor)
	:mFilter(filter)
    ,mEditor(editor)
    ,mOscIpAddress(NULL)
    ,mNeedToEnd(false)
	{
        
		const int m = 10, dh = 18, cw = 130, iw = 120, pw = 60;
		int x = m, y = m;

		mReceive = new ToggleButton();
		mReceive->setButtonText("Receive on ip, port");
		mReceive->setSize(cw, dh);
		mReceive->setTopLeftPosition(x, y);
		mReceive->addListener(this);
		mReceive->setToggleState(mFilter->getOscReceiveEnabled(), dontSendNotification);
		addAndMakeVisible(mReceive);
		
        x += cw + m;
        
        mReceiveIp = new TextEditor();
        mReceiveIp->setColour(TextEditor::textColourId, juce::Colour::greyLevel(.6));
        mReceiveIp->setText(getLocalIPAddress());
        mReceiveIp->setSize(iw, dh);
        mReceiveIp->setTopLeftPosition(x, y);
        mReceiveIp->setReadOnly(true);
        mReceiveIp->setCaretVisible(false);
        addAndMakeVisible(mReceiveIp);
        
        x += iw + m;
		
		mReceivePort = new TextEditor();
		mReceivePort->setText(String(mFilter->getOscReceivePort()));
		mReceivePort->setSize(pw, dh);
		mReceivePort->setTopLeftPosition(x, y);
		mReceivePort->addListener(this);
		addAndMakeVisible(mReceivePort);
		
		x = m; y += dh + m;
		
		mSend = new ToggleButton();
		mSend->setButtonText("Send on ip, port");
		mSend->setSize(cw, dh);
		mSend->setTopLeftPosition(x, y);
		mSend->addListener(this);
		mSend->setToggleState(mFilter->getOscSendEnabled(), dontSendNotification);
		addAndMakeVisible(mSend);
		
		x += cw + m;
		
		mSendIp = new TextEditor();
		mSendIp->setText(String(mFilter->getOscSendIp()));
		mSendIp->setSize(iw, dh);
		mSendIp->setTopLeftPosition(x, y);
		addAndMakeVisible(mSendIp);
		
		x += iw + m;
		
		mSendPort = new TextEditor();
		mSendPort->setText(String(mFilter->getOscSendPort()));
		mSendPort->setSize(pw, dh);
		mSendPort->setTopLeftPosition(x, y);
		mSendPort->addListener(this);
		addAndMakeVisible(mSendPort);
		
		if (mReceive->getToggleState()) buttonClicked(mReceive);
		if (mSend->getToggleState()) buttonClicked(mSend);
	}
    
    void updateInfo(){
        mReceive->setToggleState(mFilter->getOscReceiveEnabled(), dontSendNotification);
        mReceiveIp->setText(getLocalIPAddress());
        mReceivePort->setText(String(mFilter->getOscReceivePort()));
        mSend->setToggleState(mFilter->getOscSendEnabled(), dontSendNotification);
        mSendIp->setText(String(mFilter->getOscSendIp()));
        mSendPort->setText(String(mFilter->getOscSendPort()));
        
        if (mReceive->getToggleState()) buttonClicked(mReceive);
        if (mSend->getToggleState()) buttonClicked(mSend);
    };
	
	~OscComponent() {
		//LIBLO
		//lo_server_thread_free(mServer);
		//lo_address_free(mAddress);
        disconnect();
        mEditor->getMover()->end(kOsc);
    }
	
	void textEditorTextChanged (TextEditor &te) override{
		if (&te == mReceivePort) {
			mFilter->setOscReceivePort(mReceivePort->getText().getIntValue());
		} else if (&te == mSendPort) {
			mFilter->setOscSendPort(mSendPort->getText().getIntValue());
		} else if (&te == mSendIp){
			mFilter->setOscSendIp(mSendIp->getText().toRawUTF8());
		}
	}
	
    void buttonClicked (Button *button) override{
        try {
            if (button == mReceive){
                //we're trying to connect
                if (mReceive->getToggleState()) {
                    int p = mReceivePort->getText().getIntValue();
                    if (!connect(p)) {
                        DBG("Error: could not connect to UDP port.");
                    } else {
                        addListener(this, "/Octo/SourceXY");
                        addListener(this, "/Octo/Source1");
                        addListener(this, "/Octo/Source2");
                        addListener(this, "/Octo/Source3");
                        addListener(this, "/Octo/Source4");
                        addListener(this, "/Octo/Source5");
                        addListener(this, "/Octo/Source6");
                        addListener(this, "/Octo/Source7");
                        addListener(this, "/Octo/Source8");
                        mReceivePort->setEnabled(true);
                    }
                //trying to disconnect
                } else {
                    if (disconnect()) {
                        mReceivePort->setEnabled(false);
                    } else {
                        DBG("lo_server_thread_new failed (port in use ?)");
                    }
                }
                mFilter->setOscReceiveEnabled(mReceive->getToggleState());
            } else if (button == mSend) {
                if (mSend->getToggleState()) {
                    mOscIpAddress = mSendIp->getText();
                    int iSendPort = mSendPort->getText().getIntValue();
                    if(!mOscSender.connect(mOscIpAddress, iSendPort)){
                        DBG("OSC cannot connect");
                        mSend->setToggleState(false, dontSendNotification);
                        mSendIp->setEnabled(true);
                        mSendPort->setEnabled(true);
                    } else {
                        mSourceXY = FPoint(-1, -1);
                        mSource = -1;
                        mSendIp->setEnabled(false);
                        mSendPort->setEnabled(false);
                    }
                } else {
                    mSendIp->setEnabled(true);
                    mSendPort->setEnabled(true);
                }
                mFilter->setOscSendEnabled(mSend->getToggleState());
            } else {
                printf("unknown button clicked...\n");
            }
        } catch (exception& e) {
            cout << e.what() << '\n';
        }
    }
	void oscMessageReceived(const OSCMessage& message) override {
		string address = message.getAddressPattern().toString().toStdString();
		//set position for current source
		if (address == kOscPathSourceXY && message.size() == 2 && message[0].isFloat32() && message[1].isFloat32()){
			float y = message[0].getFloat32();
			float x = message[1].getFloat32();

			mEditor->getMover()->begin(mEditor->getOscLeapSource(), kOsc);
			mEditor->getMover()->move(FPoint(x, y), kOsc);
			mEditor->fieldChanged();

			mNeedToEnd = true;
			mLastXYTime = Time::getCurrentTime(); 
		//set current source
		} else if (address.substr(0, address.size()-1) == kOscPathSelectSource && address.size() == kOscPathSelectSource.length() + 1
			&& message.size() == 1 && message[0].isFloat32() && message[0].getFloat32() < .5) {
			string src_str = address.substr(address.size() - 1);
			String src_jstr(src_str);
			int iSrc = src_jstr.getIntValue()-1;
			mEditor->setOscLeapSource(iSrc);
		}
	}
	
	void heartbeat() override{
		if (mNeedToEnd) {
			Time now = Time::getCurrentTime();
			RelativeTime dt = (now - mLastXYTime);
			if (dt.inMilliseconds() > 200) {
				mEditor->getMover()->end(kOsc);
				mNeedToEnd = false;
			}
		}
        if (!mSend->getToggleState()){
            return;
        }
		int src = mEditor->getOscLeapSource();
		if (src != mSource) {
			String s = "Source "; 
			s << (src+1);
			if (!mOscSender.send(String(kOscPathSelectSource), s)) {
				DBG("Error: could not send OSC message.");
			}
			mSource = src;
		}
		
		FPoint p = mFilter->getSourceXY01(src);
		if (mSourceXY != p) {
            JUCE_COMPILER_WARNING("make kOscPathSourceXY a String")
            String ridiculous(kOscPathSourceXY);
            OSCAddressPattern oscPattern(ridiculous);
			OSCMessage message(oscPattern);
			message.addFloat32(p.y);
			message.addFloat32(p.x);

			if (!mOscSender.send(message)) {
				DBG("Error: could not send OSC message.");
			}
			mSourceXY = p;
		}
	}


private:
	OctogrisAudioProcessor *mFilter;
	OctogrisAudioProcessorEditor *mEditor;
	
	ScopedPointer<ToggleButton> mReceive;
    ScopedPointer<TextEditor>   mReceiveIp;
	ScopedPointer<TextEditor>   mReceivePort;
	
	ScopedPointer<ToggleButton> mSend;
	ScopedPointer<TextEditor> mSendIp;
	ScopedPointer<TextEditor> mSendPort;
	
	//LIBLO
	//lo_server_thread mServer;
	//lo_address mAddress;
	OSCSender mOscSender;
	String mOscIpAddress;
	
	bool mNeedToEnd;
	Time mLastXYTime;
	
	FPoint mSourceXY;
	int mSource;
	
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (OscComponent)
};

HeartbeatComponent * CreateOscComponent(OctogrisAudioProcessor *filter, OctogrisAudioProcessorEditor *editor){
	return new OscComponent(filter, editor);
}

void updateOscComponent(HeartbeatComponent* oscComponent){
    dynamic_cast<OscComponent*>(oscComponent)->updateInfo();
}

//LIBLO
//static int osc_method_handler(const char *path, const char *types, lo_arg ** argv, int argc, lo_message msg, void *user_data){
//    return ((OscComponent*)user_data)->method_handler(path, types, argv, argc, msg, user_data);
//}