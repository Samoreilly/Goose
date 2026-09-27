
#include "dsa/RingBuffer.h"

void producer(RingBuffer<int, 65536>* ring_buffer) {

    for (int i = 0; i < 100; i++) 
        ring_buffer->add(i); 
}

void consumer(RingBuffer<int, 65536>* ring_buffer) {
    for (int i = 0; i < 100; i++) 
        ring_buffer->read();
    
}

int main() {

    std::println("GooseTSDB started");

    RingBuffer<int, 65536> ring_buffer;

    std::thread producers[1] = {
        std::thread(producer, &ring_buffer)
    };

    std::thread consumers[3] = {
        std::thread(consumer, &ring_buffer),
        std::thread(consumer, &ring_buffer),
        std::thread(consumer, &ring_buffer),
    };

    for (auto& t : producers)
        t.join();
    for (auto& t : consumers)
        t.join();


    return 0;
}
