#include "App.h"
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

using json = nlohmann::json;
struct PerSocketData {
    int userID;
    std::string username;
};
int users = 0;
std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*> clients;
std::unordered_set<std::string> usernames;
std::string get_mime_type(std::string_view path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".js"))   return "application/javascript";
    if (path.ends_with(".css"))  return "text/css";
    if (path.ends_with(".png"))  return "image/png";
    if (path.ends_with(".jpg"))  return "image/jpeg";
    return "text/plain";
}
std::string getName(PerSocketData* ps){
    if(ps->username==""){
        return "User "+std::to_string(ps->userID);
    }else{
        return ps->username;
    }
}
void parseCommand(uWS::WebSocket<false, true, PerSocketData>* ws, std::string msg){
    //split
    if(!msg.size()) return;
    std::stringstream ss(msg);
    std::string cur;
    std::getline(ss,cur,' ');
    if(cur == "/help"){
        ws->send("/help : display this message\n/online : lists online users\n/username : sets username\n/click : clicks someone( user ID)", uWS::OpCode(1));
    }else if (cur == "/username"){
        std::getline(ss,cur,' ');
        if(usernames.count(cur)){
            ws->send("Username taken!", uWS::OpCode(1));
            return;
        }else{
            usernames.insert(cur);
        }
        ws->getUserData()->username=cur;
        ws->send("Username set to : " + cur, uWS::OpCode(1));
    }else if (cur == "/list")
    {
        /* code */
    }else if (cur == "/click")
    {
        /* code */
    }
    
    
    
}

int main() {
    uWS::App()
    .get("/*", [](auto *res, auto *req) {
        std::string url = std::string(req->getUrl());
        if (url == "/") url = "/index.html";
        
        std::string path = "public" + url;
        std::cout<<"Requested path: "+ path<<std::endl;
        if (std::filesystem::exists(path) && !std::filesystem::is_directory(path)) {
            // Set the content type
            res->writeHeader("Content-Type", get_mime_type(path));
            
            // Read file into string (Note: for very large files, use streaming/chunking)
            std::ifstream file(path, std::ios::binary);
            std::stringstream buffer;
            buffer << file.rdbuf();
            res->end(buffer.str());
        } else {
            res->writeStatus("404 Not Found")->end("File not found");
        }
    })
    // WebSocket endpoint
    .ws<PerSocketData>("/*", {
        .open = [](auto *ws) {
            PerSocketData *userData = (PerSocketData*) ws->getUserData();
            userData->userID = users;
            users++;
            clients.insert(ws);
            json respose;
            response["type"]="message";
            response["data"]="Your user ID is: " + std::to_string(userData->userID);
            ws->send(response.dump(), (uWS::OpCode) 1);
            std::cout << "Client connected\n";
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
            // Echo back or ignore
            if(message.size()>0 && message[0]=='/'){
                //command
                parseCommand(ws, std::string(message));
                return ;
            }
            for(auto& c : clients){
                response["type"]="message";
                response["data"]=getName(ws->getUserData()) +" says: "+std::string(message);
                c->send(response.dump(), opCode);
            }

        },
        .close = [](auto *ws, int /*code*/, std::string_view /*msg*/) {
            clients.erase(ws);
            usernames.erase(ws->getUserData()->username);
            for(auto& c : clients){
                response["type"]="message";
                response["data"]=getName(ws->getUserData()) +" disconnected";
                c->send(response.dump(), uWS::OpCode(1));
            }
            std::cout << "Client disconnected\n";
        }
    })

    // POST endpoint
    .post("/send", [](auto *res, auto *req) {
    /* 1. Create a container for the data */
    std::string* body = new std::string();

    /* 2. You MUST define onAborted! 
       This is called if the client hangs up before the data is finished. */
    res->onAborted([body]() {
        std::cerr << "Request aborted by client!" << std::endl;
        delete body; // Clean up memory
    });

    /* 3. Handle incoming data chunks */
    res->onData([res, body](std::string_view data, bool last) {
        body->append(data);

        if (last) {
            // Broadcast to all connected WebSocket clients
            for (auto *client : clients) {
                client->send(*body, uWS::OpCode::TEXT);
            }

            // 4. Send the response to the POST request
            res->end("Message broadcasted!");
            
            // 5. Clean up memory after finishing
            delete body;
        }
    });
})
    

    .listen(8080, [](auto *token) {
        if (token) {
            std::cout << "Server running on port 8080\n";
        }
    })

    .run();
}