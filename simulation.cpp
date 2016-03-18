#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <queue>
#include <iostream>

using namespace std;

// Thread to poll device
DWORD WINAPI PollingThreadFunction( LPVOID lpParam );

int id;                  // Device ID
int len;                 // Number of I/Q pairs returned by buffer
float *iq_array;         // Array to store returns of saGetIQ_32f
queue<float*> container; // Queue of pointers to iq_arrays
                         // PollingThread pushes, main thread pops

int _tmain()
{
    // Setup device...

    id = 123; // value returned by saOpenDevice
    len = 30; // value returned by saQueryStreamInfo

    // Start polling thread
    HANDLE pollingThread;
    DWORD pollingThreadID;

    pollingThread = CreateThread(
            NULL,                   // default security attributes
            0,                      // use default stack size
            PollingThreadFunction,  // thread function name
            &container,             // argument to thread function
            0,                      // use default creation flags
            &pollingThreadID);      // returns the thread identifier

    WaitForSingleObject(pollingThread, INFINITE);

    // Get next sample set from polling thread
    float *t = container.front();
    // Print
    for (int i=0; i<len*2; i=i+2) {
        cout << "(" << t[i] << ", " << t[i+1] << ")" << endl;
    }
    // Destroy
    container.pop();
    delete [] t;

    return 0;
}

DWORD WINAPI PollingThreadFunction( LPVOID lpParam )
{
    iq_array = new float[len * 2];

    // Simulate saGetIQ_32f
    // Fill iq_array
    for (int i=0; i<len*2; i=i+2) {
        iq_array[i] = i*6.5; // arbitrary
        iq_array[i+1] = i*13.5; // arbitrary
    }

    container.push(iq_array);

    return 0;
}
