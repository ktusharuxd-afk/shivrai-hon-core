// Shivrai Node — HTTP JSON-RPC
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <csignal>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include <map>

struct Json {
    enum T { NUL, BOOL, NUM, STR, ARR, OBJ } t = NUL;
    bool b = false;
    double n = 0;
    std::string s;
    std::vector<Json> arr;
    std::map<std::string, Json> obj;

    Json& operator[](const std::string& k) { t = OBJ; return obj[k]; }
    Json& operator[](const char* k) { t = OBJ; return obj[std::string(k)]; }

    Json& operator=(const std::string& v) { t = STR; s = v; return *this; }
    Json& operator=(const char* v) { t = STR; s = v ? v : ""; return *this; }
    Json& operator=(double v) { t = NUM; n = v; return *this; }
    Json& operator=(int v) { t = NUM; n = v; return *this; }
    Json& operator=(bool v) { t = BOOL; b = v; return *this; }
};

static void skip_ws(const std::string& s, size_t& i) {
    while (i < s.size() && (s[i]==' '||s[i]=='\t'||s[i]=='\n'||s[i]=='\r')) i++;
}

static std::string parse_str(const std::string& s, size_t& i) {
    std::string out;
    if (i >= s.size() || s[i] != '"') return out;
    i++;
    while (i < s.size() && s[i] != '"') {
        if (s[i] == '\\' && i+1 < s.size()) {
            char c = s[i+1];
            if (c == 'n') out += '\n';
            else if (c == 't') out += '\t';
            else if (c == 'r') out += '\r';
            else if (c == '"') out += '"';
            else if (c == '\\') out += '\\';
            else out += c;
            i += 2;
        } else out += s[i++];
    }
    if (i < s.size()) i++;
    return out;
}

static Json parse_val(const std::string& s, size_t& i) {
    skip_ws(s, i);
    Json j;
    if (i >= s.size()) return j;
    char c = s[i];
    if (c == '{') {
        j.t = Json::OBJ; i++;
        skip_ws(s, i);
        while (i < s.size() && s[i] != '}') {
            std::string k = parse_str(s, i);
            skip_ws(s, i);
            if (i < s.size() && s[i] == ':') i++;
            skip_ws(s, i);
            j.obj[k] = parse_val(s, i);
            skip_ws(s, i);
            if (i < s.size() && s[i] == ',') i++;
            skip_ws(s, i);
        }
        if (i < s.size()) i++;
    } else if (c == '[') {
        j.t = Json::ARR; i++;
        skip_ws(s, i);
        while (i < s.size() && s[i] != ']') {
            j.arr.push_back(parse_val(s, i));
            skip_ws(s, i);
            if (i < s.size() && s[i] == ',') i++;
            skip_ws(s, i);
        }
        if (i < s.size()) i++;
    } else if (c == '"') {
        j.t = Json::STR; j.s = parse_str(s, i);
    } else if (c == 't') { j.t = Json::BOOL; j.b = true; i += 4; }
    else if (c == 'f') { j.t = Json::BOOL; j.b = false; i += 5; }
    else if (c == 'n') { j.t = Json::NUL; i += 4; }
    else {
        j.t = Json::NUM;
        size_t st = i;
        while (i < s.size() && (isdigit(s[i])||s[i]=='-'||s[i]=='.'||s[i]=='e'||s[i]=='E'||s[i]=='+')) i++;
        j.n = std::atof(s.substr(st, i-st).c_str());
    }
    return j;
}

static std::string json_escape(const std::string& s) {
    std::string o;
    for (char c : s) {
        if (c == '"') o += "\\\"";
        else if (c == '\\') o += "\\\\";
        else if (c == '\n') o += "\\n";
        else o += c;
    }
    return o;
}

static std::string stringify(const Json& j) {
    switch (j.t) {
        case Json::NUL: return "null";
        case Json::BOOL: return j.b ? "true" : "false";
        case Json::NUM: { char buf[64]; snprintf(buf, 64, "%.0f", j.n); return buf; }
        case Json::STR: return "\"" + json_escape(j.s) + "\"";
        case Json::ARR: {
            std::string o = "[";
            for (size_t i = 0; i < j.arr.size(); i++) {
                if (i) o += ",";
                o += stringify(j.arr[i]);
            }
            return o + "]";
        }
        case Json::OBJ: {
            std::string o = "{";
            bool first = true;
            for (auto& kv : j.obj) {
                if (!first) o += ",";
                first = false;
                o += "\"" + json_escape(kv.first) + "\":" + stringify(kv.second);
            }
            return o + "}";
        }
    }
    return "null";
}

