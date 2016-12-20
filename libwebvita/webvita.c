/*
* Copyright (c) 2016 - TheoryWrong
*/

#include "webvita.h"

// Initialize NET Library
void init_net() {
    sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

    int ret = sceNetShowNetstat();
    if (ret == SCE_NET_ERROR_ENOTINIT) {
        net_memory = malloc(NET_MEMORY_SIZE);
        SceNetInitParam netInitParam;
        netInitParam.memory = net_memory;
        netInitParam.size = NET_MEMORY_SIZE;
        netInitParam.flags = 0;
        sceNetInit(&netInitParam);
    }

    sceNetCtlInit();
}

// Close NET Library
void close_net() {
    if (net_memory)
        free(net_memory);

    sceNetCtlTerm();
    sceNetTerm();
}

// Create server
int create_server() {
    SceNetSockaddrIn serverAddress;

    char server_name[32];
    snprintf(server_name, 32, "WEBSERVER");
    int sock = sceNetSocket(server_name, SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);

    serverAddress.sin_family = SCE_NET_AF_INET;
    serverAddress.sin_port = sceNetHtons(port);
    serverAddress.sin_addr.s_addr = sceNetHtonl(SCE_NET_INADDR_ANY);

    if (sceNetBind(sock, (SceNetSockaddr *) &serverAddress, sizeof(serverAddress)) < 0) {
        return -1;
    }
    sceNetListen(sock, 128);

    return sock;
}

// Accept client
int accept_client(int server_sock) {
    SceNetSockaddrIn clientAddress;
    unsigned int c = sizeof(clientAddress);
    return sceNetAccept(server_sock, (SceNetSockaddr *) &clientAddress, &c);
}

// Read client data
int read_client(int client_sock, void* buffer) {
    memset(buffer, 0, max_size);
    int read_size = sceNetRecv(client_sock, buffer, max_size, 0);
    if (read_size <= 0) {
        return -1;
    }
    return read_size;
}

// Split char* into an array of char (splitted by character)
char** split_strings(char* str, char* character, int* count) {
    char** strings;
    strings = malloc(1 * sizeof(char*));

    int n = 0;

    char *split = strtok(str, character);

    while(split != NULL)
    {
        strings = realloc(strings, sizeof(char*) * (n+1));
        strings[n] = strdup(split);
        split = strtok(NULL, character);
        n++;
    }

    *count = n;

    return strings;
}

// Free memory of array of char
void free_strings(char** str, int count) {
    for (int i = 0; i < count; ++i)
    {
        free(str[i]);
    }
}

// Free memory of request
void free_request(Request* req) {
    free(req->path);
    free(req->absolute_path);
    free(req);
}

// Free memory of response
void free_response(Response* res) {
    free(res->data);
}

// Get request parse the buffer into Request struct
Request* get_request(void* buffer, int len) {
    int headers_count;

    char** headers = split_strings((char*)buffer, "\r\n", &headers_count);

    int infos_count;
    char** infos = split_strings(headers[0], " ", &infos_count);

    if (infos_count < 3) {
        return NULL;
    }

    Request* req = malloc(sizeof(Request));
    
    if (!strcmp(infos[0], "GET")) {
        req->type = HTTP_GET;
    } else if (!strcmp(infos[0], "POST")) {
        req->type = HTTP_POST;
    }

    req->path = malloc(strlen(infos[1]) + 1);
    strcpy(req->path, infos[1]);

    req->absolute_path  = malloc(strlen(default_path) + strlen(req->path) + 1);
    sprintf(req->absolute_path, "%s%s", default_path, req->path);

    free_strings(headers, headers_count);

    return req;
}

// Build response transform a Response struct to a buffer
char* build_response(Response res, int* res_size) {

    char* header = "HTTP/1.1 %d OK\r\nServer: VitaWeb\r\nConnection: close\r\nContent-Type: %s\r\n\r\n";
    int data_len = strlen(header) - 4 + 3 + strlen(mime_types[res.mime].mime) + res.data_size;

    char *data = malloc(data_len);
    memset (data,'\0',data_len);
    sprintf(data, header, res.statut_code, mime_types[res.mime].mime);
    memcpy(data+strlen(data), res.data, res.data_size);

    *res_size = data_len;

    return data;
}

// Get mime type from extention
int getMime(char* ext) {
    for (int i = 0; i < N_MIME_TYPE; ++i)
    {
        if (!strcmp(ext, mime_types[i].ext)) {
            return i;
        }
    }

    return 0;
}

