#define _NO_CRT_STDIO
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h> 
#include <stdio.h>
#include <conio.h>
#include <string.h>
#define BUFSIZE 512

int main(int argc, char* argv[])
{
    HANDLE hPipe, hThread;
    BOOL fSuccess = FALSE;
    DWORD  cbRead, cbToWrite, cbWritten, dwMode, dwThreadId;
    char lpvMessage[BUFSIZE] = { 0 }, chBuf[BUFSIZE] = { 0 };
    char name[10] = { 0 };
    LPCSTR lpszPipename = "\\\\.\\pipe\\mynamedpipe";

    //  Preparing name client'
    printf("ENTER UR NAME:\t");
    scanf_s("%s", name, 10);
    name[strlen(name)] = 0;

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

    // Send a message to the pipe server. 
    while (1) {

        // preparing a signed message
        memset(lpvMessage, 0, sizeof(lpvMessage));
        printf("\nEnter the message to server:\t");
        scanf_s("%s", lpvMessage, BUFSIZE);
        if (strncmp(lpvMessage, "close",
            strlen(lpvMessage)) == 0)
            break;
        char temp_buffer[BUFSIZE] = { 0 };
        strcpy_s(temp_buffer, BUFSIZE, name);
        strcat_s(temp_buffer, BUFSIZE, ":  ");
        strcat_s(temp_buffer, BUFSIZE, lpvMessage);
        memset(lpvMessage, 0, sizeof(lpvMessage));
        strcpy_s(lpvMessage, BUFSIZE, temp_buffer);

        cbToWrite = (strlen(lpvMessage) + 1) * sizeof(char);
        printf("Sending %d byte message: \"%s\"\n",
            cbToWrite, lpvMessage);

        //  Sending...
        fSuccess = WriteFile(
            hPipe,                  // pipe handle 
            lpvMessage,             // message 
            BUFSIZE * sizeof(char),   // message length 
            &cbWritten,             // bytes written 
            NULL);                  // not overlapped 
        if (!fSuccess) {
            printf("WriteFile to pipe failed. GLE=%d\n",
                GetLastError());
            return -1;
        }
        else {
            printf("\nMessage sent to server, receiving reply as follows:\n");
        }

        //////////////////////////////////////////////////////////////
        // Preparing the buffer to reading                          //
        memset(chBuf, 0, sizeof(chBuf));                            //
        cbRead = 0;                                                 //
                                                                    //
        //  Reading                                                 //
        fSuccess = ReadFile(                                        //
            hPipe,    // pipe handle                                //
            chBuf,    // buffer to receive reply                    //
            BUFSIZE * sizeof(char),  // size of buffer              //
            &cbRead,  // number of bytes read                       //
            NULL);    // not overlapped                             //
                                                                    //
        if (cbRead > 0) {                                           //
            const int countName = (int const)strlen(name);          //
            char chBufA[10] = {0};                                  //
            strncpy(chBufA, chBuf, strlen(name));                   //
            if (chBufA == name)                                     //
                continue;                                           //
                // Print a mesage                                   //
            printf("Message:\t%s\n", chBuf);                        //
                                                                    //
            //  Clean file after to read                            //
            memset(lpvMessage, 0, sizeof(lpvMessage));              //
            cbToWrite = (strlen(lpvMessage) + 1) * sizeof(char);    //
            cbWritten = NULL;                                       //
                                                                    //
            fSuccess = WriteFile(                                   //
                hPipe,        // handle to pipe                     //
                lpvMessage,    // buffer to receive data            //
                NULL, // size of buffer                             //
                &cbWritten, // number of bytes read                 //
                NULL);        // not overlapped I/O                 //
            if (!fSuccess)                                          //
                printf("InstanceThread WriteFile failed, GLE=%d.\n",//
                    GetLastError());                                //
        }                                                           //
    } ////////////////////////////////////////////////////////////////

    printf("\n<End of message, press ENTER to terminate connection and exit>\n");
    _getch();
    printf("\nGoodbye!:)\n");

    CloseHandle(hPipe);
    return 0;
}