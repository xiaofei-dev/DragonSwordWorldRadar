Option Explicit

Dim fso, shell, scriptDir, modDir, runtimeDir, logDir, logPath
Dim requestPath, stopPath, lastStamp, currentStamp, command, rc
Set fso = CreateObject("Scripting.FileSystemObject")
Set shell = CreateObject("WScript.Shell")
shell.CurrentDirectory = shell.ExpandEnvironmentStrings("%TEMP%")
scriptDir = fso.GetParentFolderName(WScript.ScriptFullName)
modDir = fso.GetParentFolderName(scriptDir)
If WScript.Arguments.Count > 0 Then modDir = fso.GetAbsolutePathName(WScript.Arguments(0))
runtimeDir = fso.BuildPath(modDir, "runtime")
logDir = fso.BuildPath(runtimeDir, "logs")
If Not fso.FolderExists(runtimeDir) Then fso.CreateFolder runtimeDir
If Not fso.FolderExists(logDir) Then fso.CreateFolder logDir
logPath = fso.BuildPath(logDir, "DragonSwordWorldRadar.Watcher.log")
requestPath = fso.BuildPath(runtimeDir, "launch.request")
stopPath = fso.BuildPath(runtimeDir, "watcher.stop")
lastStamp = ""

Sub LogLine(message)
    On Error Resume Next
    Dim stream
    Set stream = fso.OpenTextFile(logPath, 8, True, -1)
    stream.WriteLine "[" & Replace(CStr(Now), "/", "-") & "] " & message
    stream.Close
    On Error GoTo 0
End Sub

Function Quote(value)
    Quote = Chr(34) & Replace(value, Chr(34), Chr(34) & Chr(34)) & Chr(34)
End Function

LogLine "WATCHER_START version=0.4.0-dev7-stable6 host=wscript pidless=true"
Do
    If fso.FileExists(stopPath) Then
        On Error Resume Next
        fso.DeleteFile stopPath, True
        On Error GoTo 0
        LogLine "WATCHER_STOP_REQUESTED"
        Exit Do
    End If

    If fso.FileExists(requestPath) Then
        On Error Resume Next
        Dim input
        Set input = fso.OpenTextFile(requestPath, 1, False, 0)
        currentStamp = Trim(input.ReadAll)
        input.Close
        If Err.Number <> 0 Then
            Err.Clear
            currentStamp = ""
        End If
        On Error GoTo 0

        If Len(currentStamp) > 0 And currentStamp <> lastStamp Then
            lastStamp = currentStamp
            command = Quote(shell.ExpandEnvironmentStrings("%SystemRoot%\System32\WindowsPowerShell\v1.0\powershell.exe")) & _
                " -NoLogo -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File " & _
                Quote(fso.BuildPath(scriptDir, "DragonSwordWorldRadar.Watcher.ps1")) & _
                " -ModDir " & Quote(modDir) & " -RequestStamp " & Quote(currentStamp)
            LogLine "OVERLAY_PROCESS_START stamp=" & currentStamp
            rc = shell.Run(command, 0, True)
            LogLine "OVERLAY_PROCESS_EXIT stamp=" & currentStamp & " rc=" & CStr(rc)
        End If
    End If
    WScript.Sleep 250
Loop
LogLine "WATCHER_EXIT"
