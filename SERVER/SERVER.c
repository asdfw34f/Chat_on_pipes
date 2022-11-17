#define _CRT_SECURE_NO_WARNINGS
#include <windows.h> 
#include <stdio.h> 
#include <strsafe.h>
#define BUFSIZE 512

DWORD WINAPI InstanceThread(LPVOID);

int main(VOID)
{
    BOOL   fConnected = FALSE;
    DWORD  dwThreadId = 0;
    HANDLE hPipe = INVALID_HANDLE_VALUE, hThread = NULL;
    LPCSTR lpszPipename = "\\\\.\\pipe\\mynamedpipe";
    char  chBuf[BUFSIZE] = { 0 }, lpvMessage[BUFSIZE] = { 0 };
    DWORD  cbRead = NULL, cbWrite = NULL;
    // The main loop creates an instance of the named pipe and 
    // then waits for a client to connect to it. When the client 
    // connects, a thread is created to handle communications 
    // with that client, and this loop is free to wait for the
    // next client connect request. It is an infinite loop.

    for (;;) {
        printf("\nPipe Server: Main thread awaiting client connection on %s\n",
            lpszPipename);
        hPipe = CreateNamedPipeA(
            lpszPipename,             // pipe name 
            PIPE_ACCESS_DUPLEX,       // read/write access 
            PIPE_TYPE_MESSAGE |       // message type pipe 
            PIPE_READMODE_MESSAGE |   // message-read mode 
            PIPE_WAIT,                // blocking mode 
            PIPE_UNLIMITED_INSTANCES, // max. instances  
            BUFSIZE,                  // output buffer size 
            BUFSIZE,                  // input buffer size 
            0,                        // client time-out 
            NULL);                    // default security attribute 

        if (hPipe == INVALID_HANDLE_VALUE) {
            printf("CreateNamedPipe failed, GLE=%d.\n",
                GetLastError());
            return -1;
        }

        // Wait for the client to connect; if it succeeds, 
        // the function returns a nonzero value. If the function
        // returns zero, GetLastError returns ERROR_PIPE_CONNECTED. 

        fConnected = ConnectNamedPipe(hPipe, NULL) ?
            TRUE : (GetLastError() == ERROR_PIPE_CONNECTED);

        if (fConnected) {
            printf("Client connected, creating a processing thread.\n");

            // Create a thread for this client. 
            hThread = CreateThread(
                NULL,              // no security attribute 
                0,                 // default stack size 
                InstanceThread,    // thread proc
                (LPVOID)hPipe,    // thread parameter 
                0,                 // not suspended 
                &dwThreadId);      // returns thread ID 

            if (hThread == NULL) {
                printf("CreateThread failed, GLE=%d.\n",
                    GetLastError());
                return -1;
            }
            else CloseHandle(hThread);
        }
        else
            // The client could not connect, so close the pipe. 
            CloseHandle(hPipe);
    }

    return 0;
}

DWORD WINAPI InstanceThread(LPVOID lpvParam)
// This routine is a thread processing function to read from and reply to a client
// via the open pipe connection passed from the main loop. Note this allows
// the main loop to continue executing, potentially creating more threads of
// of this procedure to run concurrently, depending on the number of incoming
// client connections.
{
    char pchRequest[BUFSIZE];
    DWORD cbBytesRead = 0, cbReplyBytes = 0;
    BOOL fSuccess = FALSE;
    HANDLE hPipe = NULL;

    // Do some extra error checking since the app will keep running even if this
    // thread fails.

    if (lpvParam == NULL) {
        printf("\nERROR - Pipe Server Failure:\n");
        printf("\tInstanceThread got an unexpected NULL value in lpvParam.\n");
        printf("\tInstanceThread exitting.\n");
        return (DWORD)-1;
    }

    // Print verbose messages. In production code, this should be for debugging only.
    printf("InstanceThread created, receiving and processing messages.\n");

    // The thread's parameter is a handle to a pipe object instance. 
    hPipe = (HANDLE)lpvParam;

    // Loop until done reading
    while (1) {
        // Read client requests from the pipe. 
        // This simplistic code only allows messages
        // up to BUFSIZE characters in length.
        memset(pchRequest, 0, sizeof(pchRequest));
        cbBytesRead = 0;

        fSuccess = ReadFile(
            hPipe,        // handle to pipe 
            pchRequest,    // buffer to receive data 
            BUFSIZE * sizeof(CHAR), // size of buffer 
            &cbBytesRead, // number of bytes read 
            NULL);        // not overlapped I/O 
        if (cbBytesRead == 0)
            continue;
        else if (cbBytesRead > 0)
            printf("\t%s\n", pchRequest);
        else
            if (GetLastError() == ERROR_BROKEN_PIPE)
                printf("InstanceThread: client disconnected.\n");
            else
                printf("InstanceThread ReadFile failed, GLE=%d.\n",
                    GetLastError());
    }

    // Flush the pipe to allow the client to read the pipe's contents 
    // before disconnecting. Then disconnect the pipe, and close the 
    // handle to this pipe instance. 

    FlushFileBuffers(hPipe);
    DisconnectNamedPipe(hPipe);
    CloseHandle(hPipe);

    printf("InstanceThread exiting.\n");
    return 1;
}