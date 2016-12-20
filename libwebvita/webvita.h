/*
* Copyright (c) 2016 - TheoryWrong
*/

#ifndef WEB_H
#define WEB_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <psp2/sysmodule.h>
#include <psp2/kernel/threadmgr.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/io/fcntl.h>
#include <psp2/io/dirent.h>

#define NET_MEMORY_SIZE 1048576

enum HTTPMethod
{
    HTTP_GET,
    HTTP_POST
};

enum HTTPStatutCode
{
    HTTP_OK = 200,
    HTTP_NOT_FOUND = 404,
    HTTP_INTERNAL_ERROR = 500
};

typedef struct {
	char* ext;
	char* mime;
} MimeType;

static MimeType mime_types[] = {
	{"bin", "application/octet-stream"},
	{"css", "text/css"},
	{"js", "application/javascript"},
	{"html", "text/html"},
	{"xml", "application/xml"},
	{"json", "application/json"},
	{"png", "image/png"},
	{"jpg", "image/jpeg"},
	{"jpeg", "image/jpeg"},
	{"gif", "image/gif"},
	{"pdf", "application/pdf"},
	{"txt", "text/plain"},
	{"mp3", "audio/mpeg"},
	{"mp4", "video/mp4"}
};

#define N_MIME_TYPE (sizeof(mime_types) / sizeof(MimeType))

typedef struct {
	char* path_call;
	void* call_func;
} call;

#define N_call (sizeof(calls) / sizeof(call))

typedef struct
{
    int type;
    char* path;
    char* absolute_path;
} Request;

typedef struct
{
	int statut_code;
	int mime;
	void* data;
	int data_size;
} Response;

static int port = 8080;
static int max_client = 50;
static int max_size = 5000;
static int launched = -1;
static char* default_path = "ux0:data";
static char* error404 = "<!DOCTYPE HTML><html><head><title>404 Not Found</title></head><body><h1>Not Found</h1><p>The requested URL %s was not found on this server.</p></body></html>";
static char* error500 = "<!DOCTYPE HTML><html><head><title>500 Internal Server Error</title></head><body><h1>Internal Server Error</h1><p>Please retry later.</p></body></html>";
static void* net_memory;
static int calls_nbr = 0;
static call* calls;

typedef Response (*call_call)(Request* req);

void initWebServer();
void setPort(int value);
void setMaxSize(int value);
void setMaxClient(int value);
void setDefaultPath(char* value);
void set404error(char* value);
void set500error(char* value);
void launchWebServer();
void stopWebServer();
int isServerStop();
void addCall(char* path_call, void* call_func);
int getMime(char* ext);

#endif // WEB_H
