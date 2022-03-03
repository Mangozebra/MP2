#include <iostream>
#include <string>
#include <memory>
#include <unistd.h>
#include <thread>
#include <fstream>
#include <sstream>
#include <grpc++/grpc++.h>
#include "client.h"
#include <stdexcept>
#include "sns.grpc.pb.h"
#include <sys/time.h>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>

#include <google/protobuf/util/time_util.h>

using grpc::ClientContext;

class Client : public IClient
{
    public:
        Client(const std::string& hname,
               const std::string& uname,
               const std::string& p)
            :hostname(hname), username(uname), port(p)
            {}
    protected:
        virtual int connectTo();
        virtual IReply processCommand(std::string& input);
        virtual void processTimeline();
    private:
        std::string hostname;
        std::string username;
        std::string port;
        
        // You can have an instance of the client stub
        // as a member variable.
        std::unique_ptr<csce438::SNSService::Stub> stub_;
};

int main(int argc, char** argv) {

    std::string hostname = "localhost";
    std::string username = "default";
    std::string port = "3010";
    int opt = 0;
    while ((opt = getopt(argc, argv, "h:u:p:")) != -1){
        switch(opt) {
            case 'h':
                hostname = optarg;break;
            case 'u':
                username = optarg;break;
            case 'p':
                port = optarg;break;
            default:
                std::cerr << "Invalid Command Line Argument\n";
        }
    }

    Client myc(hostname, username, port);
    // You MUST invoke "run_client" function to start business logic
    myc.run_client();

    return 0;
}

int Client::connectTo()
{
	// ------------------------------------------------------------
    // In this function, you are supposed to create a stub so that
    // you call service methods in the processCommand/porcessTimeline
    // functions. That is, the stub should be accessible when you want
    // to call any service methods in those functions.
    // I recommend you to have the stub as
    // a member variable in your own Client class.
    // Please refer to gRpc tutorial how to create a stub.
	// ------------------------------------------------------------
	
	std::string server_address(hostname);
    server_address += ":";
    server_address += port;
    
    std::cout << "Server address is " << server_address << std::endl;
	
    std::shared_ptr<grpc::Channel> channel = grpc::CreateChannel(server_address, grpc::InsecureChannelCredentials());
    
    if(channel->GetState(false) == 3){ // 3 denotes TRANSIENT_FAILURE
        std::cout << "Unsuccessful" << std::endl;
        return -1;
    }
   
    stub_ = csce438::SNSService::NewStub(channel);
     
    // Duplicate Username Handling
    ClientContext context;
    
    csce438::Request request;
    csce438::Reply reply;
    
    request.set_username(username);
    
    grpc::Status status = stub_->Login(&context, request, &reply);
    
    if (!status.ok()) {
        std::cout << "Status Not Okay" << std::endl;
        return -1;
    }
    
    
    
    // Seeing if username exists
    // If message doesn't go through, then that also counts as a connection issue
    //std::cout << "Status error code: " << status.error_code() << std::endl;
    
    
    return 1; // return 1 if success, otherwise return -1
}

