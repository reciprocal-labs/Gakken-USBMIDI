import javax.sound.midi.MidiDevice;
import javax.sound.midi.MidiSystem;
import javax.sound.midi.MidiUnavailableException;
import javax.sound.midi.*;

class MyMidi implements Receiver {

  public String decodeMessage(ShortMessage message)
  {
    String	strMessage = null;
    switch (message.getCommand())
    {
      case 0x80:
        byte d = 0;
        myPort.write(d);
	break;

      case 0x90:
        myPort.write(message.getData1());
        break;
      }
    return "";
  }

  public void send(MidiMessage message, long lTimeStamp){
    System.out.println("Message received");
    String strMessage = null;
      if (message instanceof ShortMessage)
      {
        strMessage = decodeMessage((ShortMessage) message);
        System.out.println(strMessage);
      }
  }
  
  public void close(){
    
  }

  public void printMidiDevices() {
    MidiDevice.Info[] aInfos = MidiSystem.getMidiDeviceInfo();
    for (int i = 0; i < aInfos.length; i++) {
    try {
     MidiDevice device = MidiSystem.getMidiDevice(aInfos[i]);
       boolean bAllowsInput = (device.getMaxTransmitters() != 0);
       boolean bAllowsOutput = (device.getMaxReceivers() != 0);
       System.out.println("" + i + "  "
           + (bAllowsInput ? "IN " : "   ")
           + (bAllowsOutput ? "OUT " : "    ")
           + aInfos[i].getName() + ", " + aInfos[i].getVendor()
           + ", " + aInfos[i].getVersion() + ", "
           + aInfos[i].getDescription());
    }  
    catch (MidiUnavailableException e) {
       // device is obviously not available...
       System.err.println(e);
    }
  } 
  
    }
    MidiDevice inputDevice = null;
    Transmitter t = null;
    Receiver r = null;
    public void openForInput(int idx){
        try{
          MidiDevice.Info[] aInfos = MidiSystem.getMidiDeviceInfo();
          inputDevice = MidiSystem.getMidiDevice(aInfos[idx]);
          inputDevice.open();
          t = inputDevice.getTransmitter();
          t.setReceiver(this);
        }catch(MidiUnavailableException e){
          System.err.println(e);
          
        }
    }
}

import processing.serial.*;
Serial myPort;
MyMidi myMidi;

void setup(){
  size(480,360);
  myMidi = new MyMidi();
  myMidi.printMidiDevices();
  myMidi.openForInput(3);
  
  println(Serial.list());
  println(">>> " + Serial.list()[3]);
  myPort = new Serial(this, Serial.list()[3], 9600);
}

void draw()
{
  background(255);
}
