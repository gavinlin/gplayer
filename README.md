android-15

##how to ant

~/adt-bundle-linux/sdk/tools/android list target //find -t number

~/adt-bundle-linux/sdk/tools/android update project -n gplayer -t 1 -p ~/workspace/gplayer/

if have subproject,add`--subprojects`

``` bash
~/adt-bundle-linux-x86_64/sdk/tools/android update project -n gplayer -t 2 -p ~/workspace/gplayer/ --subprojects
```

ant debug
