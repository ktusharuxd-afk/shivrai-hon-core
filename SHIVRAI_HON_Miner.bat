@echo off
title SHIVRAI HON Miner
color 0A
cls
echo.
echo  =====================================================
echo   SHIVRAI HON (HON) - Mining Software v1.0
echo   Decentralized Digital Currency
echo  =====================================================
echo.

set DATADIR=%APPDATA%\SHIVRAI-HON
set RPCUSER=shivraiuser
set RPCPASS=honminer123
set RPCPORT=8332
set BINDIR=%~dp0

if not exist "%DATADIR%" mkdir "%DATADIR%"

if not exist "%DATADIR%\shivrai-hon.conf" (
    echo  Creating config...
    (
        echo rpcuser=shivraiuser
        echo rpcpassword=honminer123
        echo server=1
        echo daemon=1
        echo txindex=1
        echo rpcbind=127.0.0.1
        echo rpcallowip=127.0.0.1
        echo rpcport=8332
    ) > "%DATADIR%\shivrai-hon.conf"
    echo  Config created!
)

echo  [1/4] Starting Node...
start /min "" "%BINDIR%bitcoind.exe" -datadir="%DATADIR%" -conf="%DATADIR%\shivrai-hon.conf"
echo  Waiting 30 seconds...
timeout /t 30 /nobreak > nul

echo.
echo  [2/4] Setting up Wallet...
"%BINDIR%bitcoin-cli.exe" -datadir="%DATADIR%" -rpcport=%RPCPORT% -rpcuser=%RPCUSER% -rpcpassword=%RPCPASS% createwallet "shivrai-mainnet" 2>nul
"%BINDIR%bitcoin-cli.exe" -datadir="%DATADIR%" -rpcport=%RPCPORT% -rpcuser=%RPCUSER% -rpcpassword=%RPCPASS% loadwallet "shivrai-mainnet" 2>nul
echo  Wallet ready!

echo.
echo  [3/4] Getting Address...
"%BINDIR%bitcoin-cli.exe" -datadir="%DATADIR%" -rpcport=%RPCPORT% -rpcuser=%RPCUSER% -rpcpassword=%RPCPASS% getnewaddress > "%TEMP%\hon_addr.txt"
set /p ADDR=<"%TEMP%\hon_addr.txt"
echo  Address: %ADDR%

echo.
echo  [4/4] Mining Started!
echo  =====================================================
echo   Each block = 200 HON
echo   Press Ctrl+C to stop
echo  =====================================================
echo.

:MINE_LOOP
"%BINDIR%bitcoin-cli.exe" -datadir="%DATADIR%" -rpcport=%RPCPORT% -rpcuser=%RPCUSER% -rpcpassword=%RPCPASS% getblockcount > "%TEMP%\hon_blocks.txt" 2>nul
set /p BLOCKS=<"%TEMP%\hon_blocks.txt"
"%BINDIR%bitcoin-cli.exe" -datadir="%DATADIR%" -rpcport=%RPCPORT% -rpcuser=%RPCUSER% -rpcpassword=%RPCPASS% generatetoaddress 1 "%ADDR%" > nul 2>nul
echo  [%TIME%] Block #%BLOCKS% mined! +200 HON
timeout /t 5 /nobreak > nul
goto MINE_LOOP
