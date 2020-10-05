/*-----------------------------------------------------------
 * client.cpp: Sender process that utilizes command args
 *   to determine which IPC method to use to speak to server
 * (uses correct code, not necesarrily the same as P1)
 * Dean Orenstein
 * UIN: 127008117
 * CSCE 313-909
 * Date: 10/04/2020
 *----------------------------------------------------------*/

#include "common.h"
#include <sys/wait.h>
#include "FIFOreqchannel.h"
#include "MQreqchannel.h"
#include "SHMreqchannel.h"
#include <vector>
using namespace std;


timeval start_time;
timeval end_time;

int main(int argc, char *argv[]) {
    int c;
    int buffercapacity = MAX_MESSAGE;
    int p = 0, ecg = 1;
    double t = -1.0;
    bool isnewchan = false;
    bool isfiletransfer = false;
    string filename;
    string ipcmethod = "f";
    int nchannels = 1;

    while ((c = getopt (argc, argv, "p:t:e:m:f:c:i:")) != -1) {
        switch (c) {
            case 'p':
                p = atoi (optarg);
                break;
            case 't':
                t = atof (optarg);
                break;
            case 'e':
                ecg = atoi (optarg);
                break;
            case 'm':
                buffercapacity = atoi (optarg);
                break;
            case 'c':
                isnewchan = true;
                nchannels = atoi (optarg);
                break;
            case 'f':
                isfiletransfer = true;
                filename = optarg;
                break;
            case 'i':
                ipcmethod = optarg;
        }
    }
    
    // Child process
    if (fork() == 0) {
		char* args [] = {"./server", "-m", (char *) to_string(buffercapacity).c_str(), "-i", (char*) ipcmethod.c_str(), NULL};
        if (execvp (args [0], args) < 0) {
            perror ("exec filed");
            exit (0);
        }
    }

    RequestChannel* control_chan = NULL;
    if (ipcmethod == "f")
        control_chan = new FIFORequestChannel ("control", RequestChannel::CLIENT_SIDE);
    else if (ipcmethod == "q")
        control_chan = new MQRequestChannel ("control", RequestChannel::CLIENT_SIDE, buffercapacity);
    else if (ipcmethod == "s")
        control_chan = new SHMRequestChannel ("control", RequestChannel::CLIENT_SIDE, buffercapacity);
    
    // Initialize a vector of RequestChannels to the single request channel, but replace this and possibly push more channels
    vector<RequestChannel*> channels;
    channels.push_back(control_chan);
    RequestChannel* chan = control_chan;
    if (isnewchan) {
        channels.pop_back();
        MESSAGE_TYPE m = NEWCHANNEL_MSG;
        cout << "Using new channel(s)" << endl;

        for (int i = 0; i < nchannels; i++) {
            control_chan -> cwrite (&m, sizeof (m));
            char newchanname [100];
            control_chan -> cread (newchanname, sizeof (newchanname));
            if (ipcmethod == "f")
                chan = new FIFORequestChannel (newchanname, RequestChannel::CLIENT_SIDE);
            else if (ipcmethod == "q")
                chan = new MQRequestChannel (newchanname, RequestChannel::CLIENT_SIDE, buffercapacity);
            else if (ipcmethod == "s")
                chan = new SHMRequestChannel (newchanname, RequestChannel::CLIENT_SIDE, buffercapacity);
            channels.push_back(chan);
        }
        //cout << "New channel by the name " << newchanname << " is created" << endl;
        //cout << "All further communication will happen through it instead of the main channel" << endl;
    }

    double timeTaken;

    // If requesting data msgs
    if (!isfiletransfer) {
        if (t >= 0) {    // 1 data point
            datamsg d (p, t, ecg);

            gettimeofday(&start_time, NULL);
            chan -> cwrite (&d, sizeof (d));
            double ecgvalue;
            chan -> cread (&ecgvalue, sizeof (double));
            gettimeofday(&end_time, NULL);

            timeTaken = (end_time.tv_usec - start_time.tv_usec) * .0000001;
            cout << "Ecg " << ecg << " value for patient "<< p << " at time " << t << " is: " << ecgvalue << endl;
            cout << "Took " << timeTaken << " seconds to complete" << endl;
        }
        // bulk (i.e., 1K) data requests (for multiple channels too)
        else {
            double ts = 0;  
            datamsg d (p, ts, ecg);
            double ecgvalue;

            gettimeofday(&start_time, NULL);
            for (int i = 0; i < nchannels; i++) {
                cout << "DEBUG: on channel " << i << endl;
                for (int j = 0; j < 1000; j++) {
                    channels.at(i) -> cwrite (&d, sizeof (d));
                    channels.at(i) -> cread (&ecgvalue, sizeof (double));
                    d.seconds += 0.004;  // Increment the timestamp by 4ms
                    //cout << ecgvalue << endl;
                }
                d.seconds = 0;

                // Clean up
                MESSAGE_TYPE q = QUIT_MSG;
                channels.at(i) -> cwrite (&q, sizeof (MESSAGE_TYPE));
            }
            gettimeofday(&end_time, NULL);

            timeTaken = (end_time.tv_usec - start_time.tv_usec) * .0000001;
            cout << "Took " << timeTaken << " seconds to complete" << endl;
        }
    }
    
    // Requesting a file
    else if (isfiletransfer) {
        filemsg f (0,0);  // special first message to get file size
        int to_alloc = sizeof (filemsg) + filename.size() + 1; // extra byte for NULL
        char* buf = new char [to_alloc];
        memcpy (buf, &f, sizeof(filemsg));
        strcpy (buf + sizeof (filemsg), filename.c_str());

        gettimeofday(&start_time, NULL);

        // Get the file size
        chan -> cwrite (buf, to_alloc);
        __int64_t filesize;
        chan -> cread (&filesize, sizeof (__int64_t));
        cout << "File size: " << filesize << endl;

        //int transfers = ceil (1.0 * filesize / MAX_MESSAGE);
        string outfilepath = string ("received/") + filename;
        FILE* outfile = fopen (outfilepath.c_str (), "wb");
        char* recv_buffer = new char [MAX_MESSAGE];

        // Each channel is responsible for its own portion of the file contents
        __int64_t sizePerChan = ceil (1.0 * filesize / nchannels);
        for (int i = 0; i < nchannels; i++) {
            cout << "DEBUG: Channel " << i << " is responible for " << i * sizePerChan << " - " << i * sizePerChan + sizePerChan << endl;
            //sleep(1);1111111
            filemsg* fm = (filemsg*) buf;
            __int64_t rem = sizePerChan;
            fm -> offset = i * sizePerChan;

            // Cut off any excess if on the last iteration
            if (fm -> offset + rem > filesize) {
                rem = fm -> offset + rem - filesize;
                cout << "last iter rem is " << rem << endl;
            }

            while (rem > 0) {
                //cout << "rem is " << rem << endl;
                // If there is overflowing bytes passed the file size, subtract the extra amount from current remainding for length
                // if (rem -= fm -> length <= 0) {
                //     fm -> length = (int) rem - (fm -> length - (int) rem);
                //     cout << "ok yeah we good, its " << fm -> length << endl;
                // }
                // // Otherwise, length is the min of rem and MAX_MESSAGE
                // else {
                fm -> length = (int) min (rem, (__int64_t) MAX_MESSAGE);
                //}
                
                //cout << "Gonna write " << fm -> length << " and " << to_alloc << endl;
                channels.at(i) -> cwrite (buf, to_alloc);
                channels.at(i) -> cread (recv_buffer, MAX_MESSAGE);
                fwrite (recv_buffer, 1, fm -> length, outfile);
                rem -= fm -> length;
                fm -> offset += fm -> length;
                //cout << fm -> offset << endl;
            }
            char* recv_buffer = new char [MAX_MESSAGE];

            // Clean up
            MESSAGE_TYPE q = QUIT_MSG;
            channels.at(i) -> cwrite (&q, sizeof (MESSAGE_TYPE));
        }
        
        gettimeofday(&end_time, NULL);

        fclose (outfile);
        delete recv_buffer;
        delete buf;

        timeTaken = (end_time.tv_usec - start_time.tv_usec) * .0000001;
        cout << "File transfer completed" << endl;
        cout << "Took " << timeTaken << " seconds to complete" << endl;
    }

    // Clean up
    MESSAGE_TYPE q = QUIT_MSG;
    control_chan -> cwrite (&q, sizeof (MESSAGE_TYPE));


    // if (chan != control_chan) { // this means that the user requested a new channel, so the control_channel must be destroyed as well 
    //     control_chan -> cwrite (&q, sizeof (MESSAGE_TYPE));
    // }
	// wait for the child process running server
    // this will allow the server to properly do clean up
    // if wait is not used, the server may sometimes crash
	wait (0); 
}
