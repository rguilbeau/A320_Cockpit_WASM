@echo off

SET community_folder=C:\Users\Romai\AppData\Local\Packages\Microsoft.FlightSimulator_8wekyb3d8bbwe\LocalCache\Packages\Community
SET post_build_path=%~dp0



ECHO Clean module in community folder
RMDIR /S /Q %community_folder%\A320_Cockpit_WASM

ECHO Create module in community folder
MKDIR %community_folder%\A320_Cockpit_WASM
MKDIR %community_folder%\A320_Cockpit_WASM\modules

ECHO Copy A320_Cockpit_WASM.wasm 
COPY %1\A320_Cockpit_WASM.wasm %community_folder%\A320_Cockpit_WASM\modules\A320_Cockpit_WASM.wasm

ECHO Copy manifest.json
COPY %post_build_path:~0,-1%\manifest.json %community_folder%\A320_Cockpit_WASM\manifest.json

ECHO Copy layout.json
COPY %post_build_path:~0,-1%\layout.json %community_folder%\A320_Cockpit_WASM\layout.json

ECHO Update layout.json
%post_build_path:~0,-1%\MSFSLayoutGenerator.exe %community_folder%\A320_Cockpit_WASM\layout.json