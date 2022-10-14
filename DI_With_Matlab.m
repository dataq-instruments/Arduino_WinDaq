
% Dataq DI 1100, DI2100, DI4000 series with MATLAB R2019b
%
% The device must be in CDC mode, follow README discussion
%
% See README in the project for useful discussion
%================================================

clear, clc

numOfData = 100;   
% port = "/dev/tty.usbmodemFA131";    % For Mac OS, follow README discussion
port = "COM6";    % For Windows, in device manager's Ports (COM & LPT)
baudrate = 115200; %this is just like a place holder, it has no meaning
s = serialport(port, baudrate, "Timeout", 5);
configureTerminator(s, "CR")
% Clear serial buffer
flush(s)

% Check if device responds
writeline(s, "info 1")
pause(1)
data = readline(s)

% Configure

% Stop in case device was already running
writeline(s, "stop")
pause(0.1)

% Ending char for output in ascii mode is CR
writeline(s, "eol 0")
pause(0.1)

% Set up for ascii communication, follow README discussion
writeline(s, "encode 1")
pause(0.1)
data = readline(s);

% Set up 2 channels to scan
writeline(s, "slist 0 0")
pause(0.1)
writeline(s, "slist 1 1")
pause(0.1)

% Rate and other setup
writeline(s, "srate 6000")
pause(0.1)
writeline(s, "dec 200")
pause(0.1)
writeline(s, "deca 1")
pause(1)
data = readline(s);

% Let's turn D0/1/2 from input to switch so that we can output 
writeline(s, "endo 7") 
pause(0.1)

% Output 1 on D0/1/2
writeline(s, "dout 7") 
pause(0.1)


% Flush the port buffer
flush(s)

% Start scanning
writeline(s, "start")
pause(0.1)


% Read data, follow README discussion
for i=1:numOfData
    % Read the data
    data = readline(s);
    C = strsplit(data, ','); 
    % Convert to number
    NumData(i) = str2double(C(1));
    NumData2(i) = str2double(C(2));
end

% Output 0 on D0/1/2
writeline(s, "dout 0") 
pause(0.1)

% Stop data acquisition
writeline(s, "stop")

% Plot data
figure(1)
plot(NumData, 'o-');
grid on; ylabel('Value'); xlabel('sample')

figure(2)
plot(NumData2, 'o-');
grid on; ylabel('Value'); xlabel('sample')

% Calculate average
aveValue = mean(NumData)
aveValue2 = mean(NumData2)

% Close the port
clear s