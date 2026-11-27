# 第五部分接入约定

当前状态：供组长及各成员评审的新增接入层；成员 1–4 尚无代码入库，真实联调待完成。
不要求其他成员更换内部结构、存储格式或现有函数；在真实代码入库后编写小型适配器。
如适配必须修改他人公共接口，应先协调，再修改。

## 责任映射

| 成员 | AppServices 回调 | 责任 |
| --- | --- | --- |
| 1：客户账户 | `register_customer`, `login_customer`, `edit_profile`, `manage_customers` | 唯一账户号、姓名电话、身份验证、经理增删改客户 |
| 2：菜单和价格 | `manage_menu` | 经理修改披萨类型、尺寸、配料、价格；客户选餐由下单模块调用菜单模块 |
| 3：点餐和下单 | `place_order`, `load_orders`, `release_orders` | 点餐、确认、结算、保存；提供完整订单快照 |
| 4：余额和历史 | `show_balance`, `deposit`, `order_history` | 当前客户余额、充值、历史；下单过程中余额检查及扣款由 3/4 协作 |
| 5：统计和整合 | `app_run`, `sales_report_build`, `sales_report_print` | 菜单路由、角色入口、只读统计，不再次扣款或保存订单 |

经理验证通过 `login_manager` 接入，由小组确定验证方式和维护成员。缺少此回调时经理
入口关闭；正式程序不包含默认密码。客户验证成功需返回非零 `AppSession.account_id`。
每次登录从空会话开始，注销后不会复用上一次账户号。

## 回调返回值与运行

- 回调成功返回 `0`，失败返回非零；菜单显示错误并允许重试。
- 所有回调收到同一 `context`、输入流和输出流。客户操作额外收到只读的当前会话。
- 回调负责读取完整业务输入行；建议使用 `fgets`，避免 `scanf` 留下换行影响主菜单。
- 回调内部必须验证账户是否存在及权限；此菜单层提供路由，不代替业务校验。
- 回调不得关闭或保留借用的流；`context` 至少在 `app_run` 返回前有效。
- 菜单输入支持前后空白，拒绝空行、尾随文字、超范围数字和过长输入；EOF 正常退出。
- 尚未设置的回调保留为 `NULL`，菜单显示 `Module not connected`，不伪造成功结果。
- 返回值 `app_run == 1` 表示输入输出错误或参数非法，`0` 表示正常结束。

正式入口 `src/main.c` 当前绑定空服务表。真实模块入库后，将适配器源文件加入 CMake，
按下面模式设置 `context` 和相应回调，再调用 `app_run`。以下仅示意，不是已实现业务：

```c
AppServices services = {0};
services.context = &application_state;
services.login_customer = account_login_adapter;
services.login_manager = manager_login_adapter;
services.place_order = order_adapter;
services.load_orders = order_snapshot_adapter;
services.release_orders = order_snapshot_release;
/* Bind the remaining callbacks from the table above. */
return app_run(stdin, stdout, &services);
```

可编译的完整接入样例见 `examples/demo.c`，但其中登录回调只用于演示，不能复制为正式验证。

## 销售快照和口径

`SalesOrder`/`SalesLine` 是统计的只读投影，与真实订单结构解耦。适配器应遵循：

1. 每次 `load_orders` 读取当前完整且一致的订单快照；不能返回分页、截断或半写入数据。
   若加载失败，由加载器释放临时资源并返回非零。禁止将失败伪装成空订单列表。
2. 只有完整支付且未退款的订单映射为 `SALES_PAID`。未支付、取消、全额退款分别映射
   为 `SALES_PENDING`、`SALES_CANCELLED`、`SALES_REFUNDED`，这些订单不计入销量和销售额。
   当前不支持部分退款、日期筛选和跨币种汇总，需要时先由小组约定扩展。
3. 已支付订单必须有非零且唯一的 `order_id`，至少一条合法明细。同一披萨可在一个或
   多个订单中重复出现。披萨使用稳定的非零 `pizza_id`；不同类型不能重复使用 ID。
4. `quantity` 是正整数；`revenue_cents` 是该条明细**全部数量的历史实际实收小计**，
   已包含尺寸、配料及折扣。不是单价，统计时不能再乘数量，不能按现价重新计算。
   例如两个披萨总计 65.00，应传 `quantity=2, revenue_cents=6500`。
   金额为同一币种的非负整数分；免费赠品可为 0。订单总实收需与明细合计一致。
5. 名称为最多 63 字节、以 NUL 结束的历史名称，不允许控制字符。UTF-8 中文按字节计算。
   统计按 `pizza_id` 合并，保留快照中首次出现的名称，按 ID 升序输出；更名不会拆分销量。
6. `load_orders` 成功后，数组及各 `lines` 必须有效，直到对应的 `release_orders`。
   空快照允许 `orders=NULL, count=0`。加载/释放回调必须成对绑定。
7. 菜单对每次成功加载调用一次释放，即使快照非法或统计失败；报告自己持有名称和汇总，
   释放源快照后仍可输出。静态数组适配器也须提供空释放函数。

统计 API 的 `SalesReport` 应先初始化为 `{0}`。构建成功后由调用者
`sales_report_free` 释放，再重新构建；失败不会改变输出。重复的已支付订单 ID、非法明细、
未知状态和溢出会返回错误，不显示部分结果。函数不会写入订单、余额或持久化文件。
统计为整个快照的累计值，不声称是“今日销售”。

## 完整系统验收前必须补齐

- 成员 1–4 的真实代码和接口映射；经理的真实认证方式。
- 注册→登录→充值→点餐→扣款及保存→订单历史→经理统计的真实端到端测试。
- 余额不足、取消、失败保存不计入统计；跨程序重启后历史订单与统计一致。
- 小组确认币种、订单状态映射、披萨 ID 及实收小计来源；移除正式运行中的空服务表。

这些工作是当前 Draft PR 的验收阻塞；演示和测试替身不能替代真实联调。
