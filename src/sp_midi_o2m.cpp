// MIT License

// Copyright (c) 2016-2020 Luis Lloret

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <stdexcept>
#include <chrono>
#include <iostream>
#include <atomic>
#include "sp_midi.h"
#include "midiout.h"
#include "oscin.h"
#include "oscout.h"
#include "oscinprocessor.h"
#include "osc/OscOutboundPacketStream.h"
#include "version.h"
#include "utils.h"
#include "monitorlogger.h"

static const int MONITOR_LEVEL = 0;

static std::unique_ptr<OscInProcessor> oscInputProcessor;

using namespace std;

void listAvailablePorts()
{
    auto outputs = MidiOut::getOutputNames();
    cout << "Found " << outputs.size() << " MIDI outputs." << endl;
    for (unsigned int i = 0; i < outputs.size(); i++) {
        cout << "   (" << i << "): " << outputs[i] << endl;
    }
}


static mutex g_oscinMutex;

static void prepareOscProcessorOutputs(unique_ptr<OscInProcessor>& oscInputProcessor)
{
    // Open all MIDI devices. This is what Sonic Pi does
    vector<string> midiOutputsToOpen = MidiOut::getOutputNames();
    {
        lock_guard<mutex> lock(g_oscinMutex);
        oscInputProcessor->prepareOutputs(midiOutputsToOpen);
    }
}


void sp_midi_send(const char* c_message, unsigned int size)
{
    oscInputProcessor->ProcessMessage(c_message, size);
}

int sp_midi_init()
{
    MonitorLogger::getInstance().setLogLevel(MONITOR_LEVEL);

     oscInputProcessor = make_unique<OscInProcessor>();
    // Prepare the OSC input and MIDI outputs
    try {
        prepareOscProcessorOutputs(oscInputProcessor);
    } catch (const std::out_of_range&) {
        cout << "Error opening MIDI outputs" << endl;
        return -1;
    }
    return 0;
}

void sp_midi_deinit()
{
    oscInputProcessor.reset(nullptr);
}


ERL_NIF_TERM sp_midi_init_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    int ret = sp_midi_init();
    return enif_make_int(env, ret);
}

ERL_NIF_TERM sp_midi_deinit_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{
    sp_midi_deinit();
    return enif_make_int(env, 0);
}

ERL_NIF_TERM sp_midi_send_nif(ErlNifEnv* env, int argc, const ERL_NIF_TERM argv[])
{    
    printf("Entering sp_midi_send_nif\n");
    ErlNifBinary bin;
    int ret = enif_inspect_binary(env, argv[0], &bin);
    if (!ret)
    {
        printf("Error 1\n");
        return 0;
    }
    const char *c_message = (char *)bin.data;
    int size = bin.size;

    sp_midi_send(c_message, size);
    return enif_make_int(env, 0);
}


static ErlNifFunc nif_funcs[] = {
    {"sp_midi_init", 0, sp_midi_init_nif},
    {"sp_midi_deinit", 0, sp_midi_deinit_nif},
    {"sp_midi_send", 1, sp_midi_send_nif}
};

ERL_NIF_INIT(complex6, nif_funcs, NULL, NULL, NULL, NULL);