REM Usage post-build.bat project_dir target_filename target_path ffmpeg_dir
REM Parameter %1 project_dir		Path to the VS project, usually $(ProjectDir)
REM Parameter %2 target_filename	Name of the file built by VS, usually $(TargetFilename) 
REM Parameter %3 target_path		Path to the target file, usually $(TargetPath)
REM Parameter %4 ffmpeg_dir			Path to the FFMPEG windows build folder, set up in $(FFMPEG_WIN)
REM @ECHO off 

REM Check parameters 
IF "%1" == "" OR "%2" == "" OR "%3" == "" (
	ECHO Missing parameter; Usage post-build.bat project_dir target_filename target_path ffmpeg_dir
)

REM Get the destination path relative to the project 
SET relativeDest=..\..\..\UnityVol\Plugins\x64\
ECHO %relativeDest%

REM Create the absolute path to the destination 
SET dest=%1%relativeDest%

REM Check that the destination diretory exits and create it if not
IF NOT EXIST %dest% (
	MKDIR %dest% 
) 

REM Check if a dll already exists and remove it  
SET destFile=%dest%%2

IF EXIST %destFile% ( 
	DEL %destFile% 
)

REM Copy the dll to the Unity plugin folder
COPY %3 %destFile% 

REM Check that the ffmpeg path is not empty
IF "%4" == "" (
	ECHO Ffmpeg path empty
	EXIT
)

REM Check that the ffmpeg folder exists 
IF NOT EXIST %4 (
	EXIT
)

REM These are the dependencies
SET dependencies=avcodec avformat avutil swresample swscale

REM Copy dependencies from ffmpeg 
FOR %%d in (%dependencies%) DO (
	CALL :CopyDependency %4 %%d %dest%
)
EXIT /B 0

REM This function is used to copy a dependency to the Unity folder
:CopyDependency
FOR /F "tokens=* USEBACKQ" %%f IN (`DIR /b /s %1bin\%2-*.dll`) DO (
	SET dependency=%%f 
	SET dep_filename=%%~nxf
)
COPY %dependency% %3%dep_filename%
EXIT /B 0 
