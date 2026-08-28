//
// GoomControlServer.h
// goom-sdl desktop: HTTP+JSON remote control for goom's PluginParams.
//

#ifndef GOOM_CONTROL_SERVER_H
#define GOOM_CONTROL_SERVER_H

#include "goom.h" // PluginInfo (already extern-"C"-wrapped)
#include <string>

// Non-blocking HTTP/1.1 server exposing the visualizer's parameters.
// Single-threaded by design: tick() runs once per game-loop idle on the
// main thread, so reads/writes race neither goom_update nor the sound
// update, and writes apply before the next frame is computed.
//
// Endpoints (the control page is served same-origin, so no CORS):
//   GET  /               embedded control page
//   GET  /api/catalog    group/param metadata incl. current values
//   GET  /api/values     flat id -> value snapshot (poll ~5 Hz)
//   PUT  /api/params/G.P body {"value": X}; clamps, writes, echoes
//
// Security: loopback bind unless 'lan'; Host header must be localhost /
// 127.x / [::1] / IP-literal (DNS-rebinding defense); only GET/PUT are
// served; writes to rw=0 (feedback) params get 403.

class GoomControlServer {
public:
    // Tries port..port+9 until one binds. Never brings the app down:
    // isRunning() reports whether control is actually live.
    GoomControlServer(PluginInfo *goom, int port, bool lan);
    ~GoomControlServer();

    bool isRunning() const { return m_listenFd >= 0; }
    int  port() const { return m_port; } // actual bound port, 0 if down

    void tick(); // service all sockets once; call every game-loop idle

private:
    GoomControlServer(const GoomControlServer &);
    GoomControlServer &operator=(const GoomControlServer &);

    struct Conn;
    enum { MAX_CONNS = 8 };

    void acceptPending();
    bool serviceConn(Conn &c);       // false: close the connection
    bool handleRequest(const std::string &req, std::string &response,
                       bool &keepAlive);
    int  applyWrite(const char *idStr, const std::string &body,
                    std::string &outJson); // returns HTTP status

    PluginInfo *m_goom;
    int         m_listenFd;
    int         m_port;
    Conn       *m_conns;
};

#endif
