Perfect 🔥 — let’s expand your **defense cheat sheet** so that each section has a **theory note**, just like you asked for the subnet masks. This way you’ll be prepared to answer both *“what did you do?”* and *“why/how does it work?”* in the defense.

---

# 📑 Full Lab Defense Cheat Sheet (with embedded theory)

---

### **Network Addressing (Static + DHCP)**

* **Static IPs** → Servers (DNS, Mail, Web, FTP, DHCP) need fixed addresses so clients always know where to find them.
* **DHCP (UDP 67/68)** → Automatically assigns addresses to clients.
* **DHCP process:**

  1. **DISCOVER** – client broadcasts for IP.
  2. **OFFER** – server proposes IP + config.
  3. **REQUEST** – client accepts.
  4. **ACK** – server confirms, lease created.
* **Subnet Masks**: define network vs host portion.

  * `255.255.255.0` = `/24`, 254 hosts.
  * Devices with the same network bits can talk directly.
  * Routers use masks for forwarding decisions.
* **Verification**: `ipconfig` on clients, manual config on servers.

---

### **DNS**

* **Protocol**: DNS (UDP/TCP 53).
* **What it does**: Maps domain names → IP addresses (A Records).
* **How it works**: Client queries DNS → DNS replies with IP.
* **Theory notes**:

  * DNS avoids memorizing IPs.
  * Hierarchical system (root, TLD, authoritative).
  * Local DNS server speeds up resolution in LAN.
* **Verification**: Configured A Records (`ftp.labredes.com → 192.168.1.33`), tested with `ping web.labredes.com`, `nslookup dns.labredes.com`.

---

### **Connectivity Tests (Ping, nslookup)**

* **Protocol**: ICMP (not TCP/UDP).
* **Ping**: sends Echo Request, expects Echo Reply → proves host reachability at the network layer.
* **nslookup**: queries DNS server → shows domain resolution.
* **Theory notes**:

  * Ping measures latency (ms) and packet loss.
  * ICMP is diagnostic, not transport.
  * If ping fails → higher protocols (HTTP, SMTP, FTP) won’t work.
* **Verification**: `ping` between clients, servers, and domains.

---

### **Web Server (HTTP/HTTPS)**

* **Protocols**:

  * **HTTP (TCP 80)** → transfers web pages in plain text.
  * **HTTPS (TCP 443)** → HTTP + TLS encryption.
* **What it does**: Browser sends GET request → server responds with HTML page.
* **Theory notes**:

  * HTTPS ensures confidentiality & authenticity.
  * Web uses **client/server** model (browser is client, server hosts pages).
  * Lab: each student had a personal profile page.
* **Verification**: Accessed server by IP and by DNS name; pages loaded over HTTP and HTTPS.

---

### **Email (SMTP + POP3)**

* **SMTP (TCP 25)** → *sending mail* from client → server.
* **POP3 (TCP 110)** → *retrieving mail* from server → client.
* **Why two protocols?** Sending vs. receiving are different jobs.
* **Theory notes**:

  * SMTP = “outgoing postman” (pushes mail).
  * POP3 = “mailbox pickup” (downloads + deletes mail).
  * Authentication ensures only the mailbox owner can retrieve mail.
* **Flow**: User1 → SMTP → Mail Server → mailbox of User2 → POP3 → User2.
* **Verification**:

  * Created accounts (username + password).
  * Pinged Mail Server for connectivity.
  * Configured clients with email accounts.
  * Sent & received test messages.

---

### **FTP (File Transfer Protocol)**

* **Protocols/Ports**:

  * Control channel (TCP 21) → commands (`USER`, `PASS`, `LIST`, `GET`, `PUT`).
  * Data channel (TCP 20, active mode) → actual file transfer.
* **What it does**: Moves files between client and server.
* **Theory notes**:

  * Two channels separate “instructions” from “content.”
  * FTP is unencrypted (credentials & files in plaintext).
  * Modern practice uses SFTP/FTPS for security.
* **Lab flow**:

  1. Client pings FTP server to check connectivity.
  2. Connects with `ftp ftp.labredes.com`.
  3. Logs in with user credentials.
  4. Runs `dir` to list files.
  5. Runs `get filename` → server opens data channel, sends file → client saves it locally.
* **Verification**: Screenshots of ping, login, `dir`, and successful `get`.

---

## 🔑 Quick Recap

For **defense questions**, always frame answers like this:

* **Protocol & Port** (e.g., SMTP = TCP/25).
* **Purpose** (send, receive, translate names, transfer files).
* **Theory** (how it works under the hood).
* **Verification** (command you ran + screenshot you got).

---

👉 Do you want me to make this into a **visual diagram (one-page flow: DNS, Web, Email, FTP all mapped on TCP/IP stack)** so you can also explain visually how each service sits on the model?
