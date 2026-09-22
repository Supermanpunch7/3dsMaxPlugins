param([string]$ScriptPath = '', [string]$Report = 'mcp-status.json')
$ErrorActionPreference = 'Stop'
$client = [Net.Sockets.TcpClient]::new()
try {
    $client.Connect('127.0.0.1', 60640)
    $client.ReceiveTimeout = 120000
    $stream = $client.GetStream()
    $writer = [IO.StreamWriter]::new($stream, [Text.UTF8Encoding]::new($false))
    $reader = [IO.StreamReader]::new($stream)
    $writer.AutoFlush = $true
    if ($ScriptPath) {
        $request = @{action='maxscript'; code=[IO.File]::ReadAllText($ScriptPath); undo=$false; timeout=110}
    } else {
        $request = @{action='maxscript'; code='(#(maxVersion(), maxFilePath, maxFileName, objects.count, selection.count, (for c in modifier.classes where matchPattern (c as string) pattern:"*Chamfer*" collect #(c as string, c.classID))))'; timeout=30}
    }
    $writer.WriteLine(($request | ConvertTo-Json -Compress))
    $response = $reader.ReadLine()
    if (!$response) { throw 'Bridge closed without response' }
    [IO.File]::WriteAllText((Join-Path $PSScriptRoot $Report), $response)
} catch {
    [IO.File]::WriteAllText((Join-Path $PSScriptRoot $Report), ($_ | Out-String))
    throw
} finally { $client.Dispose() }