// ailee_zmq_listener.h
#include <zmq.hpp>
#include <iostream>
#include <string>

// This function hooks into a real Bitcoin Core node (e.g., 127.0.0.1:28332)
void listenToBitcoinNetwork() {
    zmq::context_t context(1);
    zmq::socket_t subscriber(context, ZMQ_SUB);
    
    // Connect to local Bitcoin Core node
    subscriber.connect("tcp://127.0.0.1:28332");
    
    // Subscribe to 'rawtx' (Transaction stream) and 'hashblock' (New blocks)
    subscriber.set(zmq::sockopt::subscribe, "rawtx");
    subscriber.set(zmq::sockopt::subscribe, "hashblock");

    while (true) {
        zmq::message_t topic;
        zmq::message_t payload;
        
        subscriber.recv(topic, zmq::recv_flags::none);
        subscriber.recv(payload, zmq::recv_flags::none);

        std::string topic_str(static_cast<char*>(topic.data()), topic.size());
        
        if (topic_str == "rawtx") {
            // A new transaction appeared on Bitcoin L1
            // TODO: Decode transaction and check if it sent money to our Bridge Address
            // If YES -> Trigger ailee_sidechain_bridge.h -> initiatePegIn()
            std::cout << "[Bridge] New Transaction Detected on Mainnet" << std::endl;
        }
    }
}
