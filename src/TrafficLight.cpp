#include <iostream>
#include <random>
#include "TrafficLight.h"

/* Implementation of class "MessageQueue" */

template <typename T>
T MessageQueue<T>::receive()
{
    // FP.5a : The method receive should use std::unique_lock<std::mutex> and _condition.wait() 
    // to wait for and receive new messages and pull them from the queue using move semantics. 
    // The received object should then be returned by the receive function. 

    // perform queue modification under the lock
    std::unique_lock<std::mutex> uLock(_mutex);  // lock_guard cant be used here since lock needs to be temporarily released during wait

    // !_queue.empty()  gaurds againist spurious wakeups or random wakeups
    _condition.wait(uLock, [this] { return !_queue.empty(); }); // pass unique lock to condition variable

    // remove last vector element from queue
    T msg = std::move(_queue.back());
    _queue.pop_back();
    return msg; // will not be copied due to return value optimization (RVO) in C++
}

template <typename T>
void MessageQueue<T>::send(T &&msg)
{
    // FP.4a : The method send should use the mechanisms std::lock_guard<std::mutex> 
    // as well as _condition.notify_one() to add a new message to the queue and afterwards send a notification.

        // perform vector modification under the lock
        std::lock_guard<std::mutex> uLock(_mutex);  // lock is released once ulock gets out of scope

        // add vector to queue
        // std::cout << "   Message " << msg << " has been sent to the queue" << std::endl;
        _queue.clear();
        _queue.emplace_back(std::move(msg));
        _condition.notify_one(); // notify client after pushing new Vehicle into vector

}

/* Implementation of class "TrafficLight" */

 
TrafficLight::TrafficLight()
{
    _currentPhase = TrafficLightPhase::red;
}

void TrafficLight::waitForGreen()
{
    // FP.5b : add the implementation of the method waitForGreen, in which an infinite while-loop 
    // runs and repeatedly calls the receive function on the message queue. 
    // Once it receives TrafficLightPhase::green, the method returns.

    while(true)
    {
        if( _traffic_light_phase_msg_queue.receive() ==TrafficLightPhase::green)
        {
            return;
        }
    }

}

TrafficLightPhase TrafficLight::getCurrentPhase()
{
    return _currentPhase;
}

void TrafficLight::simulate()
{
    // FP.2b : Finally, the private method „cycleThroughPhases“ should be started in a thread when the public method „simulate“ is called. To do this, use the thread queue in the base class. 
    threads.emplace_back(std::thread(&TrafficLight::cycleThroughPhases, this));
}

// virtual function which is executed in a thread
void TrafficLight::cycleThroughPhases()
{
    // FP.2a : Implement the function with an infinite loop that measures the time between two loop cycles 
    // and toggles the current phase of the traffic light between red and green and sends an update method 
    // to the message queue using move semantics. The cycle duration should be a random value between 4 and 6 seconds. 
    // Also, the while-loop should use std::this_thread::sleep_for to wait 1ms between two cycles. 

    int lower_boundary(4000);  //  4 seconds
    int higher_boundary(6000); //  6 seconds
    
    double cycleDuration = lower_boundary + static_cast <double> (rand()) /( static_cast <double> (RAND_MAX/(higher_boundary-lower_boundary)));// duration of a single simulation cycle in ms
    std::chrono::time_point<std::chrono::system_clock> lastUpdate;

    while(true)
    {
        // sleep at every iteration to reduce CPU usage
        std::this_thread::sleep_for(std::chrono::milliseconds(1));

        // compute time difference to stop watch
        long timeSinceLastUpdate = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now() - lastUpdate).count();
        if (timeSinceLastUpdate >= cycleDuration)
        {
            _currentPhase = this->_currentPhase == TrafficLightPhase::red ? TrafficLightPhase::green : TrafficLightPhase::red;
            _traffic_light_phase_msg_queue.send(std::move(_currentPhase));
            // reset stop watch for next cycle
            lastUpdate = std::chrono::system_clock::now();
        }

    }
}

