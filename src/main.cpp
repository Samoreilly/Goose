
#include <print>

#include "dsa/RingBuffer.h"
#include "ingestion.h"

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

    Ingestion<int, 65536> ingestor;
    ingestor.start();

    return 0;
}
