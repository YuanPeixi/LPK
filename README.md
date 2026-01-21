# Loop Killer
---
## Introduction
This is used to ban some executable files

## Loop Killer
It will loop and check process name in the snapshot
It's config is setted in config.txt
```text
[checkInterval in ms]
[Match Ambiguously in Boolean]
[Use API or CLI in Boolean]

Keyword1
...
KeywordN
```


## Keeper
Keeper.exe will keep the process running by restarting it if it quit

Keeper.exe(Keep the process running) all use same format config file in the current directory
```text
[checkInterval in ms]

processname1.exe
...
PathToProcessN
```
Although, both relative and absolute path are acceptable.

Absolute path can deliver real **current directory** to target exe instead of the position keeper.exe is

## Notice
This is actually a renew version

This Only support ASCII use **LPK 2.0** for Unicode support

Boolean means any not 0 or FALSE will treat as TRUE