IReply Client::processCommand(std::string& input)
{
	// ------------------------------------------------------------
	// GUIDE 1:
	// In this function, you are supposed to parse the given input
    // command and create your own message so that you call an 
    // appropriate service method. The input command will be one
    // of the followings:
	//
	// FOLLOW <username>
	// UNFOLLOW <username>
	// LIST
    // TIMELINE
	//
	// ------------------------------------------------------------
	
	std::stringstream ss(std::string{input});
	
	std::string command1;
	std::string command2;
	
	ss >> command1;
	
	// If there's anything left, load it to command2
	if (ss.rdbuf()->in_avail() != 0)
	{
		ss >> command2;
	}
	
	 std::cout << "Command 1 is " << command1 << std::endl;
	 std::cout << "Command 2 is " << command2 << std::endl;	
    // ------------------------------------------------------------
	// GUIDE 2:
	// Then, you should create a variable of IReply structure
	// provided by the client.h and initialize it according to
	// the result. Finally you can finish this function by returning
    // the IReply.
	// ------------------------------------------------------------
    
	// ------------------------------------------------------------
    // HINT: How to set the IReply?
    // Suppose you have "Follow" service method for FOLLOW command,
    // IReply can be set as follow:
    // 
    //     // some codes for creating/initializing parameters for
    //     // service method
    //     IReply ire;
    //     grpc::Status status = stub_->Follow(&context, /* some parameters */);
    //     ire.grpc_status = status;
    //     if (status.ok()) {
    //         ire.comm_status = SUCCESS;
    //     } else {
    //         ire.comm_status = FAILURE_NOT_EXISTS;
    //     }
    //      
    //      return ire;
    // 
    // IMPORTANT: 
    // For the command "LIST", you should set both "all_users" and 
    // "following_users" member variable of IReply.
    // ------------------------------------------------------------
    ClientContext context;
    IReply ire;
    
    csce438::Request request;
    csce438::Reply reply;
    
    toUpperCase(command1);
    
    
    if(command1 == "FOLLOW" || command1 == "UNFOLLOW"){
         
        request.set_username(username);
        request.add_arguments(command2);
        
        grpc::Status status;
        
        if(command1 == "FOLLOW"){
            status = stub_->Follow(&context, request, &reply);
        }else{
            status = stub_->UnFollow(&context, request, &reply);
        }
        
        ire.grpc_status = status;
        std::cout << "Error code is: " << status.error_code() << std::endl;
        if (status.ok()) {
            if(reply.msg() == "FAILURE_ALREADY_EXISTS"){
                ire.comm_status = FAILURE_ALREADY_EXISTS;
            }else if(reply.msg() == "FAILURE_INVALID_USERNAME"){
                ire.comm_status = FAILURE_INVALID_USERNAME;
            }else{
                ire.comm_status = SUCCESS;
            }
            
        } else {
            ire.comm_status = FAILURE_INVALID_USERNAME;
        }
    
    }else if(command1 == "TIMELINE"){
        
        std::shared_ptr<grpc::ClientReaderWriter<csce438::Message, csce438::Message>> stream(stub_->Timeline(&context));
       
        csce438::Message blankMessage;
        blankMessage.set_username(username);
       
        stream->Write(blankMessage);
       
        grpc::Status status = stream->Finish();
       
        ire.grpc_status = status;
        if (status.ok()) {
            ire.comm_status = SUCCESS;
        } else {
            ire.comm_status = FAILURE_NOT_EXISTS;
        }
        
        std::cout << "Reply grpc_status is " << ire.grpc_status.ok() << std::endl;
        std::cout << "Reply comm_status is " << ire.comm_status << std::endl;
        
    }else{
        
        request.set_username(username);
        grpc::Status status = stub_->List(&context, request, &reply);
        
        std::vector<std::string> allUsers;
        std::vector<std::string> followingUsers;
        
        for(std::string i : reply.all_users())
            allUsers.push_back(i);
            
        for(std::string i : reply.following_users()) 
            followingUsers.push_back(i);
    
        ire.all_users = allUsers;
        ire.following_users = followingUsers;
    
        ire.grpc_status = status;
        if (status.ok()) {
            ire.comm_status = SUCCESS;
        } else {
            ire.comm_status = FAILURE_NOT_EXISTS;
        }
        
        
    }

    return ire;
}

void Client::processTimeline()
{   
    ClientContext context;
    
    
    
	// ------------------------------------------------------------
    // In this function, you are supposed to get into timeline mode.
    // You may need to call a service method to communicate with
    // the server. Use getPostMessage/displayPostMessage functions
    // for both getting and displaying messages in timeline mode.
    // You should use them as you did in hw1.
	// ------------------------------------------------------------
    
    //grpc::Status status = stub_->Timeline(&context, message, reply);
    std::shared_ptr<grpc::ClientReaderWriter<csce438::Message, csce438::Message>> stream(stub_->Timeline(&context));
    
    csce438::Message outgoingMessage;
    csce438::Message incomingMessage;

    // Signal thread to break out of loop if you were to back out of the timeline
    // Get threads to communicate between user input and printing text from other users
    
    // Use condition variables for communication
    std::cout << "Inside timeline function" << std::endl;
    
    std::thread writer = std::thread([stream, &outgoingMessage, this]() {
        
        while(1){
            
            outgoingMessage.set_username(username);
            outgoingMessage.set_msg(getPostMessage());
            
            //std::cout << outgoingMessage.msg() << std::endl;
            
            struct timeval current_time;
            gettimeofday(&current_time, NULL);
            //printf("seconds : %ld\nmicro seconds : %ld",
            //current_time.tv_sec, current_time.tv_usec);
            
            outgoingMessage.mutable_timestamp()->set_seconds(current_time.tv_sec);
            outgoingMessage.mutable_timestamp()->set_nanos(current_time.tv_usec*1000);
        
            stream->Write(outgoingMessage);
        }
      
    });
    
    
    std::thread reader = std::thread([stream, &incomingMessage, this]() {
        
        while(1){
        
            stream->Read(&incomingMessage);
            
            google::protobuf::Timestamp const t_stamp = incomingMessage.timestamp();
            displayPostMessage(incomingMessage.username(), incomingMessage.msg(),  google::protobuf::util::TimeUtil::TimestampToTimeT(t_stamp));
        }
      
    });    
    
    
    while(1);
    
    // ------------------------------------------------------------
    // IMPORTANT NOTICE:
    //
    // Once a user enter to timeline mode , there is no way
    // to command mode. You don't have to worry about this situation,
    // and you can terminate the client program by pressing
    // CTRL-C (SIGINT)
	// ------------------------------------------------------------
}
