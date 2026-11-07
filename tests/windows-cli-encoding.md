# Windows CLI Encoding

The application uses the CRT `wmain` entry point on Windows and converts each
UTF-16 argument to UTF-8 before the existing parser. Conversion uses
`WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS)` with checked sizing and
conversion calls, owned storage, and a null-terminated argv array. Invalid
UTF-16 is rejected, not replaced. No process locale or ANSI code page is changed.
Non-Windows entry behavior and core name limits are unchanged. MSVC uses its
normal console CRT startup; MinGW targets select Unicode startup with `-municode`.

CI launches `nozzle-spout.exe` with Japanese and supplementary Unicode source/target names, quoted text, and an empty argument, then checks the real status report. It also requires rejection of unpaired UTF-16 surrogates. This is command-line/status evidence only, not Spout host or GPU-transfer evidence.

Run the command in the Windows Unicode CLI step of `.github/workflows/ci.yml`
against that job's built executable. These checks do not establish Unicode
filesystem-path support or interactive console font/code-page behavior.

Contract: [Microsoft WideCharToMultiByte documentation](https://learn.microsoft.com/en-us/windows/win32/api/stringapiset/nf-stringapiset-widechartomultibyte).
