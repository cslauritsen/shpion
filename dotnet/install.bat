C:\Windows\Microsoft.NET\Framework64\v4.0.30319\installutil.exe SWService.exe
sc create SWService binpath= c:\pnp\dotnet\SWService.exe start= auto error= ignore
