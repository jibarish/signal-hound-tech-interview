#include <windows.h>
#include <tchar.h>
#include <strsafe.h>
#include <queue>
#include <iostream>
#include <mutex>

using namespace std;

// Thread to poll device
DWORD WINAPI PollingThreadFunction( LPVOID lpParam );

int id;                  // Device ID
int len;                 // Number of I/Q pairs returned by buffer
float *iq_array;         // Array to store a sample set from saGetIQ_32f
queue<float*> container; // Queue of pointers to iq_arrays
                         // PollingThread pushes, main thread pops
mutex mtx;               // Mutex for queue concurrency

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

    // Get samples
    int total = 0; // number of sample sets received
    while (1) {
        if (!container.empty()) {
            // Get next sample from queue
            while (!mtx.try_lock())
                Sleep(100);

            float *t = container.front();

            mtx.unlock();

            // Print
            for (int i=0; i<len*2; i=i+2) {
                cout << "(" << t[i] << ", " << t[i+1] << ")" << endl;
            }
            cout << ++total << " sample sets written" << endl;

            // Pop and destroy
            while (!mtx.try_lock())
                Sleep(100);

            container.pop();

            mtx.unlock();

            delete t;
        }
    }

    WaitForSingleObject(pollingThread, INFINITE);

    return 0;
}

DWORD WINAPI PollingThreadFunction( LPVOID lpParam )
{
    // Simulate continuous polling
    while (1) {

        iq_array = new float[len * 2];

        // Simulate getting one sample with saGetIQ_32f
        // Fill iq_array with dummy data
        for (int i=0; i<len*2; i=i+2) {
            iq_array[i] = (i*i)%100000;      // arbitrary
            iq_array[i+1] = (i*i*i)%1000000; // arbitrary
        }

        // Push
        while (!mtx.try_lock())
            Sleep(100);

        container.push(iq_array);

        mtx.unlock();
    }

    return 0;
}
