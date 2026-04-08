#include <uWebSockets/App.h>
#include <nlohmann/json.hpp>
#include <unordered_set>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <filesystem>

using json = nlohmann::json;
struct PerSocketData {
    int userID;
    std::string username;
};
int users = 0;
std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*> clients;
std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*> clickClients;
std::unordered_set<std::string> usernames;
std::map<int,uWS::WebSocket<false,true,PerSocketData>*> idMap;
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
    json response;
    if(cur == "/help"){
        response["type"]="message";
        response["data"]="/help : display this message <br>/online : lists online users<br>/username : sets username<br>/click : clicks someone( user ID)<br>/clickall : gives everyone a click :3";
    }else if (cur == "/username"){
        std::getline(ss,cur,' ');
        if(usernames.count(cur)){
            response["type"]="message";
            response["data"]="Username Taken!";
        }else{
            usernames.insert(cur);
        }
        response["type"]="message";
        response["data"]="Username set to : " + cur;
        ws->getUserData()->username=cur;
    }else if (cur == "/list"){
        response["type"]="message";
        std::string x = "";
        for(auto& i : idMap){
            auto s = i.second->getUserData()->username;
            x+="id: "+std::to_string(i.first)+" - "+(s.size()?s:" no name") + "<br>";
        }
        response["data"]=x;

    }else if (cur == "/click")
    {
        response["type"]="click :3";
        std::getline(ss,cur,' ');
        int id;
        try
        {
            id = std::stoi(cur);
        }
        catch(const std::exception& e)
        {
            std::cerr << e.what() << '\n';
        }
        if(idMap.contains(id)){
            idMap[id]->send(response.dump(),uWS::OpCode(1));
        }else{
            response["type"]="message";
            response["data"]="used not found";
            ws->send(response.dump(), uWS::OpCode(1));
        }
        return;
    }else if (cur == "/clickall")
    {
        response["type"]="message";
        response["data"]="okie clicking everone";

        json click;
        click["type"]="click :3";
        for (auto *client : clickClients) {
            client->send(click.dump(), uWS::OpCode::TEXT);
        }
        for (auto *client : clients) {
            client->send(click.dump(), uWS::OpCode::TEXT);
        }
    }
    ws->send(response.dump(), uWS::OpCode(1));
    
    
}

int main() {
    uWS::App()
    .get("/*", [](auto *res, auto *req) {
        std::string url = std::string(req->getUrl());
        if(url == "/click"){
            json response;
            response["type"]="click :3";
            for (auto *client : clickClients) {
                client->send(response.dump(), uWS::OpCode::TEXT);
            }
            res->writeStatus("200 OK")->end("meow");
            return;
        }
        
        if (url == "/") url = "/index.html";
        if (url == "/clickme") url = "/clickme.html";
        
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
    .ws<PerSocketData>("/", {
        .open = [](auto *ws) {
            PerSocketData *userData = (PerSocketData*) ws->getUserData();
            userData->userID = users;
            users++;
            clients.insert(ws);
            json response;
            response["type"]="message";
            response["data"]="Your user ID is: " + std::to_string(userData->userID);
            idMap[userData->userID]=ws;
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
                json response;
                response["type"]="message";
                response["data"]=getName(ws->getUserData()) +" : "+std::string(message);
                c->send(response.dump(), opCode);
            }

        },
        
        .close = [](auto *ws, int /*code*/, std::string_view /*msg*/) {
            clients.erase(ws);
            idMap.erase(ws->getUserData()->userID);
            usernames.erase(ws->getUserData()->username);
            for(auto& c : clients){
                json response;
                response["type"]="message";
                response["data"]=getName(ws->getUserData()) +" disconnected";
                c->send(response.dump(), uWS::OpCode(1));
            }
            std::cout << "Client disconnected\n";
        }
    })
    .ws<PerSocketData>("/sub", {
        .open = [](auto *ws) {
            clickClients.insert(ws);
            std::cout << "click client connected\n";
        },
        
        .close = [](auto *ws, int /*code*/, std::string_view /*msg*/) {
            clickClients.erase(ws);
            std::cout << "sub disconnected\n";
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
}).post("/click", [](auto *res, auto *req) {
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
            std::cout<<"click requewted"<<std::endl;
            //ping my phone lol
            std::string cmd = "curl -d \"OwO!\" ntfy.sh/clickuwupls";
            system(cmd.c_str());
            // Broadcast to all connected WebSocket clients
            json response;
            response["type"]="click :3";
            
            for (auto *client : clickClients) {
                client->send(response.dump(), uWS::OpCode::TEXT);
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