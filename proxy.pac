function FindProxyForURL(url, host) {
  if (url.substring(0, 5) == "http:") {
    return "PROXY 172.26.255.52:8080";
  }
  return "DIRECT";
}
// file://wsl.localhost/Ubuntu/GhostPort/proxy.pac
// file:///C:/Data_Structures_and_algorithms_material/C-programming/proxy.pac

// python3 -m http.server 9090
// http://172.26.255.52:8080/https://www.cs.princeton.edu/

// curl -v http://localhost:8080/https://www.cs.princeton.edu/
