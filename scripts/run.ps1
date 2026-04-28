# 1. 定义 Qt 的 bin 目录路径
$qtBinPath = "H:\Qt\6.10.2\msvc2022_64\bin"

# 2. 将其添加到当前会话的 Path 环境变量中（放在最前面，确保优先执行）
$env:Path = "$qtBinPath;" + $env:Path

# 1. 定义 Qt 的 bin 目录路径
$opencvBinPath = "F:\core\opencv\build\x64\vc16\bin"
#
# 2. 将其添加到当前会话的 Path 环境变量中（放在最前面，确保优先执行）
$env:Path = "$opencvBinPath;" + $env:Path

.\build\Release\calib_tool.exe
