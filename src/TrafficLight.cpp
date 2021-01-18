#include <iostream>
#include <random>
#include <future>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */


template <typename T>
T MessageQueue<T>::receive()
{
      
    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);
    _cond.wait(uLock, [this] { return !_messages.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_messages.back());
    _messages.pop_back();

    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // add a new message to the queue and afterwards send a notification.
    
    // perform vector modification under the lock
    std::lock_guard<std::mutex> uLock(_mutex);

    // add vector to queue
    _messages.push_back(std::move(msg));
    _cond.notify_one(); // notify client after pushing new message into queue
}


/* Implementation of class "TrafficLight" */


TrafficLight::TrafficLight()
{
    std::lock_guard<std::mutex> uLock(_mutex);
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    do {
    }
    while (_messages.receive() != green);
}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    std::lock_guard<std::mutex> uLock(_mutex);
    return _currentPhase;
}



void TrafficLight::simulate()
{

    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // Toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration is a random value between 4 and 6 seconds. 

    std::random_device rd;  //Will be used to obtain a seed for the random number engine
    std::mt19937 gen(rd()); //Standard mersenne_twister_engine seeded with rd()
    std::uniform_real_distribution<float> dis(4.0, 6.0);
    while (true) {
      double cycleDuration = dis(gen); // duration of a single phase in s
      std::chrono::time_point<std::chrono::system_clock> lastUpdate;
      // init stop watch
      lastUpdate = std::chrono::system_clock::now();
      long timeSinceLastUpdate;
      do {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();     
      }
      while (timeSinceLastUpdate / 1000.0 < cycleDuration);
      std::lock_guard<std::mutex> uLock(_mutex);
      _currentPhase = (_currentPhase == green) ? red : green; 
      _messages.send(std::move(_currentPhase));
      _condition.notify_one();
    }
}

