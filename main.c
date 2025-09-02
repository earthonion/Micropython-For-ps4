#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <orbis/Net.h>
#include <orbis/Sysmodule.h>
#include <orbis/libkernel.h>

#include "micropython_embed/port/micropython_embed.h"

#define NET_POOLSIZE (4 * 1024)
#define LISTEN_PORT 9025
#define BUFFER_SIZE 4096
#define HEAP_SIZE (64 * 1024)

static int libnetMemId = 0;
static int server_socket = -1;
static int client_socket = -1;
static char mp_heap[HEAP_SIZE];

// Output function for MicroPython
void mp_embed_write(const char *str, size_t len) {
    if (client_socket >= 0) {
        send(client_socket, str, len, 0);
    }
    // Also print to console
    printf("%.*s", (int)len, str);
}

// Initialize network
int net_init() {
    int ret;
    
    printf("[MicroPython PS4] Loading network modules...\n");
    
    if (sceSysmoduleLoadModuleInternal(ORBIS_SYSMODULE_INTERNAL_NET) < 0)
        return -1;
    
    printf("[MicroPython PS4] Initializing network...\n");
    
    ret = sceNetInit();
    if (ret < 0) {
        printf("[MicroPython PS4] sceNetInit() error: 0x%08X\n", ret);
        return -1;
    }
    
    ret = sceNetPoolCreate("netPool", NET_POOLSIZE, 0);
    if (ret < 0) {
        printf("[MicroPython PS4] sceNetPoolCreate() error: 0x%08X\n", ret);
        return -1;
    }
    libnetMemId = ret;
    
    printf("[MicroPython PS4] Network initialized successfully\n");
    return 0;
}

// Setup network server
int setup_server() {
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    
    printf("[MicroPython PS4] Starting network server on port %d...\n", LISTEN_PORT);
    
    // Create server socket
    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket < 0) {
        printf("[MicroPython PS4] Failed to create socket\n");
        return -1;
    }
    
    // Allow socket reuse
    int optval = 1;
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    
    // Setup server address
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(LISTEN_PORT);
    
    // Bind socket
    if (bind(server_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        printf("[MicroPython PS4] Failed to bind socket\n");
        close(server_socket);
        return -1;
    }
    
    // Listen for connections
    if (listen(server_socket, 1) < 0) {
        printf("[MicroPython PS4] Failed to listen on socket\n");
        close(server_socket);
        return -1;
    }
    
    printf("[MicroPython PS4] Server listening on port %d. Use 'nc <PS4_IP> %d' to connect.\n", 
           LISTEN_PORT, LISTEN_PORT);
    
    // Accept connection
    client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_len);
    if (client_socket < 0) {
        printf("[MicroPython PS4] Failed to accept connection\n");
        return -1;
    }
    
    printf("[MicroPython PS4] Client connected from %s:%d\n",
           inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    
    return 0;
}

// Run MicroPython REPL
void run_micropython_repl() {
    char buffer[BUFFER_SIZE];
    
    // Send welcome message
    const char* welcome = "MicroPython v1.24.0 on PS4/Orbis\nType 'help()' for more information.\n";
    mp_embed_write(welcome, strlen(welcome));
    
    // Initialize MicroPython
    int stack_top;
    mp_embed_init(&mp_heap[0], sizeof(mp_heap), &stack_top);
    
    // Send prompt
    const char* prompt = ">>> ";
    mp_embed_write(prompt, strlen(prompt));
    
    while (1) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t bytes = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
        
        if (bytes <= 0) {
            printf("[MicroPython PS4] Client disconnected\n");
            break;
        }
        
        // Remove newline
        char* newline = strchr(buffer, '\n');
        if (newline) *newline = '\0';
        newline = strchr(buffer, '\r');
        if (newline) *newline = '\0';
        
        // Check for help command
        if (strcmp(buffer, "help") == 0 || strcmp(buffer, "help()") == 0) {
            const char* help_text = 
                "\n=== MicroPython PS4 Help ===\n"
                "MicroPython v1.24.0 running on PS4/Orbis\n\n"
                "Available modules:\n"
                "  gc          - Garbage collection control\n"
                "  math        - Mathematical functions (if enabled)\n"
                "  sys         - System-specific parameters (if enabled)\n"
                "\n"
                "Basic commands:\n"
                "  help()      - Show this help message\n"
                "  exit()      - Disconnect and exit REPL\n"
                "  quit()      - Same as exit()\n"
                "  print()     - Print objects\n"
                "  dir()       - List available names\n"
                "  dir(obj)    - List attributes of object\n"
                "\n"
                "GC commands:\n"
                "  import gc   - Import garbage collector module\n"
                "  gc.collect()- Run garbage collection\n"
                "  gc.mem_free() - Get free memory\n"
                "  gc.mem_alloc()- Get allocated memory\n"
                "\n"
                "Examples:\n"
                "  >>> 2 ** 10\n"
                "  1024\n"
                "  >>> def hello(name):\n"
                "  ...     return f'Hello {name}!'\n"
                "  >>> hello('PS4')\n"
                "  'Hello PS4!'\n"
                "  >>> [x*2 for x in range(5)]\n"
                "  [0, 2, 4, 6, 8]\n"
                "\n"
                "Network REPL:\n"
                "  - Connected via port 9025\n"
                "  - Single line execution mode\n"
                "  - Use Ctrl+C to interrupt long operations\n"
                "\n"
                "Limitations:\n"
                "  - 64KB heap (increase HEAP_SIZE to expand)\n"
                "  - No file system access yet\n"
                "  - No PS4-specific APIs yet\n"
                "\n";
            mp_embed_write(help_text, strlen(help_text));
            mp_embed_write(prompt, strlen(prompt));
            continue;
        }
        
        // Check for exit commands
        if (strcmp(buffer, "exit") == 0 || strcmp(buffer, "exit()") == 0 ||
            strcmp(buffer, "quit") == 0 || strcmp(buffer, "quit()") == 0) {
            const char* goodbye = "Goodbye!\n";
            mp_embed_write(goodbye, strlen(goodbye));
            break;
        }
        
        // Execute MicroPython code
        if (strlen(buffer) > 0) {
            mp_embed_exec_str(buffer);
        }
        
        // Send prompt again
        mp_embed_write(prompt, strlen(prompt));
    }
    
    // Cleanup MicroPython
    mp_embed_deinit();
}

int main(void) {
    // Initialize kernel
    sceKernelLoadStartModule("libSceNet.sprx", 0, NULL, 0, 0, 0);
    
    printf("[MicroPython PS4] Starting MicroPython PS4 REPL...\n");
    
    // Initialize network
    if (net_init() < 0) {
        printf("[MicroPython PS4] Failed to initialize network\n");
        for(;;) {} // Keep running
    }
    
    // Setup server and wait for connection
    if (setup_server() < 0) {
        printf("[MicroPython PS4] Failed to setup server\n");
        for(;;) {} // Keep running
    }
    
    // Run MicroPython REPL
    printf("[MicroPython PS4] Running MicroPython REPL...\n");
    run_micropython_repl();
    
    // Cleanup
    if (client_socket >= 0) {
        close(client_socket);
    }
    if (server_socket >= 0) {
        close(server_socket);
    }
    
    printf("[MicroPython PS4] Server terminated\n");
    
    // Keep the process running
    for (;;) {}
    
    return 0;
}