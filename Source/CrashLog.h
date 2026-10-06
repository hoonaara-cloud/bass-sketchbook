#pragma once

//==============================================================================
// Startup breadcrumbs for crash diagnosis.
// Appends to %TEMP%/GrooveSketchbook_crash.log, flushed on every line so a
// hard crash still leaves the trail behind. Only a handful of lines are
// written at startup; nothing in the steady-state audio path.
//==============================================================================
#include <juce_core/juce_core.h>

#include <atomic>

struct CrashLog
{
    static void write (const juce::String& line)
    {
        const juce::ScopedLock sl (getLock());

        if (auto* os = getStream())
        {
            if (os->openedOk())
            {
                *os << juce::Time::getCurrentTime().toString (true, true) << "  " << line << "\n";
                os->flush();
            }
        }
    }

private:
    static juce::CriticalSection& getLock()
    {
        static juce::CriticalSection lock;
        return lock;
    }

    static juce::FileOutputStream* getStream()
    {
        static juce::FileOutputStream* stream = nullptr;
        if (stream == nullptr)
        {
            auto f = juce::File::getSpecialLocation (juce::File::tempDirectory)
                         .getChildFile ("GrooveSketchbook_crash.log");
            f.deleteFile();
            stream = new juce::FileOutputStream (f); // process-lifetime, flushed per write
        }
        return stream;
    }
};
