
#include <ctime>

#include <google/protobuf/timestamp.pb.h>
#include <google/protobuf/duration.pb.h>
#include <google/protobuf/repeated_field.h>

#include <iostream>
#include <fstream>
#include <iostream>
#include <dirent.h>
#include <memory>
#include <string>
#include <stdlib.h>
#include <unistd.h>
#include <google/protobuf/util/time_util.h>
#include <grpc++/grpc++.h>
#include <map>
#include <fstream>

#include "sns.grpc.pb.h"

using google::protobuf::Timestamp;
using google::protobuf::Duration;
using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::ServerReader;
using grpc::ServerReaderWriter;
using grpc::ServerWriter;
using grpc::Status;
using csce438::Message;
using csce438::Request;
using csce438::Reply;
using csce438::SNSService;

std::map<std::string, csce438::UserInfo> userList;
std::map<ServerReaderWriter<Message, Message>*, std::string> connectionList;

bool userExists(const std::string userName){
  
  std::map<std::string, csce438::UserInfo>::iterator it = userList.find(userName);

  if (it != userList.end()){ // If it exists
    return true;
  }
  
  return false;
  
}

class SNSServiceImpl final : public SNSService::Service {
  
  Status List(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // LIST request from the user. Ensure that both the fields
    // all_users & following_users are populated
    // ------------------------------------------------------------
    
    for(std::map<std::string,csce438::UserInfo>::const_iterator it = userList.begin(); it != userList.end(); ++it){

      reply->add_all_users(it->first);
      
      if(it->first == request->username()){
        
        for(int i = 0; i < it->second.followers_size(); i++){
          reply->add_following_users(it->second.followers(i));
        }
        
      }
      
    }
    
    
    
    return Status::OK;
  }

  Status Follow(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // request from a user to follow one of the existing
    // users
    // ------------------------------------------------------------
    
    // Keep user from following themselves
    if( request->arguments(0) == request->username()){
      reply->set_msg("FAILURE_ALREADY_EXISTS");
      return Status::OK;
    }
    
    std::map<std::string, csce438::UserInfo>::iterator it;
    std::string username = request->arguments(0);
    
    for(std::map<std::string,csce438::UserInfo>::const_iterator it = userList.begin(); it != userList.end(); ++it){
      std::cout << it->first << " " << it->second.name() << " " << "\n";
    }
    
    // If the user exists
    if(userExists(username)){
      
      
      bool exists = false;
  
      for(int i = 0; i < userList[request->arguments(0)].followers_size(); i++){
        std::cout << "Checking " << userList[request->arguments(0)].followers(i) << std::endl;
        if(userList[request->arguments(0)].followers(i) == request->username()){
          exists = true;
        }
      }
      
      if(!exists){
        
        userList[request->arguments(0)].add_followers(request->username());
        
        // Add User to Disk
        std::fstream output("./UserData/" +  request->arguments(0), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!userList[request->arguments(0)].SerializeToOstream(&output)) {
          std::cerr << "Failed to write user data." << std::endl;
           return Status::CANCELLED;
        }
      
      }
        
      
    }else{
        reply->set_msg("FAILURE_INVALID_USERNAME");
        return Status::OK;
    }
    
    std::cout << request->arguments(0) << std::endl;
    
    return Status::OK; 
  }

 Status UnFollow(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // request from a user to unfollow one of his/her existing
    // followers
    // ------------------------------------------------------------
    
    if( request->arguments(0) == request->username()){
      reply->set_msg("FAILURE_ALREADY_EXISTS");
      return Status::OK;
    }
    
    std::map<std::string, csce438::UserInfo>::iterator it;
    std::string username = request->arguments(0);
    
    for(std::map<std::string,csce438::UserInfo>::const_iterator it = userList.begin(); it != userList.end(); ++it){
      std::cout << it->first << " " << it->second.name() << " " << "\n";
    }
    
    if(userExists(username)){
      
      
      int iterator = -1;
      
      for(int i = 0; i < userList[request->arguments(0)].followers_size(); i++){
        if(userList[request->arguments(0)].followers(i) == request->username()){
          iterator = i;
        }
      }
      
      if(iterator != -1){
        userList[request->arguments(0)].mutable_followers()->DeleteSubrange(iterator, 1);
        
          // Add User to Disk
        std::fstream output("./UserData/" +  request->arguments(0), std::ios::out | std::ios::trunc | std::ios::binary);
        if (!userList[request->arguments(0)].SerializeToOstream(&output)) {
          std::cerr << "Failed to write user data." << std::endl;
           return Status::CANCELLED;
        }
      }
        
      
    }else{
        reply->set_msg("FAILURE_INVALID_USERNAME");
        return Status::OK;
    }
    
    std::cout << request->arguments(0) << std::endl;
    
    
    return Status::OK;
  }
  
 Status Login(ServerContext* context, const Request* request, Reply* reply) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // a new user and verify if the username is available
    // or already taken
    // ------------------------------------------------------------
    std::cout << "Username is " << request->username() << std::endl;
    
    for(std::map<std::string,csce438::UserInfo>::const_iterator it = userList.begin(); it != userList.end(); ++it){
      std::cout << it->first << " " << it->second.name() << " " << "\n";
    }
    
