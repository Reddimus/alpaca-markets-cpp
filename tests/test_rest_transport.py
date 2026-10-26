"""Exercise the real REST transport with dummy credentials and loopback only."""
import http.server
import os
import subprocess
import sys
import threading
import time


class Handler(http.server.BaseHTTPRequestHandler):
    protocol_version = "HTTP/1.1"
    attempts = 0
    mode = ""

    def log_message(self, *args):
        pass

    def do_GET(self):
        type(self).attempts += 1
        if self.mode == "query":
            expected = ["/v2/orders:by_client_order_id?client_order_id=a%26nested%3Dtrue",
                        "/v2/positions/BTC%2FUSD%3Fx%3D1",
                        "/v2/calendar?start=2026-10-25&end=2026-10-26"]
            assert self.path == expected[self.attempts - 1], self.path
        else:
            assert self.path == "/v2/account"
        assert self.headers["APCA-API-KEY-ID"] == "fixture-key"
        assert self.headers["APCA-API-SECRET-KEY"] == "fixture-secret"
        if self.mode == "timeout":
            time.sleep(2)
        status = 503 if self.mode == "retry" and self.attempts < 3 else 200
        self.reply(status, b"[]" if self.path.startswith("/v2/calendar") else b"{}")

    def do_DELETE(self):
        type(self).attempts += 1
        assert self.path == "/v2/orders/fixture"
        self.reply(503)

    def reply(self, status, body=b"{}"):
        self.send_response(status)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Connection", "close")
        self.end_headers()
        try:
            self.wfile.write(body)
        except (BrokenPipeError, ConnectionResetError):
            pass


class Server(http.server.ThreadingHTTPServer):
    failures = []

    def handle_error(self, request, client_address):
        self.failures.append(sys.exc_info()[1])


server = Server(("127.0.0.1", 0), Handler)
worker = threading.Thread(target=server.serve_forever, daemon=True)
worker.start()
environment = dict(os.environ)
environment.update(
    ALPACA_MARKETS_KEY_ID="fixture-key",
    ALPACA_MARKETS_SECRET_KEY="fixture-secret",
    ALPACA_MARKETS_TRADING_URL=f"http://127.0.0.1:{server.server_port}",
    ALPACA_MARKETS_DATA_URL=f"http://127.0.0.1:{server.server_port}",
    ALPACA_MARKETS_STREAM_URL="ws://127.0.0.1/unused",
)
try:
    for mode, expected in [("retry", 3), ("write", 1), ("timeout", 1), ("query", 3)]:
        Handler.mode, Handler.attempts = mode, 0
        subprocess.run([sys.argv[1], mode], env=environment, check=True, timeout=8)
        assert Handler.attempts == expected, (mode, Handler.attempts)
        assert not server.failures, server.failures
finally:
    server.shutdown()
    server.server_close()
    worker.join()
