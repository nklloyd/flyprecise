#!/usr/bin/env python3

import socket
import sys
import time

def main():
    if len(sys.argv) < 2:
        print("Usage: python read_esp32.py <esp32_ip> [port]")
        print("Example: python read_esp32.py 192.168.1.100 8080")
        sys.exit(1)

    esp32_ip = sys.argv[1]
    port = int(sys.argv[2]) if len(sys.argv) > 2 else 8080

    print(f"Connecting to ESP32 at {esp32_ip}:{port}...")
    print("Press Ctrl+C to stop.\n")

    try:
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.connect((esp32_ip, port))
        sock.settimeout(1.0)  # 1 second timeout for reads

        print(f"Connected! Reading data from ESP32...\n")

        buffer = ""
        while True:
            try:
                data = sock.recv(1024).decode('utf-8', errors='replace')
                if not data:
                    print("Connection closed by ESP32")
                    break

                buffer += data

                while '\n' in buffer:
                    line, buffer = buffer.split('\n', 1)
                    line = line.rstrip('\r')
                    if line:
                        print(line, flush=True)

            except socket.timeout:
                continue
            except KeyboardInterrupt:
                print("\n\nStopped by user")
                break

    except ConnectionRefusedError:
        print(f"Connection refused. Make sure ESP32 is running and IP is correct: {esp32_ip}:{port}")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        try:
            sock.close()
        except:
            pass
        print("Connection closed.")


if __name__ == "__main__":
    main()