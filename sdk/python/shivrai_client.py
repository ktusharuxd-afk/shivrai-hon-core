"""Shivrai Python SDK - HTTP JSON-RPC client."""
import json
import urllib.request
import urllib.error


class ShivraiClient:
    def __init__(self, url="http://localhost:8545"):
        self.url = url.rstrip("/")
        self.rpc_url = self.url + "/rpc"
        self._id = 0

    def _call(self, method, params=None):
        self._id += 1
        req = {"jsonrpc": "2.0", "method": method,
               "params": params or [], "id": self._id}
        data = json.dumps(req).encode("utf-8")
        r = urllib.request.Request(self.rpc_url, data=data,
                                   headers={"Content-Type": "application/json"})
        try:
            with urllib.request.urlopen(r, timeout=10) as resp:
                body = json.loads(resp.read().decode("utf-8"))
                return body.get("result", body)
        except urllib.error.HTTPError as e:
            return {"error": "http " + str(e.code)}
        except Exception as e:
            return {"error": str(e)}

    def health(self):
        return self._call("system.health")

    def info(self):
        return self._call("system.info")

    def echo(self, msg):
        return self._call("system.echo", [msg])

    def rpc(self, method, params=None):
        return self._call(method, params)


if __name__ == "__main__":
    c = ShivraiClient()
    print("health:", c.health())
    print("info:  ", c.info())
    print("echo:  ", c.echo("hello shivrai"))

if __name__ == "__main__":
    c = ShivraiClient()
    print("health:", c.health())
    print("info:  ", c.info())
    print("echo:  ", c.echo("hello shivrai"))
