REM^
Args:
    Output Directory
    Image1
    Image2

C:/Projects/Programming/DeltaImage/out/build/x64-Debug/DeltaImage.exe ^
Diff ^
C:/Projects/Programming/DeltaImage/testImages/output ^
C:/Projects/Programming/DeltaImage/testImages/White.png ^
C:/Projects/Programming/DeltaImage/testImages/Test1.png

REM^
Args:
    Diff.json Directory
    Output Directory
    Keywords matching json file keys

C:/Projects/Programming/DeltaImage/out/build/x64-Debug/DeltaImage.exe ^
Filter ^
C:/Projects/Programming/DeltaImage/testImages/output ^
C:/Projects/Programming/DeltaImage/testImages/output/filteredData ^
MatchPercent ^
ColorDelta ^
Other