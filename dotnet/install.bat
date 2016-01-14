C:\Windows\Microsoft.NET\Framework64\v4.0.30319\installutil.exe shp_service.exe
sc create shp_service binpath= c:\pnp\dotnet\shp_service.exe start= auto error= ignore
