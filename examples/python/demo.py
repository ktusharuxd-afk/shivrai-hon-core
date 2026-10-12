#!/usr/bin/env python3
import sys, os
sys.path.insert(0, os.path.join(os.path.dirname(__file__), "..", "..", "sdk", "python"))
from shivrai_client import ShivraiClient

def main():
    c = ShivraiClient()
    print("=== shivrai-node demo ===")
    print("health:", c.health())
    print("info:  ", c.info())
    print("echo:  ", c.echo("hello from python"))
    print("done")

if __name__ == "__main__":
    main()
