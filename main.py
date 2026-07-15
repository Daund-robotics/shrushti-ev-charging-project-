import os
import subprocess
import webbrowser
import time
import sys

def check_node():
    """Check if Node.js and npm are installed and working."""
    try:
        subprocess.run(["node", "-v"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True)
        subprocess.run("npm -v", stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=True, shell=True)
        return True
    except (subprocess.CalledProcessError, FileNotFoundError):
        return False

def run_vite():
    """Install dependencies and run Vite server."""
    print("\n[INFO] Node.js detected. Using Vite for an optimal experience.")
    
    if not os.path.exists("node_modules"):
        print("[INFO] Installing dependencies (npm install)...")
        subprocess.run("npm install", shell=True)
    
    print("[INFO] Starting Vite development server...")
    url = "http://localhost:5173"
    
    print(f"\n" + "="*40)
    print(f"WEB APP RUNNING AT: {url}")
    print("="*40 + "\n")
    
    time.sleep(2)
    webbrowser.open(url)
    
    try:
        subprocess.run("npm run dev", shell=True)
    except KeyboardInterrupt:
        print("\n[INFO] Shutting down.")

def run_python_server():
    """Fallback to Python's built-in HTTP server if Node.js is missing."""
    import http.server
    import socketserver
    
    PORT = 8080
    url = f"http://localhost:{PORT}"
    
    print("\n[WARNING] Node.js not found. Falling back to Python's built-in server.")
    
    print(f"\n" + "="*40)
    print(f"WEB APP RUNNING AT: {url}")
    print("="*40 + "\n")
    
    class Handler(http.server.SimpleHTTPRequestHandler):
        def log_message(self, format, *args):
            return # Minimal terminal log spamming

    with socketserver.TCPServer(("", PORT), Handler) as httpd:
        webbrowser.open(url)
        try:
            httpd.serve_forever()
        except KeyboardInterrupt:
            print("\n[INFO] Server stopped.")
            httpd.shutdown()

if __name__ == "__main__":
    print("--- VoltCharge IoT Dashboard Launcher ---")
    if check_node():
        run_vite()
    else:
        run_python_server()