    std::map<std::string, csce438::UserInfo>::iterator it;
    
    
    if (userExists(request->username())){ // If it exists
      std::cout << "Username exists" << std::endl;
      reply->set_msg("USERNAME_EXISTS");
    }else{
      std::cout << "Username avaialbe" << std::endl;
      
      csce438::UserInfo userData;
      userData.set_name(request->username());
      
      
      // Add User to Disk
      std::fstream output("./UserData/" + request->username(), std::ios::out | std::ios::trunc | std::ios::binary);
      if (!userData.SerializeToOstream(&output)) {
        std::cerr << "Failed to write user data." << std::endl;
         return Status::CANCELLED;
      }
      
      std::vector<ServerReaderWriter<Message, Message>*> vect;
      
      userList.insert(std::pair<std::string, csce438::UserInfo>(request->username(), userData));
      
      for(std::map<std::string,csce438::UserInfo>::const_iterator it = userList.begin(); it != userList.end(); ++it){
        std::cout << it->first << " " << it->second.name() << " " << "\n";
      }
      
      reply->set_msg("USERNAME_AVAILABLE");
    }
    
    std::cout << "Returning" << std::endl;
    return Status::OK;
  }

  Status Timeline(ServerContext* context, ServerReaderWriter<Message, Message>* stream) override {
    // ------------------------------------------------------------
    // In this function, you are to write code that handles 
    // receiving a message/post from a user, recording it in a file
    // and then making it available on his/her follower's streams
    // ------------------------------------------------------------
    
    csce438::Message incomingMsg;
    std::string clientName;
    std::string messageSender;
    
    
    while (stream->Read(&incomingMsg)) {
      
      // Getting the sender of the message
      messageSender = incomingMsg.username();
      
      if(!incomingMsg.mutable_msg()->empty()){
        
        clientName = connectionList[stream];
    
        std::cout << clientName << " is selected" << std::endl;
        
        if(clientName == messageSender){
          
          std::map<std::string, csce438::UserInfo>::iterator it = userList.find(clientName);
          
          if (it != userList.end()){ 
            
            std::cout << "Finding followers of " << it->first << std::endl;
            
            for(int i = 0; i < it->second.followers_size(); i++){ // Iterate through followers and send to them
              std::cout << "User has " << it->second.followers_size() << " followers" << std::endl;
              std::string follower =  it->second.followers(i);
              std::cout << "Sending to " << follower << std::endl;
                
                
                for (std::map<ServerReaderWriter<Message, Message>*, std::string>::iterator j=connectionList.begin(); j!=connectionList.end(); ++j){ //Searching through connectionList to find streams belonging to the follower
                
                    if(j->second == follower){
                    
                      
                      if(!j->first->Write(incomingMsg)){
                      
                        connectionList.erase(j);
                      }
                      
                    }
                }
              
            }
          }
        }
        
        
      }else{
        // Preventing duplicates
        std::map<ServerReaderWriter<Message, Message>*, std::string>::iterator it = connectionList.find(stream);
        
        if(it == connectionList.end()){
          connectionList.insert(std::pair<ServerReaderWriter<Message, Message>*, std::string>(stream, incomingMsg.username()));
          break;
        }
        
      }
    
      
      
    }
  
    
    return Status::OK;
    
  }
    

};

void RunServer(std::string port_no) {
  // ------------------------------------------------------------
  // In this function, you are to write code 
  // which would start the server, make it listen on a particular
  // port number.
  // ------------------------------------------------------------
  
  // Scanning UserData 
  
  std::string destination = "./UserData";
  
  if (auto dir = opendir(destination.c_str())) {
    
    if (dir) {
      while (auto f = readdir(dir)) {
        
        csce438::UserInfo userData;
        
        if (!f->d_name || f->d_name[0] == '.')
            continue; // Skip everything that starts with a dot
            
        std::string oldDest = destination;
        destination += "/";
        
        destination +=  f->d_name;
        std::fstream input(destination.c_str(), std::ios::in | std::ios::binary); 
        
        if (!userData.ParseFromIstream(&input)) {
          std::cerr << "Failed to parse file." << std::endl;
          exit(EXIT_FAILURE);
        }
        
        userList.insert(std::pair<std::string, csce438::UserInfo>(f->d_name, userData));
        
        destination = oldDest;
        input.close();
        
      }
      
      closedir(dir);
    }
    
  } else if (ENOENT == errno) {
        std::cout << "Directory Doesn't exist." << std::endl;
    } else {
        std::cerr << "operndir error" << std::endl;
  }
  
  std::string server_address("127.0.0.1:");
  server_address += port_no;
  SNSServiceImpl service;
  
  grpc::ServerBuilder builder;
  builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
  builder.RegisterService(&service);
  
  
  std::unique_ptr<Server> server(builder.BuildAndStart());
  std::cout << "waiting" << std::endl;
  server->Wait();

}

int main(int argc, char** argv) {
  
  std::string port = "3010";
  int opt = 0;
  while ((opt = getopt(argc, argv, "p:")) != -1){
    switch(opt) {
      case 'p':
          port = optarg;
          break;
      default:
	         std::cerr << "Invalid Command Line Argument\n";
    }
  }
  RunServer(port);
  return 0;
}
