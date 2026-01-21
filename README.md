# Loop Killer
---
## Introduction
This is used to ban window with keywords

## Kill Window
It will enumerate windows and check their title
It will terminate process or close the windows with keywords
```text
[checkInterval in ms]
[Match Ambiguously in Boolean]
[Kill Process (FALSE for CloseWindow) in Boolean]

Keyword1
...
KeywordN
```


## KeeperCLI
KeeperCLI.exe will keep the process running by restarting it if it quit

KeeperCLI.exe gets parameters from command line.

KeeperCLI.exe <checkInterval in ms> <Path to target exe>