// Read file check if the file exist and read this
void* readFile(char* path, int* sz, int* mime) {
    char* buffer;
    SceUID fd;

    if ((fd = sceIoOpen(path, SCE_O_RDONLY, 0777)) >= 0) {
        int size = sceIoLseek32(fd, 0, SEEK_END);
        sceIoLseek32(fd, 0, SEEK_SET);
    
        buffer = malloc(size);
        if (buffer == NULL) {
            return NULL;
        }

        sceIoRead (fd, buffer, size);
        sceIoClose(fd);
        *sz = size;

        int n = 0;
        char** path_cut = split_strings(path, ".", &n);
        *mime = getMime(path_cut[n-1]);

        return (void*)buffer;
    } else {
        return NULL;
    }
}

// Add new custom response
void addCall(char* path_call, void* call_func) {
    calls = realloc(calls, sizeof(call) * (N_call+1));
    calls[N_call].path_call = path_call;
    calls[N_call].call_func = call_func;
    calls_nbr++;
}

// (Thread) Execute this went client was connected
int execute_client(SceSize args, void *argp) {
    int client_sock = *((int *) argp);
    char *buffer = malloc(max_size);

    int request_size = read_client(client_sock, (void*)buffer);

    if (request_size > 0) {
        Request* req = get_request((void*)buffer, request_size);
        free(buffer);

        if (req != NULL) {            
            int find_call = 0;
            int cb;

            for (cb = 0; cb < calls_nbr; ++cb)
            {
                if (!strcmp(calls[cb].path_call, req->path)) {
                    find_call = 1;
                    break;
                }
            }

            Response res;

            if (find_call) {
                res = ((call_call) calls[cb].call_func)(req);
            } else {
                int f_size = 0;
                int mime = 0;
                void* f_data = readFile(req->absolute_path, &f_size, &mime);

                if (f_data != NULL) {  
                    res.statut_code = HTTP_OK;
                    res.mime = mime;
                    res.data = f_data;
                    res.data_size = f_size;
                } else {
                    res.statut_code = HTTP_NOT_FOUND;
                    char* error = malloc(strlen(error404) + strlen(req->path) + 1);
                    sprintf(error, error404, req->path);
                    res.mime = getMime("html");
                    res.data =  error;
                    res.data_size = strlen(error) + 1;
                }
            }

            free_request(req);

            int res_size = 0;
            char* res_data = build_response(res, &res_size);

            sceNetSend(client_sock, res_data, res_size, 0);

            free_response(&res);
            free(res_data);
        }
    } else { free(buffer); }
    
    sceNetSocketClose(client_sock);
    sceKernelExitDeleteThread(0);
    return 0;
} 

// Wait client
int wait_client() {
    int server = create_server();

    int client_sock;

    while (launched) {
        while ((client_sock = accept_client(server)) && launched) {
            SceUID thid = sceKernelCreateThread("web_client_thread", execute_client, 0x40, 0x5000, 0, 0, NULL);
            if (thid >= 0)
                sceKernelStartThread(thid, sizeof(void*), (void *)&client_sock);
        }
    }

    sceNetSocketClose(server);
    close_net();

    launched = -1;
    sceKernelExitDeleteThread(0);
    return 0;
}

// Set a port (default: 8080)
void setPort(int value) {
    port = value;
}

// Set max data size (default: 5000)
void setMaxSize(int value) {
    max_size = value;
}

// Set max client (default: 50)
void setMaxClient(int value) {
    max_client = value;
}

// set default page (default: ux0:data)
void setDefaultPath(char* value) {
    default_path = value;
}

// set 404 error page
void set404error(char* value) {
    error404 = value;
}

// set 500 error page
void set500error(char* value) {
    error500 = value;
}

// Init WebServer
void initWebServer() {
    init_net();
    launched = 1;
    calls = malloc(1 * sizeof(call));
}

// launch the server
void launchWebServer() {
    SceUID thid = sceKernelCreateThread("web_wait_thread", wait_client, 0x40, 0x5000, 0, 0, NULL);
    if (thid >= 0)
        sceKernelStartThread(thid, 0, NULL);
}

// stop the server
void stopWebServer() {
    launched = 0;
}

// check if the server was stopped
int isServerStop() {
    if (launched == -1) {
        return 1;
    }

    return 0;
}
