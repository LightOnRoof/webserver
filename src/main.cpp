#include <uWebSockets/App.h>
#include <unordered_set>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>

struct PerSocketData {};

std::unordered_set<uWS::WebSocket<false, true, PerSocketData>*> clients;

std::string get_mime_type(std::string_view path) {
    if (path.ends_with(".html")) return "text/html";
    if (path.ends_with(".js"))   return "application/javascript";
    if (path.ends_with(".css"))  return "text/css";
    if (path.ends_with(".png"))  return "image/png";
    if (path.ends_with(".jpg"))  return "image/jpeg";
    return "text/plain";
}


int main() {
    uWS::App()
    .get("/*", [](auto *res, auto *req) {
        std::string url = std::string(req->getUrl());
        if (url == "/") url = "/index.html";
        
        std::string path = "public" + url;

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
            clients.insert(ws);
            std::cout << "Client connected\n";
        },
        .message = [](auto *ws, std::string_view message, uWS::OpCode opCode) {
            // Echo back or ignore
            ws->send("Received: " + std::string(message), opCode);
        },
        .close = [](auto *ws, int /*code*/, std::string_view /*msg*/) {
            clients.erase(ws);
            std::cout << "Client disconnected\n";
        }
    })

    // POST endpoint
    .post("/send", [](auto *res, auto *req) {
        res->onData([res](std::string_view data, bool last) {
            static std::string body;

            body.append(data);

            if (last) {
                // Broadcast to all clients
                for (auto *client : clients) {
                    client->send(body, uWS::OpCode::TEXT);
                }

                res->end("Message broadcasted!");
                body.clear();
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