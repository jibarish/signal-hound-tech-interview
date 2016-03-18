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
    // Usual remote keyless entry frequencies
    const double NORTH_AMERICAN = 315.0e6;
    const double EURO_ASIAN = 433.92e6;

    // Setup device...

    saStatus stat;

    // Open device
    stat = saOpenDevice(&id);
    if(stat != saNoError) {
        // Unable to open device, ensure the device
        // is connected to the PC and try again
    }

    // Configure
    saConfigCenterSpan(id, NORTH_AMERICAN, 0); // span ignored
    saConfigLevel(id, -20.0);
    saConfigIQ(id, 8, 50.0e3);

    saInitiate(id, SA_IQ, 0);

    // Get info
    double bandwidth, sampleRate;
    saQueryStreamInfo(id, &len, &bandwidth, &sampleRate);

    // Start polling thread
    HANDLE pollingThread;
    DWORD pollingThreadID;

    pollingThread = CreateThread(
            NULL,                   // Default security attributes
            0,                      // Use default stack size
            PollingThreadFunction,  // Thread function name
            &container,             // Argument to thread function
            0,                      // Use default creation flags
            &pollingThreadID);      // Returns the thread identifier

    // Continuously retrieve sample sets from queue
    int total = 0; // number of sample sets retrieved
    while (1) {
        if (!container.empty()) {
            // Get next sample set from queue
            while (!mtx.try_lock())
                Sleep(100);

            float *t = container.front();

            mtx.unlock();

            // Print (I, Q) pairs and stats
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

    saCloseDevice(id);

    return 0;
}

DWORD WINAPI PollingThreadFunction( LPVOID lpParam )
{
    // Continuously poll device buffer
    while (1) {
        // Allocate memory for sample set
        iq_array = new float[len * 2];

        // Get sample set
        saGetIQ_32f(id, iq_array);

        // Push
        while (!mtx.try_lock())
            Sleep(100);

        container.push(iq_array);

        mtx.unlock();
    }

    return 0;
}
