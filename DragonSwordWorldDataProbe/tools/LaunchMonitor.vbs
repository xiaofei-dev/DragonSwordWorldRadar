Option Explicit
Dim shell, root, command
If WScript.Arguments.Count < 1 Then WScript.Quit 2
root = WScript.Arguments(0)
Set shell = CreateObject("WScript.Shell")
command = "powershell.exe -NoProfile -ExecutionPolicy Bypass -File """ & root & "\tools\Start-Monitor.ps1"" -Root """ & root & """"
shell.Run command, 0, False
