//
// GoomControlServer.cpp
// goom-sdl desktop: HTTP+JSON remote control for goom's PluginParams.
//

#include "GoomControlServer.h"

#include "GTLog.h"

#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>
// Reap connections idle longer than this (slowloris guard: a client that
// never finishes its request must not pin one of the MAX_CONNS slots).
#define IDLE_TIMEOUT_S 10

using gametools::GTLogf;
namespace {

// ---------------------------------------------------------------------------
// Embedded control page (single file, no network resources: the app is
// offline, so fonts/scripts must not come from a CDN).
// ---------------------------------------------------------------------------

const char kControlPage[] = R"HTML(<!doctype html>
<html lang="en">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>goom · control</title>
<style>
:root{
  --bg:#0b0d10; --panel:#12161b; --line:#232a33; --text:#c9cdd6; --dim:#6d7683;
  --amber:#ffb454; --amber-dim:#8a6428; --teal:#3fd9c0; --red:#ff5d5d;
}
*{box-sizing:border-box;margin:0;padding:0}
body{
  background:var(--bg); color:var(--text);
  font:13px/1.45 ui-monospace,'SF Mono',Menlo,Consolas,monospace;
  padding:0 16px 64px;
}
body::before{
  content:""; position:fixed; inset:0; pointer-events:none; z-index:50;
  background:repeating-linear-gradient(0deg,rgba(255,255,255,.018) 0 1px,transparent 1px 3px);
}
.wrap{max-width:720px;margin:0 auto}
header.top{
  position:sticky; top:0; z-index:10; display:flex; align-items:center; gap:12px;
  padding:14px 4px 10px; background:linear-gradient(var(--bg) 85%,transparent);
  border-bottom:1px solid var(--line);
}
h1{font-size:12px;font-weight:600;letter-spacing:.22em;text-transform:uppercase}
h1 b{color:var(--amber)}
#status{width:8px;height:8px;border-radius:50%;background:var(--red);margin-left:auto;transition:background .3s}
#status.ok{background:var(--teal);box-shadow:0 0 6px var(--teal)}
section{margin-top:22px;border:1px solid var(--line);background:var(--panel)}
section>header{
  display:flex;gap:10px;padding:8px 12px;border-bottom:1px solid var(--line);
  font-size:10px;letter-spacing:.18em;text-transform:uppercase;color:var(--dim);
}
section>header .idx{color:var(--amber-dim)}
.param{padding:10px 12px;border-top:1px solid #1a2027}
.param:first-of-type{border-top:none}
.param:hover{background:#161b21}
.prow{display:flex;justify-content:space-between;align-items:baseline;margin-bottom:7px}
.ro .pname{color:var(--dim)}
.pval{color:var(--amber);font-size:12px;min-width:64px;text-align:right}
.ro .pval{color:var(--teal)}
input[type=range]{-webkit-appearance:none;appearance:none;width:100%;height:18px;background:transparent;cursor:pointer}
input[type=range]::-webkit-slider-runnable-track{height:2px;background:#2a323d}
input[type=range]::-webkit-slider-thumb{-webkit-appearance:none;appearance:none;width:10px;height:14px;margin-top:-6px;background:var(--amber);border:none;border-radius:0}
input[type=range]:hover::-webkit-slider-thumb{box-shadow:0 0 8px var(--amber)}
input[type=range]::-moz-range-track{height:2px;background:#2a323d}
input[type=range]::-moz-range-thumb{width:10px;height:14px;background:var(--amber);border:none;border-radius:0}
.meter{height:4px;background:#1b222b;overflow:hidden}
.meter .fill{height:100%;width:0;background:var(--teal);box-shadow:0 0 6px rgba(63,217,192,.6);transition:width .12s linear}
.toggle{
  font:inherit;font-size:10px;letter-spacing:.14em;text-transform:uppercase;
  background:transparent;color:var(--dim);border:1px solid var(--line);
  padding:4px 14px;cursor:pointer;min-width:72px;
}
.toggle.on{color:#0b0d10;background:var(--amber);border-color:var(--amber)}
footer{margin-top:28px;color:#3d454f;font-size:10px;letter-spacing:.12em;text-transform:uppercase}
</style>
</head>
<body>
<div class="wrap">
<header class="top"><h1>goom <b>·</b> control</h1><div id="status"></div></header>
<div id="groups"></div>
<footer>goom-sdl &middot; http+json &middot; values poll 5hz</footer>
</div>
<script>
const meta = {};
const dirty = new Set();
const timers = {};
const $ = s => document.querySelector(s);

function fmt(p, v){
  if (typeof v !== 'number') return String(v);
  return p.type === 'int' ? String(Math.round(v)) : v.toFixed(2);
}

async function boot(){
  const cat = await (await fetch('api/catalog')).json();
  const root = $('#groups');
  cat.groups.forEach((g, gi) => {
    const sec = document.createElement('section');
    const h = document.createElement('header');
    h.innerHTML = '<span class="idx">' + String(gi).padStart(2,'0') + '</span>' + g.name;
    sec.appendChild(h);
    g.params.forEach(p => { meta[p.id] = p; sec.appendChild(row(p)); });
    root.appendChild(sec);
  });
  poll();
  setInterval(poll, 200);
}

function row(p){
  const d = document.createElement('div');
  d.className = 'param' + (p.rw ? '' : ' ro');
  const head = '<div class="prow"><span class="pname">' + p.name + '</span>' +
               '<span class="pval" id="v-' + p.id + '"></span></div>';
  if (!p.rw){
    d.innerHTML = head + '<div class="meter"><div class="fill" id="m-' + p.id + '"></div></div>';
  } else if (p.type === 'bool'){
    d.innerHTML = head.replace(' id="v-' + p.id + '"', '') +
                 '<button class="toggle" id="t-' + p.id + '">off</button>';
    d.querySelector('.toggle').addEventListener('click', () => put(p.id, p.value ? 0 : 1, true));
  } else {
    d.innerHTML = head + '<input type="range" id="s-' + p.id + '" min="' + p.min +
                  '" max="' + p.max + '" step="' + p.step + '" value="' + p.value + '">';
    const s = d.querySelector('input');
    s.addEventListener('input',  () => { dirty.add(p.id); put(p.id, +s.value, false); });
    s.addEventListener('change', () => { put(p.id, +s.value, true);
                                         setTimeout(() => dirty.delete(p.id), 250); });
  }
  return d;
}

function put(id, value, now){
  clearTimeout(timers[id]);
  const send = () => fetch('api/params/' + id,
                           {method:'PUT', body:JSON.stringify({value})}).catch(() => {});
  if (now) send(); else timers[id] = setTimeout(send, 60);
}

async function poll(){
  try{
    const j = await (await fetch('api/values')).json();
    for (const [id, v] of Object.entries(j.v)){
      const p = meta[id]; if (!p) continue;
      p.value = v;
      const lbl = document.getElementById('v-' + id);
      if (lbl) lbl.textContent = fmt(p, v);
      const s = document.getElementById('s-' + id);
      if (s && !dirty.has(id)) s.value = v;
      const t = document.getElementById('t-' + id);
      if (t){ t.classList.toggle('on', !!v); t.textContent = v ? 'on' : 'off'; }
      const m = document.getElementById('m-' + id);
      if (m){
        const lo = (p.min !== undefined ? p.min : 0), hi = (p.max !== undefined ? p.max : 1);
        m.style.width = Math.max(0, Math.min(100, (v - lo) / (hi - lo || 1) * 100)) + '%';
      }
    }
    $('#status').className = 'ok';
  }catch(e){ $('#status').className = ''; }
}

boot();
</script>
</body>
</html>
)HTML";

// ---------------------------------------------------------------------------
// Small helpers
// ---------------------------------------------------------------------------

void setNonBlocking(int fd) {
    fcntl(fd, F_SETFL, fcntl(fd, F_GETFL, 0) | O_NONBLOCK);
}

bool sendAll(int fd, const std::string &s) {
    size_t off = 0;
    while (off < s.size()) {
        ssize_t n = send(fd, s.data() + off, s.size() - off, 0);
        if (n > 0) off += (size_t)n;
        else if (n < 0 && errno == EINTR) continue;
        else return false; // EAGAIN: drop rather than stall the frame loop
    }
    return true;
}

std::string httpResponse(int code, const char *reason, const char *contentType,
                         const std::string &body, bool keepAlive) {
    char head[256];
    snprintf(head, sizeof head,
             "HTTP/1.1 %d %s\r\n"
             "Content-Type: %s\r\n"
             "Content-Length: %u\r\n"
             "Cache-Control: no-store\r\n"
             "Connection: %s\r\n\r\n",
             code, reason, contentType, (unsigned)body.size(),
             keepAlive ? "keep-alive" : "close");
    return std::string(head) + body;
}

void jsonEscape(std::string &s, const char *v) {
    s += '"';
    if (v) {
        for (const unsigned char *p = (const unsigned char *)v; *p; ++p) {
            if (*p == '"' || *p == '\\') { s += '\\'; s += (char)*p; }
            else if (*p >= 32 && *p < 127) s += (char)*p;
            else { char b[8]; snprintf(b, sizeof b, "\\u%04x", *p); s += b; }
        }
    }
    s += '"';
}

const char *typeName(ParamType t) {
    switch (t) {
        case PARAM_INTVAL:   return "int";
        case PARAM_FLOATVAL: return "float";
        case PARAM_BOOLVAL:  return "bool";
        case PARAM_STRVAL:   return "str";
        case PARAM_LISTVAL:  return "list";
    }
    return "?";
}

void appendValue(std::string &s, const PluginParam &p) {
    char b[64];
    s += ",\"value\":";
    switch (p.type) {
        case PARAM_INTVAL:
            snprintf(b, sizeof b, "%d", IVAL(p)); s += b; break;
        case PARAM_FLOATVAL:
            snprintf(b, sizeof b, "%g", (double)FVAL(p)); s += b; break;
        case PARAM_BOOLVAL:
            s += BVAL(p) ? "true" : "false"; break;
        default:
            s += "null"; break;
    }
}

// Host header gate: loopback names or IP literals only. A web page on an
// attacker domain can otherwise reach this server via DNS rebinding; its
// requests always carry the attacker hostname in Host, which this rejects.
bool hostAllowed(const std::string &h) {
    if (h.empty()) return true; // curl & friends; browsers always send Host
    if (h.compare(0, 9, "localhost") == 0 && (h.size() == 9 || h[9] == ':'))
        return true;
    if (h.compare(0, 5, "[::1]") == 0 && (h.size() == 5 || h[5] == ':'))
        return true;
    int dots = 0;
    for (size_t i = 0; i < h.size(); ++i) {
        char c = h[i];
        if (c == ':') break;
        if (c == '.') { ++dots; continue; }
        if (c < '0' || c > '9') return false;
    }
    return dots == 3; // IPv4 literal
}

// Case-insensitive header lookup inside the header block [0, hdrEnd).
std::string headerValue(const std::string &req, size_t hdrEnd,
                        const char *name) {
    const size_t nameLen = strlen(name);
    size_t pos = req.find("\r\n");
    if (pos == std::string::npos) return "";
    pos += 2;
    while (pos < hdrEnd) {
        size_t eol = req.find("\r\n", pos);
        if (eol == std::string::npos || eol > hdrEnd) eol = hdrEnd;
        size_t colon = req.find(':', pos);
        if (colon != std::string::npos && colon < eol &&
            eol - pos > nameLen &&
            strncasecmp(req.data() + pos, name, nameLen) == 0 &&
            colon == pos + nameLen) {
            size_t v = colon + 1;
            while (v < eol && (req[v] == ' ' || req[v] == '\t')) ++v;
            return req.substr(v, eol - v);
        }
        pos = eol + 2;
    }
    return "";
}

} // namespace

// ---------------------------------------------------------------------------
// GoomControlServer
// ---------------------------------------------------------------------------

struct GoomControlServer::Conn {
    int fd = -1;
    time_t lastActive = 0; // accept/recv stamp; tick() reaps when idle
    std::string in;
};

GoomControlServer::GoomControlServer(PluginInfo *goom, int port, bool lan)
    : m_goom(goom), m_listenFd(-1), m_port(0), m_conns(new Conn[MAX_CONNS]) {
    for (int p = port; p < port + 10; ++p) {
        int fd = socket(AF_INET, SOCK_STREAM, 0);
        if (fd < 0) break;
        int one = 1;
        setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof one);
        struct sockaddr_in addr;
        memset(&addr, 0, sizeof addr);
        addr.sin_family = AF_INET;
        addr.sin_port = htons((uint16_t)p);
        addr.sin_addr.s_addr = lan ? htonl(INADDR_ANY) : htonl(INADDR_LOOPBACK);
        if (bind(fd, (struct sockaddr *)&addr, sizeof addr) == 0 &&
            listen(fd, 4) == 0) {
            setNonBlocking(fd);
            m_listenFd = fd;
            m_port = p;
            break;
        }
        close(fd);
    }
    if (m_listenFd < 0) {
        GTLogf("goom: control: no free port in %d-%d, disabled", port, port + 9);
        return;
    }
    GTLogf("goom: control UI: http://localhost:%d/", m_port);
    if (lan) {
        struct ifaddrs *ifas = NULL;
        if (getifaddrs(&ifas) == 0) {
            for (struct ifaddrs *ifa = ifas; ifa; ifa = ifa->ifa_next) {
                if (!ifa->ifa_addr || ifa->ifa_addr->sa_family != AF_INET) continue;
                if (ifa->ifa_flags & IFF_LOOPBACK) continue;
                if (!(ifa->ifa_flags & IFF_UP)) continue;
                char ip[INET_ADDRSTRLEN];
                struct sockaddr_in *sin = (struct sockaddr_in *)ifa->ifa_addr;
                if (inet_ntop(AF_INET, &sin->sin_addr, ip, sizeof ip))
                    GTLogf("goom: control UI (LAN): http://%s:%d/", ip, m_port);
            }
            freeifaddrs(ifas);
        }
    }
}

GoomControlServer::~GoomControlServer() {
    for (int i = 0; i < MAX_CONNS; ++i)
        if (m_conns[i].fd >= 0) close(m_conns[i].fd);
    if (m_listenFd >= 0) close(m_listenFd);
    delete[] m_conns;
}

void GoomControlServer::tick() {
    if (m_listenFd < 0) return;
    acceptPending();
    time_t now = time(NULL);
    for (int i = 0; i < MAX_CONNS; ++i) {
        Conn &c = m_conns[i];
        if (c.fd < 0) continue;
        if (now - c.lastActive > IDLE_TIMEOUT_S || !serviceConn(c)) {
            close(c.fd);
            c.fd = -1;
            c.in.clear();
        }
    }
}

void GoomControlServer::acceptPending() {
    for (;;) {
        int fd = accept(m_listenFd, NULL, NULL);
        if (fd < 0) return;
        int slot = -1;
        for (int i = 0; i < MAX_CONNS; ++i)
            if (m_conns[i].fd < 0) { slot = i; break; }
        if (slot < 0) { close(fd); return; }
        int one = 1;
        #ifdef SO_NOSIGPIPE
        setsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, &one, sizeof one);
#endif
        setNonBlocking(fd);
        m_conns[slot].fd = fd;
        m_conns[slot].lastActive = time(NULL);
        m_conns[slot].in.clear();
    }
}

bool GoomControlServer::serviceConn(Conn &c) {
    char buf[4096];
    for (;;) {
        ssize_t n = recv(c.fd, buf, sizeof buf, 0);
        if (n > 0) {
            c.in.append(buf, (size_t)n);
            c.lastActive = time(NULL);
            if (c.in.size() > 16384) return false; // header never ends
        } else if (n == 0) {
            return false; // peer closed
        } else {
            if (errno != EAGAIN && errno != EWOULDBLOCK) return false;
            break;
        }
    }
    for (int handled = 0; handled < 4; ++handled) {
        size_t hdrEnd = c.in.find("\r\n\r\n");
        if (hdrEnd == std::string::npos) return true; // wait for more
        std::string cl = headerValue(c.in, hdrEnd, "content-length");
        size_t bodyLen = cl.empty() ? 0 : (size_t)atoi(cl.c_str());
        if (bodyLen > 4096) return false;
        size_t total = hdrEnd + 4 + bodyLen;
        if (c.in.size() < total) return true; // body incomplete
        std::string response;
        bool keepAlive = true;
        bool ok = handleRequest(c.in.substr(0, total), response, keepAlive);
        c.in.erase(0, total);
        if (!sendAll(c.fd, response)) return false;
        if (!ok || !keepAlive) return false;
    }
    return true;
}

bool GoomControlServer::handleRequest(const std::string &req,
                                      std::string &response,
                                      bool &keepAlive) {
    size_t hdrEnd = req.find("\r\n\r\n");
    size_t sp1 = req.find(' ');
    size_t sp2 = sp1 == std::string::npos ? sp1 : req.find(' ', sp1 + 1);
    if (sp1 == std::string::npos || sp2 == std::string::npos) {
        response = httpResponse(400, "Bad Request", "text/plain",
                                "bad request\n", keepAlive);
        return true;
    }
    std::string method = req.substr(0, sp1);
    std::string path = req.substr(sp1 + 1, sp2 - sp1 - 1);
    size_t q = path.find('?');
    if (q != std::string::npos) path.erase(q);
    if (path.size() > 512) {
        response = httpResponse(414, "URI Too Long", "text/plain",
                                "uri too long\n", keepAlive);
        return true;
    }

    std::string connHdr = headerValue(req, hdrEnd, "connection");
    if (strcasestr(connHdr.c_str(), "close")) keepAlive = false;

    if (method != "GET" && method != "PUT") {
        response = httpResponse(405, "Method Not Allowed", "text/plain",
                                "method not allowed\n", keepAlive);
        return true;
    }
    if (!hostAllowed(headerValue(req, hdrEnd, "host"))) {
        response = httpResponse(403, "Forbidden", "text/plain",
                                "forbidden\n", keepAlive);
        return true;
    }

    if (method == "GET" && path == "/") {
        response = httpResponse(200, "OK", "text/html; charset=utf-8",
                                kControlPage, keepAlive);
        return true;
    }
    if (method == "GET" && (path == "/api/catalog" || path == "/api/values")) {
        std::string body;
        if (path == "/api/catalog") {
            body = "{\"groups\":[";
            for (int g = 0; g < m_goom->nbParams; ++g) {
                PluginParameters &grp = m_goom->params[g];
                if (g) body += ',';
                body += "{\"name\":";
                jsonEscape(body, grp.name);
                body += ",\"params\":[";
                bool first = true;
                for (int p = 0; p < grp.nbParams; ++p) {
                    PluginParam *pp = grp.params[p];
                    if (!pp) continue; // separator slot; index preserved in id
                    if (!first) body += ',';
                    first = false;
                    char id[32];
                    snprintf(id, sizeof id, "%d.%d", g, p);
                    body += "{\"id\":\"";
                    body += id;
                    body += "\",\"name\":";
                    jsonEscape(body, pp->name);
                    body += ",\"type\":\"";
                    body += typeName(pp->type);
                    body += "\",\"rw\":";
                    body += pp->rw ? "true" : "false";
                    appendValue(body, *pp);
                    char num[64];
                    if (pp->type == PARAM_INTVAL) {
                        snprintf(num, sizeof num, ",\"min\":%d,\"max\":%d,\"step\":%d",
                                 IMIN(*pp), IMAX(*pp), ISTEP(*pp));
                        body += num;
                    } else if (pp->type == PARAM_FLOATVAL) {
                        snprintf(num, sizeof num, ",\"min\":%g,\"max\":%g,\"step\":%g",
                                 (double)FMIN(*pp), (double)FMAX(*pp), (double)FSTEP(*pp));
                        body += num;
                    }
                    body += '}';
                }
                body += "]}";
            }
            body += "]}";
        } else {
            body = "{\"v\":{";
            bool first = true;
            for (int g = 0; g < m_goom->nbParams; ++g) {
                PluginParameters &grp = m_goom->params[g];
                for (int p = 0; p < grp.nbParams; ++p) {
                    PluginParam *pp = grp.params[p];
                    if (!pp) continue;
                    if (!first) body += ',';
                    first = false;
                    char id[40];
                    snprintf(id, sizeof id, "\"%d.%d\"", g, p);
                    body += id;
                    body += ':';
                    size_t vpos = body.size();
                    appendValue(body, *pp); // emits ,"value":X; strip prefix
                    body.erase(vpos, 9);
                }
            }
            body += "}}";
        }
        response = httpResponse(200, "OK", "application/json", body, keepAlive);
        return true;
    }
    if (method == "PUT" && path.compare(0, 12, "/api/params/") == 0) {
        std::string body;
        size_t bodyStart = hdrEnd + 4;
        if (bodyStart < req.size()) body = req.substr(bodyStart);
        std::string out;
        int code = applyWrite(path.c_str() + 12, body, out);
        const char *reason = code == 200 ? "OK" :
                             code == 400 ? "Bad Request" :
                             code == 403 ? "Forbidden" :
                             code == 404 ? "Not Found" : "Not Implemented";
        response = httpResponse(code, reason, "application/json", out, keepAlive);
        return true;
    }

    response = httpResponse(404, "Not Found", "text/plain", "not found\n",
                            keepAlive);
    return true;
}

int GoomControlServer::applyWrite(const char *idStr, const std::string &body,
                                  std::string &out) {
    int g = -1, p = -1;
    if (sscanf(idStr, "%d.%d", &g, &p) != 2 ||
        g < 0 || g >= m_goom->nbParams) {
        out = "{\"error\":\"unknown param\"}";
        return 404;
    }
    PluginParameters &grp = m_goom->params[g];
    if (p < 0 || p >= grp.nbParams || !grp.params[p]) {
        out = "{\"error\":\"unknown param\"}";
        return 404;
    }
    PluginParam *pp = grp.params[p];
    if (!pp->rw) {
        out = "{\"error\":\"read-only\"}";
        return 403;
    }

    // Minimal JSON scan: locate "value" then parse the token after ':'.
    const char *v = strstr(body.c_str(), "\"value\"");
    if (!v) {
        out = "{\"error\":\"missing value\"}";
        return 400;
    }
    v = strchr(v, ':');
    if (!v) {
        out = "{\"error\":\"missing value\"}";
        return 400;
    }
    ++v;
    while (*v == ' ' || *v == '\t') ++v;

    switch (pp->type) {
        case PARAM_INTVAL: {
            char *end = NULL;
            long val = strtol(v, &end, 10);
            if (end == v) { out = "{\"error\":\"bad value\"}"; return 400; }
            if (val < IMIN(*pp)) val = IMIN(*pp);
            if (val > IMAX(*pp)) val = IMAX(*pp);
            IVAL(*pp) = (int)val;
            break;
        }
        case PARAM_FLOATVAL: {
            char *end = NULL;
            errno = 0;
            double val = strtod(v, &end);
            if (end == v) { out = "{\"error\":\"bad value\"}"; return 400; }
            // strtod also accepts "nan"/"inf"/hex spellings, and -ffast-math
            // makes isnan/isfinite-style checks unreliable: reject non-JSON
            // tokens lexically; ERANGE then catches overflow to Inf.
            for (const char *t = v; t < end; ++t) {
                char c = *t;
                if (!(c == 'e' || c == 'E' || c == '+' || c == '-' ||
                      c == '.' || (c >= '0' && c <= '9'))) {
                    out = "{\"error\":\"bad value\"}"; return 400;
                }
            }
            if (errno == ERANGE) { out = "{\"error\":\"bad value\"}"; return 400; }
            if (val < FMIN(*pp)) val = FMIN(*pp);
            if (val > FMAX(*pp)) val = FMAX(*pp);
            FVAL(*pp) = (float)val;
            break;
        }
        case PARAM_BOOLVAL: {
            if (*v == 't' || *v == '1') BVAL(*pp) = 1;
            else if (*v == 'f' || *v == '0') BVAL(*pp) = 0;
            else { out = "{\"error\":\"bad value\"}"; return 400; }
            break;
        }
        default:
            out = "{\"error\":\"type not writable\"}";
            return 501;
    }

    pp->changed(pp); // empty unless the effect wired an apply-hook (e.g. IFS)

    char id[40];
    snprintf(id, sizeof id, "{\"id\":\"%d.%d\"", g, p);
    out = id;
    appendValue(out, *pp); // echo effective (post-clamp, post-hook) value
    out += '}';
    return 200;
}
