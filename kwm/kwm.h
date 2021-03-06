#ifndef KWM_H
#define KWM_H

#include "types.h"

extern "C" void NSApplicationLoad(void);
extern void CreateWorkspaceWatcher(void *Watcher);

CGEventRef CGEventCallback(CGEventTapProxy Proxy, CGEventType Type, CGEventRef Event, void *Refcon);
void * KwmWindowMonitor(void*);
void * KwmStartThreadedSystemCommand(void *Args);
void KwmExecuteThreadedSystemCommand(std::string Command);

void KwmExecuteSystemCommand(std::string Command);
void KwmExecuteConfig();
void KwmExecuteInitScript();
void KwmExecuteFile(std::string File);
void KwmReloadConfig();
void KwmClearSettings();
void KwmSubstitueVariables(std::map<std::string, std::string> &Defines, std::string &Line);
void KwmDefineVariable(std::map<std::string, std::string> &Defines, std::string Line);

void KwmInit();
void KwmQuit();
bool GetKwmFilePath();
bool CheckPrivileges();
bool CheckArguments(int argc, char **argv);
void SignalHandler(int Signum);
void Fatal(const std::string &Err);
int main(int argc, char **argv);

#endif
