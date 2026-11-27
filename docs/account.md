# 账户认证与客户资料模块

## 当前边界

`include/account.h` 是仓库 `main` 的公共契约。它把账户操作绑定到不透明的
`AppState`，因此账户记录、密码验证记录和持久化提交必须由账户模块与公共存储模块共同实现。
本分支保留的 `src/account.c` 是合并公共契约前的内存原型，接口为 `AccountStore`；它用于说明
认证算法和测试思路，不能直接替代 `account.h` 中的正式实现。

## 组件

| 组件 | 作用 |
| --- | --- |
| `CustomerRegistration` | 借用注册资料和密码；密码只在调用期间有效 |
| `account_register` / `account_add_customer` | 创建客户、分配不重复的 `AccountId`，初始化余额为零 |
| `account_login_customer` / `account_login_manager` | 验证凭据并产生带角色的 `Session` |
| `account_get_customer` / `account_update_profile` | 按会话权限读取或修改姓名、电话 |
| `account_set_active` / `account_list_customers` | 经理停用、恢复和列出客户 |
| `storage_*` | 负责 `AppState` 的生命周期、凭据记录保存和原子提交 |

## 请求流

1. 注册或经理新增时，适配器收集完整输入并构造 `CustomerRegistration`。
2. `account_register` 校验非空字符串、长度和控制字符，检查用户名唯一性，分配单调递增的账户编号。
3. 登录前先把非空 `out_session` 清零；客户登录提交账户号和密码，经理登录提交用户名和密码。
4. 账户模块从私有凭据记录读取盐值和验证摘要，重新计算摘要并比较。成功只写入 `Session.account_id` 与 `Session.role`；调用者不能自行伪造角色。
5. 客户资料接口再次检查会话角色、账户归属和账户是否启用，然后返回独立的 `Customer` 副本。
6. 成功的注册、资料修改、停用或恢复必须在返回前通过存储层原子保存；失败时内存和文件都保持原值。
7. 注销调用 `account_logout` 清零会话。状态关闭后，原会话立即失效。

## 凭据与令牌

- 公共 `Customer` 结构不包含密码、盐值或验证摘要；这些字段只能存在于 `AppState` 的私有凭据记录中。
- 密码长度按 `PIZZA_PASSWORD_MIN_BYTES` 至 `PIZZA_PASSWORD_MAX_BYTES` 校验，保留用户输入的空格，不写日志、不写销售快照，也不提交到 Git。
- `RESULT_AUTH_FAILED` 对错误密码、未知账户和错误经理用户名统一返回；已正确认证但已停用的客户返回 `RESULT_ACCOUNT_INACTIVE`。
- 当前程序是本地 C 控制台，没有 HTTP 边界，因此没有 JWT、Cookie 或 Bearer token。`Session` 是绑定到单个 `AppState` 生命周期的进程内身份，不是可跨网络传输的访问令牌。
- 若未来增加网络服务，服务层应在登录成功后签发短期随机令牌，只保存令牌摘要和过期时间，并通过 TLS 传输；注销、过期或账户停用后拒绝该令牌。

## 代码核对

旧原型 `src/account.c` 的 `password_digest` 使用“盐值 + 摘要”而不是明文密码，`account_login` 在失败时把输出账户号清零；
`account_manager_delete_customer` 采用停用而非物理删除，保留编号以便订单历史继续引用。这些行为与
`docs/public-api.md` 的安全意图一致，但原型仍未接入正式 `AppState` 和持久化事务。

## 集成阻塞

`origin/main` 尚未提供 `AppState` 的私有布局、凭据存储和原子提交辅助接口。账户实现若自行定义布局会与负责公共存储的成员冲突，因此正式账户业务暂以 Draft PR 交付，待小组统一内部接口后再接入并补充端到端持久化测试。
