# LO02 对局流程与模块接口

本目录使用 C++17。A 负责创建玩家、跟踪当前玩家、提交操作、推进回合并保存结算。示例中的“标记并确认”是验证对局流程用的自定义模拟动作。

## 在 VS Code 中打开和运行

在 Windows 上，用 VS Code 打开本 README 所在的 `source` 文件夹，然后按 **Ctrl+Shift+B**。默认任务会编译并自动运行双人模拟。结果显示在 VS Code 底部的终端中。也可以从仓库根目录运行 `source\run-demo.cmd`，或在 `source` 目录内运行 `run-demo.cmd`。

每个仓库副本使用自己的 `build/` 目录，Git 会忽略该目录。打包 `Projet.zip` 时排除 `build/`。如需手动编译，在 `source` 目录的 PowerShell 终端执行以下命令。

```powershell
cmake -S . -B ../build
cmake --build ../build --config Debug
ctest --test-dir ../build -C Debug --output-on-failure
```

使用 Windows 上的 Visual Studio 生成器时，可直接运行下列四组模拟用例。各用例会检查对局状态，发现错误时以非零状态退出。使用单配置生成器时，可执行文件位于 `../build`，路径中省略 `Debug`。

```powershell
../build/Debug/lo02_flow_demo.exe
../build/Debug/lo02_flow_demo.exe Alice
../build/Debug/lo02_flow_demo.exe Alice Bob Carol
../build/Debug/lo02_flow_demo.exe Alice Bob Carol Dave
```

## 联调约定

接口在 `include/lo02/game.hpp`。成员接入前一起确认以下约定。

| 成员 | 接入点 | 约定 |
|---|---|---|
| A | `Game` | 接收 1–4 个真人姓名。暂按单人局自动创建一名电脑玩家处理。玩家 ID 从 0 开始，与姓名独立。A 循环推进座次并保存结算结果。 |
| B | `TurnRules` | 初始化棋盘等对局数据。返回当前可用操作的 ID 和短标签。操作 ID 在当前状态下唯一，可在状态改变后失效。提交时重新检查 ID 和合法性。拒绝操作时保持状态不变。 |
| C | `OutcomeRules::evaluateAfterTurn` | 每个完整回合完成时调用，失败后可用相同参数重试。接收玩家表、刚结束回合的玩家 ID 和已完成回合数。通过 B 的只读状态取计分数据。仍可继续时返回空值，结束时返回各玩家分数和规则判定的所有赢家。抛出异常时要让结算逻辑保持可重试。 |
| D | `Game::currentPlayer` 和 `availableActions` | 当前玩家类型为 `Computer` 时，从当前可用操作中选择，并用 `submitAction` 提交。根据返回状态决定是否为同一玩家继续选操作。 |
| E | `Game` 的查询接口、B 的只读棋盘和手牌视图 | 显示当前玩家、可用操作和结算。根据玩家实际选中的操作 ID 调用 `submitAction`，并显示返回的消息。棋盘、手牌视图由 B 与 E 对齐。 |

每局为 `Game` 提供一组独立的规则对象，并让它们在对局期间保持有效。示例展示了 C 如何通过构造函数拿到 B 的只读引用。

| 提交结果 | A 的处理 |
|---|---|
| `Rejected` | 保持当前玩家和已完成回合数，允许重试。 |
| `AwaitingAction` | 保持当前玩家，允许该玩家提交下一步操作。 |
| `TurnComplete` | 增加已完成回合数并检查终局。C 给出结算时结束，否则切到下一位玩家。 |

如果结算检查抛出异常，异常会传给调用者。已执行的操作和已完成回合数会保留，`outcomePending()` 返回 `true`，当前玩家和可用操作暂为空。界面可调用 `resolvePendingOutcome()` 重试结算，此时不会再次执行玩家操作。结算检查成功后，按结果换人或结束对局。

终局后当前玩家为空，可用操作列表为空，提交操作会被拒绝。B 和 C 可自行组织内部类，只需符合对局接口。界面和电脑玩家使用同一个提交入口。

## 当前已覆盖

演示为 1、2、3、4 名真人分别验证开局、越权操作、非法操作、多步回合、回合轮转、终局结算、并列赢家的具体 ID 与终局后的操作拒绝。每次运行演示还会检查回合结算失败后的重试，以及只有一名赢家的情况。开发时可用这个示例作为 A 模块的联调基线。
