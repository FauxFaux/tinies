#include <iostream>
#include <map>
#include <cmath>
#include <stdio.h>

#include <unistd.h>

#include <Leap.h>

using Leap::Controller;
using Leap::Frame;
using Leap::Tool;
using Leap::ToolList;
using Leap::Gesture;
using Leap::GestureList;
using Leap::CircleGesture;
using Leap::SwipeGesture;

const float RAD_TO_DEG = 57.2957795f;

class SampleListener : public Leap::Listener {
  public:
    virtual void onInit(const Controller&);
    virtual void onConnect(const Controller&);
    virtual void onDisconnect(const Controller&);
    virtual void onExit(const Controller&);
    virtual void onFrame(const Controller&);
    virtual void onFocusGained(const Controller&);
    virtual void onFocusLost(const Controller&);
    virtual void onDeviceChange(const Controller&);
    virtual void onServiceConnect(const Controller&);
    virtual void onServiceDisconnect(const Controller&);

  private:
};

const std::string fingerNames[] = {"Thumb", "Index", "Middle", "Ring", "Pinky"};
const std::string boneNames[] = {"Metacarpal", "Proximal", "Middle", "Distal"};
const std::string stateNames[] = {"STATE_INVALID", "STATE_START", "STATE_UPDATE", "STATE_END"};

void SampleListener::onInit(const Controller& controller) {
}

void SampleListener::onConnect(const Controller& controller) {
  controller.enableGesture(Gesture::TYPE_CIRCLE);
  controller.enableGesture(Gesture::TYPE_SWIPE);
}

void SampleListener::onDisconnect(const Controller& controller) {
}

void SampleListener::onExit(const Controller& controller) {
}

std::map<int, int> circle_pos;
void dump(char *position, Leap::Image &img) {
  char name[60];
  sprintf(name, "%s%06d.pgm", postion, counter);
  FILE *f = fopen(name, "wb+");
  std::cout << img.width() << ", " << img.height() << std::endl;
  fprintf(f, "P5\n%d %d\n255\n", img.width(), img.height());
  fflush(f);
  fclose(f);
}

long counter = 0;

void SampleListener::onFrame(const Controller& controller) {
  const Frame frame = controller.frame();

  const Leap::ImageList imgs = frame.images();
  dump("left", imgs[0]);
  dump("right", imgs[1]);
}

void SampleListener::onFocusGained(const Controller& controller) {
}

void SampleListener::onFocusLost(const Controller& controller) {
}

void SampleListener::onDeviceChange(const Controller& controller) {
}

void SampleListener::onServiceConnect(const Controller& controller) {
}

void SampleListener::onServiceDisconnect(const Controller& controller) {
}

int main(int argc, char** argv) {
  SampleListener listener;
  Controller controller;

  controller.addListener(listener);

//  controller.setPolicyFlags(Leap::Controller::POLICY_BACKGROUND_FRAMES);
  controller.setPolicy(Leap::Controller::POLICY_IMAGES);

  sleep(1000 * 1000 * 1000l);

  // Remove the sample listener when done
  controller.removeListener(listener);

  return 0;
}
