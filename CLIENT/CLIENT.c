#define _NO_CRT_STDIO
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <string.h>
#define BUFSIZE 512
int main(int argc, char* argv[])
{
    printf("ENTER UR NAME:\t");
    char name[10] = { 0 };
    scanf_s("%s", name, 10);
    name[strlen(name)] = 0;
    HANDLE hPipe;
    char lpvMessage[BUFSIZE] = "Default message from client.";
    CHAR  chBuf[BUFSIZE];
    BOOL   fSuccess = FALSE;
    DWORD  cbRead, cbToWrite, cbWritten, dwMode;
    LPCSTR lpszPipename = "\\\\.\\pipe\\mynamedpipe";

    // Try to open a named pipe; wait for it, if necessary. 
    while (1) {
        hPipe = CreateFileA(
            lpszPipename,   // pipe name 
            GENERIC_READ |  // read and write access 
            GENERIC_WRITE,
            0,              // no sharing 
            NULL,           // default security attributes
            OPEN_EXISTING,  // opens existing pipe 
            0,              // default attributes 
            NULL);          // no template file 

        // Break if the pipe handle is valid. 
        if (hPipe != INVALID_HANDLE_VALUE)
            break;

        // Exit if an error other than ERROR_PIPE_BUSY occurs. 
        if (GetLastError() != ERROR_PIPE_BUSY) {
            printf("Could not open pipe. GLE=%d\n",
                GetLastError());
            return -1;
        }

        // All pipe instances are busy, so wait for 20 seconds. 
        if (!WaitNamedPipeA(lpszPipename, 20000)) {
            printf("Could not open pipe: 20 second wait timed out.");
            return -1;
        }
    }

    // The pipe connected; change to message-read mode. 
    dwMode = PIPE_READMODE_MESSAGE;
    fSuccess = SetNamedPipeHandleState(
        hPipe,    // pipe handle 
        &dwMode,  // new pipe mode 
        NULL,     // don't set maximum bytes 
        NULL);    // don't set maximum time 
    if (!fSuccess) {
        printf("SetNamedPipeHandleState failed. GLE=%d\n",
            GetLastError());
        return -1;
    }

    int run = 1;
    // Send a message to the pipe server. 
    while (run) {
        char temp_buffer[512] = { 0 };
        printf("\nEnter the message to server:\t"); 
        scanf_s("%s", lpvMessage, BUFSIZE);
        strcpy_s(temp_buffer, BUFSIZE, name);
        strcat_s(temp_buffer, BUFSIZE, ":  ");
        strcat_s(temp_buffer, BUFSIZE, lpvMessage);
        //sprintf_s(temp_buffer, "%s:    %s", name, lpvMessage);
        if (strncmp(lpvMessage,
            "close", strlen(lpvMessage)) == 0) {

            printf("\nGoodbye!:)\n");
            goto exit;
        }
        memset(lpvMessage, 0, sizeof(lpvMessage));
        strcpy_s(lpvMessage, BUFSIZE, temp_buffer);
        cbToWrite = (strlen(lpvMessage) + 1) * sizeof(char);
        printf("Sending %d byte message: \"%s\"\n",
            cbToWrite, lpvMessage);
        fSuccess = WriteFile(
            hPipe,                  // pipe handle 
            lpvMessage,             // message 
            cbToWrite,              // message length 
            &cbWritten,             // bytes written 
            NULL);                  // not overlapped 
        if (!fSuccess) {
            printf("WriteFile to pipe failed. GLE=%d\n",
                GetLastError());
            return -1;
        }
        printf("\nMessage sent to server, receiving reply as follows:\n");
    }

    do {
        // Read from the pipe. 
        fSuccess = ReadFile(
            hPipe,    // pipe handle 
            chBuf,    // buffer to receive reply 
            BUFSIZE * sizeof(char),  // size of buffer 
            &cbRead,  // number of bytes read 
            NULL);    // not overlapped 

        if (!fSuccess && GetLastError() != ERROR_MORE_DATA)
            break;

        printf("message: %s\n", chBuf);
    } while (!fSuccess);  // repeat loop if ERROR_MORE_DATA 

    if (!fSuccess) {
        printf("ReadFile from pipe failed. GLE=%d\n",
            GetLastError());
        return -1;
    }

exit:
    printf("\n<End of message, press ENTER to terminate connection and exit>");
    _getch();

    CloseHandle(hPipe);
    return 0;
}