#include "pch.h"
#include <iostream>
#include <string>
#include <vector>
#include <windows.h>

MIDIHDR buffer;
std::vector<unsigned char> lpData(255);
bool allocateBuffers = true;

void sendKey(int scancode) {
  INPUT i;
  i.type = INPUT_KEYBOARD;
  i.ki.wScan = 0;
  i.ki.time = 0;
  i.ki.dwExtraInfo = 0;
  i.ki.wVk = scancode;
  i.ki.dwFlags = 0;
  SendInput(1, &i, sizeof(i));
  i.ki.dwFlags = KEYEVENTF_KEYUP;
  SendInput(1, &i, sizeof(i));
}

BOOL AddHeader(LPMIDIHDR lpHdr, HMIDIIN hMidiIn) {
  if (lpHdr->dwFlags & MHDR_PREPARED) {
    MMRESULT res = midiInUnprepareHeader(hMidiIn, lpHdr, sizeof(MIDIHDR));
    lpHdr->dwBufferLength = lpData.size();
  }
  lpHdr->dwBytesRecorded = 0;
  lpHdr->dwFlags &= ~MHDR_DONE;
  if (midiInPrepareHeader(hMidiIn, lpHdr, sizeof(MIDIHDR)))
    return (FALSE);
  if (midiInAddBuffer(hMidiIn, lpHdr, sizeof(MIDIHDR)))
    return (FALSE);
  return (TRUE);
}

void CALLBACK MidiInProc(HMIDIIN hMidiIn, UINT wMsg, DWORD dwInstance,
                         DWORD dwParam1, DWORD dwParam2) {
  switch (wMsg) {
  case MIM_LONGDATA: {
    if (buffer.dwBytesRecorded) {
      std::wcout << "SysEx: ";
      for (unsigned int i = 0; i < buffer.dwBytesRecorded; i++) {
        std::wcout << std::hex << lpData.at(i) << " ";
      }
      if (buffer.dwBytesRecorded == 10 && lpData.at(0) == 0xf0 &&
          lpData.at(1) == 0x43 && lpData.at(2) == 0x73 &&
          lpData.at(3) == 0x01 && lpData.at(4) == 0x50 &&
          lpData.at(5) == 0x15 && lpData.at(6) == 0x00 &&
          lpData.at(8) == 0x00 && lpData.at(9) == 0xf7) {
        switch (lpData.at(7)) {
        case 0x00:
          sendKey(0x20); //SPACE
          break;
        case 0x01:
          sendKey(0x6e); //NUM,
          break;
        case 0x03:
          sendKey(0x6b); //NUM+
          break;
        case 0x04:
          sendKey(0x6d); //NUM-
          break;
        case 0x05:
          sendKey(0x43); //C
          break;
        default:
          break;
        }
      }
      std::wcout << std::endl;
    }
  }
  case MIM_LONGERROR:
    if (allocateBuffers)
      AddHeader(&buffer, hMidiIn);
    break;
  case MIM_DATA:
  case MIM_OPEN:
  case MIM_CLOSE:
  case MIM_ERROR:
  case MIM_MOREDATA:
  default:
    break;
  }
  return;
}

int main() {
  // enumerate and prompt for Midi Devices
  UINT numDevs = midiInGetNumDevs();
  std::cout << "Please select a Midi Input Device from this list:" << std::endl;
  for (unsigned int i = 0; i < numDevs; i++) {
    MIDIINCAPS caps;
    MMRESULT r = midiInGetDevCaps(i, &caps, sizeof(MIDIINCAPS));
    if (r == MMSYSERR_NOERROR) {
      std::wcout << "[" << i << "]\t" << caps.szPname << std::endl;
    }
  }
  // assume midi device 0
  UINT selectedDevice = 0;
  // open midi device and callback to MidiInProc on event arrival
  HMIDIIN midiDevice;
  MMRESULT r = midiInOpen(&midiDevice, selectedDevice,
                          (DWORD)(void *)MidiInProc, 0, CALLBACK_FUNCTION);
  if (r == MMSYSERR_NOERROR) {
    buffer.dwBufferLength = lpData.size();
    buffer.lpData = (LPSTR)lpData.data();
    AddHeader(&buffer, midiDevice);
    midiInStart(midiDevice);
    // poll user induced exit request
    while (!(GetKeyState('Q') & 0x8000))
      SleepEx(150, true);
    ;
    allocateBuffers = false;
    midiInStop(midiDevice);
    midiInReset(midiDevice);
    midiInUnprepareHeader(midiDevice, &buffer, sizeof(MIDIHDR));
    midiInClose(midiDevice);
    return 0;
  }
}