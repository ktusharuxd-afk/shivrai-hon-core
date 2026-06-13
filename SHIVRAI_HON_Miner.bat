@echo off
title SHIVRAI HON Miner
color 0A
echo.
echo  ========================================
echo   SHIVRAI HON (HON) - Mining Software
echo   Decentralized Digital Currency
echo  ========================================
echo.
echo  [1/4] Starting SHIVRAI HON Node...
cd /d "%~dp0"
start /min "" "bitcoind.exe" -datadir="%APPDATA%\SHIVRAI-HON" -conf="%APPDATA%\SHIVRAI-HON\shivrai-hon.conf" -server=1 -txindex=1
echo  Node starting... please wait 20 seconds
timeout /t 20 /nobreak > nul
echo.
echo  [2/4] Loading Wallet...
"bitcoin-cli.exe" -datadir="%APPDATA%\SHIVRAI-HON" -rpcport=8332 -rpcuser=shivraiuser -rpcpassword=YOUR_PASSWORD loadwallet "shivrai-mainnet" 2>nul
echo  Wallet loaded!
echo.
echo  [3/4] Starting Mining Dashboard...
start /min "" "node.exe" "%~dp0explorer\index.js"
timeout /t 3 /nobreak > nul
echo.
echo  [4/4] Opening Browser...
start "" "http://localhost:3000/mining.html"
echo.
echo  ========================================
echo   Mining Dashboard: http://localhost:3000/mining.html
echo   Press any key to stop mining...
echo  ========================================
pause > nul
echo.
echo  Stopping node...
"bitcoin-cli.exe" -datadir="%APPDATA%\SHIVRAI-HON" -rpcport=8332 -rpcuser=shivraiuser -rpcpassword=YOUR_PASSWORD stop
echo  Node stopped. Goodbye!
timeout /t 3 /nobreak > nul
