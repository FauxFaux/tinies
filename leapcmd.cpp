#include <iostream>
#include <map>
#include <cmath>

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

void SampleListener::onFrame(const Controller& controller) {
  const Frame frame = controller.frame();

  const ToolList tools = frame.tools();
  for (ToolList::const_iterator tl = tools.begin(); tl != tools.end(); ++tl) {
    const Tool tool = *tl;
#if _DEBUG
    std::cerr << std::string(2, ' ') <<  "Tool, id: " << tool.id()
              << ", position: " << tool.tipPosition()
              << ", direction: " << tool.direction() << std::endl;
#endif
  }

  const GestureList gestures = frame.gestures();
  for (int g = 0; g < gestures.count(); ++g) {
    Gesture gesture = gestures[g];

    switch (gesture.type()) {
      case Gesture::TYPE_CIRCLE:
      {
        CircleGesture circle = gesture;

        const bool clockwise = circle.pointable().direction().angleTo(circle.normal()) <= M_PI/2;

        if (CircleGesture::STATE_UPDATE == gesture.state()) {
            const int progress = circle.progress();
            if (progress != circle_pos[gesture.id()])
                if (clockwise)
                    std::cout << "forward" << std::endl;
                else
                    std::cout << "backward" << std::endl;
            circle_pos[gesture.id()] = progress;
        }

#ifdef _DEBUG
        std::cerr << std::string(2, ' ')
                  << "Circle id: " << gesture.id()
                  << ", state: " << stateNames[gesture.state()]
                  << ", progress: " << circle.progress()
                  << ", radius: " << circle.radius();
#endif
        break;
      }
      case Gesture::TYPE_SWIPE:
      {
        SwipeGesture swipe = gesture;
#ifdef _DEBUG
        std::cerr << std::string(2, ' ')
          << "Swipe id: " << gesture.id()
          << ", state: " << stateNames[gesture.state()]
          << ", direction: " << swipe.direction()
          << ", speed: " << swipe.speed() << std::endl;
#endif
        break;
      }
     default:
        std::cerr << std::string(2, ' ')  << "Unknown gesture type." << std::endl;
        break;
    }
  }
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

  controller.setPolicyFlags(Leap::Controller::POLICY_BACKGROUND_FRAMES);

  sleep(1000 * 1000 * 1000l);

  // Remove the sample listener when done
  controller.removeListener(listener);

  return 0;
}
