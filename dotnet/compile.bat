set csc=C:\Windows\Microsoft.NET\Framework64\v4.0.30319\csc.exe 

%csc% /reference:M2Mqtt.Net.dll SWService.cs 
%csc% /reference:M2Mqtt.Net.dll RegistryExample.cs
