# Pizza ordering system

SOE103 小组项目，使用 C11 编写。本次提交实现第一部分“客户账户”。

## 已实现功能

- 客户注册并生成唯一账户编号
- 客户登录验证
- 客户查看和修改姓名、电话号码
- 经理添加、修改、停用客户账户
- 密码不以明文保存，登录只返回账户编号
- 输入校验、重复用户名检查和内存生命周期测试

当前模块是可嵌入其他成员代码的内存组件。它不负责菜单、订单、余额或持久化文件，
这些功能通过其他成员的模块接入。认证流程和接口约定见 [`docs/account.md`](docs/account.md)。

## 构建和测试

需要 CMake 3.20+ 和 C11 编译器。

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
.\build\account_demo.exe
```

Linux/macOS 可省略 `-G "MinGW Makefiles"`，再运行 `./build/account_demo`。

## 接入边界

`AccountStore` 当前使用内存数组，程序重启后数据会清空。正式系统需要由小组决定持久化格式，
并由存储适配器把记录映射到该接口。经理权限验证由应用层负责，账户模块只管理客户记录。
