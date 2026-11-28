# 客户账户模块

## 组件和边界

`include/account.h` 提供注册、客户/经理登录、资料管理、停用恢复和客户列表接口。
账户模块不定义 `AppState`，也不读取键盘或打印菜单；它通过
`src/account_private.h` 中的四个后端函数读取、复制和原子提交账户状态。
公共存储实现负责把 `AccountStore` 放进自己的不透明状态，并在提交函数中完成文件事务。

`Customer` 只包含账户号、姓名、电话、余额和启用状态。盐值、密码验证摘要和经理凭据只存在
`AccountStore` 的私有记录中，不会复制到 `Customer`。

## 请求流程

1. `account_register` 或 `account_add_customer` 先验证姓名、电话和 8～64 字节密码，拒绝空值、控制字符和超长值。
2. 模块从后端复制候选账户状态，分配递增的非零 `AccountId`，将余额设为 0，生成独立盐值并保存验证摘要。
3. 候选状态通过 `account_backend_commit` 原子发布；提交失败时释放候选副本，原状态和输出参数保持不变。
4. `account_login_customer` 和 `account_login_manager` 在任何参数检查前清零非空 `out_session`。客户用账户号登录，经理用用户名登录；成功后分别产生 `SESSION_CUSTOMER` 或 `SESSION_MANAGER`。
5. 错误密码、未知账户和错误经理用户名返回 `RESULT_AUTH_FAILED`；正确密码对应停用客户时返回 `RESULT_ACCOUNT_INACTIVE`。
6. 客户资料接口再次检查会话归属。客户只能访问自己的账户，经理可以访问所有账户；输出始终是独立值副本。
7. 经理停用客户只修改 `active`，保留账户号、余额和历史关联。列表接口先查询所需容量，再按账户号升序复制记录。

## 凭据和令牌

- 密码只在调用期间借用，记录中不保存明文；`AccountStore` 保存盐值和多轮 64 位验证摘要。
- `Customer`、订单快照和错误文本不包含密码、盐值或摘要。测试只检查摘要存在且公开输出不泄露私有字段。
- 当前项目是本地 C 控制台程序，没有 HTTP 边界，因此没有 JWT、Cookie 或 Bearer token。`Session` 是绑定到单个 `AppState` 生命周期的进程内身份，不能跨进程或跨网络使用。
- 若后续增加网络层，应由服务层签发短期、随机、可撤销的令牌，只保存令牌摘要和过期时间，并通过 TLS 传输。

## 测试后端

`tests/test_account.c` 提供一个仅用于测试的 `AppState` 和后端实现，覆盖注册、递增编号、登录失败清零、客户/经理权限、停用恢复、独立副本、非法输入和列表容量约定。
正式存储模块接入时应复用 `src/account_private.h` 的所有权规则，并补充关闭后重新打开文件的持久化测试。

公共接口没有客户用户名字段；客户身份由 `AccountId + password` 登录，用户名只用于经理凭据。这一点与旧的 `AccountStore` 原型不同，不能再通过新增公共字段绕过现有接口。
