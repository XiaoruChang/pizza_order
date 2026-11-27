# Pizza Ordering System

SOE103 小组项目，使用 C11 编写。本分支负责第五部分：销售统计与系统整合。

## 当前状态

仓库初始版本只有 README，成员 1–4 的账户、菜单、下单和余额模块尚未提交。
本分支提供统计模块、客户/经理主菜单和回调接入层，**尚未完成真实业务联调**。
默认程序不会创建账户、扣款或读取虚构销售记录；未绑定的操作会显示模块未接入。
`pizza_demo` 是独立的内存演示程序，明确标记 DEMO，不提供真实身份认证或持久化。

## 构建和运行

需要 CMake 3.20+ 和 C11 编译器。Windows MinGW 示例：

```powershell
cmake -S . -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
.\build\pizza_order.exe
.\build\pizza_demo.exe
```

Linux/macOS 可省略 `-G "MinGW Makefiles"`，运行 `./build/pizza_order` 或 `./build/pizza_demo`。
MSVC 可使用默认生成器，运行 `ctest --test-dir build -C Debug --output-on-failure`。
构建产物不提交 Git；最终作业的可执行文件由组长在验收后打包。

演示路径：选择 `2` 进入演示经理菜单，再选择 `3` 查看固定样例；`0` 返回，`0` 退出。
预期为 2 笔已支付订单、5 个披萨、销售额 151.50；未支付、取消和退款订单不计入。
演示客户入口为 `1`，使用演示账户 1001；客户业务操作仍显示未接入。
演示不需要用户名或密码，不能作为真实身份认证使用。

## 目录和成员接入

| 路径 | 用途 |
| --- | --- |
| `include/sales.h`, `src/sales.c` | 只读销售快照、统计和报表输出 |
| `include/app.h`, `src/app.c` | 登录入口、客户/经理菜单和功能分发 |
| `src/main.c` | 正式入口，等待绑定成员模块 |
| `examples/demo.c` | 独立演示接入和固定内存样例 |
| `tests/` | 统计、菜单集成和可执行程序冒烟测试 |
| `docs/integration.md` | 回调约定、统计口径和待联调事项 |
| `docs/part5-report.md` | 第五部分需求分析、设计和测试记录 |

其他成员可保留自己的账户、订单和存储结构，通过 `AppServices` 适配器接入。
这里新增的接口仅服务于第五部分，没有修改其他成员已有接口（初始仓库不存在这些接口）。
接入前由小组确认 `docs/integration.md` 中的映射，避免各模块重复实现扣款或订单保存。

## 检查

所有目标开启严格编译警告并视为错误。`ctest` 覆盖统计口径、金额/数量溢出、异常快照、
角色隔离、功能分发、登录失败、输入校验、快照释放，以及两个可执行程序的实际运行。
实际执行结果和剩余验收阻塞见 `docs/part5-report.md`。
