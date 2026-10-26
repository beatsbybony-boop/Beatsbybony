#pragma once
#include <juce_core/juce_core.h>

namespace bony
{
/** Tiny breadcrumb log: %APPDATA%\BeatsByBony\splitter-log.txt. If the splitter ever crashes, the last line says where. */
inline juce::File logFile()
{
    return juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory).getChildFile ("BeatsByBony").getChildFile ("splitter-log.txt");
}
inline void logLine (const juce::String& s)
{
    static juce::CriticalSection cs;
    const juce::ScopedLock sl (cs);
    auto f = logFile();
    f.getParentDirectory().createDirectory();
    if (f.getSize() > 512 * 1024) f.deleteFile();
    f.appendText (juce::Time::getCurrentTime().formatted ("%Y-%m-%d %H:%M:%S  ") + s + "\n");
}
}