struct Node {
    Json handle(const std::string& method, const Json& params) {
        Json r;
        if (method == "system.health") {
            r["status"] = "ok";
            r["version"] = "0.1.0";
            r["modules"] = 29;
            return r;
        }
        if (method == "system.echo") {
            r["echo"] = params.t == Json::ARR && !params.arr.empty() ? params.arr[0].s : "";
            return r;
        }
        if (method == "system.info") {
            r["name"] = "shivrai-hon-node";
            r["chain_id"] = "shivrai-1";
            return r;
        }
        r["error"] = "unknown method: " + method;
        return r;
    }
};

static Node g_node;
static volatile bool g_running = true;

static void sig_handler(int) { g_running = false; }

static std::string http_response(int code, const std::string& body) {
    std::string status = (code == 200) ? "200 OK" :
                         (code == 404) ? "404 Not Found" : "500 Error";
    std::ostringstream o;
    o << "HTTP/1.1 " << status << "\r\n"
      << "Content-Type: application/json\r\n"
      << "Content-Length: " << body.size() << "\r\n"
      << "Access-Control-Allow-Origin: *\r\n"
      << "Connection: close\r\n\r\n"
      << body;
    return o.str();
}

static void handle_client(int fd) {
    char buf[65536];
    ssize_t n = recv(fd, buf, sizeof(buf)-1, 0);
    if (n <= 0) { close(fd); return; }
    buf[n] = 0;
    std::string req(buf, n);

    size_t hdr_end = req.find("\r\n\r\n");
    std::string body = (hdr_end != std::string::npos) ? req.substr(hdr_end + 4) : "";

    if (req.rfind("GET ", 0) == 0) {
        if (req.find(" /health ") != std::string::npos) {
            std::string r = http_response(200, "{\"status\":\"ok\"}");
            send(fd, r.c_str(), r.size(), 0);
        } else {
            std::string r = http_response(404, "{\"error\":\"not found\"}");
            send(fd, r.c_str(), r.size(), 0);
        }
        close(fd);
        return;
    }

    if (body.empty()) {
        std::string r = http_response(400, "{\"error\":\"empty body\"}");
        send(fd, r.c_str(), r.size(), 0);
        close(fd);
        return;
    }

    size_t i = 0;
    Json req_j = parse_val(body, i);
    std::string method = req_j.obj.count("method") ? req_j.obj["method"].s : "";
    Json params = req_j.obj.count("params") ? req_j.obj["params"] : Json{};
    Json id = req_j.obj.count("id") ? req_j.obj["id"] : Json{};

    Json resp;
    resp["jsonrpc"] = "2.0";
    resp["id"] = id;
    resp["result"] = g_node.handle(method, params);

    std::string out = stringify(resp);
    std::string http = http_response(200, out);
    send(fd, http.c_str(), http.size(), 0);
    close(fd);
}

int main(int argc, char** argv) {
    int port = 8545;
    if (argc > 1) port = std::atoi(argv[1]);

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    int srv = socket(AF_INET, SOCK_STREAM, 0);
    if (srv < 0) { perror("socket"); return 1; }

    int opt = 1;
    setsockopt(srv, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(srv, (sockaddr*)&addr, sizeof(addr)) < 0) { perror("bind"); return 1; }
    if (listen(srv, 16) < 0) { perror("listen"); return 1; }

    std::cout << "shivrai-node listening on http://0.0.0.0:" << port << "\n";
    std::cout << "  POST /rpc  |  GET /health\n" << std::flush;

    while (g_running) {
        sockaddr_in cli{};
        socklen_t clilen = sizeof(cli);
        int c = accept(srv, (sockaddr*)&cli, &clilen);
        if (c < 0) { if (!g_running) break; continue; }
        handle_client(c);
    }

    close(srv);
    std::cout << "\nshivrai-node stopped\n";
    return 0;
}
